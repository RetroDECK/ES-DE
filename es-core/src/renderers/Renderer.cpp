//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Renderer.cpp
//
//  General rendering functions.
//

#include "renderers/Renderer.h"

#include "ImageIO.h"
#include "Log.h"
#include "Settings.h"
#include "Shader_GL21.h"
#include "resources/ResourceManager.h"

#include <SDL2/SDL.h>
#include <stack>

#if defined(_WIN64)
#include <windows.h>
#endif

namespace Renderer
{
    static std::stack<Rect> clipStack;
    static SDL_Window* sdlWindow {nullptr};
    static glm::mat4 mProjectionMatrix {};
    static glm::mat4 mProjectionMatrixRotated {};
    static int windowWidth {0};
    static int windowHeight {0};
    static int screenWidth {0};
    static int screenHeight {0};
    static int screenOffsetX {0};
    static int screenOffsetY {0};
    static bool screenRotated {0};
    static bool initialCursorState {1};
    // Screen resolution modifiers relative to the 1920x1080 reference.
    static float screenHeightModifier {0.0f};
    static float screenWidthModifier {0.0f};
    static float screenAspectRatio {0.0f};

    static void setIcon()
    {
        size_t width {0};
        size_t height {0};
        ResourceData resData {
            ResourceManager::getInstance().getFileData(":/graphics/window_icon_256.png")};
        std::vector<unsigned char> rawData {
            ImageIO::loadFromMemoryRGBA32(resData.ptr.get(), resData.length, width, height)};

        if (!rawData.empty()) {
            ImageIO::flipPixelsVert(rawData.data(), width, height);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            unsigned int rmask {0xFF000000};
            unsigned int gmask {0x00FF0000};
            unsigned int bmask {0x0000FF00};
            unsigned int amask {0x000000FF};
#else
            unsigned int rmask {0x000000FF};
            unsigned int gmask {0x0000FF00};
            unsigned int bmask {0x00FF0000};
            unsigned int amask {0xFF000000};
#endif

            // Try creating SDL surface from logo data.
            SDL_Surface* logoSurface {SDL_CreateRGBSurfaceFrom(
                static_cast<void*>(rawData.data()), static_cast<int>(width),
                static_cast<int>(height), 32, static_cast<int>((width * 4)), rmask, gmask, bmask,
                amask)};

            if (logoSurface != nullptr) {
                SDL_SetWindowIcon(sdlWindow, logoSurface);
                SDL_FreeSurface(logoSurface);
            }
        }
    }

    static bool createWindow()
    {
        LOG(LogInfo) << "Creating window...";

        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            LOG(LogError) << "Couldn't initialize SDL: " << SDL_GetError();
            return false;
        }

        initialCursorState = (SDL_ShowCursor(0) != 0);

        int displayIndex {Settings::getInstance()->getInt("DisplayIndex")};
        // Check that an invalid value has not been manually entered in the es_settings.xml file.
        if (displayIndex != 1 && displayIndex != 2 && displayIndex != 3 && displayIndex != 4) {
            Settings::getInstance()->setInt("DisplayIndex", 1);
            displayIndex = 0;
        }
        else {
            --displayIndex;
        }

        int availableDisplays = SDL_GetNumVideoDisplays();
        if (displayIndex > availableDisplays - 1) {
            LOG(LogWarning) << "Requested display " << std::to_string(displayIndex + 1)
                            << " does not exist, changing to display 1";
            displayIndex = 0;
        }
        else {
            LOG(LogInfo) << "Using display: " << std::to_string(displayIndex + 1);
        }

        SDL_DisplayMode displayMode;
        SDL_GetDesktopDisplayMode(displayIndex, &displayMode);

#if defined(_WIN64)
        // Tell Windows that we're DPI aware so that we can set a physical resolution and
        // avoid any automatic DPI scaling.
        SetProcessDPIAware();
        // We need to set the resolution based on the actual display bounds as the numbers
        // returned by SDL_GetDesktopDisplayMode are calculated based on DPI scaling and
        // therefore do not necessarily reflect the physical display resolution.
        SDL_Rect displayBounds;
        SDL_GetDisplayBounds(displayIndex, &displayBounds);
        displayMode.w = displayBounds.w;
        displayMode.h = displayBounds.h;
#endif

        windowWidth = Settings::getInstance()->getInt("WindowWidth") ?
                          Settings::getInstance()->getInt("WindowWidth") :
                          displayMode.w;
        windowHeight = Settings::getInstance()->getInt("WindowHeight") ?
                           Settings::getInstance()->getInt("WindowHeight") :
                           displayMode.h;
        screenWidth = Settings::getInstance()->getInt("ScreenWidth") ?
                          Settings::getInstance()->getInt("ScreenWidth") :
                          windowWidth;
        screenHeight = Settings::getInstance()->getInt("ScreenHeight") ?
                           Settings::getInstance()->getInt("ScreenHeight") :
                           windowHeight;
        screenOffsetX = Settings::getInstance()->getInt("ScreenOffsetX") ?
                            Settings::getInstance()->getInt("ScreenOffsetX") :
                            0;
        screenOffsetY = Settings::getInstance()->getInt("ScreenOffsetY") ?
                            Settings::getInstance()->getInt("ScreenOffsetY") :
                            0;
        screenRotated = Settings::getInstance()->getInt("ScreenRotate") == 2;

        // Prevent the application window from minimizing when switching windows (when launching
        // games or when manually switching windows using the task switcher).
        SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

#if defined(__unix__)
        // Disabling desktop composition can lead to better framerates and a more fluid user
        // interface, but with some drivers it can cause strange behaviors when returning to
        // the desktop.
        if (Settings::getInstance()->getBool("DisableComposition"))
            SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "1");
        else
            SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif

        bool userResolution = false;
        // Check if the user has changed the resolution from the command line.
        if (windowWidth != displayMode.w || windowHeight != displayMode.h)
            userResolution = true;

        unsigned int windowFlags;
        setupWindow();

#if defined(_WIN64)
        // For Windows we use SDL_WINDOW_BORDERLESS as "real" full screen doesn't work properly.
        // The borderless mode seems to behave well and it's almost completely seamless, especially
        // with a hidden taskbar.
        if (!userResolution)
            windowFlags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL;
        else
            // If the resolution has been manually set from the command line, then keep the border.
            windowFlags = SDL_WINDOW_OPENGL;
#elif defined(__APPLE__)
        // Not sure if this could be a useful setting.
        //        SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "0");

        // The SDL_WINDOW_BORDERLESS mode seems to be the only mode that somehow works on macOS
        // as a real fullscreen mode will do lots of weird stuff like preventing window switching
        // or refusing to let emulators run at all. SDL_WINDOW_FULLSCREEN_DESKTOP almost works, but
        // it "shuffles" windows when starting the emulator and won't return properly when the game
        // has exited. With SDL_WINDOW_BORDERLESS some emulators (like RetroArch) have to be
        // configured to run in fullscreen mode or switching to its window will not work, but
        // apart from that this mode works fine.
        if (!userResolution)
            windowFlags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL;
        else
            windowFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL;
#else
        if (!userResolution)
            windowFlags = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL;
        else
            windowFlags = SDL_WINDOW_OPENGL;
#endif

        if ((sdlWindow =
                 SDL_CreateWindow("EmulationStation", SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex),
                                  SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), windowWidth,
                                  windowHeight, windowFlags)) == nullptr) {
            LOG(LogError) << "Couldn't create SDL window. " << SDL_GetError();
            return false;
        }

#if defined(__APPLE__)
        // The code below is required as the high DPI scaling on macOS is very bizarre and is
        // measured in "points" rather than pixels (even though the naming convention sure looks
        // like pixels). For example there could be a 1920x1080 entry in the OS display settings
        // that actually corresponds to something like 3840x2160 pixels while at the same time
        // there is a separate 1080p entry which corresponds to a "real" 1920x1080 resolution.
        // Therefore the --resolution flag results in different things depending on whether a high
        // DPI screen is used. E.g. 1280x720 on a 4K display would actually end up as 2560x1440
        // which is incredibly strange. No point in struggling with this strangeness though,
        // instead we simply indicate the physical pixel dimensions in parenthesis in the log
        // file and make sure to double the window and screen sizes in case of a high DPI
        // display so that the full application window is used for rendering.
        int width = 0;
        SDL_GL_GetDrawableSize(sdlWindow, &width, nullptr);
        int scaleFactor = static_cast<int>(width / windowWidth);

        LOG(LogInfo) << "Display resolution: " << std::to_string(displayMode.w) << "x"
                     << std::to_string(displayMode.h) << " (physical resolution "
                     << std::to_string(displayMode.w * scaleFactor) << "x"
                     << std::to_string(displayMode.h * scaleFactor) << ")";
        LOG(LogInfo) << "Display refresh rate: " << std::to_string(displayMode.refresh_rate)
                     << " Hz";
        LOG(LogInfo) << "EmulationStation resolution: " << std::to_string(windowWidth) << "x"
                     << std::to_string(windowHeight) << " (physical resolution "
                     << std::to_string(windowWidth * scaleFactor) << "x"
                     << std::to_string(windowHeight * scaleFactor) << ")";

        windowWidth *= scaleFactor;
        windowHeight *= scaleFactor;
        screenWidth *= scaleFactor;
        screenHeight *= scaleFactor;
#else
        LOG(LogInfo) << "Display resolution: " << std::to_string(displayMode.w) << "x"
                     << std::to_string(displayMode.h);
        LOG(LogInfo) << "Display refresh rate: " << std::to_string(displayMode.refresh_rate)
                     << " Hz";
        LOG(LogInfo) << "EmulationStation resolution: " << std::to_string(windowWidth) << "x"
                     << std::to_string(windowHeight);
#endif

        screenHeightModifier = static_cast<float>(screenHeight) / 1080.0f;
        screenWidthModifier = static_cast<float>(screenWidth) / 1920.0f;
        screenAspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

        LOG(LogInfo) << "Setting up OpenGL...";

        if (!createContext())
            return false;

        setIcon();
        setSwapInterval();

#if defined(_WIN64)
        // It seems as if Windows needs this to avoid a brief white screen flash on startup.
        // Possibly this is driver-specific rather than OS-specific. There is additional code
        // in init() to work around the white screen flash issue on all operating systems.
        swapBuffers();
#endif

        LOG(LogInfo) << "Loading shaders...";

        std::vector<std::string> shaderFiles;
        shaderFiles.push_back(":/shaders/glsl/core.glsl");
        shaderFiles.push_back(":/shaders/glsl/blur_horizontal.glsl");
        shaderFiles.push_back(":/shaders/glsl/blur_vertical.glsl");
        shaderFiles.push_back(":/shaders/glsl/scanlines.glsl");

        for (auto it = shaderFiles.cbegin(); it != shaderFiles.cend(); ++it) {
            Shader* loadShader = new Shader();

            loadShader->loadShaderFile(*it, GL_VERTEX_SHADER);
            loadShader->loadShaderFile(*it, GL_FRAGMENT_SHADER);

            if (!loadShader->createProgram()) {
                LOG(LogError) << "Could not create shader program.";
                return false;
            }

            sShaderProgramVector.push_back(loadShader);
        }

        return true;
    }

    static void destroyWindow()
    {
        for (auto it = sShaderProgramVector.cbegin(); it != sShaderProgramVector.cend(); ++it)
            delete *it;

        destroyContext();
        SDL_DestroyWindow(sdlWindow);

        sdlWindow = nullptr;

        SDL_ShowCursor(initialCursorState);
        SDL_Quit();
    }

    bool init()
    {
        if (!createWindow())
            return false;

        glm::mat4 projection {getIdentity()};
        Rect viewport {0, 0, 0, 0};

        viewport.x = windowWidth - screenOffsetX - screenWidth;
        viewport.y = windowHeight - screenOffsetY - screenHeight;
        viewport.w = screenWidth;
        viewport.h = screenHeight;
        projection = glm::ortho(0.0f, static_cast<float>(screenWidth),
                                static_cast<float>(screenHeight), 0.0f, -1.0f, 1.0f);
        projection = glm::rotate(projection, glm::radians(180.0f), {0.0f, 0.0f, 1.0f});
        mProjectionMatrixRotated =
            glm::translate(projection, {screenWidth * -1.0f, screenHeight * -1.0f, 0.0f});

        viewport.x = screenOffsetX;
        viewport.y = screenOffsetY;
        viewport.w = screenWidth;
        viewport.h = screenHeight;
        mProjectionMatrix = glm::ortho(0.0f, static_cast<float>(screenWidth),
                                       static_cast<float>(screenHeight), 0.0f, -1.0f, 1.0f);

        // This is required to avoid a brief white screen flash during startup on some systems.
        Renderer::drawRect(0.0f, 0.0f, static_cast<float>(Renderer::getScreenWidth()),
                           static_cast<float>(Renderer::getScreenHeight()), 0x000000FF, 0x000000FF);
        swapBuffers();

        return true;
    }

    void deinit()
    {
        // Destroy the window.
        destroyWindow();
    }

    void pushClipRect(const glm::ivec2& pos, const glm::ivec2& size)
    {
        Rect box(pos.x, pos.y, size.x, size.y);

        if (box.w == 0)
            box.w = screenWidth - box.x;
        if (box.h == 0)
            box.h = screenHeight - box.y;

        if (screenRotated) {
            box = Rect(windowWidth - screenOffsetX - box.x - box.w,
                       windowHeight - screenOffsetY - box.y - box.h, box.w, box.h);
        }
        else {
            box = Rect(screenOffsetX + box.x, screenOffsetY + box.y, box.w, box.h);
        }

        // Make sure the box fits within clipStack.top(), and clip further accordingly.
        if (clipStack.size()) {
            const Rect& top = clipStack.top();
            if (top.x > box.x)
                box.x = top.x;
            if (top.y > box.y)
                box.y = top.y;
            if ((top.x + top.w) < (box.x + box.w))
                box.w = (top.x + top.w) - box.x;
            if ((top.y + top.h) < (box.y + box.h))
                box.h = (top.y + top.h) - box.y;
        }

        if (box.w < 0)
            box.w = 0;
        if (box.h < 0)
            box.h = 0;

        clipStack.push(box);

        setScissor(box);
    }

    void popClipRect()
    {
        if (clipStack.empty()) {
            LOG(LogError) << "Tried to popClipRect while the stack was empty";
            return;
        }

        clipStack.pop();

        if (clipStack.empty())
            setScissor(Rect(0, 0, 0, 0));
        else
            setScissor(clipStack.top());
    }

    void drawRect(const float x,
                  const float y,
                  const float w,
                  const float h,
                  const unsigned int color,
                  const unsigned int colorEnd,
                  bool horizontalGradient,
                  const float opacity,
                  const float dim,
                  const Blend::Factor srcBlendFactor,
                  const Blend::Factor dstBlendFactor)
    {
        Vertex vertices[4];

        float wL {w};
        float hL {h};

        // If the width or height was scaled down to less than 1 pixel, then set it to
        // 1 pixel so that it will still render on lower resolutions.
        if (wL > 0.0f && wL < 1.0f)
            wL = 1.0f;
        if (hL > 0.0f && hL < 1.0f)
            hL = 1.0f;

        // clang-format off
        vertices[0] = {{x,      y     }, {0.0f, 0.0f}, color};
        vertices[1] = {{x,      y + hL}, {0.0f, 0.0f}, horizontalGradient ? color : colorEnd};
        vertices[2] = {{x + wL, y     }, {0.0f, 0.0f}, horizontalGradient ? colorEnd : color};
        vertices[3] = {{x + wL, y + hL}, {0.0f, 0.0f}, colorEnd};
        // clang-format on

        // Round vertices.
        for (int i = 0; i < 4; ++i)
            vertices[i].pos = glm::round(vertices[i].pos);

        vertices->opacity = opacity;
        vertices->dim = dim;

        bindTexture(0);
        drawTriangleStrips(vertices, 4, srcBlendFactor, dstBlendFactor);
    }

    Shader* getShaderProgram(unsigned int shaderID)
    {
        unsigned int index = 0;

        // Find the index in sShaderProgramVector by counting the number
        // of shifts required to reach 0.
        while (shaderID > 0) {
            shaderID = shaderID >> 1;
            ++index;
        }

        if (sShaderProgramVector.size() > index - 1)
            return sShaderProgramVector[index - 1];
        else
            return nullptr;
    }

    const glm::mat4& getProjectionMatrix()
    {
        if (screenRotated)
            return mProjectionMatrixRotated;
        else
            return mProjectionMatrix;
    }
    const glm::mat4& getProjectionMatrixNormal() { return mProjectionMatrix; }
    SDL_Window* getSDLWindow() { return sdlWindow; }
    const float getWindowWidth() { return static_cast<float>(windowWidth); }
    const float getWindowHeight() { return static_cast<float>(windowHeight); }
    const float getScreenWidth() { return static_cast<float>(screenWidth); }
    const float getScreenHeight() { return static_cast<float>(screenHeight); }
    const float getScreenOffsetX() { return static_cast<float>(screenOffsetX); }
    const float getScreenOffsetY() { return static_cast<float>(screenOffsetY); }
    const bool getScreenRotated() { return screenRotated; }
    const float getScreenWidthModifier() { return screenWidthModifier; }
    const float getScreenHeightModifier() { return screenHeightModifier; }
    const float getScreenAspectRatio() { return screenAspectRatio; }

} // namespace Renderer

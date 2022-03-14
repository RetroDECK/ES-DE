//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Renderer.cpp
//
//  Generic rendering functions.
//

#include "renderers/Renderer.h"

#include "ImageIO.h"
#include "Log.h"
#include "Settings.h"
#include "Shader_GL21.h"
#include "renderers/Renderer_GL21.h"
#include "resources/ResourceManager.h"

#if defined(_WIN64)
#include <windows.h>
#endif

Renderer* Renderer::getInstance()
{
    static RendererOpenGL instance;
    return &instance;
}

void Renderer::setIcon()
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
            static_cast<void*>(rawData.data()), static_cast<int>(width), static_cast<int>(height),
            32, static_cast<int>((width * 4)), rmask, gmask, bmask, amask)};

        if (logoSurface != nullptr) {
            SDL_SetWindowIcon(mSDLWindow, logoSurface);
            SDL_FreeSurface(logoSurface);
        }
    }
}

bool Renderer::createWindow()
{
    LOG(LogInfo) << "Creating window...";

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG(LogError) << "Couldn't initialize SDL: " << SDL_GetError();
        return false;
    }

    mInitialCursorState = (SDL_ShowCursor(0) != 0);

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

    mWindowWidth = Settings::getInstance()->getInt("WindowWidth") ?
                       Settings::getInstance()->getInt("WindowWidth") :
                       displayMode.w;
    mWindowHeight = Settings::getInstance()->getInt("WindowHeight") ?
                        Settings::getInstance()->getInt("WindowHeight") :
                        displayMode.h;
    sScreenWidth = Settings::getInstance()->getInt("ScreenWidth") ?
                       Settings::getInstance()->getInt("ScreenWidth") :
                       mWindowWidth;
    sScreenHeight = Settings::getInstance()->getInt("ScreenHeight") ?
                        Settings::getInstance()->getInt("ScreenHeight") :
                        mWindowHeight;
    mScreenOffsetX = Settings::getInstance()->getInt("ScreenOffsetX") ?
                         Settings::getInstance()->getInt("ScreenOffsetX") :
                         0;
    mScreenOffsetY = Settings::getInstance()->getInt("ScreenOffsetY") ?
                         Settings::getInstance()->getInt("ScreenOffsetY") :
                         0;
    mScreenRotated = Settings::getInstance()->getBool("ScreenRotate");

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
    if (mWindowWidth != displayMode.w || mWindowHeight != displayMode.h)
        userResolution = true;

    unsigned int windowFlags;
    setup();

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

    if ((mSDLWindow =
             SDL_CreateWindow("EmulationStation", SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex),
                              SDL_WINDOWPOS_UNDEFINED_DISPLAY(displayIndex), mWindowWidth,
                              mWindowHeight, windowFlags)) == nullptr) {
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
    SDL_GL_GetDrawableSize(mSDLWindow, &width, nullptr);
    int scaleFactor = static_cast<int>(width / mWindowWidth);

    LOG(LogInfo) << "Display resolution: " << std::to_string(displayMode.w) << "x"
                 << std::to_string(displayMode.h) << " (physical resolution "
                 << std::to_string(displayMode.w * scaleFactor) << "x"
                 << std::to_string(displayMode.h * scaleFactor) << ")";
    LOG(LogInfo) << "Display refresh rate: " << std::to_string(displayMode.refresh_rate) << " Hz";
    LOG(LogInfo) << "EmulationStation resolution: " << std::to_string(mWindowWidth) << "x"
                 << std::to_string(mWindowHeight) << " (physical resolution "
                 << std::to_string(mWindowWidth * scaleFactor) << "x"
                 << std::to_string(mWindowHeight * scaleFactor) << ")";

    mWindowWidth *= scaleFactor;
    mWindowHeight *= scaleFactor;
    sScreenWidth *= scaleFactor;
    sScreenHeight *= scaleFactor;
#else
    LOG(LogInfo) << "Display resolution: " << std::to_string(displayMode.w) << "x"
                 << std::to_string(displayMode.h);
    LOG(LogInfo) << "Display refresh rate: " << std::to_string(displayMode.refresh_rate) << " Hz";
    LOG(LogInfo) << "EmulationStation resolution: " << std::to_string(mWindowWidth) << "x"
                 << std::to_string(mWindowHeight);
#endif

    sScreenHeightModifier = static_cast<float>(sScreenHeight) / 1080.0f;
    sScreenWidthModifier = static_cast<float>(sScreenWidth) / 1920.0f;
    sScreenAspectRatio = static_cast<float>(sScreenWidth) / static_cast<float>(sScreenHeight);

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

    return loadShaders();
}

void Renderer::destroyWindow()
{
    destroyContext();
    SDL_DestroyWindow(mSDLWindow);

    mSDLWindow = nullptr;

    SDL_ShowCursor(mInitialCursorState);
    SDL_Quit();
}

bool Renderer::init()
{
    if (!createWindow())
        return false;

    glm::mat4 projection {getIdentity()};
    Rect viewport {0, 0, 0, 0};

    viewport.x = mWindowWidth - mScreenOffsetX - sScreenWidth;
    viewport.y = mWindowHeight - mScreenOffsetY - sScreenHeight;
    viewport.w = sScreenWidth;
    viewport.h = sScreenHeight;
    projection = glm::ortho(0.0f, static_cast<float>(sScreenWidth),
                            static_cast<float>(sScreenHeight), 0.0f, -1.0f, 1.0f);
    projection = glm::rotate(projection, glm::radians(180.0f), {0.0f, 0.0f, 1.0f});
    mProjectionMatrixRotated =
        glm::translate(projection, {sScreenWidth * -1.0f, sScreenHeight * -1.0f, 0.0f});

    viewport.x = mScreenOffsetX;
    viewport.y = mScreenOffsetY;
    viewport.w = sScreenWidth;
    viewport.h = sScreenHeight;
    mProjectionMatrix = glm::ortho(0.0f, static_cast<float>(sScreenWidth),
                                   static_cast<float>(sScreenHeight), 0.0f, -1.0f, 1.0f);

    // This is required to avoid a brief white screen flash during startup on some systems.
    drawRect(0.0f, 0.0f, static_cast<float>(getScreenWidth()),
             static_cast<float>(getScreenHeight()), 0x000000FF, 0x000000FF);
    swapBuffers();

    return true;
}

void Renderer::deinit()
{
    // Destroy the window.
    destroyWindow();
}

void Renderer::pushClipRect(const glm::ivec2& pos, const glm::ivec2& size)
{
    Rect box {pos.x, pos.y, size.x, size.y};

    if (box.w == 0)
        box.w = sScreenWidth - box.x;
    if (box.h == 0)
        box.h = sScreenHeight - box.y;

    if (mScreenRotated) {
        box = Rect(mWindowWidth - mScreenOffsetX - box.x - box.w,
                   mWindowHeight - mScreenOffsetY - box.y - box.h, box.w, box.h);
    }
    else {
        box = Rect(mScreenOffsetX + box.x, mScreenOffsetY + box.y, box.w, box.h);
    }

    // Make sure the box fits within mClipStack.top(), and clip further accordingly.
    if (mClipStack.size()) {
        const Rect& top {mClipStack.top()};
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

    mClipStack.push(box);

    setScissor(box);
}

void Renderer::popClipRect()
{
    if (mClipStack.empty()) {
        LOG(LogError) << "Tried to popClipRect while the stack was empty";
        return;
    }

    mClipStack.pop();

    if (mClipStack.empty())
        setScissor(Rect(0, 0, 0, 0));
    else
        setScissor(mClipStack.top());
}

void Renderer::drawRect(const float x,
                        const float y,
                        const float w,
                        const float h,
                        const unsigned int color,
                        const unsigned int colorEnd,
                        bool horizontalGradient,
                        const float opacity,
                        const float dimming,
                        const BlendFactor srcBlendFactorFactor,
                        const BlendFactor dstBlendFactorFactor)
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
        vertices[i].position = glm::round(vertices[i].position);

    vertices->opacity = opacity;
    vertices->dimming = dimming;

    bindTexture(0);
    drawTriangleStrips(vertices, 4, srcBlendFactorFactor, dstBlendFactorFactor);
}

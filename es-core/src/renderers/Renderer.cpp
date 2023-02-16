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
#include "renderers/RendererOpenGL.h"
#include "renderers/ShaderOpenGL.h"
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

        constexpr unsigned int bmask {0x00FF0000};
        constexpr unsigned int gmask {0x0000FF00};
        constexpr unsigned int rmask {0x000000FF};
        constexpr unsigned int amask {0xFF000000};

        // Try creating SDL surface from logo data.
        SDL_Surface* logoSurface {SDL_CreateRGBSurfaceFrom(
            static_cast<void*>(rawData.data()), static_cast<int>(width), static_cast<int>(height),
            32, static_cast<int>(width * 4), bmask, gmask, rmask, amask)};

        if (logoSurface != nullptr) {
            SDL_SetWindowIcon(mSDLWindow, logoSurface);
            SDL_FreeSurface(logoSurface);
        }
    }
}

bool Renderer::createWindow()
{
    LOG(LogInfo) << "Creating window...";

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
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

    sScreenWidth = Settings::getInstance()->getInt("ScreenWidth") ?
                       Settings::getInstance()->getInt("ScreenWidth") :
                       displayMode.w;
    sScreenHeight = Settings::getInstance()->getInt("ScreenHeight") ?
                        Settings::getInstance()->getInt("ScreenHeight") :
                        displayMode.h;
    mScreenOffsetX = glm::clamp((Settings::getInstance()->getInt("ScreenOffsetX") ?
                                     Settings::getInstance()->getInt("ScreenOffsetX") :
                                     0),
                                -(displayMode.w / 2), displayMode.w / 2);
    mScreenOffsetY = glm::clamp((Settings::getInstance()->getInt("ScreenOffsetY") ?
                                     Settings::getInstance()->getInt("ScreenOffsetY") :
                                     0),
                                -(displayMode.w / 2), displayMode.h / 2);
    mScreenRotation = Settings::getInstance()->getInt("ScreenRotate");

    if (mScreenOffsetX != 0 || mScreenOffsetY != 0) {
        LOG(LogInfo) << "Screen offset: " << mScreenOffsetX << " horizontal, " << mScreenOffsetY
                     << " vertical";
    }
    else {
        LOG(LogInfo) << "Screen offset: disabled";
    }

    mPaddingWidth = 0;
    mPaddingHeight = 0;
    bool fullscreenPadding {false};

    if (Settings::getInstance()->getBool("FullscreenPadding") && sScreenWidth <= displayMode.w &&
        sScreenHeight <= displayMode.h) {
        mWindowWidth = displayMode.w;
        mWindowHeight = displayMode.h;
        mPaddingWidth = displayMode.w - sScreenWidth;
        mPaddingHeight = displayMode.h - sScreenHeight;
        mScreenOffsetX -= mPaddingWidth / 2;
        mScreenOffsetY -= mPaddingHeight / 2;
        fullscreenPadding = true;
    }

    if (!fullscreenPadding) {
        mWindowWidth = sScreenWidth;
        mWindowHeight = sScreenHeight;
    }

    // In case someone manually added an invalid value to es_settings.xml.
    if (mScreenRotation != 0 && mScreenRotation != 90 && mScreenRotation != 180 &&
        mScreenRotation != 270) {
        LOG(LogWarning) << "Invalid screen rotation value " << mScreenRotation
                        << " defined, changing it to 0/disabled";
        mScreenRotation = 0;
    }

    LOG(LogInfo) << "Screen rotation: "
                 << (mScreenRotation == 0 ? "disabled" :
                                            std::to_string(mScreenRotation) + " degrees");

    if (mScreenRotation == 90 || mScreenRotation == 270) {
        const int tempVal {sScreenWidth};
        sScreenWidth = sScreenHeight;
        sScreenHeight = tempVal;
    }

    if (sScreenHeight > sScreenWidth)
        sIsVerticalOrientation = true;
    else
        sIsVerticalOrientation = false;

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

    bool userResolution {false};
    // Check if the user has changed the resolution from the command line.
    if (mWindowWidth != displayMode.w || mWindowHeight != displayMode.h)
        userResolution = true;

    unsigned int windowFlags {0};
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
    int width {0};
    SDL_GL_GetDrawableSize(mSDLWindow, &width, nullptr);
    int scaleFactor {static_cast<int>(width / mWindowWidth)};

    LOG(LogInfo) << "Display resolution: " << std::to_string(displayMode.w) << "x"
                 << std::to_string(displayMode.h) << " (physical resolution "
                 << std::to_string(displayMode.w * scaleFactor) << "x"
                 << std::to_string(displayMode.h * scaleFactor) << ")";
    LOG(LogInfo) << "Display refresh rate: " << std::to_string(displayMode.refresh_rate) << " Hz";
    LOG(LogInfo) << "EmulationStation resolution: " << std::to_string(sScreenWidth) << "x"
                 << std::to_string(sScreenHeight) << " (physical resolution "
                 << std::to_string(sScreenWidth * scaleFactor) << "x"
                 << std::to_string(sScreenHeight * scaleFactor) << ")";

    mWindowWidth *= scaleFactor;
    mWindowHeight *= scaleFactor;
    sScreenWidth *= scaleFactor;
    sScreenHeight *= scaleFactor;
    mPaddingWidth *= scaleFactor;
    mPaddingHeight *= scaleFactor;
    mScreenOffsetX *= scaleFactor;
    mScreenOffsetY *= scaleFactor;

#else
    LOG(LogInfo) << "Display resolution: " << std::to_string(displayMode.w) << "x"
                 << std::to_string(displayMode.h);
    LOG(LogInfo) << "Display refresh rate: " << std::to_string(displayMode.refresh_rate) << " Hz";
    LOG(LogInfo) << "EmulationStation resolution: " << std::to_string(sScreenWidth) << "x"
                 << std::to_string(sScreenHeight);
#endif

    sScreenHeightModifier = static_cast<float>(sScreenHeight) / 1080.0f;
    sScreenWidthModifier = static_cast<float>(sScreenWidth) / 1920.0f;
    sScreenAspectRatio = static_cast<float>(sScreenWidth) / static_cast<float>(sScreenHeight);

    if (sIsVerticalOrientation)
        sScreenResolutionModifier = sScreenWidth / 1080.0f;
    else
        sScreenResolutionModifier = sScreenHeight / 1080.0f;

    if (Settings::getInstance()->getBool("FullscreenPadding")) {
        if (!fullscreenPadding) {
            LOG(LogWarning) << "Fullscreen padding can't be applied when --resolution is set "
                               "higher than the display resolution";
            LOG(LogInfo) << "Screen mode: windowed";
        }
        else {
            LOG(LogInfo) << "Screen mode: fullscreen padding";
        }
    }
    else if (userResolution) {
        LOG(LogInfo) << "Screen mode: windowed";
    }
    else {
        LOG(LogInfo) << "Screen mode: fullscreen";
    }

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

    if (mScreenRotation == 0) {
        mViewport.x = mWindowWidth + mScreenOffsetX - sScreenWidth;
        mViewport.y = mWindowHeight + mScreenOffsetY - sScreenHeight;
        mViewport.w = sScreenWidth;
        mViewport.h = sScreenHeight;
        mProjectionMatrix = glm::ortho(0.0f, static_cast<float>(sScreenWidth),
                                       static_cast<float>(sScreenHeight), 0.0f, -1.0f, 1.0f);
    }
    else if (mScreenRotation == 90) {
        mViewport.x = mWindowWidth + mScreenOffsetX - sScreenHeight;
        mViewport.y = mWindowHeight + mScreenOffsetY - sScreenWidth;
        mViewport.w = sScreenHeight;
        mViewport.h = sScreenWidth;
        projection = glm::ortho(0.0f, static_cast<float>(sScreenHeight),
                                static_cast<float>(sScreenWidth), 0.0f, -1.0f, 1.0f);
        projection = glm::rotate(projection, glm::radians(90.0f), {0.0f, 0.0f, 1.0f});
        mProjectionMatrix = glm::translate(projection, {0.0f, sScreenHeight * -1.0f, 0.0f});
    }
    else if (mScreenRotation == 180) {
        mViewport.x = mWindowWidth + mScreenOffsetX - sScreenWidth;
        mViewport.y = mWindowHeight + mScreenOffsetY - sScreenHeight;
        mViewport.w = sScreenWidth;
        mViewport.h = sScreenHeight;
        projection = glm::ortho(0.0f, static_cast<float>(sScreenWidth),
                                static_cast<float>(sScreenHeight), 0.0f, -1.0f, 1.0f);
        projection = glm::rotate(projection, glm::radians(180.0f), {0.0f, 0.0f, 1.0f});
        mProjectionMatrix =
            glm::translate(projection, {sScreenWidth * -1.0f, sScreenHeight * -1.0f, 0.0f});
    }
    else if (mScreenRotation == 270) {
        mViewport.x = mWindowWidth + mScreenOffsetX - sScreenHeight;
        mViewport.y = mWindowHeight + mScreenOffsetY - sScreenWidth;
        mViewport.w = sScreenHeight;
        mViewport.h = sScreenWidth;
        projection = glm::ortho(0.0f, static_cast<float>(sScreenHeight),
                                static_cast<float>(sScreenWidth), 0.0f, -1.0f, 1.0f);
        projection = glm::rotate(projection, glm::radians(270.0f), {0.0f, 0.0f, 1.0f});
        mProjectionMatrix = glm::translate(projection, {sScreenWidth * -1.0f, 0.0f, 0.0f});
    }

    mProjectionMatrixNormal = glm::ortho(0.0f, static_cast<float>(sScreenWidth),
                                         static_cast<float>(sScreenHeight), 0.0f, -1.0f, 1.0f);
    setViewport(mViewport);

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

    if (mScreenRotation == 0) {
        box = {mScreenOffsetX + box.x + mPaddingWidth, mScreenOffsetY + box.y + mPaddingHeight,
               box.w, box.h};
    }
    else if (mScreenRotation == 90) {
        box = {mScreenOffsetX + mWindowWidth - (box.y + box.h), mScreenOffsetY + box.x, box.h,
               box.w + mPaddingHeight};
    }
    else if (mScreenRotation == 270) {
        box = {mScreenOffsetX + box.y + mPaddingWidth,
               mScreenOffsetY + mWindowHeight - (box.x + box.w), box.h, box.w};
    }
    else if (mScreenRotation == 180) {
        box = {mWindowWidth + mScreenOffsetX - box.x - box.w,
               mWindowHeight + mScreenOffsetY - box.y - box.h, box.w, box.h};
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
        setScissor(Rect());
    else
        setScissor(mClipStack.top());
}

void Renderer::drawRect(const float x,
                        const float y,
                        const float w,
                        const float h,
                        const unsigned int color,
                        const unsigned int colorEnd,
                        const bool horizontalGradient,
                        const float opacity,
                        const float dimming,
                        const BlendFactor srcBlendFactor,
                        const BlendFactor dstBlendFactor)
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
    drawTriangleStrips(vertices, 4, srcBlendFactor, dstBlendFactor);
}

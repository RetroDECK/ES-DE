//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Renderer.cpp
//
//  General rendering functions.
//

#include "renderers/Renderer.h"

#include "math/Transform4x4f.h"
#include "math/Vector2i.h"
#include "resources/ResourceManager.h"
#include "ImageIO.h"
#include "Log.h"
#include "Settings.h"
#include "Shader_GL21.h"

#include <SDL2/SDL.h>
#include <stack>

#if defined (_WIN64)
#include <windows.h>
#endif

namespace Renderer
{
    static std::stack<Rect> clipStack;
    static SDL_Window* sdlWindow = nullptr;
    static int windowWidth = 0;
    static int windowHeight = 0;
    static int screenWidth = 0;
    static int screenHeight = 0;
    static int screenOffsetX = 0;
    static int screenOffsetY = 0;
    static int screenRotate = 0;
    static bool initialCursorState = 1;
    // Screen resolution modifiers relative to the 1920x1080 reference.
    static float screenHeightModifier;
    static float screenWidthModifier;

    static void setIcon()
    {
        size_t width = 0;
        size_t height = 0;
        ResourceData resData = ResourceManager::getInstance()->
                getFileData(":/graphics/window_icon_256.png");
        std::vector<unsigned char> rawData =
                ImageIO::loadFromMemoryRGBA32(resData.ptr.get(), resData.length, width, height);

        if (!rawData.empty()) {
            ImageIO::flipPixelsVert(rawData.data(), width, height);

            #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            unsigned int rmask = 0xFF000000;
            unsigned int gmask = 0x00FF0000;
            unsigned int bmask = 0x0000FF00;
            unsigned int amask = 0x000000FF;
            #else
            unsigned int rmask = 0x000000FF;
            unsigned int gmask = 0x0000FF00;
            unsigned int bmask = 0x00FF0000;
            unsigned int amask = 0xFF000000;
            #endif

            // Try creating SDL surface from logo data.
            SDL_Surface* logoSurface = SDL_CreateRGBSurfaceFrom(
                    static_cast<void*>(rawData.data()),
                    static_cast<int>(width), static_cast<int>(height), 32,
                    static_cast<int>((width * 4)), rmask, gmask, bmask, amask);

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
            LOG(LogError) << "Couldn't initialize SDL: " << SDL_GetError() << ".";
            return false;
        }

        initialCursorState = (SDL_ShowCursor(0) != 0);

        SDL_DisplayMode displayMode;
        SDL_GetDesktopDisplayMode(0, &displayMode);

        #if defined (_WIN64)
        // Tell Windows that we're DPI aware so that we can set a physical resolution and
        // avoid any automatic DPI scaling.
        if (SetProcessDPIAware()) {
            HDC screen = GetDC(nullptr);
            auto screenWidth = GetDeviceCaps(screen, HORZRES);
            auto screenHeight = GetDeviceCaps(screen, VERTRES);
            if (screenWidth != 0 && screenHeight != 0) {
                displayMode.w = screenWidth;
                displayMode.h = screenHeight;
            }
        }
        #endif

        windowWidth = Settings::getInstance()->getInt("WindowWidth") ?
                Settings::getInstance()->getInt("WindowWidth") : displayMode.w;
        windowHeight = Settings::getInstance()->getInt("WindowHeight") ?
                Settings::getInstance()->getInt("WindowHeight") : displayMode.h;
        screenWidth = Settings::getInstance()->getInt("ScreenWidth") ?
                Settings::getInstance()->getInt("ScreenWidth") : windowWidth;
        screenHeight = Settings::getInstance()->getInt("ScreenHeight") ?
                Settings::getInstance()->getInt("ScreenHeight") : windowHeight;
        screenOffsetX = Settings::getInstance()->getInt("ScreenOffsetX") ?
                Settings::getInstance()->getInt("ScreenOffsetX") : 0;
        screenOffsetY = Settings::getInstance()->getInt("ScreenOffsetY") ?
                Settings::getInstance()->getInt("ScreenOffsetY") : 0;
        screenRotate = Settings::getInstance()->getInt("ScreenRotate") ?
                Settings::getInstance()->getInt("ScreenRotate") : 0;

        screenHeightModifier = static_cast<float>(screenHeight) / 1080.0f;
        screenWidthModifier = static_cast<float>(screenWidth) / 1920.0f;

        // Prevent ES window from minimizing when switching windows (when launching
        // games or when manually switching windows using task switcher).
        SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

        #if defined(__APPLE__) || defined(__unix__)
        bool userResolution = false;
        // Check if the user has changed the resolution from the command line.
        if (windowWidth != displayMode.w || windowHeight != displayMode.h)
            userResolution = true;
        // Not sure if this could be a useful setting for some users.
//        SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "0");
        #endif

        setupWindow();

        unsigned int windowFlags;

        #if defined(_WIN64)
        // For Windows, always set the mode to windowed, as full screen mode seems to
        // behave quite erratic. There may be a proper fix for this, but for now windowed
        // mode seems to behave well and it's almost completely seamless, especially with
        // a hidden taskbar.
        windowFlags = getWindowFlags();
        #elif defined(__APPLE__)
        // This seems to be the only full window mode that somehow works on macOS as a real
        // fullscreen mode will do lots of weird stuff like preventing window switching
        // or refusing to let emulators run at all. SDL_WINDOW_FULLSCREEN_DESKTOP almost
        // works, but it "shuffles" windows when starting the emulator and won't return
        // properly when the game has exited. With the current mode, the top menu is visible
        // and hides that part of the ES window. Also, the splash screen is not displayed
        // until the point where ES has almost completely finished loading. I'm not sure
        // if anything can be done to improve these things as it's quite obvious that
        // Apple has shipped a broken and/or dysfunctional window manager with their
        // operating system.
        if (!userResolution)
            windowFlags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI | getWindowFlags();
        else
            // If the user has changed the resolution from the command line, then add a
            // border to the window.
            windowFlags = SDL_WINDOW_ALLOW_HIGHDPI | getWindowFlags();
        #else
        if (Settings::getInstance()->getBool("Windowed")) {
            windowFlags = getWindowFlags();
        }
        else if (Settings::getInstance()->getString("FullscreenMode") == "borderless") {
            if(!userResolution)
                windowFlags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP | getWindowFlags();
            else
                // If the user has changed the resolution from the command line, then add a
                // border to the window and don't make it stay on top.
                windowFlags = getWindowFlags();
        }
        else {
            windowFlags = SDL_WINDOW_FULLSCREEN | getWindowFlags();
        }
        #endif

        if ((sdlWindow = SDL_CreateWindow("EmulationStation", SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, windowFlags)) == nullptr) {
            LOG(LogError) << "Couldn't create SDL window. " << SDL_GetError();
            return false;
        }

        LOG(LogInfo) << "Setting up OpenGL...";

        if (!createContext())
            return false;

        setIcon();
        setSwapInterval();

        #if defined(USE_OPENGL_21)
        LOG(LogInfo) << "Loading shaders...";

        std::vector<std::string> shaderFiles;
        shaderFiles.push_back(":/shaders/glsl/desaturate.glsl");
        shaderFiles.push_back(":/shaders/glsl/opacity.glsl");
        shaderFiles.push_back(":/shaders/glsl/dim.glsl");
        shaderFiles.push_back(":/shaders/glsl/blur_horizontal.glsl");
        shaderFiles.push_back(":/shaders/glsl/blur_vertical.glsl");
        shaderFiles.push_back(":/shaders/glsl/scanlines.glsl");

        for (auto it = shaderFiles.cbegin(); it != shaderFiles.cend(); it++) {
            Shader* loadShader = new Shader();

            loadShader->loadShaderFile(*it, GL_VERTEX_SHADER);
            loadShader->loadShaderFile(*it, GL_FRAGMENT_SHADER);

            if (!loadShader->createProgram()) {
                LOG(LogError) << "Could not create shader program.";
                return false;
            }

            sShaderProgramVector.push_back(loadShader);
        }
        #endif

        return true;
    }

    static void destroyWindow()
    {
        #if defined(USE_OPENGL_21)
        for (auto it = sShaderProgramVector.cbegin();
                it != sShaderProgramVector.cend(); it++) {
            delete *it;
        }
        #endif

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

        Transform4x4f projection = Transform4x4f::Identity();
        Rect viewport = Rect(0, 0, 0, 0);

        switch (screenRotate) {
            case 0: {
                viewport.x = screenOffsetX;
                viewport.y = screenOffsetY;
                viewport.w = screenWidth;
                viewport.h = screenHeight;
                projection.orthoProjection(0.0f, static_cast<float>(screenWidth),
                        static_cast<float>(screenHeight), 0.0f, -1.0f, 1.0f);
            }
            break;
            case 1: {
                viewport.x = windowWidth - screenOffsetY - screenHeight;
                viewport.y = screenOffsetX;
                viewport.w = screenHeight;
                viewport.h = screenWidth;
                projection.orthoProjection(0.0f, static_cast<float>(screenHeight),
                        static_cast<float>(screenWidth), 0.0f, -1.0f, 1.0f);
                projection.rotate(static_cast<float>(ES_DEG_TO_RAD(90)), {0, 0, 1});
                projection.translate({0, screenHeight * -1.0f, 0});
            }
            break;
            case 2: {
                viewport.x = windowWidth  - screenOffsetX - screenWidth;
                viewport.y = windowHeight - screenOffsetY - screenHeight;
                viewport.w = screenWidth;
                viewport.h = screenHeight;
                projection.orthoProjection(0.0f, static_cast<float>(screenWidth),
                        static_cast<float>(screenHeight), 0.0f, -1.0f, 1.0f);
                projection.rotate(static_cast<float>(ES_DEG_TO_RAD(180)), {0, 0, 1});
                projection.translate({screenWidth * -1.0f, screenHeight * -1.0f, 0});
            }
            break;
            case 3: {
                viewport.x = screenOffsetY;
                viewport.y = windowHeight - screenOffsetX - screenWidth;
                viewport.w = screenHeight;
                viewport.h = screenWidth;
                projection.orthoProjection(0.0f, static_cast<float>(screenHeight),
                        static_cast<float>(screenWidth), 0.0f, -1.0f, 1.0f);
                projection.rotate(static_cast<float>(ES_DEG_TO_RAD(270)), {0, 0, 1});
                projection.translate({screenWidth * -1.0f, 0, 0});
            }
            break;
        }

        mProjectionMatrix = projection;

        setViewport(viewport);
        setProjection(projection);
        swapBuffers();

        return true;
    }

    void deinit()
    {
        destroyWindow();
    }

    void pushClipRect(const Vector2i& _pos, const Vector2i& _size)
    {
        Rect box(_pos.x(), _pos.y(), _size.x(), _size.y());

        if (box.w == 0)
            box.w = screenWidth - box.x;
        if (box.h == 0)
            box.h = screenHeight - box.y;

        switch (screenRotate) {
            case 0:
                box = Rect(screenOffsetX + box.x, screenOffsetY + box.y, box.w, box.h);
            break;
            case 1:
                box = Rect(windowWidth - screenOffsetY - box.y - box.h,
                        screenOffsetX + box.x, box.h, box.w);
            break;
            case 2:
                box = Rect(windowWidth - screenOffsetX - box.x - box.w, windowHeight -
                        screenOffsetY - box.y - box.h, box.w, box.h);
            break;
            case 3:
                box = Rect(screenOffsetY + box.y, windowHeight -
                        screenOffsetX - box.x - box.w, box.h, box.w);
            break;
        }

        // Make sure the box fits within clipStack.top(), and clip further accordingly.
        if (clipStack.size()) {
            const Rect& top = clipStack.top();
            if ( top.x >  box.x) box.x = top.x;
            if ( top.y >  box.y) box.y = top.y;
            if ((top.x + top.w) < (box.x + box.w)) box.w = (top.x + top.w) - box.x;
            if ((top.y + top.h) < (box.y + box.h)) box.h = (top.y + top.h) - box.y;
        }

        if (box.w < 0) box.w = 0;
        if (box.h < 0) box.h = 0;

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

    void drawRect(
            const float _x,
            const float _y,
            const float _w,
            const float _h,
            const unsigned int _color,
            const unsigned int _colorEnd,
            bool horizontalGradient,
            const float _opacity,
            const Transform4x4f& _trans,
            const Blend::Factor _srcBlendFactor,
            const Blend::Factor _dstBlendFactor)
    {
        const unsigned int color = convertRGBAToABGR(_color);
        const unsigned int colorEnd = convertRGBAToABGR(_colorEnd);
        Vertex vertices[4];

        float _wL = _w;
        float _hL = _h;

        // If the width or height was scaled down to less than 1 pixel, then set it to
        // 1 pixel so that it will still render on lower resolutions.
        if (_wL > 0.0f && _wL < 1.0f)
            _wL = 1.0f;
        if (_hL > 0.0f && _hL < 1.0f)
            _hL = 1.0f;

        vertices[0] = { { _x      , _y       }, { 0.0f, 0.0f }, color };
        vertices[1] = { { _x      , _y + _hL }, { 0.0f, 0.0f }, horizontalGradient ? colorEnd : color };
        vertices[2] = { { _x + _wL, _y       }, { 0.0f, 0.0f }, horizontalGradient ? color : colorEnd };
        vertices[3] = { { _x + _wL, _y + _hL }, { 0.0f, 0.0f }, colorEnd };

        // Round vertices.
        for (int i = 0; i < 4; ++i)
            vertices[i].pos.round();

        if (_opacity < 1.0) {
            vertices[0].shaders = SHADER_OPACITY;
            vertices[0].opacity = _opacity;
        }
        else {
            bindTexture(0);
        }
        drawTriangleStrips(vertices, 4, _trans, _srcBlendFactor, _dstBlendFactor);
    }

    unsigned int convertRGBAToABGR(const unsigned int _color)
    {
        unsigned char red = ((_color & 0xff000000) >> 24) & 255;
        unsigned char green = ((_color & 0x00ff0000) >> 16) & 255;
        unsigned char blue = ((_color & 0x0000ff00) >> 8) & 255;
        unsigned char alpha = ((_color & 0x000000ff)) & 255;

        return alpha << 24 | blue << 16 | green << 8 | red;
    }

    unsigned int convertABGRToRGBA(const unsigned int _color)
    {
        unsigned char alpha = ((_color & 0xff000000) >> 24) & 255;
        unsigned char blue = ((_color & 0x00ff0000) >> 16) & 255;
        unsigned char green = ((_color & 0x0000ff00) >> 8) & 255;
        unsigned char red = ((_color & 0x000000ff)) & 255;

        return red << 24 | green << 16 | blue << 8 | alpha;
    }

    Shader* getShaderProgram(unsigned int shaderID)
    {
        unsigned int index = 0;

        // Find the index in sShaderProgramVector by counting the number
        // of shifts required to reach 0.
        while (shaderID > 0) {
            shaderID = shaderID >> 1;
            index++;
        }

        if (sShaderProgramVector.size() > index-1)
            return sShaderProgramVector[index-1];
        else
            return nullptr;
    };

    const Transform4x4f getProjectionMatrix()
    {
        return mProjectionMatrix;
    }

    SDL_Window* getSDLWindow() { return sdlWindow; }
    int getWindowWidth() { return windowWidth; }
    int getWindowHeight() { return windowHeight; }
    int getScreenWidth() { return screenWidth; }
    int getScreenHeight() { return screenHeight; }
    int getScreenOffsetX() { return screenOffsetX; }
    int getScreenOffsetY() { return screenOffsetY; }
    int getScreenRotate() { return screenRotate; }
    float getScreenWidthModifier() { return screenWidthModifier; }
    float getScreenHeightModifier() { return screenHeightModifier; }

} // Renderer::

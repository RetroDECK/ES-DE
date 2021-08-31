//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Window.cpp
//
//  Window management, screensaver management, and help prompts.
//  The input stack starts here as well, as this is the first instance called by InputManager.
//

#include "Window.h"

#include "components/HelpComponent.h"
#include "components/ImageComponent.h"
#if defined(BUILD_VLC_PLAYER)
#include "components/VideoVlcComponent.h"
#endif
#include "AudioManager.h"
#include "InputManager.h"
#include "Log.h"
#include "Sound.h"
#include "resources/Font.h"

#include <algorithm>
#include <iomanip>

#define CLOCK_BACKGROUND_CREATION false

Window::Window()
    : mScreensaver(nullptr)
    , mMediaViewer(nullptr)
    , mLaunchScreen(nullptr)
    , mInfoPopup(nullptr)
    , mNormalizeNextUpdate(false)
    , mFrameTimeElapsed(0)
    , mFrameCountElapsed(0)
    , mAverageDeltaTime(10)
    , mAllowSleep(true)
    , mSleeping(false)
    , mTimeSinceLastInput(0)
    , mRenderScreensaver(false)
    , mRenderMediaViewer(false)
    , mRenderLaunchScreen(false)
    , mGameLaunchedState(false)
    , mAllowTextScrolling(true)
    , mCachedBackground(false)
    , mInvalidatedCachedBackground(false)
    , mVideoPlayerCount(0)
    , mTopScale(0.5)
    , mListScrollOpacity(0)
    , mChangedThemeSet(false)
{
    mHelp = new HelpComponent(this);
    mBackgroundOverlay = new ImageComponent(this);
}

Window::~Window()
{
    delete mBackgroundOverlay;

    // Delete all our GUIs.
    while (peekGui())
        delete peekGui();

    if (mInfoPopup)
        delete mInfoPopup;

    delete mHelp;
}

void Window::pushGui(GuiComponent* gui)
{
    if (mGuiStack.size() > 0) {
        auto& top = mGuiStack.back();
        top->topWindow(false);
    }
    mGuiStack.push_back(gui);
    gui->updateHelpPrompts();
}

void Window::removeGui(GuiComponent* gui)
{
    for (auto it = mGuiStack.cbegin(); it != mGuiStack.cend(); it++) {
        if (*it == gui) {
            it = mGuiStack.erase(it);

            // We just popped the stack and the stack is not empty.
            if (it == mGuiStack.cend() && mGuiStack.size()) {
                mGuiStack.back()->updateHelpPrompts();
                mGuiStack.back()->topWindow(true);
            }

            return;
        }
    }
}

GuiComponent* Window::peekGui()
{
    if (mGuiStack.size() == 0)
        return nullptr;

    return mGuiStack.back();
}

bool Window::init()
{
    if (!Renderer::init()) {
        LOG(LogError) << "Renderer failed to initialize.";
        return false;
    }

    InputManager::getInstance()->init();

    ResourceManager::getInstance()->reloadAll();

    // Keep a reference to the default fonts, so they don't keep getting destroyed/recreated.
    if (mDefaultFonts.empty()) {
        mDefaultFonts.push_back(Font::get(FONT_SIZE_SMALL));
        mDefaultFonts.push_back(Font::get(FONT_SIZE_MEDIUM));
        mDefaultFonts.push_back(Font::get(FONT_SIZE_LARGE));
    }

    mBackgroundOverlay->setImage(":/graphics/screen_gradient.png");
    mBackgroundOverlay->setResize(static_cast<float>(Renderer::getScreenWidth()),
                                  static_cast<float>(Renderer::getScreenHeight()));

    mListScrollFont = Font::get(FONT_SIZE_LARGE);

    // Update our help because font sizes probably changed.
    if (peekGui())
        peekGui()->updateHelpPrompts();

    return true;
}

void Window::deinit()
{
    // Hide all GUI elements on uninitialisation - this disable.
    for (auto it = mGuiStack.cbegin(); it != mGuiStack.cend(); it++)
        (*it)->onHide();

    InputManager::getInstance()->deinit();
    ResourceManager::getInstance()->unloadAll();
#if defined(BUILD_VLC_PLAYER)
    VideoVlcComponent::deinit();
#endif
    Renderer::deinit();
}

void Window::input(InputConfig* config, Input input)
{
    mTimeSinceLastInput = 0;

    // The DebugSkipInputLogging option has to be set manually in es_settings.xml as
    // it does not have any settings menu entry.
    if (Settings::getInstance()->getBool("Debug") &&
        !Settings::getInstance()->getBool("DebugSkipInputLogging")) {
        logInput(config, input);
    }

    if (mMediaViewer && mRenderMediaViewer) {
        if (config->isMappedLike("right", input) && input.value != 0)
            mMediaViewer->showNext();
        else if (config->isMappedLike("left", input) && input.value != 0)
            mMediaViewer->showPrevious();
        else if (input.value != 0)
            // Any other input than left or right stops the media viewer.
            stopMediaViewer();
        return;
    }

    if (mGameLaunchedState && mLaunchScreen && mRenderLaunchScreen) {
        if (input.value != 0) {
            mLaunchScreen->closeLaunchScreen();
            mRenderLaunchScreen = false;
        }
    }

    if (mScreensaver) {
        if (mScreensaver->isScreensaverActive() &&
            Settings::getInstance()->getBool("ScreensaverControls") &&
            ((Settings::getInstance()->getString("ScreensaverType") == "video") ||
             (Settings::getInstance()->getString("ScreensaverType") == "slideshow"))) {
            bool customImageSlideshow = false;
            if (Settings::getInstance()->getString("ScreensaverType") == "slideshow" &&
                Settings::getInstance()->getBool("ScreensaverSlideshowCustomImages"))
                customImageSlideshow = true;

            if ((customImageSlideshow || mScreensaver->getCurrentGame() != nullptr) &&
                (config->isMappedTo("a", input) || config->isMappedTo("y", input) ||
                 config->isMappedLike("left", input) || config->isMappedLike("right", input))) {
                // Left or right browses to the next video or image.
                if (config->isMappedLike("left", input) || config->isMappedLike("right", input)) {
                    if (input.value != 0) {
                        // Handle screensaver control.
                        mScreensaver->nextGame();
                    }
                    return;
                }
                else if (config->isMappedTo("a", input) && input.value != 0) {
                    // Launch game.
                    stopScreensaver();
                    mScreensaver->launchGame();
                    // To force handling the wake up process.
                    mSleeping = true;
                }
                else if (config->isMappedTo("y", input) && input.value != 0) {
                    // Jump to the game in its gamelist, but do not launch it.
                    stopScreensaver();
                    NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
                    mScreensaver->goToGame();
                    // To force handling the wake up process.
                    mSleeping = true;
                }
            }
        }
    }

    if (mSleeping) {
        // Wake up.
        stopScreensaver();
        mSleeping = false;
        onWake();
        return;
    }

    // Any keypress cancels the screensaver.
    if (input.value != 0 && isScreensaverActive()) {
        stopScreensaver();
        return;
    }

    if (config->isMappedTo("a", input) && input.value != 0 &&
        Settings::getInstance()->getString("MenuOpeningEffect") == "scale-up" && mTopScale < 1.0f &&
        mGuiStack.size() == 2) {
        // The user has entered a submenu when the initial menu screen has not finished scaling
        // up. So scale it to full size so it won't be stuck at a smaller size when returning
        // from the submenu.
        mTopScale = 1.0f;
        GuiComponent* menu = mGuiStack.back();
        glm::vec2 menuCenter{menu->getCenter()};
        menu->setOrigin(0.5f, 0.5f);
        menu->setPosition(menuCenter.x, menuCenter.y, 0.0f);
        menu->setScale(1.0f);
    }

    if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_g &&
        SDL_GetModState() & KMOD_LCTRL && Settings::getInstance()->getBool("Debug")) {
        // Toggle debug grid with Ctrl-G.
        Settings::getInstance()->setBool("DebugGrid",
                                         !Settings::getInstance()->getBool("DebugGrid"));
    }
    else if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_t &&
             SDL_GetModState() & KMOD_LCTRL && Settings::getInstance()->getBool("Debug")) {
        // Toggle TextComponent debug view with Ctrl-T.
        Settings::getInstance()->setBool("DebugText",
                                         !Settings::getInstance()->getBool("DebugText"));
    }
    else if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_i &&
             SDL_GetModState() & KMOD_LCTRL && Settings::getInstance()->getBool("Debug")) {
        // Toggle ImageComponent debug view with Ctrl-I.
        Settings::getInstance()->setBool("DebugImage",
                                         !Settings::getInstance()->getBool("DebugImage"));
    }
    else {
        if (peekGui())
            // This is where the majority of inputs will be consumed: the GuiComponent Stack.
            this->peekGui()->input(config, input);
    }
}

void Window::textInput(const std::string& text)
{
    if (peekGui())
        peekGui()->textInput(text);
}

void Window::logInput(InputConfig* config, Input input)
{
    std::string mapname = "";
    std::vector<std::string> maps = config->getMappedTo(input);

    for (auto mn : maps) {
        mapname += mn;
        mapname += ", ";
    }

    LOG(LogDebug) << "Window::logInput(" << config->getDeviceName() << "): " << input.string()
                  << ", isMappedTo=" << mapname << "value=" << input.value;
}

void Window::update(int deltaTime)
{
    if (mNormalizeNextUpdate) {
        mNormalizeNextUpdate = false;
        mTimeSinceLastInput = 0;
        if (deltaTime > mAverageDeltaTime)
            deltaTime = mAverageDeltaTime;
    }

    mFrameTimeElapsed += deltaTime;
    mFrameCountElapsed++;
    if (mFrameTimeElapsed > 500) {
        mAverageDeltaTime = mFrameTimeElapsed / mFrameCountElapsed;

        if (Settings::getInstance()->getBool("DisplayGPUStatistics")) {
            std::stringstream ss;

            // FPS.
            ss << std::fixed << std::setprecision(1)
               << (1000.0f * static_cast<float>(mFrameCountElapsed) /
                   static_cast<float>(mFrameTimeElapsed))
               << " FPS (";
            ss << std::fixed << std::setprecision(2)
               << (static_cast<float>(mFrameTimeElapsed) / static_cast<float>(mFrameCountElapsed))
               << " ms)";

            // The following calculations are not accurate, and the font calculation is completely
            // broken. For now, still report the figures as it's somehow useful to locate memory
            // leaks and similar. But this needs to be completely overhauled later on.
            // VRAM.
            float textureVramUsageMiB = TextureResource::getTotalMemUsage() / 1024.0f / 1024.0f;
            float textureTotalUsageMiB = TextureResource::getTotalTextureSize() / 1024.0f / 1024.0f;
            float fontVramUsageMiB = Font::getTotalMemUsage() / 1024.0f / 1024.0f;

            ss << "\nFont VRAM: " << fontVramUsageMiB
               << " MiB\nTexture VRAM: " << textureVramUsageMiB
               << " MiB\nMax Texture VRAM: " << textureTotalUsageMiB << " MiB";
            mFrameDataText = std::unique_ptr<TextCache>(mDefaultFonts.at(0)->buildTextCache(
                ss.str(), Renderer::getScreenWidth() * 0.02f, Renderer::getScreenHeight() * 0.02f,
                0xFF00FFFF, 1.3f));
        }

        mFrameTimeElapsed = 0;
        mFrameCountElapsed = 0;
    }

    mTimeSinceLastInput += deltaTime;

    if (peekGui())
        peekGui()->update(deltaTime);

    // If the theme set changed, we need to update the background once so that the camera
    // will be moved. This is required as theme set changes always makes a transition to
    // the system view. If we wouldn't make this update, the camera movement would take
    // place once the menu has been closed.
    if (mChangedThemeSet && mGuiStack.size() > 1) {
        mGuiStack.front()->update(deltaTime);
        mChangedThemeSet = false;
    }

    if (mMediaViewer && mRenderMediaViewer)
        mMediaViewer->update(deltaTime);

    if (mLaunchScreen && mRenderLaunchScreen)
        mLaunchScreen->update(deltaTime);

    if (mScreensaver && mRenderScreensaver)
        mScreensaver->update(deltaTime);
}

bool Window::isBackgroundDimmed()
{
    return !mGuiStack.empty() && (mGuiStack.front() != mGuiStack.back() || mRenderLaunchScreen);
}

void Window::render()
{
    glm::mat4 trans{Renderer::getIdentity()};

    mRenderedHelpPrompts = false;

    // Draw only bottom and top of GuiStack (if they are different).
    if (!mGuiStack.empty()) {
        auto& bottom = mGuiStack.front();
        auto& top = mGuiStack.back();

        if (mRenderMediaViewer || mRenderScreensaver) {
            bottom->cancelAllAnimations();
            bottom->stopAllAnimations();
        }

        // Don't render the system view or gamelist view if the media viewer is active or if the
        // video or slideshow screensaver is running. The exception is if the fallback screensaver
        // is active due to a lack of videos or images.
        bool renderBottom = true;
        if (mRenderMediaViewer)
            renderBottom = false;
        else if (mRenderScreensaver && mScreensaver->isFallbackScreensaver())
            renderBottom = true;
        else if (mRenderScreensaver &&
                 Settings::getInstance()->getString("ScreensaverType") == "video")
            renderBottom = false;
        else if (mRenderScreensaver &&
                 Settings::getInstance()->getString("ScreensaverType") == "slideshow")
            renderBottom = false;

        if (renderBottom)
            bottom->render(trans);

        if (bottom != top || mRenderLaunchScreen) {
#if defined(USE_OPENGL_21)
            if (!mCachedBackground) {
                // Generate a cache texture of the shaded background when opening the menu, which
                // will remain valid until the menu is closed. This is way faster than having to
                // render the shaders for every frame.
#if (CLOCK_BACKGROUND_CREATION)
                const auto backgroundStartTime = std::chrono::system_clock::now();
#endif

                std::shared_ptr<TextureResource> mPostprocessedBackground;
                mPostprocessedBackground = TextureResource::get("");
                unsigned char* processedTexture =
                    new unsigned char[Renderer::getScreenWidth() * Renderer::getScreenHeight() * 4];

                // De-focus the background using multiple passes of gaussian blur, with the number
                // of iterations relative to the screen resolution.
                Renderer::shaderParameters backgroundParameters;

                if (Settings::getInstance()->getBool("MenuBlurBackground")) {
                    float heightModifier = Renderer::getScreenHeightModifier();
                    // clang-format off
                    if (heightModifier < 1)
                        backgroundParameters.blurPasses = 2;        // Below 1080
                    else if (heightModifier >= 4)
                        backgroundParameters.blurPasses = 12;       // 8K
                    else if (heightModifier >= 2.9)
                        backgroundParameters.blurPasses = 10;       // 6K
                    else if (heightModifier >= 2.6)
                        backgroundParameters.blurPasses = 8;        // 5K
                    else if (heightModifier >= 2)
                        backgroundParameters.blurPasses = 5;        // 4K
                    else if (heightModifier >= 1.3)
                        backgroundParameters.blurPasses = 3;        // 1440
                    else if (heightModifier >= 1)
                        backgroundParameters.blurPasses = 2;        // 1080
                    // clang-format on

                    // Also dim the background slightly.
                    backgroundParameters.fragmentDimValue = 0.60f;

                    Renderer::shaderPostprocessing(Renderer::SHADER_BLUR_HORIZONTAL |
                                                       Renderer::SHADER_BLUR_VERTICAL |
                                                       Renderer::SHADER_DIM,
                                                   backgroundParameters, processedTexture);
                }
                else {
                    // Dim the background slightly.
                    backgroundParameters.fragmentDimValue = 0.60f;
                    Renderer::shaderPostprocessing(Renderer::SHADER_DIM, backgroundParameters,
                                                   processedTexture);
                }

                mPostprocessedBackground->initFromPixels(
                    processedTexture, Renderer::getScreenWidth(), Renderer::getScreenHeight());

                mBackgroundOverlay->setImage(mPostprocessedBackground);

                // The following is done to avoid fading in if the cached image was
                // invalidated (rather than the menu being opened).
                if (mInvalidatedCachedBackground) {
                    mBackgroundOverlayOpacity = 255;
                    mInvalidatedCachedBackground = false;
                }
                else {
                    mBackgroundOverlayOpacity = 25;
                }

                delete[] processedTexture;
                mCachedBackground = true;

#if (CLOCK_BACKGROUND_CREATION)
                const auto backgroundEndTime = std::chrono::system_clock::now();
                LOG(LogDebug) << "Window::render(): Time to create cached background: "
                              << std::chrono::duration_cast<std::chrono::milliseconds>(
                                     backgroundEndTime - backgroundStartTime)
                                     .count()
                              << " ms";
#endif
            }
            // Fade in the cached background if the menu opening effect has been set to scale-up.
            if (Settings::getInstance()->getString("MenuOpeningEffect") == "scale-up") {
                mBackgroundOverlay->setOpacity(mBackgroundOverlayOpacity);
                if (mBackgroundOverlayOpacity < 255)
                    mBackgroundOverlayOpacity = glm::clamp(mBackgroundOverlayOpacity + 30, 0, 255);
            }
#endif // USE_OPENGL_21

            mBackgroundOverlay->render(trans);

            // Scale-up menu opening effect.
            if (Settings::getInstance()->getString("MenuOpeningEffect") == "scale-up") {
                if (mTopScale < 1.0f) {
                    mTopScale = glm::clamp(mTopScale + 0.07f, 0.0f, 1.0f);
                    glm::vec2 topCenter{top->getCenter()};
                    top->setOrigin(0.5f, 0.5f);
                    top->setPosition(topCenter.x, topCenter.y, 0.0f);
                    top->setScale(mTopScale);
                }
            }

            if (!mRenderLaunchScreen)
                top->render(trans);
        }
        else {
            mCachedBackground = false;
            mTopScale = 0.5f;
        }
    }

    // Render the quick list scrolling overlay, which is triggered in IList.
    if (mListScrollOpacity != 0) {
        Renderer::setMatrix(Renderer::getIdentity());
        Renderer::drawRect(0.0f, 0.0f, static_cast<float>(Renderer::getScreenWidth()),
                           static_cast<float>(Renderer::getScreenHeight()),
                           0x00000000 | mListScrollOpacity, 0x00000000 | mListScrollOpacity);

        glm::vec2 offset{mListScrollFont->sizeText(mListScrollText)};
        offset.x = (Renderer::getScreenWidth() - offset.x) * 0.5f;
        offset.y = (Renderer::getScreenHeight() - offset.y) * 0.5f;

        TextCache* cache = mListScrollFont->buildTextCache(mListScrollText, offset.x, offset.y,
                                                           0xFFFFFF00 | mListScrollOpacity);
        mListScrollFont->renderTextCache(cache);
        delete cache;
    }

    if (!mRenderedHelpPrompts)
        mHelp->render(trans);

    unsigned int screensaverTimer =
        static_cast<unsigned int>(Settings::getInstance()->getInt("ScreensaverTimer"));
    if (mTimeSinceLastInput >= screensaverTimer && screensaverTimer != 0) {
        // If the media viewer is running or if a menu is open, reset the screensaver timer so
        // that the screensaver won't start.
        if (mRenderMediaViewer || mGuiStack.front() != mGuiStack.back())
            mTimeSinceLastInput = 0;
        // If a game has been launched, reset the screensaver timer as we don't want to start
        // the screensaver in the background when running a game.
        else if (mGameLaunchedState)
            mTimeSinceLastInput = 0;
        else if (!isProcessing() && !mScreensaver->isScreensaverActive())
            startScreensaver();
    }

    // Always call the screensaver render function regardless of whether the screensaver is active
    // or not because it may perform a fade on transition.
    renderScreensaver();

    if (!mRenderScreensaver && mInfoPopup)
        mInfoPopup->render(trans);

    if (mTimeSinceLastInput >= screensaverTimer && screensaverTimer != 0) {
        if (!isProcessing() && mAllowSleep && (!mScreensaver)) {
            // Go to sleep.
            if (mSleeping == false) {
                mSleeping = true;
                onSleep();
            }
        }
    }

    if (mRenderMediaViewer)
        mMediaViewer->render();

    if (mRenderLaunchScreen)
        mLaunchScreen->render();

    if (Settings::getInstance()->getBool("DisplayGPUStatistics") && mFrameDataText) {
        Renderer::setMatrix(Renderer::getIdentity());
        mDefaultFonts.at(1)->renderTextCache(mFrameDataText.get());
    }
}

void Window::renderLoadingScreen(std::string text)
{
    glm::mat4 trans{Renderer::getIdentity()};
    Renderer::setMatrix(trans);
    Renderer::drawRect(0.0f, 0.0f, static_cast<float>(Renderer::getScreenWidth()),
                       static_cast<float>(Renderer::getScreenHeight()), 0x000000FF, 0x000000FF);

    ImageComponent splash(this, true);
    splash.setResize(Renderer::getScreenWidth() * 0.6f, 0.0f);
    splash.setImage(":/graphics/splash.svg");
    splash.setPosition((Renderer::getScreenWidth() - splash.getSize().x) / 2.0f,
                       (Renderer::getScreenHeight() - splash.getSize().y) / 2.0f * 0.6f);
    splash.render(trans);

    auto& font = mDefaultFonts.at(1);
    TextCache* cache = font->buildTextCache(text, 0.0f, 0.0f, 0x656565FF);

    float x = std::round((Renderer::getScreenWidth() - cache->metrics.size.x) / 2.0f);
    float y = std::round(Renderer::getScreenHeight() * 0.835f);
    trans = glm::translate(trans, glm::vec3{x, y, 0.0f});
    Renderer::setMatrix(trans);
    font->renderTextCache(cache);
    delete cache;

    Renderer::swapBuffers();
}

void Window::renderListScrollOverlay(unsigned char opacity, const std::string& text)
{
    mListScrollOpacity = static_cast<unsigned char>(opacity * 0.6f);
    mListScrollText = text;
}

void Window::renderHelpPromptsEarly()
{
    mHelp->render(Renderer::getIdentity());
    mRenderedHelpPrompts = true;
}

void Window::setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style)
{
    mHelp->clearPrompts();
    mHelp->setStyle(style);

    std::vector<HelpPrompt> addPrompts;

    std::map<std::string, bool> inputSeenMap;
    std::map<std::string, int> mappedToSeenMap;
    for (auto it = prompts.cbegin(); it != prompts.cend(); it++) {
        // Only add it if the same icon hasn't already been added.
        if (inputSeenMap.emplace(it->first, true).second) {
            // This symbol hasn't been seen yet, what about the action name?
            auto mappedTo = mappedToSeenMap.find(it->second);
            if (mappedTo != mappedToSeenMap.cend()) {
                // Yes, it has!

                // Can we combine? (dpad only).
                if ((it->first == "up/down" &&
                     addPrompts.at(mappedTo->second).first != "left/right") ||
                    (it->first == "left/right" &&
                     addPrompts.at(mappedTo->second).first != "up/down")) {
                    // Yes.
                    addPrompts.at(mappedTo->second).first = "up/down/left/right";
                }
                else {
                    addPrompts.push_back(*it);
                }
            }
            else {
                mappedToSeenMap.emplace(it->second, static_cast<int>(addPrompts.size()));
                addPrompts.push_back(*it);
            }
        }
    }

    // Sort prompts so it goes [dpad_all] [dpad_u/d] [dpad_l/r] [a/b/x/y/l/r] [start/back].
    std::sort(addPrompts.begin(), addPrompts.end(),
              [](const HelpPrompt& a, const HelpPrompt& b) -> bool {
                  static const std::vector<std::string> map = {"up/down/left/right",
                                                               "up/down",
                                                               "left/right",
                                                               "a",
                                                               "b",
                                                               "x",
                                                               "y",
                                                               "l",
                                                               "r",
                                                               "start",
                                                               "back"};
                  int i = 0;
                  int aVal = 0;
                  int bVal = 0;
                  while (i < map.size()) {
                      if (a.first == map[i])
                          aVal = i;
                      if (b.first == map[i])
                          bVal = i;
                      i++;
                  }

                  return aVal > bVal;
              });

    mHelp->setPrompts(addPrompts);
}

void Window::reloadHelpPrompts()
{
    if (mHelp) {
        delete mHelp;
        mHelp = new HelpComponent(this);
    }
}

void Window::setInfoPopup(InfoPopup* infoPopup)
{
    delete mInfoPopup;
    mInfoPopup = infoPopup;
}

void Window::stopInfoPopup()
{
    if (mInfoPopup)
        mInfoPopup->stop();
}

void Window::startScreensaver()
{
    if (mScreensaver && !mRenderScreensaver) {
        // Tell the GUI components the screensaver is starting.
        for (auto it = mGuiStack.cbegin(); it != mGuiStack.cend(); it++)
            (*it)->onScreensaverActivate();

        stopInfoPopup();
        mScreensaver->startScreensaver(true);
        mRenderScreensaver = true;
    }
}

bool Window::stopScreensaver()
{
    if (mScreensaver && mRenderScreensaver) {
        mScreensaver->stopScreensaver();
        mRenderScreensaver = false;

        // Tell the GUI components the screensaver has stopped.
        for (auto it = mGuiStack.cbegin(); it != mGuiStack.cend(); it++) {
            (*it)->onScreensaverDeactivate();
            // If the menu is open, pause the video so it won't start playing beneath the menu.
            if (mGuiStack.front() != mGuiStack.back())
                (*it)->onPauseVideo();
        }

        return true;
    }

    return false;
}

void Window::renderScreensaver()
{
    if (mScreensaver)
        mScreensaver->renderScreensaver();
}

void Window::startMediaViewer(FileData* game)
{
    if (mMediaViewer) {
        if (mMediaViewer->startMediaViewer(game))
            mRenderMediaViewer = true;
    }
}

void Window::stopMediaViewer()
{
    if (mMediaViewer)
        mMediaViewer->stopMediaViewer();

    mRenderMediaViewer = false;
}

void Window::displayLaunchScreen(FileData* game)
{
    if (mLaunchScreen) {
        mLaunchScreen->displayLaunchScreen(game);
        mRenderLaunchScreen = true;
    }
}

void Window::closeLaunchScreen()
{
    if (mLaunchScreen)
        mLaunchScreen->closeLaunchScreen();

    mRenderLaunchScreen = false;
}

void Window::increaseVideoPlayerCount()
{
    mVideoCountMutex.lock();
    mVideoPlayerCount++;
    mVideoCountMutex.unlock();
}

void Window::decreaseVideoPlayerCount()
{
    mVideoCountMutex.lock();
    mVideoPlayerCount--;
    mVideoCountMutex.unlock();
}

int Window::getVideoPlayerCount()
{
    int videoPlayerCount;
    mVideoCountMutex.lock();
    videoPlayerCount = mVideoPlayerCount;
    mVideoCountMutex.unlock();
    return videoPlayerCount;
};

void Window::setLaunchedGame()
{
    // Tell the GUI components that a game has been launched.
    for (auto it = mGuiStack.cbegin(); it != mGuiStack.cend(); it++)
        (*it)->onGameLaunchedActivate();

    mGameLaunchedState = true;
}

void Window::unsetLaunchedGame()
{
    // Tell the GUI components that the user is back in ES-DE again.
    for (auto it = mGuiStack.cbegin(); it != mGuiStack.cend(); it++)
        (*it)->onGameLaunchedDeactivate();

    mGameLaunchedState = false;
}

void Window::invalidateCachedBackground()
{
    mCachedBackground = false;
    mInvalidatedCachedBackground = true;
}

bool Window::isProcessing()
{
    return count_if(mGuiStack.cbegin(), mGuiStack.cend(),
                    [](GuiComponent* c) { return c->isProcessing(); }) > 0;
}

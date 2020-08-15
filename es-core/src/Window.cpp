//
//  Window.cpp
//
//  Window management, screensaver and help prompts.
//

#include "Window.h"

#include "components/HelpComponent.h"
#include "components/ImageComponent.h"
#include "resources/Font.h"
#include "resources/TextureResource.h"
#include "InputManager.h"
#include "Log.h"
#include "Scripting.h"

#include <algorithm>
#include <iomanip>

Window::Window()
        : mNormalizeNextUpdate(false),
        mFrameTimeElapsed(0),
        mFrameCountElapsed(0),
        mAverageDeltaTime(10),
        mAllowSleep(true),
        mSleeping(false),
        mTimeSinceLastInput(0),
        mScreenSaver(nullptr),
        mRenderScreenSaver(false),
        mGameLaunchedState(false),
        mInfoPopup(nullptr)
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
        LOG(LogError) << "Renderer failed to initialize!";
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

    mBackgroundOverlay->setImage(":/graphics/scroll_gradient.png");
    mBackgroundOverlay->setResize((float)Renderer::getScreenWidth(),
            (float)Renderer::getScreenHeight());

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
    Renderer::deinit();
}

void Window::textInput(const char* text)
{
    if (peekGui())
        peekGui()->textInput(text);
}

void Window::input(InputConfig* config, Input input)
{
    if (mScreenSaver) {
        if (mScreenSaver->isScreenSaverActive() &&
                Settings::getInstance()->getBool("ScreenSaverControls") &&
                ((Settings::getInstance()->getString("ScreenSaverBehavior") == "video") ||
                (Settings::getInstance()->getString("ScreenSaverBehavior") == "slideshow"))) {
            if (mScreenSaver->getCurrentGame() != nullptr &&
                    (config->isMappedTo("a", input) ||
                    config->isMappedLike("left", input) || config->isMappedLike("right", input))) {
                // Left or right browses to the next video or image.
                if (config->isMappedLike("left", input) || config->isMappedLike("right", input)) {
                    if (input.value != 0) {
                        // Handle screensaver control.
                        mScreenSaver->nextGame();
                    }
                    return;
                }
                else if (config->isMappedTo("a", input) && input.value != 0) {
                    // Launch game.
                    cancelScreenSaver();
                    mScreenSaver->launchGame();
                    // To force handling the wake up process.
                    mSleeping = true;
                }
            }
        }
    }

    if (mSleeping) {
        // Wake up.
        mTimeSinceLastInput = 0;
        cancelScreenSaver();
        mSleeping = false;
        onWake();
        return;
    }

    mTimeSinceLastInput = 0;
    if (!config->isMappedTo("select", input))
        if (cancelScreenSaver())
            return;

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
        // Toggle TextComponent debug view with Ctrl-I.
        Settings::getInstance()->setBool("DebugImage",
                !Settings::getInstance()->getBool("DebugImage"));
    }
    else {
        if (peekGui())
            // This is where the majority of inputs will be consumed: the GuiComponent Stack.
            this->peekGui()->input(config, input);
    }
}

void Window::update(int deltaTime)
{
    if (mNormalizeNextUpdate) {
        mNormalizeNextUpdate = false;
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
            ss << std::fixed << std::setprecision(1) <<
                    (1000.0f * (float)mFrameCountElapsed / (float)mFrameTimeElapsed) << " FPS (";
            ss << std::fixed << std::setprecision(2) <<
                    ((float)mFrameTimeElapsed / (float)mFrameCountElapsed) << " ms)";

            // The following calculations are not accurate, and the font calculation is completely
            // broken. For now, still report the figures as it's somehow useful to locate memory
            // leaks and similar. But this needs to be completely overhauled later on.
            // VRAM.
            float textureVramUsageMiB = TextureResource::getTotalMemUsage() / 1024.0f / 1024.0f;
            float textureTotalUsageMiB = TextureResource::getTotalTextureSize() / 1024.0f / 1024.0f;
            float fontVramUsageMiB = Font::getTotalMemUsage() / 1024.0f / 1024.0f;

            ss << "\nFont VRAM: " << fontVramUsageMiB << " MiB\nTexture VRAM: " <<
                    textureVramUsageMiB << " MiB\nMax Texture VRAM: " <<
                    textureTotalUsageMiB << " MiB";
            mFrameDataText = std::unique_ptr<TextCache>
                    (mDefaultFonts.at(1)->buildTextCache(ss.str(), 30.f, 30.f, 0xFF00FFFF));
        }

        mFrameTimeElapsed = 0;
        mFrameCountElapsed = 0;
    }

    mTimeSinceLastInput += deltaTime;

    if (peekGui())
        peekGui()->update(deltaTime);

    // Update the screensaver.
    if (mScreenSaver)
        mScreenSaver->update(deltaTime);
}

void Window::render()
{
    Transform4x4f transform = Transform4x4f::Identity();

    mRenderedHelpPrompts = false;

    // Draw only bottom and top of GuiStack (if they are different).
    if (mGuiStack.size()) {
        auto& bottom = mGuiStack.front();
        auto& top = mGuiStack.back();

        bottom->render(transform);
        if (bottom != top) {
            mBackgroundOverlay->render(transform);
            top->render(transform);
        }
    }

    if (!mRenderedHelpPrompts)
        mHelp->render(transform);

    if (Settings::getInstance()->getBool("DisplayGPUStatistics") && mFrameDataText) {
        Renderer::setMatrix(Transform4x4f::Identity());
        mDefaultFonts.at(1)->renderTextCache(mFrameDataText.get());
    }

    unsigned int screensaverTime = (unsigned int)Settings::getInstance()->getInt("ScreenSaverTime");
    // If a game has been launched, reset the screensaver timer when it's been reached as we
    // don't want to start the screensaver in the background when running a game.
    if (mTimeSinceLastInput >= screensaverTime && screensaverTime != 0) {
        if (mGameLaunchedState)
            mTimeSinceLastInput = 0;
        else if (!isProcessing() && !mScreenSaver->isScreenSaverActive())
            startScreenSaver();
    }

    // Always call the screensaver render function regardless of whether the screensaver is active
    // or not because it may perform a fade on transition.
    renderScreenSaver();

    if (!mRenderScreenSaver && mInfoPopup)
        mInfoPopup->render(transform);

    if (mTimeSinceLastInput >= screensaverTime && screensaverTime != 0) {
        if (!isProcessing() && mAllowSleep && (!mScreenSaver || mScreenSaver->allowSleep())) {
            // Go to sleep.
            if (mSleeping == false) {
                mSleeping = true;
                onSleep();
            }
        }
    }
}

void Window::normalizeNextUpdate()
{
    mNormalizeNextUpdate = true;
}

bool Window::getAllowSleep()
{
    return mAllowSleep;
}

void Window::setAllowSleep(bool sleep)
{
    mAllowSleep = sleep;
}

void Window::renderLoadingScreen(std::string text)
{
    Transform4x4f trans = Transform4x4f::Identity();
    Renderer::setMatrix(trans);
    Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(),
            Renderer::getScreenHeight(), 0x000000FF, 0x000000FF);

    ImageComponent splash(this, true);
    splash.setResize(Renderer::getScreenWidth() * 0.6f, 0.0f);
    splash.setImage(":/graphics/splash.svg");
    splash.setPosition((Renderer::getScreenWidth() - splash.getSize().x()) / 2,
            (Renderer::getScreenHeight() - splash.getSize().y()) / 2 * 0.6f);
    splash.render(trans);

    auto& font = mDefaultFonts.at(1);
    TextCache* cache = font->buildTextCache(text, 0, 0, 0x656565FF);

    float x = Math::round((Renderer::getScreenWidth() - cache->metrics.size.x()) / 2.0f);
    float y = Math::round(Renderer::getScreenHeight() * 0.835f);
    trans = trans.translate(Vector3f(x, y, 0.0f));
    Renderer::setMatrix(trans);
    font->renderTextCache(cache);
    delete cache;

    Renderer::swapBuffers();
}

void Window::renderHelpPromptsEarly()
{
    mHelp->render(Transform4x4f::Identity());
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
                    // Yes!
                    addPrompts.at(mappedTo->second).first = "up/down/left/right";
                    // Don't need to add this to addPrompts since we just merged.
                }
                else {
                    // No, we can't combine!
                    addPrompts.push_back(*it);
                }
            }
            else {
                // No, it hasn't!
                mappedToSeenMap.emplace(it->second, (int)addPrompts.size());
                addPrompts.push_back(*it);
            }
        }
    }

    // Sort prompts so it goes [dpad_all] [dpad_u/d] [dpad_l/r] [a/b/x/y/l/r] [start/select].
    std::sort(addPrompts.begin(), addPrompts.end(),
            [](const HelpPrompt& a, const HelpPrompt& b) -> bool {

        static const char* map[] = {
            "up/down/left/right",
            "up/down",
            "left/right",
            "a", "b", "x", "y", "l", "r",
            "start", "select",
            nullptr
        };

        int i = 0;
        int aVal = 0;
        int bVal = 0;
        while (map[i] != nullptr) {
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

void Window::onSleep()
{
    Scripting::fireEvent("sleep");
}

void Window::onWake()
{
    Scripting::fireEvent("wake");
}

bool Window::isProcessing()
{
    return count_if (mGuiStack.cbegin(), mGuiStack.cend(),
            [](GuiComponent* c) { return c->isProcessing(); }) > 0;
}

void Window::setLaunchedGame()
{
    // Tell the GUI components that a game has been launched.
    for (auto it = mGuiStack.cbegin(); it != mGuiStack.cend(); it++)
        (*it)->onGameLaunchedActivate();

    mGameLaunchedState = true;
}

void Window::unsetLaunchedGame()
{
    // Tell the GUI components that the user is back in ES again.
    for (auto it = mGuiStack.cbegin(); it != mGuiStack.cend(); it++)
        (*it)->onGameLaunchedDeactivate();

    mGameLaunchedState = false;
}

void Window::startScreenSaver()
{
    if (mScreenSaver && !mRenderScreenSaver) {
        // Tell the GUI components the screensaver is starting.
        for (auto it = mGuiStack.cbegin(); it != mGuiStack.cend(); it++)
            (*it)->onScreenSaverActivate();

        mScreenSaver->startScreenSaver();
        mRenderScreenSaver = true;
    }
}

bool Window::cancelScreenSaver()
{
    if (mScreenSaver && mRenderScreenSaver) {
        mScreenSaver->stopScreenSaver();
        mRenderScreenSaver = false;
        mScreenSaver->resetCounts();

        // Tell the GUI components the screensaver has stopped.
        for (auto it = mGuiStack.cbegin(); it != mGuiStack.cend(); it++)
            (*it)->onScreenSaverDeactivate();

        return true;
    }

    return false;
}

void Window::renderScreenSaver()
{
    if (mScreenSaver)
        mScreenSaver->renderScreenSaver();
}

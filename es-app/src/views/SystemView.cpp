//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SystemView.cpp
//
//  Main system view.
//

#include "views/SystemView.h"

#include "Log.h"
#include "Settings.h"
#include "Sound.h"
#include "UIModeController.h"
#include "Window.h"
#include "animations/LambdaAnimation.h"
#include "guis/GuiMsgBox.h"
#include "views/ViewController.h"

#if defined(_WIN64)
#include <cmath>
#endif

namespace
{
    // Buffer values for scrolling velocity (left, stopped, right).
    const int logoBuffersLeft[] {-5, -2, -1};
    const int logoBuffersRight[] {1, 2, 5};

} // namespace

SystemView::SystemView()
    : mCamOffset {0.0f}
    , mFadeOpacity {0.0f}
    , mPreviousScrollVelocity {0}
    , mUpdatedGameCount {false}
    , mViewNeedsReload {true}
    , mLegacyMode {false}
    , mHoldingKey {false}
    , mNavigated {false}
{
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());

    mCarousel = std::make_unique<CarouselComponent>();
    mCarousel->setCursorChangedCallback([&](const CursorState& state) { onCursorChanged(state); });
    mCarousel->setCancelTransitionsCallback([&] {
        ViewController::getInstance()->cancelViewTransitions();
        mNavigated = true;
        for (auto& anim : mSystemElements[mCarousel->getCursor()].lottieAnimComponents)
            anim->setPauseAnimation(true);
    });

    populate();
}

SystemView::~SystemView()
{
    if (mLegacyMode) {
        // Delete any existing extras.
        for (auto& entry : mSystemElements) {
            for (auto extra : entry.legacyExtras)
                delete extra;
            entry.legacyExtras.clear();
        }
    }
}

void SystemView::onTransition()
{
    for (auto& anim : mSystemElements[mCarousel->getCursor()].lottieAnimComponents)
        anim->setPauseAnimation(true);
}

void SystemView::goToSystem(SystemData* system, bool animate)
{
    mCarousel->setCursor(system);

    for (auto& selector : mSystemElements[mCarousel->getCursor()].gameSelectors) {
        if (selector->getGameSelection() == GameSelectorComponent::GameSelection::RANDOM)
            selector->setNeedsRefresh();
    }

    for (auto& video : mSystemElements[mCarousel->getCursor()].videoComponents)
        video->setStaticVideo();

    for (auto& anim : mSystemElements[mCarousel->getCursor()].lottieAnimComponents)
        anim->resetFileAnimation();

    updateGameSelectors();
    updateGameCount();
    startViewVideos();

    if (!animate)
        finishSystemAnimation(0);
}

bool SystemView::input(InputConfig* config, Input input)
{
    mNavigated = false;

    if (input.value != 0) {
        mHoldingKey = true;

        if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_r &&
            SDL_GetModState() & KMOD_LCTRL && Settings::getInstance()->getBool("Debug")) {
            LOG(LogDebug) << "SystemView::input(): Reloading all";
            ViewController::getInstance()->reloadAll();
            return true;
        }

        if (config->isMappedTo("a", input)) {
            mCarousel->stopScrolling();
            pauseViewVideos();
            ViewController::getInstance()->goToGamelist(mCarousel->getSelected());
            NavigationSounds::getInstance().playThemeNavigationSound(SELECTSOUND);
            return true;
        }
        if (Settings::getInstance()->getBool("RandomAddButton") &&
            (config->isMappedTo("leftthumbstickclick", input) ||
             config->isMappedTo("rightthumbstickclick", input))) {
            // Get a random system and jump to it.
            NavigationSounds::getInstance().playThemeNavigationSound(SYSTEMBROWSESOUND);
            mCarousel->setCursor(SystemData::getRandomSystem(mCarousel->getSelected()));
            return true;
        }

        if (!UIModeController::getInstance()->isUIModeKid() && config->isMappedTo("back", input) &&
            Settings::getInstance()->getBool("ScreensaverControls")) {
            if (!mWindow->isScreensaverActive()) {
                ViewController::getInstance()->stopScrolling();
                ViewController::getInstance()->cancelViewTransitions();
                mWindow->startScreensaver();
                mWindow->renderScreensaver();
            }
            return true;
        }
    }
    else {
        mHoldingKey = false;
    }

    return mCarousel->input(config, input);
}

void SystemView::update(int deltaTime)
{
    mCarousel->update(deltaTime);

    for (auto& video : mSystemElements[mCarousel->getCursor()].videoComponents)
        video->update(deltaTime);

    for (auto& anim : mSystemElements[mCarousel->getCursor()].lottieAnimComponents)
        anim->update(deltaTime);

    GuiComponent::update(deltaTime);
}

void SystemView::render(const glm::mat4& parentTrans)
{
    if (mCarousel->getNumEntries() == 0)
        return; // Nothing to render.

    bool fade {false};

    if (mNavigated && mCarousel->isAnimationPlaying(0) &&
        Settings::getInstance()->getString("TransitionStyle") == "fade")
        fade = true;

    if (!fade)
        renderElements(parentTrans, false);
    glm::mat4 trans {getTransform() * parentTrans};

    // During fade transitions draw a black rectangle above all elements placed below the carousel.
    if (mFadeOpacity != 0.0f) {
        unsigned int fadeColor {0x00000000 | static_cast<unsigned int>(mFadeOpacity * 255.0f)};
        Renderer::setMatrix(trans);
        Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, fadeColor, fadeColor);
    }

    mCarousel->render(trans);

    // For legacy themes the carousel is always rendered on top of all other elements.
    if (!mLegacyMode)
        renderElements(parentTrans, true);
}

void SystemView::onThemeChanged(const std::shared_ptr<ThemeData>& /*theme*/)
{
    LOG(LogDebug) << "SystemView::onThemeChanged()";
    mViewNeedsReload = true;
    populate();
}

std::vector<HelpPrompt> SystemView::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (mCarousel->getType() == CarouselComponent::VERTICAL ||
        mCarousel->getType() == CarouselComponent::VERTICAL_WHEEL)
        prompts.push_back(HelpPrompt("up/down", "choose"));
    else
        prompts.push_back(HelpPrompt("left/right", "choose"));

    prompts.push_back(HelpPrompt("a", "select"));

    if (Settings::getInstance()->getBool("RandomAddButton"))
        prompts.push_back(HelpPrompt("thumbstickclick", "random"));

    if (!UIModeController::getInstance()->isUIModeKid() &&
        Settings::getInstance()->getBool("ScreensaverControls"))
        prompts.push_back(HelpPrompt("back", "screensaver"));

    return prompts;
}

HelpStyle SystemView::getHelpStyle()
{
    HelpStyle style;
    style.applyTheme(mCarousel->getEntry(mCarousel->getCursor()).object->getTheme(), "system");
    return style;
}

void SystemView::onCursorChanged(const CursorState& /*state*/)
{
    int cursor {mCarousel->getCursor()};

    for (auto& selector : mSystemElements[cursor].gameSelectors) {
        if (selector->getGameSelection() == GameSelectorComponent::GameSelection::RANDOM)
            selector->setNeedsRefresh();
    }

    for (auto& video : mSystemElements[cursor].videoComponents)
        video->setStaticVideo();

    for (auto& anim : mSystemElements[mCarousel->getCursor()].lottieAnimComponents)
        anim->resetFileAnimation();

    updateGameSelectors();
    startViewVideos();
    updateHelpPrompts();

    int scrollVelocity {mCarousel->getScrollingVelocity()};
    float startPos {mCamOffset};
    float posMax {static_cast<float>(mCarousel->getNumEntries())};
    float target {static_cast<float>(cursor)};

    // Find the shortest path to the target.
    float endPos {target}; // Directly.
    float dist {fabs(endPos - startPos)};

    if (fabs(target + posMax - startPos - scrollVelocity) < dist)
        endPos = target + posMax; // Loop around the end (0 -> max).
    if (fabs(target - posMax - startPos - scrollVelocity) < dist)
        endPos = target - posMax; // Loop around the start (max - 1 -> -1).

    // Make sure transitions do not animate in reverse.
    bool changedDirection {false};
    if (mPreviousScrollVelocity != 0 && mPreviousScrollVelocity != scrollVelocity)
        changedDirection = true;

    if (!changedDirection && scrollVelocity > 0 && endPos < startPos)
        endPos = endPos + posMax;

    if (!changedDirection && scrollVelocity < 0 && endPos > startPos)
        endPos = endPos - posMax;

    if (scrollVelocity != 0)
        mPreviousScrollVelocity = scrollVelocity;

    std::string transition_style {Settings::getInstance()->getString("TransitionStyle")};

    Animation* anim;

    if (transition_style == "fade") {
        float startFade {mFadeOpacity};
        anim = new LambdaAnimation(
            [this, startFade, startPos, endPos, posMax](float t) {
                t -= 1;
                float f {glm::mix(startPos, endPos, t * t * t + 1.0f)};
                if (f < 0.0f)
                    f += posMax;
                if (f >= posMax)
                    f -= posMax;

                t += 1;

                if (t < 0.3f)
                    mFadeOpacity =
                        glm::mix(0.0f, 1.0f, glm::clamp(t / 0.2f + startFade, 0.0f, 1.0f));
                else if (t < 0.7f)
                    mFadeOpacity = 1.0f;
                else
                    mFadeOpacity = glm::mix(1.0f, 0.0f, glm::clamp((t - 0.6f) / 0.3f, 0.0f, 1.0f));

                if (t > 0.5f)
                    mCamOffset = endPos;

                // Update the game count when the entire animation has been completed.
                if (mFadeOpacity == 1.0f)
                    updateGameCount();
            },
            500);
    }
    else if (transition_style == "slide") {
        mUpdatedGameCount = false;
        anim = new LambdaAnimation(
            [this, startPos, endPos, posMax](float t) {
                t -= 1;
                float f {glm::mix(startPos, endPos, t * t * t + 1.0f)};
                if (f < 0.0f)
                    f += posMax;
                if (f >= posMax)
                    f -= posMax;

                mCamOffset = f;

                // Hack to make the game count being updated in the middle of the animation.
                bool update {false};
                if (endPos == -1.0f && fabs(fabs(posMax) - fabs(mCamOffset)) > 0.5f &&
                    !mUpdatedGameCount) {
                    update = true;
                }
                else if (endPos > posMax && fabs(endPos - posMax - fabs(mCamOffset)) < 0.5f &&
                         !mUpdatedGameCount) {
                    update = true;
                }
                else if (fabs(fabs(endPos) - fabs(mCamOffset)) < 0.5f && !mUpdatedGameCount) {
                    update = true;
                }

                if (update) {
                    mUpdatedGameCount = true;
                    updateGameCount();
                }
            },
            500);
    }
    else {
        // Instant.
        updateGameCount();
        anim = new LambdaAnimation(
            [this, startPos, endPos, posMax](float t) {
                t -= 1;
                float f {glm::mix(startPos, endPos, t * t * t + 1.0f)};
                if (f < 0.0f)
                    f += posMax;
                if (f >= posMax)
                    f -= posMax;

                mCamOffset = endPos;
            },
            500);
    }

    setAnimation(anim, 0, nullptr, false, 0);
}

void SystemView::populate()
{
    LOG(LogDebug) << "SystemView::populate(): Populating carousel";

    auto themeSets = ThemeData::getThemeSets();
    std::map<std::string, ThemeData::ThemeSet>::const_iterator selectedSet {
        themeSets.find(Settings::getInstance()->getString("ThemeSet"))};

    assert(selectedSet != themeSets.cend());
    mLegacyMode = selectedSet->second.capabilities.legacyTheme;

    if (mLegacyMode) {
        mLegacySystemInfo = std::make_unique<TextComponent>(
            "SYSTEM INFO", Font::get(FONT_SIZE_SMALL), 0x33333300, ALIGN_CENTER);
    }

    for (auto it : SystemData::sSystemVector) {
        const std::shared_ptr<ThemeData>& theme {it->getTheme()};
        std::string logoPath;
        std::string defaultLogoPath;

        if (mLegacyMode && mViewNeedsReload)
            legacyApplyTheme(theme);

        if (mLegacyMode) {
            SystemViewElements elements;
            elements.name = it->getName();
            elements.legacyExtras = ThemeData::makeExtras(theme, "system");

            // Sort the extras by z-index.
            std::stable_sort(
                elements.legacyExtras.begin(), elements.legacyExtras.end(),
                [](GuiComponent* a, GuiComponent* b) { return b->getZIndex() > a->getZIndex(); });

            mSystemElements.emplace_back(std::move(elements));
        }

        if (!mLegacyMode) {
            SystemViewElements elements;
            if (theme->hasView("system")) {
                elements.name = it->getName();
                elements.fullName = it->getFullName();
                for (auto& element : theme->getViewElements("system").elements) {
                    if (element.second.type == "gameselector") {
                        elements.gameSelectors.emplace_back(
                            std::make_unique<GameSelectorComponent>(it));
                        elements.gameSelectors.back()->applyTheme(theme, "system", element.first,
                                                                  ThemeFlags::ALL);
                        elements.gameSelectors.back()->setNeedsRefresh();
                    }
                    if (element.second.type == "carousel") {
                        mCarousel->applyTheme(theme, "system", element.first, ThemeFlags::ALL);
                        if (element.second.has("logo"))
                            logoPath = element.second.get<std::string>("logo");
                        if (element.second.has("defaultLogo"))
                            defaultLogoPath = element.second.get<std::string>("defaultLogo");
                    }
                    else if (element.second.type == "image") {
                        elements.imageComponents.emplace_back(std::make_unique<ImageComponent>());
                        elements.imageComponents.back()->setDefaultZIndex(30.0f);
                        elements.imageComponents.back()->applyTheme(theme, "system", element.first,
                                                                    ThemeFlags::ALL);
                        elements.children.emplace_back(elements.imageComponents.back().get());
                    }
                    else if (element.second.type == "video") {
                        elements.videoComponents.emplace_back(
                            std::make_unique<VideoFFmpegComponent>());
                        elements.videoComponents.back()->setDefaultZIndex(30.0f);
                        elements.videoComponents.back()->setStaticVideo();
                        elements.videoComponents.back()->applyTheme(theme, "system", element.first,
                                                                    ThemeFlags::ALL);
                        elements.children.emplace_back(elements.videoComponents.back().get());
                    }
                    else if (element.second.type == "animation") {
                        elements.lottieAnimComponents.emplace_back(
                            std::make_unique<LottieAnimComponent>());
                        elements.lottieAnimComponents.back()->setDefaultZIndex(35.0f);
                        elements.lottieAnimComponents.back()->applyTheme(
                            theme, "system", element.first, ThemeFlags::ALL);
                        elements.children.emplace_back(elements.lottieAnimComponents.back().get());
                    }
                    else if (element.second.type == "text") {
                        if (element.second.has("systemdata") &&
                            element.second.get<std::string>("systemdata").substr(0, 9) ==
                                "gamecount") {
                            if (element.second.has("systemdata")) {
                                elements.gameCountComponents.emplace_back(
                                    std::make_unique<TextComponent>());
                                elements.gameCountComponents.back()->setDefaultZIndex(40.0f);
                                elements.gameCountComponents.back()->applyTheme(
                                    theme, "system", element.first, ThemeFlags::ALL);
                                elements.children.emplace_back(
                                    elements.gameCountComponents.back().get());
                            }
                        }
                        else {
                            elements.textComponents.emplace_back(std::make_unique<TextComponent>());
                            elements.textComponents.back()->setDefaultZIndex(40.0f);
                            elements.textComponents.back()->applyTheme(
                                theme, "system", element.first, ThemeFlags::ALL);
                            elements.children.emplace_back(elements.textComponents.back().get());
                        }
                    }
                    else if (element.second.type == "datetime") {
                        elements.dateTimeComponents.emplace_back(
                            std::make_unique<DateTimeComponent>());
                        elements.dateTimeComponents.back()->setDefaultZIndex(40.0f);
                        elements.dateTimeComponents.back()->applyTheme(
                            theme, "system", element.first, ThemeFlags::ALL);
                        elements.children.emplace_back(elements.dateTimeComponents.back().get());
                    }
                }
            }
            else {
                // Apply default carousel configuration.
                mCarousel->applyTheme(theme, "system", "", ThemeFlags::ALL);
            }

            std::stable_sort(
                elements.children.begin(), elements.children.end(),
                [](GuiComponent* a, GuiComponent* b) { return b->getZIndex() > a->getZIndex(); });

            std::stable_sort(elements.imageComponents.begin(), elements.imageComponents.end(),
                             [](const std::unique_ptr<ImageComponent>& a,
                                const std::unique_ptr<ImageComponent>& b) {
                                 return b->getZIndex() > a->getZIndex();
                             });
            std::stable_sort(elements.textComponents.begin(), elements.textComponents.end(),
                             [](const std::unique_ptr<TextComponent>& a,
                                const std::unique_ptr<TextComponent>& b) {
                                 return b->getZIndex() > a->getZIndex();
                             });
            mSystemElements.emplace_back(std::move(elements));
        }

        CarouselComponent::Entry entry;
        entry.name = it->getName();
        entry.object = it;
        entry.data.logoPath = logoPath;
        entry.data.defaultLogoPath = defaultLogoPath;

        mCarousel->addEntry(theme, entry, mLegacyMode);
    }

    for (auto& elements : mSystemElements) {
        for (auto& text : elements.textComponents) {
            if (text->getThemeSystemdata() != "") {
                if (text->getThemeSystemdata() == "name")
                    text->setValue(elements.name);
                else if (text->getThemeSystemdata() == "fullname")
                    text->setValue(elements.fullName);
                else
                    text->setValue(text->getThemeSystemdata());
            }
        }
    }

    if (mCarousel->getNumEntries() == 0) {
        // Something is wrong, there is not a single system to show, check if UI mode is not full.
        if (!UIModeController::getInstance()->isUIModeFull()) {
            Settings::getInstance()->setString("UIMode", "full");
            mWindow->pushGui(new GuiMsgBox(
                getHelpStyle(),
                "The selected UI mode has nothing to show,\n returning to UI mode \"Full\"", "OK",
                nullptr));
        }
    }
}

void SystemView::updateGameCount()
{
    std::pair<unsigned int, unsigned int> gameCount =
        mCarousel->getSelected()->getDisplayedGameCount();
    std::stringstream ss;
    std::stringstream ssGames;
    std::stringstream ssFavorites;
    bool games {false};

    if (!mCarousel->getSelected()->isGameSystem()) {
        ss << "Configuration";
    }
    else if (mCarousel->getSelected()->isCollection() &&
             (mCarousel->getSelected()->getName() == "favorites")) {
        ss << gameCount.first << " Game" << (gameCount.first == 1 ? " " : "s");
    }
    else if (mCarousel->getSelected()->isCollection() &&
             (mCarousel->getSelected()->getName() == "recent")) {
        // The "recent" gamelist has probably been trimmed after sorting, so we'll cap it at
        // its maximum limit of 50 games.
        ss << (gameCount.first > 50 ? 50 : gameCount.first) << " Game"
           << (gameCount.first == 1 ? " " : "s");
    }
    else {
        ss << gameCount.first << " Game" << (gameCount.first == 1 ? " " : "s ") << "("
           << gameCount.second << " Favorite" << (gameCount.second == 1 ? ")" : "s)");
        ssGames << gameCount.first << " Game" << (gameCount.first == 1 ? " " : "s ");
        ssFavorites << gameCount.second << " Favorite" << (gameCount.second == 1 ? "" : "s");
        games = true;
    }

    if (mLegacyMode) {
        mLegacySystemInfo->setText(ss.str());
    }
    else {
        for (auto& gameCount : mSystemElements[mCarousel->getCursor()].gameCountComponents) {
            if (gameCount->getThemeSystemdata() == "gamecount") {
                gameCount->setValue(ss.str());
            }
            else if (gameCount->getThemeSystemdata() == "gamecount_games") {
                if (games)
                    gameCount->setValue(ssGames.str());
                else
                    gameCount->setValue(ss.str());
            }
            else if (gameCount->getThemeSystemdata() == "gamecount_favorites") {
                gameCount->setValue(ssFavorites.str());
            }
            else {
                gameCount->setValue(gameCount->getThemeSystemdata());
            }
        }
    }
}

void SystemView::updateGameSelectors()
{
    if (mLegacyMode)
        return;

    int cursor {mCarousel->getCursor()};

    if (mSystemElements[cursor].gameSelectors.size() == 0)
        return;

    bool multipleSelectors {mSystemElements[cursor].gameSelectors.size() > 1};

    for (auto& image : mSystemElements[cursor].imageComponents) {
        if (image->getThemeImageTypes().size() == 0)
            continue;
        GameSelectorComponent* gameSelector {nullptr};
        if (multipleSelectors) {
            const std::string& imageSelector {image->getThemeGameSelector()};
            if (imageSelector == "") {
                gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                LOG(LogWarning) << "SystemView::updateGameSelectors(): Multiple gameselector "
                                   "elements defined but image element does not state which one to "
                                   "use, selecting first entry";
            }
            else {
                for (auto& selector : mSystemElements[cursor].gameSelectors) {
                    if (selector->getSelectorName() == imageSelector)
                        gameSelector = selector.get();
                }
                if (gameSelector == nullptr) {
                    LOG(LogWarning)
                        << "SystemView::updateGameSelectors(): Invalid gameselector \""
                        << imageSelector << "\" defined for image element, selecting first entry";
                    gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                }
            }
        }
        else {
            gameSelector = mSystemElements[cursor].gameSelectors.front().get();
        }
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (!games.empty()) {
            std::string path;
            for (auto& imageType : image->getThemeImageTypes()) {
                if (imageType == "image") {
                    path = games.front()->getImagePath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "miximage") {
                    path = games.front()->getMiximagePath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "marquee") {
                    path = games.front()->getMarqueePath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "screenshot") {
                    path = games.front()->getScreenshotPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "titlescreen") {
                    path = games.front()->getTitleScreenPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "cover") {
                    path = games.front()->getCoverPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "backcover") {
                    path = games.front()->getBackCoverPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "3dbox") {
                    path = games.front()->get3DBoxPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "fanart") {
                    path = games.front()->getFanArtPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "thumbnail") {
                    path = games.front()->getThumbnailPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
            }
            // This is needed so the default image is set if no game media was found.
            if (path == "" && image->getThemeImageTypes().size() > 0)
                image->setImage("");
        }
        else {
            image->setImage("");
        }
    }

    for (auto& video : mSystemElements[cursor].videoComponents) {
        // If a static video has been set, then don't attempt to find a gameselector entry.
        if (video->hasStaticVideo())
            continue;
        GameSelectorComponent* gameSelector {nullptr};
        if (multipleSelectors) {
            const std::string& videoSelector {video->getThemeGameSelector()};
            if (videoSelector == "") {
                gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                LOG(LogWarning) << "SystemView::updateGameSelectors(): Multiple gameselector "
                                   "elements defined but video element does not state which one to "
                                   "use, selecting first entry";
            }
            else {
                for (auto& selector : mSystemElements[cursor].gameSelectors) {
                    if (selector->getSelectorName() == videoSelector)
                        gameSelector = selector.get();
                }
                if (gameSelector == nullptr) {
                    LOG(LogWarning)
                        << "SystemView::updateGameSelectors(): Invalid gameselector \""
                        << videoSelector << "\" defined for video element, selecting first entry";
                    gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                }
            }
        }
        else {
            gameSelector = mSystemElements[cursor].gameSelectors.front().get();
        }
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (!games.empty()) {
            if (!video->setVideo(games.front()->getVideoPath()))
                video->setDefaultVideo();
        }
    }

    for (auto& video : mSystemElements[cursor].videoComponents) {
        if (video->getThemeImageTypes().size() == 0)
            continue;
        GameSelectorComponent* gameSelector {nullptr};
        if (multipleSelectors) {
            const std::string& imageSelector {video->getThemeGameSelector()};
            if (imageSelector == "") {
                gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                LOG(LogWarning) << "SystemView::updateGameSelectors(): Multiple gameselector "
                                   "elements defined but video element does not state which one to "
                                   "use, selecting first entry";
            }
            else {
                for (auto& selector : mSystemElements[cursor].gameSelectors) {
                    if (selector->getSelectorName() == imageSelector)
                        gameSelector = selector.get();
                }
                if (gameSelector == nullptr) {
                    LOG(LogWarning)
                        << "SystemView::updateGameSelectors(): Invalid gameselector \""
                        << imageSelector << "\" defined for video element, selecting first entry";
                    gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                }
            }
        }
        else {
            gameSelector = mSystemElements[cursor].gameSelectors.front().get();
        }
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (!games.empty()) {
            std::string path;
            for (auto& imageType : video->getThemeImageTypes()) {
                if (imageType == "image") {
                    path = games.front()->getImagePath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "miximage") {
                    path = games.front()->getMiximagePath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "marquee") {
                    path = games.front()->getMarqueePath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "screenshot") {
                    path = games.front()->getScreenshotPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "titlescreen") {
                    path = games.front()->getTitleScreenPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "cover") {
                    path = games.front()->getCoverPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "backcover") {
                    path = games.front()->getBackCoverPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "3dbox") {
                    path = games.front()->get3DBoxPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "fanart") {
                    path = games.front()->getFanArtPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "thumbnail") {
                    path = games.front()->getThumbnailPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
            }
            // This is needed so the default image is set if no game media was found.
            if (path == "" && video->getThemeImageTypes().size() > 0)
                video->setImage("");
        }
        else {
            video->setImage("");
        }
    }

    for (auto& text : mSystemElements[cursor].textComponents) {
        if (text->getThemeMetadata() == "")
            continue;
        GameSelectorComponent* gameSelector {nullptr};
        if (multipleSelectors) {
            const std::string& textSelector {text->getThemeGameSelector()};
            if (textSelector == "") {
                gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                LOG(LogWarning) << "SystemView::updateGameSelectors(): Multiple gameselector "
                                   "elements defined but text element does not state which one to "
                                   "use, selecting first entry";
            }
            else {
                for (auto& selector : mSystemElements[cursor].gameSelectors) {
                    if (selector->getSelectorName() == textSelector)
                        gameSelector = selector.get();
                }
                if (gameSelector == nullptr) {
                    LOG(LogWarning)
                        << "SystemView::updateGameSelectors(): Invalid gameselector \""
                        << textSelector << "\" defined for text element, selecting first entry";
                    gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                }
            }
        }
        else {
            gameSelector = mSystemElements[cursor].gameSelectors.front().get();
        }
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (!games.empty()) {
            const std::string metadata {text->getThemeMetadata()};
            if (metadata == "name")
                text->setValue(games.front()->metadata.get("name"));
            if (metadata == "description")
                text->setValue(games.front()->metadata.get("desc"));
            if (metadata == "developer")
                text->setValue(games.front()->metadata.get("developer"));
            if (metadata == "publisher")
                text->setValue(games.front()->metadata.get("publisher"));
            if (metadata == "genre")
                text->setValue(games.front()->metadata.get("genre"));
            if (metadata == "players")
                text->setValue(games.front()->metadata.get("players"));
            if (metadata == "favorite")
                text->setValue(games.front()->metadata.get("favorite") == "true" ? "yes" : "no");
            if (metadata == "completed")
                text->setValue(games.front()->metadata.get("completed") == "true" ? "yes" : "no");
            if (metadata == "kidgame")
                text->setValue(games.front()->metadata.get("kidgame") == "true" ? "yes" : "no");
            if (metadata == "broken")
                text->setValue(games.front()->metadata.get("broken") == "true" ? "yes" : "no");
            if (metadata == "playcount")
                text->setValue(games.front()->metadata.get("playcount"));
            if (metadata == "altemulator")
                text->setValue(games.front()->metadata.get("altemulator"));
        }
        else {
            text->setValue("");
        }
    }

    for (auto& dateTime : mSystemElements[cursor].dateTimeComponents) {
        if (dateTime->getThemeMetadata() == "")
            continue;
        GameSelectorComponent* gameSelector {nullptr};
        if (multipleSelectors) {
            const std::string& dateTimeSelector {dateTime->getThemeGameSelector()};
            if (dateTimeSelector == "") {
                gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                LOG(LogWarning) << "SystemView::updateGameSelectors(): Multiple gameselector "
                                   "elements defined but datetime element does not state which one "
                                   "to use, selecting first entry";
            }
            else {
                for (auto& selector : mSystemElements[cursor].gameSelectors) {
                    if (selector->getSelectorName() == dateTimeSelector)
                        gameSelector = selector.get();
                }
                if (gameSelector == nullptr) {
                    LOG(LogWarning) << "SystemView::updateGameSelectors(): Invalid gameselector \""
                                    << dateTimeSelector
                                    << "\" defined for datetime element, selecting first entry";
                    gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                }
            }
        }
        else {
            gameSelector = mSystemElements[cursor].gameSelectors.front().get();
        }
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (!games.empty()) {
            const std::string metadata {dateTime->getThemeMetadata()};
            if (metadata == "releasedate")
                dateTime->setValue(games.front()->metadata.get("releasedate"));
            if (metadata == "lastplayed")
                dateTime->setValue(games.front()->metadata.get("lastplayed"));
        }
        else {
            dateTime->setValue("19700101T000000");
        }
    }
}

void SystemView::legacyApplyTheme(const std::shared_ptr<ThemeData>& theme)
{
    if (theme->hasView("system"))
        mViewNeedsReload = false;
    else
        mViewNeedsReload = true;

    mCarousel->applyTheme(theme, "system", "carousel_systemcarousel", ThemeFlags::ALL);

    mLegacySystemInfo->setSize(mSize.x, mLegacySystemInfo->getFont()->getLetterHeight() * 2.2f);
    mLegacySystemInfo->setPosition(0.0f, mCarousel->getPosition().y + mCarousel->getSize().y);
    mLegacySystemInfo->setBackgroundColor(0xDDDDDDD8);
    mLegacySystemInfo->setRenderBackground(true);
    mLegacySystemInfo->setFont(
        Font::get(static_cast<int>(0.035f * mSize.y), Font::getDefaultPath()));
    mLegacySystemInfo->setColor(0x000000FF);
    mLegacySystemInfo->setUppercase(true);
    mLegacySystemInfo->setZIndex(49.0f);
    mLegacySystemInfo->setDefaultZIndex(49.0f);

    const ThemeData::ThemeElement* sysInfoElem {
        theme->getElement("system", "text_systemInfo", "text")};

    if (sysInfoElem)
        mLegacySystemInfo->applyTheme(theme, "system", "text_systemInfo", ThemeFlags::ALL);
}

void SystemView::renderElements(const glm::mat4& parentTrans, bool abovePrimary)
{
    glm::mat4 trans {getTransform() * parentTrans};

    // Adding texture loading buffers depending on scrolling speed and status.
    int bufferIndex {mCarousel->getScrollingVelocity() + 1};

    const float primaryZIndex {mCarousel->getZIndex()};

    for (int i = static_cast<int>(mCamOffset) + logoBuffersLeft[bufferIndex];
         i <= static_cast<int>(mCamOffset) + logoBuffersRight[bufferIndex]; ++i) {
        int index {i};
        while (index < 0)
            index += static_cast<int>(mCarousel->getNumEntries());
        while (index >= static_cast<int>(mCarousel->getNumEntries()))
            index -= static_cast<int>(mCarousel->getNumEntries());

        if (mCarousel->isAnimationPlaying(0) || index == mCarousel->getCursor()) {
            glm::mat4 elementTrans {trans};
            if (mCarousel->getType() == CarouselComponent::HORIZONTAL ||
                mCarousel->getType() == CarouselComponent::HORIZONTAL_WHEEL)
                elementTrans = glm::translate(elementTrans,
                                              glm::vec3 {(i - mCamOffset) * mSize.x, 0.0f, 0.0f});
            else
                elementTrans = glm::translate(elementTrans,
                                              glm::vec3 {0.0f, (i - mCamOffset) * mSize.y, 0.0f});

            Renderer::pushClipRect(
                glm::ivec2 {static_cast<int>(glm::round(elementTrans[3].x)),
                            static_cast<int>(glm::round(elementTrans[3].y))},
                glm::ivec2 {static_cast<int>(mSize.x), static_cast<int>(mSize.y)});

            if (mLegacyMode && mSystemElements.size() > static_cast<size_t>(index)) {
                for (auto element : mSystemElements[index].legacyExtras)
                    element->render(elementTrans);
            }
            else if (!mLegacyMode && mSystemElements.size() > static_cast<size_t>(index)) {
                for (auto child : mSystemElements[index].children) {
                    if (abovePrimary && child->getZIndex() > primaryZIndex && mFadeOpacity == 0.0f)
                        child->render(elementTrans);
                    else if (!abovePrimary && child->getZIndex() <= primaryZIndex)
                        child->render(elementTrans);
                }
            }

            if (mLegacyMode)
                mLegacySystemInfo->render(elementTrans);

            Renderer::popClipRect();
        }
    }
}

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

SystemView::SystemView()
    : mRenderer {Renderer::getInstance()}
    , mPrimary {nullptr}
    , mPrimaryType {PrimaryType::CAROUSEL}
    , mLastCursor {-1}
    , mCamOffset {0.0f}
    , mFadeOpacity {0.0f}
    , mPreviousScrollVelocity {0}
    , mUpdatedGameCount {false}
    , mViewNeedsReload {true}
    , mLegacyMode {false}
    , mNavigated {false}
    , mMaxFade {false}
    , mFadeTransitions {false}
    , mTransitionAnim {false}
{
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
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

void SystemView::onShow()
{
    finishAnimation(0);
    mFadeOpacity = 0.0f;
    mTransitionAnim = false;
}

void SystemView::onTransition()
{
    for (auto& anim : mSystemElements[mPrimary->getCursor()].lottieAnimComponents)
        anim->setPauseAnimation(true);

    for (auto& anim : mSystemElements[mPrimary->getCursor()].GIFAnimComponents)
        anim->setPauseAnimation(true);

    if (mFadeTransitions)
        mTransitionAnim = true;
}

void SystemView::goToSystem(SystemData* system, bool animate)
{
    mPrimary->setCursor(system);

    for (auto& selector : mSystemElements[mPrimary->getCursor()].gameSelectors) {
        if (selector->getGameSelection() == GameSelectorComponent::GameSelection::RANDOM)
            selector->setNeedsRefresh();
    }

    for (auto& video : mSystemElements[mPrimary->getCursor()].videoComponents)
        video->setStaticVideo();

    for (auto& anim : mSystemElements[mPrimary->getCursor()].lottieAnimComponents)
        anim->resetFileAnimation();

    for (auto& anim : mSystemElements[mPrimary->getCursor()].GIFAnimComponents)
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
        if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_r &&
            SDL_GetModState() & KMOD_LCTRL && Settings::getInstance()->getBool("Debug")) {
            LOG(LogDebug) << "SystemView::input(): Reloading all";
            TextureResource::manualUnloadAll();
            ViewController::getInstance()->reloadAll();
            return true;
        }

        if (config->isMappedTo("a", input)) {
            mPrimary->stopScrolling();
            pauseViewVideos();
            ViewController::getInstance()->goToGamelist(mPrimary->getSelected());
            NavigationSounds::getInstance().playThemeNavigationSound(SELECTSOUND);
            return true;
        }
        if (Settings::getInstance()->getBool("RandomAddButton") &&
            (config->isMappedTo("leftthumbstickclick", input) ||
             config->isMappedTo("rightthumbstickclick", input))) {
            // Get a random system and jump to it.
            NavigationSounds::getInstance().playThemeNavigationSound(SYSTEMBROWSESOUND);
            mPrimary->stopScrolling();
            mPrimary->setCursor(SystemData::getRandomSystem(mPrimary->getSelected()));
            return true;
        }

        if (config->isMappedTo("back", input) &&
            Settings::getInstance()->getBool("ScreensaverControls")) {
            if (!mWindow->isScreensaverActive()) {
                ViewController::getInstance()->stopScrolling();
                ViewController::getInstance()->cancelViewTransitions();
                mWindow->startScreensaver(false);
                mWindow->renderScreensaver();
            }
            return true;
        }
    }

    return mPrimary->input(config, input);
}

void SystemView::update(int deltaTime)
{
    if (!mPrimary->isAnimationPlaying(0))
        mMaxFade = false;

    mPrimary->update(deltaTime);

    for (auto& video : mSystemElements[mPrimary->getCursor()].videoComponents) {
        if (!isScrolling())
            video->update(deltaTime);
    }

    for (auto& anim : mSystemElements[mPrimary->getCursor()].lottieAnimComponents)
        anim->update(deltaTime);

    for (auto& anim : mSystemElements[mPrimary->getCursor()].GIFAnimComponents)
        anim->update(deltaTime);

    GuiComponent::update(deltaTime);
}

void SystemView::render(const glm::mat4& parentTrans)
{
    if (mPrimary == nullptr)
        return; // Nothing to render.

    bool transitionFade {false};

    if (mNavigated && mMaxFade)
        transitionFade = true;

    if (!transitionFade)
        renderElements(parentTrans, false);
    glm::mat4 trans {getTransform() * parentTrans};

    // Make sure that the primary component doesn't render outside our designated area.
    mRenderer->pushClipRect(
        glm::ivec2 {static_cast<int>(std::round(trans[3].x)),
                    static_cast<int>(std::round(trans[3].y))},
        glm::ivec2 {static_cast<int>(std::round(mSize.x)), static_cast<int>(std::round(mSize.y))});

    mPrimary->render(trans);
    mRenderer->popClipRect();

    if (!mPrimary->getFadeAbovePrimary() || !transitionFade)
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
    if (mCarousel != nullptr) {
        if (mCarousel->getType() == CarouselComponent<SystemData*>::CarouselType::VERTICAL ||
            mCarousel->getType() == CarouselComponent<SystemData*>::CarouselType::VERTICAL_WHEEL)
            prompts.push_back(HelpPrompt("up/down", "choose"));
        else
            prompts.push_back(HelpPrompt("left/right", "choose"));
    }
    else if (mTextList != nullptr) {
        prompts.push_back(HelpPrompt("up/down", "choose"));
    }

    prompts.push_back(HelpPrompt("a", "select"));

    if (Settings::getInstance()->getBool("RandomAddButton"))
        prompts.push_back(HelpPrompt("thumbstickclick", "random"));

    if (Settings::getInstance()->getBool("ScreensaverControls"))
        prompts.push_back(HelpPrompt("back", "screensaver"));

    return prompts;
}

void SystemView::onCursorChanged(const CursorState& state)
{
    int cursor {mPrimary->getCursor()};

    // Avoid double updates.
    if (cursor != mLastCursor) {
        for (auto& selector : mSystemElements[cursor].gameSelectors) {
            if (selector->getGameSelection() == GameSelectorComponent::GameSelection::RANDOM)
                selector->setNeedsRefresh();
        }
    }

    mLastCursor = cursor;

    for (auto& video : mSystemElements[cursor].videoComponents)
        video->setStaticVideo();

    for (auto& anim : mSystemElements[mPrimary->getCursor()].lottieAnimComponents)
        anim->resetFileAnimation();

    for (auto& anim : mSystemElements[mPrimary->getCursor()].GIFAnimComponents)
        anim->resetFileAnimation();

    updateGameSelectors();
    startViewVideos();
    updateHelpPrompts();

    int scrollVelocity {mPrimary->getScrollingVelocity()};
    float startPos {mCamOffset};
    float posMax {static_cast<float>(mPrimary->getNumEntries())};
    float target {static_cast<float>(cursor)};
    float endPos {target};

    if (mPreviousScrollVelocity > 0 && scrollVelocity == 0 && mCamOffset > posMax - 1.0f)
        startPos = 0.0f;

    if (mPrimaryType == PrimaryType::CAROUSEL) {
        // Find the shortest path to the target.
        float dist {fabsf(endPos - startPos)};

        if (fabs(target + posMax - startPos - scrollVelocity) < dist)
            endPos = target + posMax; // Loop around the end (0 -> max).
        if (fabs(target - posMax - startPos - scrollVelocity) < dist)
            endPos = target - posMax; // Loop around the start (max - 1 -> -1).
    }

    // Make sure transitions do not animate in reverse.
    bool changedDirection {false};
    if (mPreviousScrollVelocity != 0 && mPreviousScrollVelocity != scrollVelocity) {
        if (scrollVelocity > 0 && startPos + scrollVelocity < posMax)
            changedDirection = true;
    }

    if (!changedDirection && scrollVelocity > 0 && endPos < startPos)
        endPos = endPos + posMax;

    if (!changedDirection && scrollVelocity < 0 && endPos > startPos)
        endPos = endPos - posMax;

    if (scrollVelocity != 0)
        mPreviousScrollVelocity = scrollVelocity;

    std::string transitionStyle {Settings::getInstance()->getString("TransitionStyle")};
    mFadeTransitions = transitionStyle == "fade";

    if (startPos == endPos)
        return;

    Animation* anim;

    if (transitionStyle == "fade") {
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

                if (t >= 0.7f && t != 1.0f)
                    mMaxFade = true;

                // Update the game count when the entire animation has been completed.
                if (mFadeOpacity == 1.0f) {
                    mMaxFade = false;
                    updateGameCount();
                }
            },
            500);
    }
    else if (transitionStyle == "slide") {
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
    if (SystemData::sSystemVector.size() == 0)
        return;

    LOG(LogDebug) << "SystemView::populate(): Populating carousel";

    auto themeSets = ThemeData::getThemeSets();
    std::map<std::string, ThemeData::ThemeSet, ThemeData::StringComparator>::const_iterator
        selectedSet {themeSets.find(Settings::getInstance()->getString("ThemeSet"))};

    assert(selectedSet != themeSets.cend());
    mLegacyMode = selectedSet->second.capabilities.legacyTheme;

    if (mLegacyMode) {
        mLegacySystemInfo = std::make_unique<TextComponent>(
            "SYSTEM INFO", Font::get(FONT_SIZE_SMALL), 0x33333300, ALIGN_CENTER);
    }

    for (auto it : SystemData::sSystemVector) {
        const std::shared_ptr<ThemeData>& theme {it->getTheme()};
        std::string itemPath;
        std::string defaultItemPath;

        if (mLegacyMode && mViewNeedsReload) {
            if (mCarousel == nullptr) {
                mCarousel = std::make_unique<CarouselComponent<SystemData*>>();
                mPrimary = mCarousel.get();
                // For legacy themes the carousel has a zIndex value of 40 instead of 50.
                mPrimary->setDefaultZIndex(40.0f);
                mPrimary->setCursorChangedCallback(
                    [&](const CursorState& state) { onCursorChanged(state); });
                mPrimary->setCancelTransitionsCallback([&] {
                    ViewController::getInstance()->cancelViewTransitions();
                    mNavigated = true;
                    if (mSystemElements.size() > 1) {
                        for (auto& anim :
                             mSystemElements[mPrimary->getCursor()].lottieAnimComponents)
                            anim->setPauseAnimation(true);
                        for (auto& anim : mSystemElements[mPrimary->getCursor()].GIFAnimComponents)
                            anim->setPauseAnimation(true);
                    }
                });
            }
            legacyApplyTheme(theme);
        }

        if (mLegacyMode) {
            SystemViewElements elements;
            elements.name = it->getName();
            elements.legacyExtras = ThemeData::makeExtras(theme, "system");

            // Sort the extras by z-index.
            std::stable_sort(
                elements.legacyExtras.begin(), elements.legacyExtras.end(),
                [](GuiComponent* a, GuiComponent* b) { return b->getZIndex() > a->getZIndex(); });

            mSystemElements.emplace_back(std::move(elements));
            mSystemElements.back().helpStyle.applyTheme(theme, "system");
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
                    if (element.second.type == "textlist" || element.second.type == "carousel") {
                        if (element.second.type == "carousel" && mTextList != nullptr) {
                            LOG(LogWarning)
                                << "SystemView::populate(): Multiple primary components "
                                << "defined, skipping <carousel> configuration entry";
                            continue;
                        }
                        if (element.second.type == "textlist" && mCarousel != nullptr) {
                            LOG(LogWarning)
                                << "SystemView::populate(): Multiple primary components "
                                << "defined, skipping <textlist> configuration entry";
                            continue;
                        }
                        if (element.second.type == "carousel" && mCarousel == nullptr) {
                            mCarousel = std::make_unique<CarouselComponent<SystemData*>>();
                            mPrimary = mCarousel.get();
                            mPrimaryType = PrimaryType::CAROUSEL;
                        }
                        else if (element.second.type == "textlist" && mTextList == nullptr) {
                            mTextList = std::make_unique<TextListComponent<SystemData*>>();
                            mPrimary = mTextList.get();
                            mPrimaryType = PrimaryType::TEXTLIST;
                        }
                        mPrimary->setDefaultZIndex(50.0f);
                        mPrimary->applyTheme(theme, "system", element.first, ThemeFlags::ALL);
                        mPrimary->setCursorChangedCallback(
                            [&](const CursorState& state) { onCursorChanged(state); });
                        mPrimary->setCancelTransitionsCallback([&] {
                            ViewController::getInstance()->cancelViewTransitions();
                            mNavigated = true;
                            if (mSystemElements.size() > 1) {
                                for (auto& anim :
                                     mSystemElements[mPrimary->getCursor()].lottieAnimComponents)
                                    anim->setPauseAnimation(true);
                                for (auto& anim :
                                     mSystemElements[mPrimary->getCursor()].GIFAnimComponents)
                                    anim->setPauseAnimation(true);
                            }
                        });
                        if (mCarousel != nullptr) {
                            if (element.second.has("staticItem"))
                                itemPath = element.second.get<std::string>("staticItem");
                            if (element.second.has("defaultItem"))
                                defaultItemPath = element.second.get<std::string>("defaultItem");
                        }
                    }
                    else if (element.second.type == "image") {
                        // If this is the first system, then forceload the images to avoid texture
                        // pop-in.
                        if (it == SystemData::sSystemVector.front())
                            elements.imageComponents.emplace_back(
                                std::make_unique<ImageComponent>(true));
                        else
                            elements.imageComponents.emplace_back(
                                std::make_unique<ImageComponent>());

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
                    else if (element.second.type == "animation" && element.second.has("path")) {
                        const std::string extension {Utils::FileSystem::getExtension(
                            element.second.get<std::string>("path"))};
                        if (extension == ".json") {
                            elements.lottieAnimComponents.emplace_back(
                                std::make_unique<LottieAnimComponent>());
                            elements.lottieAnimComponents.back()->setDefaultZIndex(35.0f);
                            elements.lottieAnimComponents.back()->applyTheme(
                                theme, "system", element.first, ThemeFlags::ALL);
                            elements.children.emplace_back(
                                elements.lottieAnimComponents.back().get());
                        }
                        else if (extension == ".gif") {
                            elements.GIFAnimComponents.emplace_back(
                                std::make_unique<GIFAnimComponent>());
                            elements.GIFAnimComponents.back()->setDefaultZIndex(35.0f);
                            elements.GIFAnimComponents.back()->applyTheme(
                                theme, "system", element.first, ThemeFlags::ALL);
                            elements.children.emplace_back(elements.GIFAnimComponents.back().get());
                        }
                        else if (extension == ".") {
                            LOG(LogWarning)
                                << "SystemView::populate(): Invalid theme configuration, "
                                   "animation file extension is missing";
                        }
                        else {
                            LOG(LogWarning)
                                << "SystemView::populate(): Invalid theme configuration, "
                                   "animation file extension defined as \""
                                << extension << "\"";
                        }
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
                        elements.dateTimeComponents.back()->setVisible(false);
                        elements.children.emplace_back(elements.dateTimeComponents.back().get());
                    }
                    else if (element.second.type == "rating") {
                        elements.ratingComponents.emplace_back(std::make_unique<RatingComponent>());
                        elements.ratingComponents.back()->setDefaultZIndex(45.0f);
                        elements.ratingComponents.back()->applyTheme(theme, "system", element.first,
                                                                     ThemeFlags::ALL);
                        elements.ratingComponents.back()->setVisible(false);
                        elements.ratingComponents.back()->setOpacity(
                            elements.ratingComponents.back()->getOpacity());
                        elements.children.emplace_back(elements.ratingComponents.back().get());
                    }
                }
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
            mSystemElements.back().helpStyle.applyTheme(theme, "system");
        }

        if (mPrimary == nullptr) {
            mCarousel = std::make_unique<CarouselComponent<SystemData*>>();
            mPrimary = mCarousel.get();
            mPrimaryType = PrimaryType::CAROUSEL;
            mPrimary->setDefaultZIndex(50.0f);
            mPrimary->applyTheme(theme, "system", "", ThemeFlags::ALL);
            mPrimary->setCursorChangedCallback(
                [&](const CursorState& state) { onCursorChanged(state); });
            mPrimary->setCancelTransitionsCallback([&] {
                ViewController::getInstance()->cancelViewTransitions();
                mNavigated = true;
                if (mSystemElements.size() > 1) {
                    for (auto& anim : mSystemElements[mPrimary->getCursor()].lottieAnimComponents)
                        anim->setPauseAnimation(true);
                    for (auto& anim : mSystemElements[mPrimary->getCursor()].GIFAnimComponents)
                        anim->setPauseAnimation(true);
                }
            });
        }

        if (mCarousel != nullptr) {
            CarouselComponent<SystemData*>::Entry entry;
            entry.name = it->getName();
            entry.object = it;
            entry.data.itemPath = itemPath;
            entry.data.defaultItemPath = defaultItemPath;
            mCarousel->addEntry(entry, theme);
        }
        if (mTextList != nullptr) {
            TextListComponent<SystemData*>::Entry entry;
            entry.name = it->getFullName();
            entry.object = it;
            entry.data.colorId = 0;
            mTextList->addEntry(entry);
        }
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

    if (mPrimary->getNumEntries() == 0) {
        // Something is wrong, there is not a single system to show, check if UI mode is not full.
        if (!UIModeController::getInstance()->isUIModeFull()) {
            Settings::getInstance()->setString("UIMode", "full");
            mWindow->pushGui(new GuiMsgBox(
                getHelpStyle(),
                "The selected UI mode has nothing to show,\n returning to UI mode \"Full\"", "OK",
                nullptr));
        }
    }

    mFadeTransitions = Settings::getInstance()->getString("TransitionStyle") == "fade";
}

void SystemView::updateGameCount()
{
    std::pair<unsigned int, unsigned int> gameCount =
        mPrimary->getSelected()->getDisplayedGameCount();
    std::stringstream ss;
    std::stringstream ssGames;
    std::stringstream ssFavorites;
    bool games {false};

    if (!mPrimary->getSelected()->isGameSystem()) {
        ss << "Configuration";
    }
    else if (mPrimary->getSelected()->isCollection() &&
             (mPrimary->getSelected()->getName() == "favorites")) {
        ss << gameCount.first << " Game" << (gameCount.first == 1 ? " " : "s");
    }
    else if (mPrimary->getSelected()->isCollection() &&
             (mPrimary->getSelected()->getName() == "recent")) {
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
        for (auto& gameCount : mSystemElements[mPrimary->getCursor()].gameCountComponents) {
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

    int cursor {mPrimary->getCursor()};

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
                    if (selector->getSelectorName() == imageSelector) {
                        gameSelector = selector.get();
                        break;
                    }
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
                else if (imageType == "physicalmedia") {
                    path = games.front()->getPhysicalMediaPath();
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
                    if (selector->getSelectorName() == videoSelector) {
                        gameSelector = selector.get();
                        break;
                    }
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
        if (video->hasStaticVideo() || video->getThemeImageTypes().size() == 0)
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
                    if (selector->getSelectorName() == imageSelector) {
                        gameSelector = selector.get();
                        break;
                    }
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
                else if (imageType == "physicalmedia") {
                    path = games.front()->getPhysicalMediaPath();
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
                    if (selector->getSelectorName() == textSelector) {
                        gameSelector = selector.get();
                        break;
                    }
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
            if (metadata == "rating")
                text->setValue(
                    RatingComponent::getRatingValue(games.front()->metadata.get("rating")));
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
                    if (selector->getSelectorName() == dateTimeSelector) {
                        gameSelector = selector.get();
                        break;
                    }
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
            dateTime->setVisible(true);
            const std::string metadata {dateTime->getThemeMetadata()};
            if (metadata == "releasedate")
                dateTime->setValue(games.front()->metadata.get("releasedate"));
            if (metadata == "lastplayed")
                dateTime->setValue(games.front()->metadata.get("lastplayed"));
        }
        else {
            dateTime->setVisible(false);
        }
    }

    for (auto& rating : mSystemElements[cursor].ratingComponents) {
        GameSelectorComponent* gameSelector {nullptr};
        if (multipleSelectors) {
            const std::string& ratingSelector {rating->getThemeGameSelector()};
            if (ratingSelector == "") {
                gameSelector = mSystemElements[cursor].gameSelectors.front().get();
                LOG(LogWarning)
                    << "SystemView::updateGameSelectors(): Multiple gameselector "
                       "elements defined but rating element does not state which one to "
                       "use, selecting first entry";
            }
            else {
                for (auto& selector : mSystemElements[cursor].gameSelectors) {
                    if (selector->getSelectorName() == ratingSelector) {
                        gameSelector = selector.get();
                        break;
                    }
                }
                if (gameSelector == nullptr) {
                    LOG(LogWarning)
                        << "SystemView::updateGameSelectors(): Invalid gameselector \""
                        << ratingSelector << "\" defined for rating element, selecting first entry";
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
            rating->setVisible(true);
            rating->setValue(games.front()->metadata.get("rating"));
        }
        else {
            rating->setVisible(false);
        }
    }
}

void SystemView::legacyApplyTheme(const std::shared_ptr<ThemeData>& theme)
{
    if (theme->hasView("system"))
        mViewNeedsReload = false;
    else
        mViewNeedsReload = true;

    if (mCarousel != nullptr)
        mPrimary->applyTheme(theme, "system", "carousel_systemcarousel", ThemeFlags::ALL);
    else if (mTextList != nullptr)
        mPrimary->applyTheme(theme, "system", "textlist_gamelist", ThemeFlags::ALL);

    mLegacySystemInfo->setSize(mSize.x, mLegacySystemInfo->getFont()->getLetterHeight() * 2.2f);
    mLegacySystemInfo->setPosition(0.0f, std::round(mPrimary->getPosition().y) +
                                             std::round(mPrimary->getSize().y));
    mLegacySystemInfo->setBackgroundColor(0xDDDDDDD8);
    mLegacySystemInfo->setRenderBackground(true);
    mLegacySystemInfo->setFont(
        Font::get(static_cast<int>(0.035f * mSize.y), Font::getDefaultPath()));
    mLegacySystemInfo->setColor(0x000000FF);
    mLegacySystemInfo->setUppercase(true);
    mLegacySystemInfo->setZIndex(50.0f);
    mLegacySystemInfo->setDefaultZIndex(50.0f);

    const ThemeData::ThemeElement* sysInfoElem {
        theme->getElement("system", "text_systemInfo", "text")};

    if (sysInfoElem)
        mLegacySystemInfo->applyTheme(theme, "system", "text_systemInfo", ThemeFlags::ALL);
}

void SystemView::renderElements(const glm::mat4& parentTrans, bool abovePrimary)
{
    glm::mat4 trans {getTransform() * parentTrans};

    const float primaryZIndex {mPrimary->getZIndex()};

    int renderBefore {static_cast<int>(mCamOffset)};
    int renderAfter {static_cast<int>(mCamOffset)};

    // If we're transitioning then also render the previous and next systems.
    if (mPrimary->isAnimationPlaying(0)) {
        renderBefore -= 1;
        renderAfter += 1;
    }

    for (int i = renderBefore; i <= renderAfter; ++i) {
        int index {i};
        while (index < 0)
            index += static_cast<int>(mPrimary->getNumEntries());
        while (index >= static_cast<int>(mPrimary->getNumEntries()))
            index -= static_cast<int>(mPrimary->getNumEntries());

        if (mPrimary->isAnimationPlaying(0) || index == mPrimary->getCursor()) {
            glm::mat4 elementTrans {trans};
            if (mCarousel != nullptr) {
                if (mCarousel->getType() ==
                        CarouselComponent<SystemData*>::CarouselType::HORIZONTAL ||
                    mCarousel->getType() ==
                        CarouselComponent<SystemData*>::CarouselType::HORIZONTAL_WHEEL)
                    elementTrans = glm::translate(
                        elementTrans,
                        glm::round(glm::vec3 {(i - mCamOffset) * mSize.x, 0.0f, 0.0f}));
                else
                    elementTrans = glm::translate(
                        elementTrans,
                        glm::round(glm::vec3 {0.0f, (i - mCamOffset) * mSize.y, 0.0f}));
            }
            else if (mTextList != nullptr) {
                elementTrans = glm::translate(
                    elementTrans, glm::round(glm::vec3 {0.0f, (i - mCamOffset) * mSize.y, 0.0f}));
            }

            mRenderer->pushClipRect(
                glm::ivec2 {static_cast<int>(glm::round(elementTrans[3].x)),
                            static_cast<int>(glm::round(elementTrans[3].y))},
                glm::ivec2 {static_cast<int>(mSize.x), static_cast<int>(mSize.y)});

            if (mLegacyMode && mSystemElements.size() > static_cast<size_t>(index)) {
                for (auto element : mSystemElements[index].legacyExtras) {
                    if (abovePrimary && element->getZIndex() < primaryZIndex)
                        continue;
                    if ((mFadeTransitions || element->getDimming() != 1.0f) &&
                        element->getZIndex() < primaryZIndex)
                        element->setDimming(1.0f - mFadeOpacity);
                    if (mFadeTransitions && mPrimary->getFadeAbovePrimary()) {
                        if (mFadeTransitions && isAnimationPlaying(0))
                            element->setOpacity(1.0f - mFadeOpacity);
                        else
                            element->setOpacity(1.0f);
                    }

                    element->render(elementTrans);
                }
            }
            else if (!mLegacyMode && mSystemElements.size() > static_cast<size_t>(index)) {
                for (auto child : mSystemElements[index].children) {
                    if (abovePrimary && (child->getZIndex() > primaryZIndex)) {
                        if (mFadeTransitions && mPrimary->getFadeAbovePrimary()) {
                            if (mFadeTransitions || child->getOpacity() != 1.0f)
                                child->setOpacity(1.0f - mFadeOpacity);
                        }
                        else {
                            child->setOpacity(1.0f);
                        }
                        child->render(elementTrans);
                    }

                    else if (!abovePrimary && child->getZIndex() <= primaryZIndex) {
                        if (mFadeTransitions || child->getDimming() != 1.0f)
                            child->setDimming(1.0f - mFadeOpacity);
                        child->render(elementTrans);
                    }
                }
            }

            if (mLegacyMode) {
                if (mFadeTransitions && !abovePrimary) {
                    if (mFadeTransitions && isAnimationPlaying(0))
                        mLegacySystemInfo->setOpacity(1.0f - mFadeOpacity);
                    else
                        mLegacySystemInfo->setOpacity(1.0f);
                }
                if ((abovePrimary && mLegacySystemInfo->getZIndex() > 40.0f) ||
                    (!abovePrimary && mLegacySystemInfo->getZIndex() <= 40.0f))
                    mLegacySystemInfo->render(elementTrans);
            }

            mRenderer->popClipRect();
        }
    }
}

//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
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
#include "utils/LocalizationUtil.h"
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
    , mNavigated {false}
    , mMaxFade {false}
    , mFadeTransitions {false}
    , mTransitionAnim {false}
{
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    populate();
}

void SystemView::onShow()
{
    finishAnimation(0);
    stopViewVideos();
    mFadeOpacity = 0.0f;
    mTransitionAnim = false;
    mPrimary->onShowPrimary();
}

void SystemView::onHide()
{
    if (mPrimary == nullptr || mPrimary->getCursor() + 1 > static_cast<int>(mSystemElements.size()))
        return;

    for (auto& video : mSystemElements[mPrimary->getCursor()].videoComponents)
        video->stopVideoPlayer(false);
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

    // Reset horizontally scrolling text.
    for (auto& text : mSystemElements[mPrimary->getCursor()].gameCountComponents)
        text->resetComponent();
    for (auto& text : mSystemElements[mPrimary->getCursor()].textComponents)
        text->resetComponent();

    for (auto& video : mSystemElements[mPrimary->getCursor()].videoComponents)
        video->setStaticVideo();

    for (auto& anim : mSystemElements[mPrimary->getCursor()].lottieAnimComponents)
        anim->resetComponent();

    for (auto& anim : mSystemElements[mPrimary->getCursor()].GIFAnimComponents)
        anim->resetComponent();

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
        if (Settings::getInstance()->getString("RandomEntryButton") == "gamessystems" &&
            (config->isMappedTo("leftthumbstickclick", input) ||
             config->isMappedTo("rightthumbstickclick", input))) {
            // Get a random system and jump to it.
            NavigationSounds::getInstance().playThemeNavigationSound(SYSTEMBROWSESOUND);
            mPrimary->stopScrolling();
            ViewController::getInstance()->cancelViewTransitions();
            mPrimary->setCursor(SystemData::getRandomSystem(mPrimary->getSelected()));
            return true;
        }

        if (config->isMappedTo("x", input) &&
            Settings::getInstance()->getBool("ScreensaverControls")) {
            if (!mWindow->isScreensaverActive()) {
                ViewController::getInstance()->stopScrolling();
                ViewController::getInstance()->cancelViewTransitions();
                mWindow->startScreensaver(false);
            }
            return true;
        }
    }

    return mPrimary->input(config, input);
}

void SystemView::update(int deltaTime)
{
    mPrimary->update(deltaTime);

    for (auto& text : mSystemElements[mPrimary->getCursor()].gameCountComponents)
        text->update(deltaTime);

    for (auto& text : mSystemElements[mPrimary->getCursor()].textComponents)
        text->update(deltaTime);

    for (auto& video : mSystemElements[mPrimary->getCursor()].videoComponents) {
        if (!isScrolling())
            video->update(deltaTime);
    }

    for (auto& anim : mSystemElements[mPrimary->getCursor()].lottieAnimComponents)
        anim->update(deltaTime);

    for (auto& anim : mSystemElements[mPrimary->getCursor()].GIFAnimComponents)
        anim->update(deltaTime);

    for (auto& container : mSystemElements[mPrimary->getCursor()].containerComponents)
        container->update(deltaTime);

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
            prompts.push_back(HelpPrompt("up/down", _("choose")));
        else
            prompts.push_back(HelpPrompt("left/right", _("choose")));
    }
    else if (mGrid != nullptr) {
        prompts.push_back(HelpPrompt("up/down/left/right", _("choose")));
    }
    else if (mTextList != nullptr) {
        prompts.push_back(HelpPrompt("up/down", _("choose")));
    }

    prompts.push_back(HelpPrompt("a", _("select")));

    if (Settings::getInstance()->getString("RandomEntryButton") == "gamessystems")
        prompts.push_back(HelpPrompt("thumbstickclick", _("random")));

    if (Settings::getInstance()->getBool("ScreensaverControls"))
        prompts.push_back(HelpPrompt("x", _("screensaver")));

    return prompts;
}

void SystemView::onCursorChanged(const CursorState& state)
{
    // Reset horizontally scrolling text.
    for (auto& text : mSystemElements[mPrimary->getCursor()].gameCountComponents)
        text->resetComponent();
    for (auto& text : mSystemElements[mPrimary->getCursor()].textComponents)
        text->resetComponent();

    const int cursor {mPrimary->getCursor()};
    const int scrollVelocity {mPrimary->getScrollingVelocity()};
    const ViewTransitionAnimation transitionAnim {static_cast<ViewTransitionAnimation>(
        Settings::getInstance()->getInt("TransitionsSystemToSystem"))};
    mFadeTransitions = (transitionAnim == ViewTransitionAnimation::FADE);

    // Some logic needed to avoid various navigation glitches with GridComponent and
    // TextListComponent.
    if (state == CursorState::CURSOR_STOPPED && mCarousel == nullptr) {
        const int numEntries {static_cast<int>(mPrimary->getNumEntries())};
        bool doStop {false};

        if (cursor == 0 && mLastCursor == numEntries - 1 && std::abs(scrollVelocity) == 1)
            doStop = false;
        else if (cursor == 0)
            doStop = true;
        else if (cursor == numEntries - 1 && mLastCursor == 0 && std::abs(scrollVelocity) == 1)
            doStop = false;
        else if (cursor == numEntries - 1)
            doStop = true;

        if (!doStop && mGrid != nullptr && std::abs(scrollVelocity) == mGrid->getColumnCount()) {
            const int columns {mGrid->getColumnCount()};
            const int columnModulus {numEntries % columns};

            if (cursor < columns)
                doStop = true;
            else if (cursor >= numEntries - (columnModulus == 0 ? columns : columnModulus))
                doStop = true;
        }

        if (doStop) {
            if (mGrid != nullptr)
                mGrid->setScrollVelocity(0);
            mPrimary->stopScrolling();
            mNavigated = false;
        }
    }

    // Avoid double updates.
    if (cursor != mLastCursor) {
        for (auto& selector : mSystemElements[cursor].gameSelectors) {
            if (selector->getGameSelection() == GameSelectorComponent::GameSelection::RANDOM)
                selector->setNeedsRefresh();
        }
    }

    if (mLastCursor >= 0 && mLastCursor <= static_cast<int>(mSystemElements.size())) {
        if (transitionAnim == ViewTransitionAnimation::INSTANT || isAnimationPlaying(0)) {
            for (auto& video : mSystemElements[mLastCursor].videoComponents)
                video->stopVideoPlayer(false);
        }
        else {
            for (auto& video : mSystemElements[mLastCursor].videoComponents)
                video->pauseVideoPlayer();
        }
    }

    for (auto& container : mSystemElements[mPrimary->getCursor()].containerComponents)
        container->resetComponent();

    // This is needed to avoid erratic camera movements during extreme navigation input when using
    // slide transitions. This should very rarely occur during normal application usage.
    if (transitionAnim == ViewTransitionAnimation::SLIDE) {
        bool resetCamOffset {false};

        if (scrollVelocity == -1 && mPreviousScrollVelocity == 1) {
            if (mLastCursor > cursor && mCamOffset > static_cast<float>(mLastCursor))
                resetCamOffset = true;
            else if (mLastCursor > cursor && mCamOffset < static_cast<float>(cursor))
                resetCamOffset = true;
            else if (mLastCursor < cursor && mCamOffset <= static_cast<float>(cursor) &&
                     mCamOffset != static_cast<float>(mLastCursor))
                resetCamOffset = true;
        }
        else if (scrollVelocity == 1 && mPreviousScrollVelocity == -1) {
            if (mLastCursor > cursor && mCamOffset < static_cast<float>(mLastCursor))
                resetCamOffset = true;
            else if (mLastCursor < cursor && mCamOffset > static_cast<float>(cursor))
                resetCamOffset = true;
        }

        if (resetCamOffset)
            mCamOffset = static_cast<float>(cursor);
    }

    const int prevLastCursor {mLastCursor};
    mLastCursor = cursor;

    for (auto& video : mSystemElements[cursor].videoComponents)
        video->setStaticVideo();

    for (auto& anim : mSystemElements[mPrimary->getCursor()].lottieAnimComponents)
        anim->resetComponent();

    for (auto& anim : mSystemElements[mPrimary->getCursor()].GIFAnimComponents)
        anim->resetComponent();

    updateGameSelectors();
    startViewVideos();
    updateHelpPrompts();

    const float posMax {static_cast<float>(mPrimary->getNumEntries())};
    const float target {static_cast<float>(cursor)};
    float startPos {mCamOffset};
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

    Animation* anim;

    float animTime {400.0f};
    float timeMin {200.0f};
    float timeDiff {1.0f};

    if (mGrid != nullptr) {
        animTime = 300.0f;
        timeMin = 100.0f;
    }

    // If startPos is inbetween two positions then reduce the time slightly as the distance will
    // be shorter meaning the animation would play for too long if not compensated for.
    if (scrollVelocity == 1)
        timeDiff = endPos - startPos;
    else if (scrollVelocity == -1)
        timeDiff = startPos - endPos;

    if (timeDiff != 1.0f)
        animTime =
            glm::clamp(std::fabs(glm::mix(0.0f, animTime, timeDiff * 1.5f)), timeMin, animTime);

    if (transitionAnim == ViewTransitionAnimation::FADE) {
        float startFade {mFadeOpacity};
        anim = new LambdaAnimation(
            [this, startFade, endPos, prevLastCursor](float t) {
                if (t < 0.3f)
                    mFadeOpacity =
                        glm::mix(0.0f, 1.0f, glm::clamp(t / 0.2f + startFade, 0.0f, 1.0f));
                else if (t < 0.7f)
                    mFadeOpacity = 1.0f;
                else
                    mFadeOpacity = glm::mix(1.0f, 0.0f, glm::clamp((t - 0.6f) / 0.3f, 0.0f, 1.0f));

                if (t > 0.5f)
                    mCamOffset = endPos;

                if (mNavigated && t >= 0.7f && t != 1.0f)
                    mMaxFade = true;

                if (t == 1.0f && prevLastCursor >= 0) {
                    for (auto& video : mSystemElements[prevLastCursor].videoComponents)
                        video->stopVideoPlayer(false);
                }

                // Update the game count when the entire animation has been completed.
                if (mFadeOpacity == 1.0f) {
                    mMaxFade = false;
                    updateGameCount();
                }
            },
            static_cast<int>(animTime * 1.3f));
    }
    else if (transitionAnim == ViewTransitionAnimation::SLIDE) {
        mUpdatedGameCount = false;
        anim = new LambdaAnimation(
            [this, startPos, endPos, posMax, prevLastCursor](float t) {
                // Non-linear interpolation.
                t = 1.0f - (1.0f - t) * (1.0f - t);
                float f {(endPos * t) + (startPos * (1.0f - t))};

                if (f < 0)
                    f += posMax;
                if (f >= posMax)
                    f -= posMax;

                mCamOffset = f;

                if (t == 1.0f && prevLastCursor >= 0) {
                    for (auto& video : mSystemElements[prevLastCursor].videoComponents)
                        video->stopVideoPlayer(false);
                }

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
            static_cast<int>(animTime));
    }
    else {
        // Instant.
        updateGameCount();
        anim = new LambdaAnimation([this, endPos](float t) { mCamOffset = endPos; },
                                   static_cast<int>(animTime));
    }

    setAnimation(anim, 0, nullptr, false, 0);
}

void SystemView::populate()
{
    if (SystemData::sSystemVector.size() == 0)
        return;

    LOG(LogDebug) << "SystemView::populate(): Populating primary element...";

    auto themes = ThemeData::getThemes();
    std::map<std::string, ThemeData::Theme, ThemeData::StringComparator>::const_iterator
        selectedTheme {themes.find(Settings::getInstance()->getString("Theme"))};

    assert(selectedTheme != themes.cend());

    for (auto it : SystemData::sSystemVector) {
        const std::shared_ptr<ThemeData>& theme {it->getTheme()};
        std::string imagePath;
        std::string defaultImagePath;
        std::string itemText;

        SystemViewElements elements;
        elements.system = it;
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
                if (element.second.type == "carousel" || element.second.type == "grid" ||
                    element.second.type == "textlist") {
                    if (element.second.type == "carousel" &&
                        (mGrid != nullptr || mTextList != nullptr)) {
                        LOG(LogWarning) << "SystemView::populate(): Multiple primary components "
                                        << "defined, skipping carousel configuration entry";
                        continue;
                    }
                    if (element.second.type == "grid" &&
                        (mCarousel != nullptr || mTextList != nullptr)) {
                        LOG(LogWarning) << "SystemView::populate(): Multiple primary components "
                                        << "defined, skipping grid configuration entry";
                        continue;
                    }
                    if (element.second.type == "textlist" &&
                        (mCarousel != nullptr || mGrid != nullptr)) {
                        LOG(LogWarning) << "SystemView::populate(): Multiple primary components "
                                        << "defined, skipping textlist configuration entry";
                        continue;
                    }
                    if (element.second.type == "carousel" && mCarousel == nullptr) {
                        mCarousel = std::make_unique<CarouselComponent<SystemData*>>();
                        mPrimary = mCarousel.get();
                        mPrimaryType = PrimaryType::CAROUSEL;
                    }
                    else if (element.second.type == "grid" && mGrid == nullptr) {
                        mGrid = std::make_unique<GridComponent<SystemData*>>();
                        mPrimary = mGrid.get();
                        mPrimaryType = PrimaryType::GRID;
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
                    if (mCarousel != nullptr || mGrid != nullptr) {
                        if (element.second.has("staticImage"))
                            imagePath = element.second.get<std::string>("staticImage");
                        if (element.second.has("defaultImage") &&
                            Utils::FileSystem::exists(
                                element.second.get<std::string>("defaultImage")))
                            defaultImagePath = element.second.get<std::string>("defaultImage");
                        if (element.second.has("text"))
                            itemText = element.second.get<std::string>("text");
                    }
                }
                else if (element.second.type == "image" &&
                         !(element.second.has("visible") && !element.second.get<bool>("visible"))) {
                    // If this is the first system then forceload to avoid texture pop-in.
                    if (it == SystemData::sSystemVector.front())
                        elements.imageComponents.emplace_back(
                            std::make_unique<ImageComponent>(true));
                    else
                        elements.imageComponents.emplace_back(std::make_unique<ImageComponent>());

                    elements.imageComponents.back()->setDefaultZIndex(30.0f);
                    elements.imageComponents.back()->applyTheme(theme, "system", element.first,
                                                                ThemeFlags::ALL);
                    elements.children.emplace_back(elements.imageComponents.back().get());
                }
                else if (element.second.type == "video" &&
                         !(element.second.has("visible") && !element.second.get<bool>("visible"))) {
                    elements.videoComponents.emplace_back(std::make_unique<VideoFFmpegComponent>());
                    elements.videoComponents.back()->setDefaultZIndex(30.0f);
                    elements.videoComponents.back()->setStaticVideo();
                    elements.videoComponents.back()->applyTheme(theme, "system", element.first,
                                                                ThemeFlags::ALL);
                    elements.children.emplace_back(elements.videoComponents.back().get());
                }
                else if (element.second.type == "animation" && element.second.has("path") &&
                         !(element.second.has("visible") && !element.second.get<bool>("visible"))) {
                    const std::string extension {
                        Utils::FileSystem::getExtension(element.second.get<std::string>("path"))};
                    if (extension == ".json") {
                        elements.lottieAnimComponents.emplace_back(
                            std::make_unique<LottieAnimComponent>());
                        elements.lottieAnimComponents.back()->setDefaultZIndex(35.0f);
                        elements.lottieAnimComponents.back()->applyTheme(
                            theme, "system", element.first, ThemeFlags::ALL);
                        elements.children.emplace_back(elements.lottieAnimComponents.back().get());
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
                        LOG(LogWarning) << "SystemView::populate(): Invalid theme configuration, "
                                           "animation file extension is missing";
                    }
                    else {
                        LOG(LogWarning) << "SystemView::populate(): Invalid theme configuration, "
                                           "animation file extension defined as \""
                                        << extension << "\"";
                    }
                }
                else if (element.second.type == "text" &&
                         !(element.second.has("visible") && !element.second.get<bool>("visible"))) {
                    // Set as container by default if metadata type is "description".
                    bool container {false};
                    if (element.second.has("container")) {
                        container = element.second.get<bool>("container");
                        if (element.second.has("containerType") &&
                            element.second.get<std::string>("containerType") == "horizontal")
                            container = false;
                    }
                    else if (element.second.has("metadata") &&
                             element.second.get<std::string>("metadata") == "description") {
                        container = true;
                    }
                    if (element.second.has("systemdata") &&
                        element.second.get<std::string>("systemdata").substr(0, 9) == "gamecount") {
                        // A vertical container can't be used if systemdata is set to a gamecount
                        // value. A horizontal container can be used though.
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
                        if (container) {
                            elements.containerComponents.push_back(
                                std::make_unique<ScrollableContainer>());
                            elements.containerComponents.back()->setDefaultZIndex(40.0f);
                            elements.containerTextComponents.push_back(
                                std::make_unique<TextComponent>());
                            elements.containerTextComponents.back()->setDefaultZIndex(40.0f);
                            elements.containerComponents.back()->addChild(
                                elements.containerTextComponents.back().get());
                            elements.containerComponents.back()->applyTheme(
                                theme, "system", element.first,
                                ThemeFlags::POSITION | ThemeFlags::SIZE | ThemeFlags::Z_INDEX |
                                    ThemeFlags::VISIBLE);
                            elements.containerComponents.back()->setAutoScroll(true);
                            elements.containerTextComponents.back()->setSize(
                                elements.containerComponents.back()->getSize().x, 0.0f);
                            elements.containerTextComponents.back()->applyTheme(
                                theme, "system", element.first,
                                ThemeFlags::ALL ^ ThemeFlags::POSITION ^ ThemeFlags::ORIGIN ^
                                    ThemeFlags::Z_INDEX ^ ThemeFlags::SIZE ^ ThemeFlags::VISIBLE ^
                                    ThemeFlags::ROTATION);
                            elements.children.emplace_back(
                                elements.containerComponents.back().get());
                        }
                        else {
                            elements.textComponents.emplace_back(std::make_unique<TextComponent>());
                            elements.textComponents.back()->setDefaultZIndex(40.0f);
                            elements.textComponents.back()->applyTheme(
                                theme, "system", element.first, ThemeFlags::ALL);
                            elements.children.emplace_back(elements.textComponents.back().get());
                        }
                    }
                }
                else if (element.second.type == "datetime" &&
                         !(element.second.has("visible") && !element.second.get<bool>("visible"))) {
                    elements.dateTimeComponents.emplace_back(std::make_unique<DateTimeComponent>());
                    elements.dateTimeComponents.back()->setDefaultZIndex(40.0f);
                    elements.dateTimeComponents.back()->applyTheme(theme, "system", element.first,
                                                                   ThemeFlags::ALL);
                    elements.dateTimeComponents.back()->setVisible(false);
                    elements.children.emplace_back(elements.dateTimeComponents.back().get());
                }
                else if (element.second.type == "rating" &&
                         !(element.second.has("visible") && !element.second.get<bool>("visible"))) {
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

        std::stable_sort(
            elements.imageComponents.begin(), elements.imageComponents.end(),
            [](const std::unique_ptr<ImageComponent>& a, const std::unique_ptr<ImageComponent>& b) {
                return b->getZIndex() > a->getZIndex();
            });
        std::stable_sort(
            elements.textComponents.begin(), elements.textComponents.end(),
            [](const std::unique_ptr<TextComponent>& a, const std::unique_ptr<TextComponent>& b) {
                return b->getZIndex() > a->getZIndex();
            });
        std::stable_sort(
            elements.containerTextComponents.begin(), elements.containerTextComponents.end(),
            [](const std::unique_ptr<TextComponent>& a, const std::unique_ptr<TextComponent>& b) {
                return b->getZIndex() > a->getZIndex();
            });
        mSystemElements.emplace_back(std::move(elements));
        mSystemElements.back().helpStyle.applyTheme(theme, "system");

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

        auto letterCaseFunc = [&it, this](std::string& name) {
            LetterCase letterCase {LetterCase::NONE};
            if (it->isCustomCollection()) {
                letterCase = mPrimary->getLetterCaseCustomCollections();
                if (letterCase == LetterCase::UNDEFINED)
                    letterCase = mPrimary->getLetterCase();
            }
            else if (it->isCollection()) {
                letterCase = mPrimary->getLetterCaseAutoCollections();
                if (letterCase == LetterCase::UNDEFINED)
                    letterCase = mPrimary->getLetterCase();
            }
            else {
                letterCase = mPrimary->getLetterCase();
            }

            if (letterCase == LetterCase::UPPERCASE)
                name = Utils::String::toUpper(name);
            else if (letterCase == LetterCase::LOWERCASE)
                name = Utils::String::toLower(name);
            else if (letterCase == LetterCase::CAPITALIZE)
                name = Utils::String::toCapitalized(name);
        };

        if (mCarousel != nullptr) {
            CarouselComponent<SystemData*>::Entry entry;
            if (itemText == "")
                entry.name = it->getFullName();
            else
                entry.name = itemText;
            letterCaseFunc(entry.name);
            entry.object = it;
            entry.data.imagePath = imagePath;
            entry.data.defaultImagePath = defaultImagePath;
            mCarousel->addEntry(entry, theme);
        }
        else if (mGrid != nullptr) {
            GridComponent<SystemData*>::Entry entry;
            if (itemText == "")
                entry.name = it->getFullName();
            else
                entry.name = itemText;
            letterCaseFunc(entry.name);
            entry.object = it;
            entry.data.imagePath = imagePath;
            entry.data.defaultImagePath = defaultImagePath;
            mGrid->addEntry(entry, theme);
        }
        else if (mTextList != nullptr) {
            TextListComponent<SystemData*>::Entry entry;
            entry.name = it->getFullName();
            letterCaseFunc(entry.name);
            entry.object = it;
            entry.data.entryType = TextListEntryType::PRIMARY;
            mTextList->addEntry(entry);
        }

        // Update the game counter here so the text doesn't pop in during initial navigation.
        updateGameCount(it);
    }

    if (mGrid != nullptr)
        mGrid->calculateLayout();

#if defined(GETTEXT_DUMMY_ENTRIES)
    _p("theme", "all");
    _p("theme", "all games");
    _p("theme", "recent");
    _p("theme", "last played");
    _p("theme", "favorites");
    _p("theme", "collections");
#endif

    for (auto& elements : mSystemElements) {
        for (auto& text : elements.textComponents) {
            if (text->getThemeSystemdata() != "") {
                const bool translate {elements.system->isCollection() &&
                                      !elements.system->isCustomCollection()};
                if (text->getThemeSystemdata() == "name") {
                    if (translate)
                        text->setValue(_p("theme", elements.name.c_str()));
                    else
                        text->setValue(elements.name);
                }
                else if (text->getThemeSystemdata() == "fullname") {
                    if (translate)
                        text->setValue(_p("theme", elements.fullName.c_str()));
                    else
                        text->setValue(elements.fullName);
                }
                else {
                    text->setValue(text->getThemeSystemdata());
                }
            }
        }
        for (auto& containerText : elements.containerTextComponents) {
            if (containerText->getThemeSystemdata() != "") {
                const bool translate {elements.system->isCollection() &&
                                      !elements.system->isCustomCollection()};
                if (containerText->getThemeSystemdata() == "name") {
                    if (translate)
                        containerText->setValue(_p("theme", elements.name.c_str()));
                    else
                        containerText->setValue(elements.name);
                }
                else if (containerText->getThemeSystemdata() == "fullname") {
                    if (translate)
                        containerText->setValue(_p("theme", elements.fullName.c_str()));
                    else
                        containerText->setValue(elements.fullName);
                }
                else {
                    containerText->setValue(containerText->getThemeSystemdata());
                }
            }
        }
    }

    mFadeTransitions = (static_cast<ViewTransitionAnimation>(Settings::getInstance()->getInt(
                            "TransitionsSystemToSystem")) == ViewTransitionAnimation::FADE);
}

void SystemView::updateGameCount(SystemData* system)
{
    SystemData* sourceSystem {system == nullptr ? mPrimary->getSelected() : system};

    std::pair<unsigned int, unsigned int> gameCount {sourceSystem->getDisplayedGameCount()};
    std::stringstream ss;
    std::stringstream ssGames;
    std::stringstream ssFavorites;
    bool games {false};
    const bool favoriteSystem {sourceSystem->getName() == "favorites"};
    const bool recentSystem {sourceSystem->getName() == "recent"};

    if (sourceSystem->isCollection() && favoriteSystem) {
        ss << Utils::String::format(_np("theme", "%i game", "%i games", gameCount.first),
                                    gameCount.first);
    }
    else if (sourceSystem->isCollection() && recentSystem) {
        // The "recent" gamelist has probably been trimmed after sorting, so we'll cap it at
        // its maximum limit of 50 games.
        const unsigned int count {gameCount.first > 50 ? 50 : gameCount.first};
        ss << Utils::String::format(_np("theme", "%i game", "%i games", count), count);
    }
    else {
        ss << Utils::String::format(_np("theme", "%i game", "%i games", gameCount.first),
                                    gameCount.first)
           << " "
           << Utils::String::format(
                  _np("theme", "(%i favorite)", "(%i favorites)", gameCount.second),
                  gameCount.second);
        ssGames << Utils::String::format(_np("theme", "%i game", "%i games", gameCount.first),
                                         gameCount.first);
        ssFavorites << Utils::String::format(
            _np("theme", "%i favorite", "%i favorites", gameCount.second), gameCount.second);
        games = true;
    }

    auto elementsIt = std::find_if(mSystemElements.cbegin(), mSystemElements.cend(),
                                   [sourceSystem](const SystemViewElements& systemElements) {
                                       return systemElements.system == sourceSystem;
                                   });
    for (auto& gameCountComp : (*elementsIt).gameCountComponents) {
        if (gameCountComp->getThemeSystemdata() == "gamecount") {
            gameCountComp->setValue(ss.str());
        }
        else if (gameCountComp->getThemeSystemdata() == "gamecountGames") {
            if (games)
                gameCountComp->setValue(ssGames.str());
            else
                gameCountComp->setValue(ss.str());
        }
        else if (gameCountComp->getThemeSystemdata() == "gamecountGamesNoText") {
            gameCountComp->setValue(std::to_string(gameCount.first));
        }
        else if (gameCountComp->getThemeSystemdata() == "gamecountFavorites") {
            gameCountComp->setValue(ssFavorites.str());
        }
        else if (gameCountComp->getThemeSystemdata() == "gamecountFavoritesNoText") {
            if (!favoriteSystem && !recentSystem) {
                gameCountComp->setValue(std::to_string(gameCount.second));
            }
        }
        else {
            gameCountComp->setValue(gameCountComp->getThemeSystemdata());
        }
    }
}

void SystemView::updateGameSelectors()
{
    const int cursor {mPrimary->getCursor()};

    if (mSystemElements[cursor].gameSelectors.size() == 0)
        return;

    const bool multipleSelectors {mSystemElements[cursor].gameSelectors.size() > 1};

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
        const size_t gameSelectorEntry {static_cast<size_t>(
            glm::clamp(image->getThemeGameSelectorEntry(), 0u,
                       static_cast<unsigned int>(gameSelector->getGameCount() - 1)))};
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (games.size() > gameSelectorEntry) {
            std::string path;
            for (auto& imageType : image->getThemeImageTypes()) {
                if (imageType == "image") {
                    path = games.at(gameSelectorEntry)->getImagePath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "miximage") {
                    path = games.at(gameSelectorEntry)->getMiximagePath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "marquee") {
                    path = games.at(gameSelectorEntry)->getMarqueePath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "screenshot") {
                    path = games.at(gameSelectorEntry)->getScreenshotPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "titlescreen") {
                    path = games.at(gameSelectorEntry)->getTitleScreenPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "cover") {
                    path = games.at(gameSelectorEntry)->getCoverPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "backcover") {
                    path = games.at(gameSelectorEntry)->getBackCoverPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "3dbox") {
                    path = games.at(gameSelectorEntry)->get3DBoxPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "physicalmedia") {
                    path = games.at(gameSelectorEntry)->getPhysicalMediaPath();
                    if (path != "") {
                        image->setImage(path);
                        break;
                    }
                }
                else if (imageType == "fanart") {
                    path = games.at(gameSelectorEntry)->getFanArtPath();
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
        if (video->hasStaticVideo() || video->getThemeGameSelector() == ":none:")
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
        const size_t gameSelectorEntry {static_cast<size_t>(
            glm::clamp(video->getThemeGameSelectorEntry(), 0u,
                       static_cast<unsigned int>(gameSelector->getGameCount() - 1)))};
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (games.size() > gameSelectorEntry) {
            if (!video->setVideo(games.at(gameSelectorEntry)->getVideoPath()))
                video->setDefaultVideo();
        }
    }

    for (auto& video : mSystemElements[cursor].videoComponents) {
        if (video->hasStaticVideo() || video->getThemeGameSelector() == ":none:" ||
            (video->getThemeImageTypes().size() == 0 && video->getDefaultImage() == ""))
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
        const size_t gameSelectorEntry {static_cast<size_t>(
            glm::clamp(video->getThemeGameSelectorEntry(), 0u,
                       static_cast<unsigned int>(gameSelector->getGameCount() - 1)))};
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (games.size() > gameSelectorEntry) {
            std::string path;
            for (auto& imageType : video->getThemeImageTypes()) {
                if (imageType == "image") {
                    path = games.at(gameSelectorEntry)->getImagePath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "miximage") {
                    path = games.at(gameSelectorEntry)->getMiximagePath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "marquee") {
                    path = games.at(gameSelectorEntry)->getMarqueePath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "screenshot") {
                    path = games.at(gameSelectorEntry)->getScreenshotPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "titlescreen") {
                    path = games.at(gameSelectorEntry)->getTitleScreenPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "cover") {
                    path = games.at(gameSelectorEntry)->getCoverPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "backcover") {
                    path = games.at(gameSelectorEntry)->getBackCoverPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "3dbox") {
                    path = games.at(gameSelectorEntry)->get3DBoxPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "physicalmedia") {
                    path = games.at(gameSelectorEntry)->getPhysicalMediaPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
                else if (imageType == "fanart") {
                    path = games.at(gameSelectorEntry)->getFanArtPath();
                    if (path != "") {
                        video->setImage(path);
                        break;
                    }
                }
            }
            // This is needed so the default image is set if no game media was found.
            if (path == "" &&
                (video->getThemeImageTypes().size() > 0 || video->getDefaultImage() != ""))
                video->setImage("");
        }
        else {
            video->setImage("");
        }
    }

    auto textSelectorFunc = [this, cursor,
                             multipleSelectors](std::unique_ptr<TextComponent>& text) {
        if (text->getThemeMetadata() == "")
            return;
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
        const size_t gameSelectorEntry {static_cast<size_t>(
            glm::clamp(text->getThemeGameSelectorEntry(), 0u,
                       static_cast<unsigned int>(gameSelector->getGameCount() - 1)))};
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (games.size() > gameSelectorEntry) {
            const std::string metadata {text->getThemeMetadata()};
            if (metadata == "name") {
                if (mPrimary->getSelected()->isCollection() && text->getSystemNameSuffix()) {
                    const LetterCase letterCase {text->getLetterCaseSystemNameSuffix()};
                    std::string suffix {" ["};
                    if (letterCase == LetterCase::UPPERCASE) {
                        suffix.append(Utils::String::toUpper(games.at(gameSelectorEntry)
                                                                 ->getSourceFileData()
                                                                 ->getSystem()
                                                                 ->getName()));
                    }
                    else if (letterCase == LetterCase::CAPITALIZE) {
                        suffix.append(Utils::String::toCapitalized(games.at(gameSelectorEntry)
                                                                       ->getSourceFileData()
                                                                       ->getSystem()
                                                                       ->getName()));
                    }
                    else {
                        suffix.append(games.at(gameSelectorEntry)
                                          ->getSourceFileData()
                                          ->getSystem()
                                          ->getName());
                    }
                    suffix.append("]");
                    text->setValue(games.at(gameSelectorEntry)->metadata.get("name") + suffix);
                }
                else {
                    text->setValue(games.at(gameSelectorEntry)->metadata.get("name"));
                }
            }
            else if (metadata == "description")
                text->setValue(games.at(gameSelectorEntry)->metadata.get("desc"));
            else if (metadata == "rating")
                text->setValue(RatingComponent::getRatingValue(
                    games.at(gameSelectorEntry)->metadata.get("rating")));
            else if (metadata == "developer")
                text->setValue(games.at(gameSelectorEntry)->metadata.get("developer") == "unknown" ?
                                   _p("theme", "unknown") :
                                   games.at(gameSelectorEntry)->metadata.get("developer"));
            else if (metadata == "publisher")
                text->setValue(games.at(gameSelectorEntry)->metadata.get("publisher") == "unknown" ?
                                   _p("theme", "unknown") :
                                   games.at(gameSelectorEntry)->metadata.get("publisher"));
            else if (metadata == "genre")
                text->setValue(games.at(gameSelectorEntry)->metadata.get("genre") == "unknown" ?
                                   _p("theme", "unknown") :
                                   games.at(gameSelectorEntry)->metadata.get("genre"));
            else if (metadata == "players")
                text->setValue(games.at(gameSelectorEntry)->metadata.get("players") == "unknown" ?
                                   _p("theme", "unknown") :
                                   games.at(gameSelectorEntry)->metadata.get("players"));
            else if (metadata == "favorite")
                text->setValue(
                    games.at(gameSelectorEntry)->metadata.get("favorite") == "true" ? "yes" : "no");
            else if (metadata == "completed")
                text->setValue(games.at(gameSelectorEntry)->metadata.get("completed") == "true" ?
                                   "yes" :
                                   "no");
            else if (metadata == "kidgame")
                text->setValue(
                    games.at(gameSelectorEntry)->metadata.get("kidgame") == "true" ? "yes" : "no");
            else if (metadata == "broken")
                text->setValue(
                    games.at(gameSelectorEntry)->metadata.get("broken") == "true" ? "yes" : "no");
            else if (metadata == "manual")
                text->setValue(games.at(gameSelectorEntry)->getManualPath() != "" ? "yes" : "no");
            else if (metadata == "playcount")
                text->setValue(games.at(gameSelectorEntry)->metadata.get("playcount"));
            else if (metadata == "altemulator")
                text->setValue(games.at(gameSelectorEntry)->metadata.get("altemulator"));
            else if (metadata == "emulator")
                text->setValue(
                    games.at(gameSelectorEntry)->metadata.get("altemulator") != "" ?
                        games.at(gameSelectorEntry)->metadata.get("altemulator") :
                        (games.at(gameSelectorEntry)->getSourceSystem()->getAlternativeEmulator() !=
                                 "" ?
                             games.at(gameSelectorEntry)
                                 ->getSourceSystem()
                                 ->getAlternativeEmulator() :
                             games.at(gameSelectorEntry)
                                 ->getSourceSystem()
                                 ->getSystemEnvData()
                                 ->mLaunchCommands.front()
                                 .second));
            else if (metadata == "physicalName")
                text->setValue(
                    Utils::FileSystem::getStem(games.at(gameSelectorEntry)->getFileName()));
            else if (metadata == "physicalNameExtension")
                text->setValue(games.at(gameSelectorEntry)->getFileName());
            else if (metadata == "systemName")
                text->setValue(games.at(gameSelectorEntry)->getSystem()->getName());
            else if (metadata == "systemFullname")
                text->setValue(games.at(gameSelectorEntry)->getSystem()->getFullName());
            else if (metadata == "sourceSystemName")
                text->setValue(
                    games.at(gameSelectorEntry)->getSourceFileData()->getSystem()->getName());
            else if (metadata == "sourceSystemFullname")
                text->setValue(
                    games.at(gameSelectorEntry)->getSourceFileData()->getSystem()->getFullName());
        }
        else {
            text->setValue("");
        }
    };

    for (auto& text : mSystemElements[cursor].textComponents)
        textSelectorFunc(text);

    for (auto& containerText : mSystemElements[cursor].containerTextComponents)
        textSelectorFunc(containerText);

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
        const size_t gameSelectorEntry {static_cast<size_t>(
            glm::clamp(dateTime->getThemeGameSelectorEntry(), 0u,
                       static_cast<unsigned int>(gameSelector->getGameCount() - 1)))};
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (games.size() > gameSelectorEntry) {
            dateTime->setVisible(true);
            const std::string metadata {dateTime->getThemeMetadata()};
            if (metadata == "releasedate")
                dateTime->setValue(games.at(gameSelectorEntry)->metadata.get("releasedate"));
            if (metadata == "lastplayed")
                dateTime->setValue(games.at(gameSelectorEntry)->metadata.get("lastplayed"));
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
        const size_t gameSelectorEntry {static_cast<size_t>(
            glm::clamp(rating->getThemeGameSelectorEntry(), 0u,
                       static_cast<unsigned int>(gameSelector->getGameCount() - 1)))};
        gameSelector->refreshGames();
        std::vector<FileData*> games {gameSelector->getGames()};
        if (games.size() > gameSelectorEntry) {
            rating->setVisible(true);
            rating->setValue(games.at(gameSelectorEntry)->metadata.get("rating"));
        }
        else {
            rating->setVisible(false);
        }
    }
}

void SystemView::renderElements(const glm::mat4& parentTrans, bool abovePrimary)
{
    glm::mat4 trans {getTransform() * parentTrans};

    const float primaryZIndex {mPrimary->getZIndex()};

    int renderBefore {static_cast<int>(mCamOffset)};
    int renderAfter {static_cast<int>(mCamOffset)};

    const ViewController::State viewState {ViewController::getInstance()->getState()};

    // If we're transitioning between systems, then also render the previous and next systems.
    if (isAnimationPlaying(0) && viewState.viewing == ViewController::ViewMode::SYSTEM_SELECT) {
        renderBefore -= 1;
        renderAfter += 1;
    }

    bool stationaryApplicable {false};

    // If it's the startup animation, then don't apply stationary properties.
    if (viewState.previouslyViewed == ViewController::ViewMode::NOTHING)
        stationaryApplicable = false;

    // If it's a system to system transition and these animations are set to slide.
    if (static_cast<ViewTransitionAnimation>(Settings::getInstance()->getInt(
            "TransitionsSystemToSystem")) == ViewTransitionAnimation::SLIDE &&
        isAnimationPlaying(0))
        stationaryApplicable = true;

    // If it's a system to gamelist transition and these animations are set to slide.
    if (static_cast<ViewTransitionAnimation>(Settings::getInstance()->getInt(
            "TransitionsSystemToGamelist")) == ViewTransitionAnimation::SLIDE &&
        viewState.viewing == ViewController::ViewMode::GAMELIST)
        stationaryApplicable = true;

    // If it's a gamelist to system transition and these animations are set to slide.
    if (static_cast<ViewTransitionAnimation>(Settings::getInstance()->getInt(
            "TransitionsGamelistToSystem")) == ViewTransitionAnimation::SLIDE &&
        viewState.previouslyViewed == ViewController::ViewMode::GAMELIST &&
        ViewController::getInstance()->isCameraMoving())
        stationaryApplicable = true;

    for (int i {renderBefore}; i <= renderAfter; ++i) {
        int index {i};
        while (index < 0)
            index += static_cast<int>(mPrimary->getNumEntries());
        while (index >= static_cast<int>(mPrimary->getNumEntries()))
            index -= static_cast<int>(mPrimary->getNumEntries());

        if (isAnimationPlaying(0) || index == mPrimary->getCursor()) {
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
            else if (mGrid != nullptr) {
                elementTrans = glm::translate(
                    elementTrans, glm::round(glm::vec3 {0.0f, (i - mCamOffset) * mSize.y, 0.0f}));
            }
            else if (mTextList != nullptr) {
                elementTrans = glm::translate(
                    elementTrans, glm::round(glm::vec3 {0.0f, (i - mCamOffset) * mSize.y, 0.0f}));
            }

            auto clipRectFunc = [this, elementTrans]() {
                mRenderer->pushClipRect(
                    glm::ivec2 {static_cast<int>(glm::round(elementTrans[3].x)),
                                static_cast<int>(glm::round(elementTrans[3].y))},
                    glm::ivec2 {static_cast<int>(mSize.x), static_cast<int>(mSize.y)});
            };

            auto renderChildCondFunc = [&viewState](GuiComponent* child, glm::mat4 trans) {
                bool renderChild {false};
                if (!ViewController::getInstance()->isCameraMoving())
                    renderChild = true;
                else if (viewState.previouslyViewed == ViewController::ViewMode::NOTHING)
                    renderChild = true;
                else if (viewState.viewing == viewState.previouslyViewed)
                    renderChild = true;
                else if (static_cast<ViewTransitionAnimation>(Settings::getInstance()->getInt(
                             "TransitionsSystemToGamelist")) != ViewTransitionAnimation::SLIDE &&
                         viewState.viewing == ViewController::ViewMode::GAMELIST)
                    renderChild = true;
                if (renderChild)
                    child->render(trans);
            };

            clipRectFunc();

            if (mSystemElements.size() > static_cast<size_t>(index)) {
                for (GuiComponent* child : mSystemElements[index].children) {
                    bool renderChild {true};
                    bool childStationary {false};
                    if (stationaryApplicable) {
                        if (child->getStationary() == Stationary::NEVER) {
                            childStationary = false;
                        }
                        else if ((child->getStationary() == Stationary::WITHIN_VIEW ||
                                  child->getStationary() == Stationary::ALWAYS) &&
                                 isAnimationPlaying(0)) {
                            childStationary = true;
                            if (index != static_cast<int>(std::round(mCamOffset))) {
                                if (mCamOffset <= mSystemElements.size() - 1)
                                    renderChild = false;
                                if (mCamOffset > static_cast<float>(mSystemElements.size() - 1) &&
                                    index != 0)
                                    renderChild = false;
                                if (mCamOffset <
                                        static_cast<float>(mSystemElements.size()) - 0.5f &&
                                    index == 0)
                                    renderChild = false;
                            }
                        }
                        else if ((child->getStationary() == Stationary::BETWEEN_VIEWS ||
                                  child->getStationary() == Stationary::ALWAYS) &&
                                 !isAnimationPlaying(0)) {
                            childStationary = true;
                        }
                    }

                    if (abovePrimary && (child->getZIndex() > primaryZIndex)) {
                        if (mFadeTransitions && mPrimary->getFadeAbovePrimary()) {
                            if (mFadeTransitions || child->getOpacity() != 1.0f)
                                child->setOpacity(1.0f - mFadeOpacity);
                        }
                        else {
                            child->setOpacity(1.0f);
                        }
                        if (renderChild) {
                            if (childStationary) {
                                mRenderer->popClipRect();
                                if (child->getRenderDuringTransitions())
                                    child->render(mRenderer->getIdentity());
                                else
                                    renderChildCondFunc(child, mRenderer->getIdentity());
                                clipRectFunc();
                            }
                            else {
                                if (child->getRenderDuringTransitions())
                                    child->render(elementTrans);
                                else
                                    renderChildCondFunc(child, elementTrans);
                            }
                        }
                    }
                    else if (!abovePrimary && child->getZIndex() <= primaryZIndex) {
                        if (mFadeTransitions || child->getDimming() != 1.0f)
                            child->setDimming(1.0f - mFadeOpacity);
                        if (renderChild) {
                            if (childStationary) {
                                mRenderer->popClipRect();
                                if (child->getRenderDuringTransitions())
                                    child->render(mRenderer->getIdentity());
                                else
                                    renderChildCondFunc(child, mRenderer->getIdentity());
                                clipRectFunc();
                            }
                            else {
                                if (child->getRenderDuringTransitions())
                                    child->render(elementTrans);
                                else
                                    renderChildCondFunc(child, elementTrans);
                            }
                        }
                    }
                }
            }

            mRenderer->popClipRect();
        }
    }
}

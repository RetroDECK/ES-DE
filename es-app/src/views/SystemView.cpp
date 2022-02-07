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
    const int logoBuffersLeft[] = {-5, -2, -1};
    const int logoBuffersRight[] = {1, 2, 5};

} // namespace

SystemView::SystemView()
    : mCamOffset {0.0f}
    , mFadeOpacity {0.0f}
    , mUpdatedGameCount {false}
    , mViewNeedsReload {true}
    , mLegacyMode {false}
{
    setSize(static_cast<float>(Renderer::getScreenWidth()),
            static_cast<float>(Renderer::getScreenHeight()));

    mCarousel = std::make_unique<CarouselComponent>();
    mCarousel->setCursorChangedCallback([&](const CursorState& state) { onCursorChanged(state); });
    mCarousel->setCancelTransitionsCallback(
        [&] { ViewController::getInstance()->cancelViewTransitions(); });

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

void SystemView::goToSystem(SystemData* system, bool animate)
{
    mCarousel->setCursor(system);
    updateGameCount();

    if (!animate)
        finishSystemAnimation(0);
}

bool SystemView::input(InputConfig* config, Input input)
{
    if (input.value != 0) {
        if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_r &&
            SDL_GetModState() & KMOD_LCTRL && Settings::getInstance()->getBool("Debug")) {
            LOG(LogDebug) << "SystemView::input(): Reloading all";
            ViewController::getInstance()->reloadAll();
            return true;
        }

        if (config->isMappedTo("a", input)) {
            mCarousel->stopScrolling();
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

    return mCarousel->input(config, input);
}

void SystemView::update(int deltaTime)
{
    mCarousel->update(deltaTime);
    GuiComponent::update(deltaTime);
}

void SystemView::render(const glm::mat4& parentTrans)
{
    if (mCarousel->getNumEntries() == 0)
        return; // Nothing to render.

    glm::mat4 trans {getTransform() * parentTrans};

    // Adding texture loading buffers depending on scrolling speed and status.
    int bufferIndex {mCarousel->getScrollingVelocity() + 1};

    Renderer::pushClipRect(glm::ivec2 {},
                           glm::ivec2 {static_cast<int>(mSize.x), static_cast<int>(mSize.y)});

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
                glm::ivec2 {static_cast<int>(elementTrans[3].x),
                            static_cast<int>(elementTrans[3].y)},
                glm::ivec2 {static_cast<int>(mSize.x), static_cast<int>(mSize.y)});

            if (mLegacyMode && mSystemElements.size() > static_cast<size_t>(index)) {
                for (auto element : mSystemElements[index].legacyExtras)
                    element->render(elementTrans);
            }
            else if (mSystemElements.size() > static_cast<size_t>(index)) {
                for (auto child : mSystemElements[index].children) {
                    if (child == mCarousel.get()) {
                        // Render black above anything lower than the zIndex of the carousel
                        // if fade transitions are in use and we're transitioning.
                        if (mFadeOpacity)
                            renderFade(trans);
                        child->render(trans);
                    }
                    else {
                        child->render(elementTrans);
                    }
                }
            }

            if (mLegacyMode)
                mLegacySystemInfo->render(elementTrans);

            Renderer::popClipRect();
        }
    }

    if (mLegacyMode) {
        if (mFadeOpacity)
            renderFade(trans);
        mCarousel->render(trans);
    }

    Renderer::popClipRect();
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
    // Update help style.
    updateHelpPrompts();

    int scrollVelocity {mCarousel->getScrollingVelocity()};

    float startPos {mCamOffset};
    float posMax {static_cast<float>(mCarousel->getNumEntries())};
    float target {static_cast<float>(mCarousel->getCursor())};

    // Find the shortest path to the target.
    float endPos {target}; // Directly.
    float dist {fabs(endPos - startPos)};

    if (fabs(target + posMax - startPos - scrollVelocity) < dist)
        endPos = target + posMax; // Loop around the end (0 -> max).
    if (fabs(target - posMax - startPos - scrollVelocity) < dist)
        endPos = target - posMax; // Loop around the start (max - 1 -> -1).

    std::string transition_style {Settings::getInstance()->getString("TransitionStyle")};

    Animation* anim;

    if (transition_style == "fade") {
        float startFade {mFadeOpacity};
        anim = new LambdaAnimation(
            [this, startFade, startPos, endPos, posMax](float t) {
                t -= 1;
                float f = glm::mix(startPos, endPos, t * t * t + 1);
                if (f < 0)
                    f += posMax;
                if (f >= posMax)
                    f -= posMax;

                t += 1;
                if (t < 0.3f)
                    this->mFadeOpacity =
                        glm::mix(0.0f, 1.0f, glm::clamp(t / 0.2f + startFade, 0.0f, 1.0f));
                else if (t < 0.7f)
                    this->mFadeOpacity = 1.0f;
                else
                    this->mFadeOpacity =
                        glm::mix(1.0f, 0.0f, glm::clamp((t - 0.6f) / 0.3f, 0.0f, 1.0f));

                if (t > 0.5f)
                    this->mCamOffset = endPos;

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
                float f = glm::mix(startPos, endPos, t * t * t + 1);
                if (f < 0)
                    f += posMax;
                if (f >= posMax)
                    f -= posMax;

                this->mCamOffset = f;

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
                float f = glm::mix(startPos, endPos, t * t * t + 1);
                if (f < 0)
                    f += posMax;
                if (f >= posMax)
                    f -= posMax;

                this->mCamOffset = endPos;
            },
            500);
    }

    setAnimation(anim, 0, nullptr, false, 0);
}

void SystemView::populate()
{
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

        if (mViewNeedsReload)
            getViewElements(theme);

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
                        if (elements.imageComponents.back()->getMetadataField() != "")
                            elements.imageComponents.back()->setScrollHide(true);
                        elements.children.emplace_back(elements.imageComponents.back().get());
                    }
                    else if (element.second.type == "text") {
                        if (element.second.has("metadata") &&
                            element.second.get<std::string>("metadata").substr(0, 12) ==
                                "sy_gamecount") {
                            if (element.second.has("metadata")) {
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
                }
            }

            elements.children.emplace_back(mCarousel.get());

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
            if (text->getMetadataField() != "") {
                if (text->getMetadataField() == "sy_name")
                    text->setValue(elements.name);
                else if (text->getMetadataField() == "sy_fullname")
                    text->setValue(elements.fullName);
                else
                    text->setValue(text->getMetadataField());
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
    bool favorites {false};

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
        favorites = true;
    }

    if (mLegacyMode) {
        mLegacySystemInfo->setText(ss.str());
    }
    else {
        for (auto& gameCount : mSystemElements[mCarousel->getCursor()].gameCountComponents) {
            if (gameCount->getMetadataField() == "sy_gamecount") {
                gameCount->setValue(ss.str());
            }
            else if (gameCount->getMetadataField() == "sy_gamecount_games") {
                if (games)
                    gameCount->setValue(ssGames.str());
                else
                    gameCount->setValue(ss.str());
            }
            else if (gameCount->getMetadataField() == "sy_gamecount_favorites") {
                gameCount->setValue(ssFavorites.str());
            }
            else {
                gameCount->setValue(gameCount->getMetadataField());
            }
        }
    }
}

void SystemView::getViewElements(const std::shared_ptr<ThemeData>& theme)
{
    LOG(LogDebug) << "SystemView::getViewElements()";

    if (theme->hasView("system"))
        mViewNeedsReload = false;
    else
        mViewNeedsReload = true;

    if (mLegacyMode) {
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
}

void SystemView::renderFade(const glm::mat4& trans)
{
    unsigned int fadeColor {0x00000000 | static_cast<unsigned int>(mFadeOpacity * 255.0f)};
    Renderer::setMatrix(trans);
    Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, fadeColor, fadeColor);
}

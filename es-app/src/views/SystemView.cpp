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
#include "SystemData.h"
#include "UIModeController.h"
#include "Window.h"
#include "animations/LambdaAnimation.h"
#include "guis/GuiMsgBox.h"
#include "views/ViewController.h"

#if defined(_WIN64)
#include <cmath>
#endif

// Buffer values for scrolling velocity (left, stopped, right).
const int logoBuffersLeft[] = {-5, -2, -1};
const int logoBuffersRight[] = {1, 2, 5};

SystemView::SystemView(Window* window)
    : IList<SystemViewData, SystemData*> {window, LIST_SCROLL_STYLE_SLOW, LIST_ALWAYS_LOOP}
    , mSystemInfo {window, "SYSTEM INFO", Font::get(FONT_SIZE_SMALL), 0x33333300, ALIGN_CENTER}
    , mPreviousScrollVelocity {0}
    , mUpdatedGameCount {false}
    , mViewNeedsReload {true}
{
    mCamOffset = 0;
    mExtrasCamOffset = 0;
    mExtrasFadeOpacity = 0.0f;

    setSize(static_cast<float>(Renderer::getScreenWidth()),
            static_cast<float>(Renderer::getScreenHeight()));
    populate();
}

SystemView::~SystemView()
{
    // Delete any existing extras.
    for (auto entry : mEntries) {
        for (auto extra : entry.data.backgroundExtras)
            delete extra;
        entry.data.backgroundExtras.clear();
    }
}

void SystemView::populate()
{
    mEntries.clear();

    for (auto it : SystemData::sSystemVector) {
        const std::shared_ptr<ThemeData>& theme = it->getTheme();

        if (mViewNeedsReload)
            getViewElements(theme);

        if (it->isVisible()) {
            Entry e;
            e.name = it->getName();
            e.object = it;

            // Component offset. Used for positioning placeholders.
            glm::vec3 offsetLogo = {0.0f, 0.0f, 0.0f};
            glm::vec3 offsetLogoPlaceholderText = {0.0f, 0.0f, 0.0f};

            // Make logo.
            const ThemeData::ThemeElement* logoElem = theme->getElement("system", "logo", "image");
            if (logoElem) {
                auto path = logoElem->get<std::string>("path");
                std::string defaultPath =
                    logoElem->has("default") ? logoElem->get<std::string>("default") : "";
                if ((!path.empty() && ResourceManager::getInstance().fileExists(path)) ||
                    (!defaultPath.empty() &&
                     ResourceManager::getInstance().fileExists(defaultPath))) {
                    auto* logo = new ImageComponent(mWindow, false, false);
                    logo->setMaxSize(glm::round(mCarousel.logoSize * mCarousel.logoScale));
                    logo->applyTheme(theme, "system", "logo", ThemeFlags::PATH | ThemeFlags::COLOR);
                    logo->setRotateByTargetSize(true);
                    e.data.logo = std::shared_ptr<GuiComponent>(logo);
                }
            }

            // No logo available? Make placeholder.
            if (!e.data.logo) {

                glm::vec2 resolution {static_cast<float>(Renderer::getScreenWidth()),
                                      static_cast<float>(Renderer::getScreenHeight())};
                glm::vec3 center {resolution.x / 2.0f, resolution.y / 2.0f, 1.0f};

                // Placeholder Image.
                logoElem = theme->getElement("system", "logoPlaceholderImage", "image");
                if (logoElem) {
                    auto path = logoElem->get<std::string>("path");
                    std::string defaultPath =
                        logoElem->has("default") ? logoElem->get<std::string>("default") : "";
                    if ((!path.empty() && ResourceManager::getInstance().fileExists(path)) ||
                        (!defaultPath.empty() &&
                         ResourceManager::getInstance().fileExists(defaultPath))) {
                        auto* logo = new ImageComponent(mWindow, false, false);
                        logo->applyTheme(theme, "system", "logoPlaceholderImage", ThemeFlags::ALL);
                        if (!logoElem->has("size"))
                            logo->setMaxSize(mCarousel.logoSize * mCarousel.logoScale);
                        offsetLogo = logo->getPosition() - center;
                        logo->setRotateByTargetSize(true);
                        e.data.logo = std::shared_ptr<GuiComponent>(logo);
                    }
                }

                // Placeholder Text.
                const ThemeData::ThemeElement* logoPlaceholderText =
                    theme->getElement("system", "logoPlaceholderText", "text");
                if (logoPlaceholderText) {
                    // Element 'logoPlaceholderText' found in theme: place text
                    auto* text =
                        new TextComponent(mWindow, it->getName(), Font::get(FONT_SIZE_LARGE),
                                          0x000000FF, ALIGN_CENTER);
                    text->setSize(mCarousel.logoSize * mCarousel.logoScale);
                    if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL) {
                        text->setHorizontalAlignment(mCarousel.logoAlignment);
                        text->setVerticalAlignment(ALIGN_CENTER);
                    }
                    else {
                        text->setHorizontalAlignment(ALIGN_CENTER);
                        text->setVerticalAlignment(mCarousel.logoAlignment);
                    }
                    text->applyTheme(it->getTheme(), "system", "logoPlaceholderText",
                                     ThemeFlags::ALL);
                    if (!e.data.logo) {
                        e.data.logo = std::shared_ptr<GuiComponent>(text);
                        offsetLogo = text->getPosition() - center;
                    }
                    else {
                        e.data.logoPlaceholderText = std::shared_ptr<GuiComponent>(text);
                        offsetLogoPlaceholderText = text->getPosition() - center;
                    }
                }
                else {
                    // Fallback to legacy centered placeholder text.
                    auto* text =
                        new TextComponent(mWindow, it->getName(), Font::get(FONT_SIZE_LARGE),
                                          0x000000FF, ALIGN_CENTER);
                    text->setSize(mCarousel.logoSize * mCarousel.logoScale);
                    text->applyTheme(it->getTheme(), "system", "logoText",
                                     ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE |
                                         ThemeFlags::COLOR | ThemeFlags::FORCE_UPPERCASE |
                                         ThemeFlags::LINE_SPACING | ThemeFlags::TEXT);
                    e.data.logo = std::shared_ptr<GuiComponent>(text);

                    if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL) {
                        text->setHorizontalAlignment(mCarousel.logoAlignment);
                        text->setVerticalAlignment(ALIGN_CENTER);
                    }
                    else {
                        text->setHorizontalAlignment(ALIGN_CENTER);
                        text->setVerticalAlignment(mCarousel.logoAlignment);
                    }
                }
            }

            if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL) {
                if (mCarousel.logoAlignment == ALIGN_LEFT)
                    e.data.logo->setOrigin(0, 0.5);
                else if (mCarousel.logoAlignment == ALIGN_RIGHT)
                    e.data.logo->setOrigin(1.0, 0.5);
                else
                    e.data.logo->setOrigin(0.5, 0.5);
            }
            else {
                if (mCarousel.logoAlignment == ALIGN_TOP)
                    e.data.logo->setOrigin(0.5, 0);
                else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
                    e.data.logo->setOrigin(0.5, 1);
                else
                    e.data.logo->setOrigin(0.5, 0.5);
            }

            glm::vec2 denormalized {mCarousel.logoSize * e.data.logo->getOrigin()};
            glm::vec3 v = {denormalized.x, denormalized.y, 0.0f};
            e.data.logo->setPosition(v + offsetLogo);
            if (e.data.logoPlaceholderText)
                e.data.logoPlaceholderText->setPosition(v + offsetLogoPlaceholderText);

            // Make background extras.
            e.data.backgroundExtras = ThemeData::makeExtras(it->getTheme(), "system", mWindow);

            // Sort the extras by z-index.
            std::stable_sort(
                e.data.backgroundExtras.begin(), e.data.backgroundExtras.end(),
                [](GuiComponent* a, GuiComponent* b) { return b->getZIndex() > a->getZIndex(); });

            this->add(e);
        }
    }
    if (mEntries.empty()) {
        // Something is wrong, there is not a single system to show, check if UI mode is not full.
        if (!UIModeController::getInstance()->isUIModeFull()) {
            Settings::getInstance()->setString("UIMode", "full");
            mWindow->pushGui(new GuiMsgBox(
                mWindow, getHelpStyle(),
                "The selected UI mode has nothing to show,\n returning to UI mode \"Full\"", "OK",
                nullptr));
        }
    }
}

void SystemView::updateGameCount()
{
    std::pair<unsigned int, unsigned int> gameCount = getSelected()->getDisplayedGameCount();
    std::stringstream ss;

    if (!getSelected()->isGameSystem())
        ss << "CONFIGURATION";
    else if (getSelected()->isCollection() && (getSelected()->getName() == "favorites"))
        ss << gameCount.first << " GAME" << (gameCount.first == 1 ? " " : "S");
    // The "recent" gamelist has probably been trimmed after sorting, so we'll cap it at
    // its maximum limit of 50 games.
    else if (getSelected()->isCollection() && (getSelected()->getName() == "recent"))
        ss << (gameCount.first > 50 ? 50 : gameCount.first) << " GAME"
           << (gameCount.first == 1 ? " " : "S");
    else
        ss << gameCount.first << " GAME" << (gameCount.first == 1 ? " " : "S ") << "("
           << gameCount.second << " FAVORITE" << (gameCount.second == 1 ? ")" : "S)");

    mSystemInfo.setText(ss.str());
}

void SystemView::goToSystem(SystemData* system, bool animate)
{
    setCursor(system);
    updateGameCount();

    if (!animate)
        finishAnimation(0);
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

        switch (mCarousel.type) {
            case VERTICAL:
            case VERTICAL_WHEEL:
                if (config->isMappedLike("up", input)) {
                    ViewController::getInstance()->cancelViewTransitions();
                    listInput(-1);
                    return true;
                }
                if (config->isMappedLike("down", input)) {
                    ViewController::getInstance()->cancelViewTransitions();
                    listInput(1);
                    return true;
                }
                break;
            case HORIZONTAL:
            case HORIZONTAL_WHEEL:
            default:
                if (config->isMappedLike("left", input)) {
                    ViewController::getInstance()->cancelViewTransitions();
                    listInput(-1);
                    return true;
                }
                if (config->isMappedLike("right", input)) {
                    ViewController::getInstance()->cancelViewTransitions();
                    listInput(1);
                    return true;
                }
                break;
        }

        if (config->isMappedTo("a", input)) {
            stopScrolling();
            ViewController::getInstance()->goToGamelist(getSelected());
            NavigationSounds::getInstance().playThemeNavigationSound(SELECTSOUND);
            return true;
        }
        if (Settings::getInstance()->getBool("RandomAddButton") &&
            (config->isMappedTo("leftthumbstickclick", input) ||
             config->isMappedTo("rightthumbstickclick", input))) {
            // Get a random system and jump to it.
            NavigationSounds::getInstance().playThemeNavigationSound(SYSTEMBROWSESOUND);
            setCursor(SystemData::getRandomSystem(getSelected()));
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
        if (config->isMappedLike("left", input) || config->isMappedLike("right", input) ||
            config->isMappedLike("up", input) || config->isMappedLike("down", input))
            listInput(0);
    }

    return GuiComponent::input(config, input);
}

void SystemView::update(int deltaTime)
{
    listUpdate(deltaTime);
    GuiComponent::update(deltaTime);
}

void SystemView::onCursorChanged(const CursorState& /*state*/)
{
    // Update help style.
    updateHelpPrompts();

    float startPos = mCamOffset;
    float posMax = static_cast<float>(mEntries.size());
    float target = static_cast<float>(mCursor);

    // Find the shortest path to the target.
    float endPos = target; // Directly.
    float dist = fabs(endPos - startPos);

    if (fabs(target + posMax - startPos - mScrollVelocity) < dist)
        endPos = target + posMax; // Loop around the end (0 -> max).
    if (fabs(target - posMax - startPos - mScrollVelocity) < dist)
        endPos = target - posMax; // Loop around the start (max - 1 -> -1).

    // This logic is only needed when there are two game systems, to prevent ugly jumps back
    // an forth when selecting the same direction rapidly several times in a row.
    if (posMax == 2) {
        if (mPreviousScrollVelocity == 0)
            mPreviousScrollVelocity = mScrollVelocity;
        else if (mScrollVelocity < 0 && startPos < endPos)
            mPreviousScrollVelocity = -1;
        else if (mScrollVelocity > 0 && startPos > endPos)
            mPreviousScrollVelocity = 1;
    }

    std::string transition_style = Settings::getInstance()->getString("TransitionStyle");

    // To prevent ugly jumps with two systems when quickly repeating the same direction.
    if (mPreviousScrollVelocity != 0 && posMax == 2 && mScrollVelocity == mPreviousScrollVelocity) {
        if (fabs(endPos - startPos) < 0.5 || fabs(endPos - startPos) > 1.5) {
            (mScrollVelocity < 0) ? endPos -= 1 : endPos += 1;
            (mCursor == 0) ? mCursor = 1 : mCursor = 0;
            updateGameCount();
            return;
        }
    }

    // No need to animate transition, we're not going anywhere (probably mEntries.size() == 1).
    if (endPos == mCamOffset && endPos == mExtrasCamOffset)
        return;

    Animation* anim;

    if (transition_style == "fade") {
        float startExtrasFade = mExtrasFadeOpacity;
        anim = new LambdaAnimation(
            [this, startExtrasFade, startPos, endPos, posMax](float t) {
                t -= 1;
                float f = glm::mix(startPos, endPos, t * t * t + 1);
                if (f < 0)
                    f += posMax;
                if (f >= posMax)
                    f -= posMax;

                this->mCamOffset = f;

                t += 1;
                if (t < 0.3f)
                    this->mExtrasFadeOpacity =
                        glm::mix(0.0f, 1.0f, glm::clamp(t / 0.2f + startExtrasFade, 0.0f, 1.0f));
                else if (t < 0.7f)
                    this->mExtrasFadeOpacity = 1.0f;
                else
                    this->mExtrasFadeOpacity =
                        glm::mix(1.0f, 0.0f, glm::clamp((t - 0.6f) / 0.3f, 0.0f, 1.0f));

                if (t > 0.5f)
                    this->mExtrasCamOffset = endPos;

                // Update the game count when the entire animation has been completed.
                if (mExtrasFadeOpacity == 1.0f)
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
                this->mExtrasCamOffset = f;

                // Hack to make the game count being updated in the middle of the animation.
                bool update = false;
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

                this->mCamOffset = f;
                this->mExtrasCamOffset = endPos;
            },
            500);
    }

    setAnimation(anim, 0, nullptr, false, 0);
}

void SystemView::render(const glm::mat4& parentTrans)
{
    if (size() == 0)
        return; // Nothing to render.

    glm::mat4 trans {getTransform() * parentTrans};

    if (mCarousel.legacyZIndexMode) {
        // Render all extras.
        renderExtras(trans, INT16_MIN, INT16_MAX);

        // Fade the screen if we're using fade transitions and we're currently transitioning.
        // This basically renders a black rectangle on top of the currently visible extras
        // (and beneath the carousel and help prompts).
        if (mExtrasFadeOpacity)
            renderFade(trans);

        // Always render the carousel on top so that it's not faded.
        renderCarousel(trans);
    }
    else {
        // Render the extras that are below the carousel.
        renderExtras(trans, INT16_MIN, mCarousel.zIndex);

        // Fade the screen if we're using fade transitions and we're currently transitioning.
        // This basically renders a black rectangle on top of the currently visible extras
        // (and beneath the carousel and help prompts).
        if (mExtrasFadeOpacity)
            renderFade(trans);

        // Render the carousel.
        renderCarousel(trans);

        // Render the rest of the extras.
        renderExtras(trans, mCarousel.zIndex, INT16_MAX);
    }
}

std::vector<HelpPrompt> SystemView::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL)
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
    style.applyTheme(mEntries.at(mCursor).object->getTheme(), "system");
    return style;
}

void SystemView::onThemeChanged(const std::shared_ptr<ThemeData>& /*theme*/)
{
    LOG(LogDebug) << "SystemView::onThemeChanged()";
    mViewNeedsReload = true;
    populate();
}

void SystemView::getViewElements(const std::shared_ptr<ThemeData>& theme)
{
    LOG(LogDebug) << "SystemView::getViewElements()";

    getDefaultElements();

    if (!theme->hasView("system"))
        return;

    const ThemeData::ThemeElement* carouselElem =
        theme->getElement("system", "systemcarousel", "carousel");

    if (carouselElem)
        getCarouselFromTheme(carouselElem);

    const ThemeData::ThemeElement* sysInfoElem = theme->getElement("system", "systemInfo", "text");

    if (sysInfoElem)
        mSystemInfo.applyTheme(theme, "system", "systemInfo", ThemeFlags::ALL);

    mViewNeedsReload = false;
}

void SystemView::renderCarousel(const glm::mat4& trans)
{
    // Background box behind logos.
    glm::mat4 carouselTrans {trans};
    carouselTrans =
        glm::translate(carouselTrans, glm::vec3 {mCarousel.pos.x, mCarousel.pos.y, 0.0f});
    carouselTrans = glm::translate(carouselTrans,
                                   glm::vec3 {mCarousel.origin.x * mCarousel.size.x * -1.0f,
                                              mCarousel.origin.y * mCarousel.size.y * -1.0f, 0.0f});

    glm::vec2 clipPos {carouselTrans[3].x, carouselTrans[3].y};
    Renderer::pushClipRect(glm::ivec2 {static_cast<int>(std::round(clipPos.x)),
                                       static_cast<int>(std::round(clipPos.y))},
                           glm::ivec2 {static_cast<int>(std::round(mCarousel.size.x)),
                                       static_cast<int>(std::round(mCarousel.size.y))});

    Renderer::setMatrix(carouselTrans);
    Renderer::drawRect(0.0f, 0.0f, mCarousel.size.x, mCarousel.size.y, mCarousel.color,
                       mCarousel.colorEnd, mCarousel.colorGradientHorizontal);

    // Draw logos.
    // Note: logoSpacing will also include the size of the logo itself.
    glm::vec2 logoSpacing {};
    float xOff = 0.0f;
    float yOff = 0.0f;

    switch (mCarousel.type) {
        case VERTICAL_WHEEL: {
            yOff = (mCarousel.size.y - mCarousel.logoSize.y) / 2.0f - (mCamOffset * logoSpacing.y);
            if (mCarousel.logoAlignment == ALIGN_LEFT)
                xOff = mCarousel.logoSize.x / 10.0f;
            else if (mCarousel.logoAlignment == ALIGN_RIGHT)
                xOff = mCarousel.size.x - (mCarousel.logoSize.x * 1.1f);
            else
                xOff = (mCarousel.size.x - mCarousel.logoSize.x) / 2.0f;
            break;
        }
        case VERTICAL: {
            logoSpacing.y = ((mCarousel.size.y - (mCarousel.logoSize.y * mCarousel.maxLogoCount)) /
                             (mCarousel.maxLogoCount)) +
                            mCarousel.logoSize.y;
            yOff = (mCarousel.size.y - mCarousel.logoSize.y) / 2.0f - (mCamOffset * logoSpacing.y);
            if (mCarousel.logoAlignment == ALIGN_LEFT)
                xOff = mCarousel.logoSize.x / 10.0f;
            else if (mCarousel.logoAlignment == ALIGN_RIGHT)
                xOff = mCarousel.size.x - (mCarousel.logoSize.x * 1.1f);
            else
                xOff = (mCarousel.size.x - mCarousel.logoSize.x) / 2.0f;
            break;
        }
        case HORIZONTAL_WHEEL: {
            xOff = std::round((mCarousel.size.x - mCarousel.logoSize.x) / 2.0f -
                              (mCamOffset * logoSpacing.y));
            if (mCarousel.logoAlignment == ALIGN_TOP)
                yOff = mCarousel.logoSize.y / 10.0f;
            else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
                yOff = mCarousel.size.y - (mCarousel.logoSize.y * 1.1f);
            else
                yOff = (mCarousel.size.y - mCarousel.logoSize.y) / 2.0f;
            break;
        }
        case HORIZONTAL: {
        }
        default: {
            logoSpacing.x = ((mCarousel.size.x - (mCarousel.logoSize.x * mCarousel.maxLogoCount)) /
                             (mCarousel.maxLogoCount)) +
                            mCarousel.logoSize.x;
            xOff = std::round((mCarousel.size.x - mCarousel.logoSize.x) / 2.0f -
                              (mCamOffset * logoSpacing.x));
            if (mCarousel.logoAlignment == ALIGN_TOP)
                yOff = mCarousel.logoSize.y / 10.0f;
            else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
                yOff = mCarousel.size.y - (mCarousel.logoSize.y * 1.1f);
            else
                yOff = (mCarousel.size.y - mCarousel.logoSize.y) / 2.0f;
            break;
        }
    }

    int center = static_cast<int>(mCamOffset);
    int logoCount = std::min(mCarousel.maxLogoCount, static_cast<int>(mEntries.size()));

    // Adding texture loading buffers depending on scrolling speed and status.
    int bufferIndex = getScrollingVelocity() + 1;
    int bufferLeft = logoBuffersLeft[bufferIndex];
    int bufferRight = logoBuffersRight[bufferIndex];
    if (logoCount == 1) {
        bufferLeft = 0;
        bufferRight = 0;
    }

    for (int i = center - logoCount / 2 + bufferLeft; // Line break.
         i <= center + logoCount / 2 + bufferRight; ++i) {
        int index = i;

        while (index < 0)
            index += static_cast<int>(mEntries.size());
        while (index >= static_cast<int>(mEntries.size()))
            index -= static_cast<int>(mEntries.size());

        glm::mat4 logoTrans {carouselTrans};
        logoTrans = glm::translate(
            logoTrans, glm::vec3 {i * logoSpacing.x + xOff, i * logoSpacing.y + yOff, 0.0f});

        float distance = i - mCamOffset;

        float scale = 1.0f + ((mCarousel.logoScale - 1.0f) * (1.0f - fabs(distance)));
        scale = std::min(mCarousel.logoScale, std::max(1.0f, scale));
        scale /= mCarousel.logoScale;

        int opacity =
            static_cast<int>(std::round(0x80 + ((0xFF - 0x80) * (1.0f - fabs(distance)))));
        opacity = std::max(static_cast<int>(0x80), opacity);

        const std::shared_ptr<GuiComponent>& comp = mEntries.at(index).data.logo;
        if (mCarousel.type == VERTICAL_WHEEL || mCarousel.type == HORIZONTAL_WHEEL) {
            comp->setRotationDegrees(mCarousel.logoRotation * distance);
            comp->setRotationOrigin(mCarousel.logoRotationOrigin);
        }

        // When running at lower resolutions, prevent the scale-down to go all the way to the
        // minimum value. This avoids potential single-pixel alignment issues when the logo
        // can't be vertically placed exactly in the middle of the carousel. Although the
        // problem theoretically exists at all resolutions, it's not visble at around 1080p
        // and above.
        if (std::min(mSize.x, mSize.y) < 1080.0f)
            scale = glm::clamp(scale, 1.0f / mCarousel.logoScale + 0.01f, 1.0f);

        comp->setScale(scale);
        comp->setOpacity(static_cast<unsigned char>(opacity));
        comp->render(logoTrans);

        if (mEntries.at(index).data.logoPlaceholderText) {
            const std::shared_ptr<GuiComponent>& comp = mEntries.at(index).data.logoPlaceholderText;
            if (mCarousel.type == VERTICAL_WHEEL || mCarousel.type == HORIZONTAL_WHEEL) {
                comp->setRotationDegrees(mCarousel.logoRotation * distance);
                comp->setRotationOrigin(mCarousel.logoRotationOrigin);
            }
            comp->setScale(scale);
            comp->setOpacity(static_cast<unsigned char>(opacity));
            comp->render(logoTrans);
        }
    }
    Renderer::popClipRect();
}

void SystemView::renderExtras(const glm::mat4& trans, float lower, float upper)
{
    int extrasCenter = static_cast<int>(mExtrasCamOffset);

    // Adding texture loading buffers depending on scrolling speed and status.
    int bufferIndex {getScrollingVelocity() + 1};

    Renderer::pushClipRect(glm::ivec2 {},
                           glm::ivec2 {static_cast<int>(mSize.x), static_cast<int>(mSize.y)});

    for (int i = extrasCenter + logoBuffersLeft[bufferIndex];
         i <= extrasCenter + logoBuffersRight[bufferIndex]; ++i) {
        int index = i;
        while (index < 0)
            index += static_cast<int>(mEntries.size());
        while (index >= static_cast<int>(mEntries.size()))
            index -= static_cast<int>(mEntries.size());

        // Only render selected system when not showing.
        if (mShowing || index == mCursor) {
            glm::mat4 extrasTrans {trans};
            if (mCarousel.type == HORIZONTAL || mCarousel.type == HORIZONTAL_WHEEL)
                extrasTrans = glm::translate(
                    extrasTrans, glm::vec3 {(i - mExtrasCamOffset) * mSize.x, 0.0f, 0.0f});
            else
                extrasTrans = glm::translate(
                    extrasTrans, glm::vec3 {0.0f, (i - mExtrasCamOffset) * mSize.y, 0.0f});

            Renderer::pushClipRect(
                glm::ivec2 {static_cast<int>(extrasTrans[3].x), static_cast<int>(extrasTrans[3].y)},
                glm::ivec2 {static_cast<int>(mSize.x), static_cast<int>(mSize.y)});
            SystemViewData data = mEntries.at(index).data;
            for (unsigned int j = 0; j < data.backgroundExtras.size(); ++j) {
                GuiComponent* extra = data.backgroundExtras[j];
                if (extra->getZIndex() >= lower && extra->getZIndex() < upper)
                    extra->render(extrasTrans);
            }
            mSystemInfo.render(extrasTrans);
            Renderer::popClipRect();
        }
    }
    Renderer::popClipRect();
}

void SystemView::renderFade(const glm::mat4& trans)
{
    unsigned int fadeColor = 0x00000000 | static_cast<unsigned char>(mExtrasFadeOpacity * 255.0f);
    Renderer::setMatrix(trans);
    Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, fadeColor, fadeColor);
}

void SystemView::getDefaultElements(void)
{
    // Carousel.
    mCarousel.type = HORIZONTAL;
    mCarousel.logoAlignment = ALIGN_CENTER;
    mCarousel.size.x = mSize.x;
    mCarousel.size.y = floorf(0.2325f * mSize.y);
    mCarousel.pos.x = 0.0f;
    mCarousel.pos.y = floorf(0.5f * (mSize.y - mCarousel.size.y));
    mCarousel.origin.x = 0.0f;
    mCarousel.origin.y = 0.0f;
    mCarousel.color = 0xFFFFFFD8;
    mCarousel.colorEnd = 0xFFFFFFD8;
    mCarousel.colorGradientHorizontal = true;
    mCarousel.logoScale = 1.2f;
    mCarousel.logoRotation = 7.5f;
    mCarousel.logoRotationOrigin.x = -5.0f;
    mCarousel.logoRotationOrigin.y = 0.5f;
    mCarousel.logoSize.x = 0.25f * mSize.x;
    mCarousel.logoSize.y = 0.155f * mSize.y;
    mCarousel.maxLogoCount = 3;
    mCarousel.zIndex = 40.0f;
    mCarousel.legacyZIndexMode = true;

    // System info bar.
    mSystemInfo.setSize(mSize.x, mSystemInfo.getFont()->getLetterHeight() * 2.2f);
    mSystemInfo.setPosition(0.0f, mCarousel.pos.y + mCarousel.size.y);
    mSystemInfo.setBackgroundColor(0xDDDDDDD8);
    mSystemInfo.setRenderBackground(true);
    mSystemInfo.setFont(Font::get(static_cast<int>(0.035f * mSize.y), Font::getDefaultPath()));
    mSystemInfo.setColor(0x000000FF);
    mSystemInfo.setZIndex(50.0f);
    mSystemInfo.setDefaultZIndex(50.0f);
}

void SystemView::getCarouselFromTheme(const ThemeData::ThemeElement* elem)
{
    if (elem->has("type")) {
        if (!(elem->get<std::string>("type").compare("vertical")))
            mCarousel.type = VERTICAL;
        else if (!(elem->get<std::string>("type").compare("vertical_wheel")))
            mCarousel.type = VERTICAL_WHEEL;
        else if (!(elem->get<std::string>("type").compare("horizontal_wheel")))
            mCarousel.type = HORIZONTAL_WHEEL;
        else
            mCarousel.type = HORIZONTAL;
    }
    if (elem->has("size"))
        mCarousel.size = elem->get<glm::vec2>("size") * mSize;
    if (elem->has("pos"))
        mCarousel.pos = elem->get<glm::vec2>("pos") * mSize;
    if (elem->has("origin"))
        mCarousel.origin = elem->get<glm::vec2>("origin");
    if (elem->has("color")) {
        mCarousel.color = elem->get<unsigned int>("color");
        mCarousel.colorEnd = mCarousel.color;
    }
    if (elem->has("colorEnd"))
        mCarousel.colorEnd = elem->get<unsigned int>("colorEnd");
    if (elem->has("gradientType"))
        mCarousel.colorGradientHorizontal =
            !(elem->get<std::string>("gradientType").compare("horizontal"));
    if (elem->has("logoScale"))
        mCarousel.logoScale = elem->get<float>("logoScale");
    if (elem->has("logoSize"))
        mCarousel.logoSize = elem->get<glm::vec2>("logoSize") * mSize;
    if (elem->has("maxLogoCount"))
        mCarousel.maxLogoCount = static_cast<int>(std::round(elem->get<float>("maxLogoCount")));
    if (elem->has("zIndex"))
        mCarousel.zIndex = elem->get<float>("zIndex");
    if (elem->has("logoRotation"))
        mCarousel.logoRotation = elem->get<float>("logoRotation");
    if (elem->has("logoRotationOrigin"))
        mCarousel.logoRotationOrigin = elem->get<glm::vec2>("logoRotationOrigin");
    if (elem->has("logoAlignment")) {
        if (!(elem->get<std::string>("logoAlignment").compare("left")))
            mCarousel.logoAlignment = ALIGN_LEFT;
        else if (!(elem->get<std::string>("logoAlignment").compare("right")))
            mCarousel.logoAlignment = ALIGN_RIGHT;
        else if (!(elem->get<std::string>("logoAlignment").compare("top")))
            mCarousel.logoAlignment = ALIGN_TOP;
        else if (!(elem->get<std::string>("logoAlignment").compare("bottom")))
            mCarousel.logoAlignment = ALIGN_BOTTOM;
        else
            mCarousel.logoAlignment = ALIGN_CENTER;
    }
    if (elem->has("legacyZIndexMode")) {
        mCarousel.legacyZIndexMode =
            elem->get<std::string>("legacyZIndexMode").compare("true") == 0 ? true : false;
    }
    else {
        mCarousel.legacyZIndexMode = true;
    }
}
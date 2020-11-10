//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SystemView.cpp
//
//  Main system view.
//

#include "views/SystemView.h"

#include "animations/LambdaAnimation.h"
#include "guis/GuiMsgBox.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "Log.h"
#include "Settings.h"
#include "Sound.h"
#include "SystemData.h"
#include "Window.h"

#if defined(_WIN64)
#include <cmath>
#endif

// Buffer values for scrolling velocity (left, stopped, right).
const int logoBuffersLeft[] = { -5, -2, -1 };
const int logoBuffersRight[] = { 1, 2, 5 };

SystemView::SystemView(
        Window* window)
        : IList<SystemViewData, SystemData*>
        (window, LIST_SCROLL_STYLE_SLOW, LIST_ALWAYS_LOOP),
        mViewNeedsReload(true),
        mSystemInfo(window, "SYSTEM INFO", Font::get(FONT_SIZE_SMALL), 0x33333300, ALIGN_CENTER)
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

    for (auto it = SystemData::sSystemVector.cbegin();
            it != SystemData::sSystemVector.cend(); it++) {
        const std::shared_ptr<ThemeData>& theme = (*it)->getTheme();

        if (mViewNeedsReload)
            getViewElements(theme);

        if ((*it)->isVisible()) {
            Entry e;
            e.name = (*it)->getName();
            e.object = *it;

            // Make logo.
            const ThemeData::ThemeElement* logoElem = theme->getElement("system", "logo", "image");
            if (logoElem) {
                std::string path = logoElem->get<std::string>("path");
                std::string defaultPath = logoElem->has("default") ?
                        logoElem->get<std::string>("default") : "";
                if ((!path.empty() && ResourceManager::getInstance()->fileExists(path)) ||
                        (!defaultPath.empty() &&
                        ResourceManager::getInstance()->fileExists(defaultPath))) {
                    ImageComponent* logo = new ImageComponent(mWindow, false, false);
                    logo->setMaxSize(mCarousel.logoSize * mCarousel.logoScale);
                    logo->applyTheme(theme, "system", "logo", ThemeFlags::PATH | ThemeFlags::COLOR);
                    logo->setRotateByTargetSize(true);
                    e.data.logo = std::shared_ptr<GuiComponent>(logo);
                }
            }
            if (!e.data.logo) {
                // No logo in theme; use text.
                TextComponent* text = new TextComponent(
                    mWindow,
                    (*it)->getName(),
                    Font::get(FONT_SIZE_LARGE),
                    0x000000FF,
                    ALIGN_CENTER);
                text->setSize(mCarousel.logoSize * mCarousel.logoScale);
                text->applyTheme((*it)->getTheme(), "system", "logoText",
                        ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE | ThemeFlags::COLOR |
                        ThemeFlags::FORCE_UPPERCASE | ThemeFlags::LINE_SPACING | ThemeFlags::TEXT);
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

            Vector2f denormalized = mCarousel.logoSize * e.data.logo->getOrigin();
            e.data.logo->setPosition(denormalized.x(), denormalized.y(), 0.0);

            // Make background extras.
            e.data.backgroundExtras = ThemeData::makeExtras((*it)->getTheme(), "system", mWindow);

            // Sort the extras by z-index.
            std::stable_sort(e.data.backgroundExtras.begin(), e.data.backgroundExtras.end(),
                    [](GuiComponent* a, GuiComponent* b) {
                return b->getZIndex() > a->getZIndex();
            });

            this->add(e);
        }
    }
    if (mEntries.size() == 0) {
        // Something is wrong, there is not a single system to show, check if UI mode is not full.
        if (!UIModeController::getInstance()->isUIModeFull()) {
            Settings::getInstance()->setString("UIMode", "full");
            mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
                    "The selected UI mode has nothing to show,\n returning to UI mode \"Full\"",
                    "OK", nullptr));
        }
    }
}

void SystemView::goToSystem(SystemData* system, bool animate)
{
    setCursor(system);

    if (!animate)
        finishAnimation(0);
}

bool SystemView::input(InputConfig* config, Input input)
{
    auto it = SystemData::sSystemVector.cbegin();
    const std::shared_ptr<ThemeData>& theme = (*it)->getTheme();

    if (input.value != 0) {
        if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_r &&
                SDL_GetModState() & KMOD_LCTRL && Settings::getInstance()->getBool("Debug")) {
            LOG(LogInfo) << "Reloading all";
            ViewController::get()->reloadAll();
            return true;
        }

        switch (mCarousel.type) {
        case VERTICAL:
        case VERTICAL_WHEEL:
            if (config->isMappedLike("up", input)) {
                ViewController::get()->resetMovingCamera();
                listInput(-1);
                return true;
            }
            if (config->isMappedLike("down", input)) {
                ViewController::get()->resetMovingCamera();
                listInput(1);
                return true;
            }
            break;
        case HORIZONTAL:
        case HORIZONTAL_WHEEL:
        default:
            if (config->isMappedLike("left", input)) {
                ViewController::get()->resetMovingCamera();
                listInput(-1);
                return true;
            }
            if (config->isMappedLike("right", input)) {
                ViewController::get()->resetMovingCamera();
                listInput(1);
                return true;
            }
            break;
        }

        if (config->isMappedTo("a", input)) {
            stopScrolling();
            ViewController::get()->goToGameList(getSelected());
            NavigationSounds::getInstance()->playThemeNavigationSound(SELECTSOUND);
            return true;
        }
        if (config->isMappedTo("x", input)) {
            // Get random system.
            // Go to system.
            NavigationSounds::getInstance()->playThemeNavigationSound(SYSTEMBROWSESOUND);
            setCursor(SystemData::getRandomSystem(getSelected()));
            return true;
        }

        if (!UIModeController::getInstance()->isUIModeKid() &&
                config->isMappedTo("select", input) &&
                Settings::getInstance()->getBool("ScreensaverControls")) {
            if (!mWindow->isScreenSaverActive()) {
                mWindow->startScreenSaver();
                mWindow->renderScreenSaver();
            }
            return true;
        }
    }
    else {
        if (config->isMappedLike("left", input) ||
            config->isMappedLike("right", input) ||
            config->isMappedLike("up", input) ||
            config->isMappedLike("down", input))
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

    // What's the shortest way to get to our target?
    // It's one of these...

    float endPos = target; // Directly.
    float dist = fabs(endPos - startPos);

    if (fabs(target + posMax - startPos - mScrollVelocity) < dist)
        endPos = target + posMax; // Loop around the end (0 -> max).
    if (fabs(target - posMax - startPos - mScrollVelocity) < dist)
        endPos = target - posMax; // Loop around the start (max - 1 -> -1).

    // Animate mSystemInfo's opacity (fade out, wait, fade back in).

    cancelAnimation(1);
    cancelAnimation(2);

    std::string transition_style = Settings::getInstance()->getString("TransitionStyle");
    bool goFast = transition_style == "instant";
    const float infoStartOpacity = mSystemInfo.getOpacity() / 255.f;

    Animation* infoFadeOut = new LambdaAnimation(
            [infoStartOpacity, this] (float t) {
        mSystemInfo.setOpacity(static_cast<unsigned char>(
                Math::lerp(infoStartOpacity, 0.f, t) * 255));
    }, static_cast<int>(infoStartOpacity * (goFast ? 10 : 150)));

    std::pair<unsigned int, unsigned int> gameCount = getSelected()->getDisplayedGameCount();

    // Also change the text after we've fully faded out.
    setAnimation(infoFadeOut, 0, [this, gameCount] {
        std::stringstream ss;

        if (!getSelected()->isGameSystem())
            ss << "CONFIGURATION";
        else if (getSelected()->isCollection() && (getSelected()->getName() == "favorites"))
            ss << gameCount.first << " GAME" << (gameCount.first == 1 ? " " : "S");
        // The 'recent' gamelist has probably been trimmed after sorting, so we'll cap it at
        // its maximum limit of 50 games.
        else if (getSelected()->isCollection() && (getSelected()->getName() == "recent"))
            ss << (gameCount.first > 50 ? 50 : gameCount.first) << " GAME" <<
                    (gameCount.first == 1 ? " " : "S");
        else
            ss << gameCount.first << " GAME" << (gameCount.first == 1 ? " " : "S ") << "(" <<
                    gameCount.second << " FAVORITE" << (gameCount.second == 1 ? ")" : "S)");

        mSystemInfo.setText(ss.str());
    }, false, 1);

    Animation* infoFadeIn = new LambdaAnimation(
            [this](float t) {
        mSystemInfo.setOpacity(static_cast<unsigned char>(Math::lerp(0.f, 1.f, t) * 255));
    }, goFast ? 10 : 300);

    // Wait 150ms to fade in.
    setAnimation(infoFadeIn, goFast ? 0 : 500, nullptr, false, 2);

    // No need to animate transition, we're not going anywhere (probably mEntries.size() == 1).
    if (endPos == mCamOffset && endPos == mExtrasCamOffset)
        return;

    Animation* anim;
    bool carousel_transitions = Settings::getInstance()->getBool("CarouselTransitions");
    if (transition_style == "fade") {
        float startExtrasFade = mExtrasFadeOpacity;
        anim = new LambdaAnimation(
                [this, startExtrasFade, startPos, endPos, posMax, carousel_transitions](float t) {
            t -= 1;
            float f = Math::lerp(startPos, endPos, t*t*t + 1);
            if (f < 0)
                f += posMax;
            if (f >= posMax)
                f -= posMax;

            this->mCamOffset = carousel_transitions ? f : endPos;

            t += 1;
            if (t < 0.3f)
                this->mExtrasFadeOpacity = Math::lerp(0.0f, 1.0f, t / 0.2f + startExtrasFade);
            else if (t < 0.7f)
                this->mExtrasFadeOpacity = 1.0f;
            else
                this->mExtrasFadeOpacity = Math::lerp(1.0f, 0.0f, (t - 0.6f) / 0.3f);

            if (t > 0.5f)
                this->mExtrasCamOffset = endPos;

        }, 500);
    }
    else if (transition_style == "slide") {
        // Slide.
        anim = new LambdaAnimation(
                [this, startPos, endPos, posMax, carousel_transitions](float t) {
            t -= 1;
            float f = Math::lerp(startPos, endPos, t*t*t + 1);
            if (f < 0)
                f += posMax;
            if (f >= posMax)
                f -= posMax;

            this->mCamOffset = carousel_transitions ? f : endPos;
            this->mExtrasCamOffset = f;
        }, 500);
    }
    else {
        // Instant.
        anim = new LambdaAnimation(
                [this, startPos, endPos, posMax, carousel_transitions ](float t) {
            t -= 1;
            float f = Math::lerp(startPos, endPos, t*t*t + 1);
            if (f < 0)
                f += posMax;
            if (f >= posMax)
                f -= posMax;

            this->mCamOffset = carousel_transitions ? f : endPos;
            this->mExtrasCamOffset = endPos;
        }, carousel_transitions ? 500 : 1);
    }

    setAnimation(anim, 0, nullptr, false, 0);
}

void SystemView::render(const Transform4x4f& parentTrans)
{
    if (size() == 0)
        return;  // Nothing to render.

    Transform4x4f trans = getTransform() * parentTrans;

    auto systemInfoZIndex = mSystemInfo.getZIndex();
    auto minMax = std::minmax(mCarousel.zIndex, systemInfoZIndex);

    renderExtras(trans, INT16_MIN, minMax.first);
    renderFade(trans);

    if (mCarousel.zIndex > mSystemInfo.getZIndex()) {
        renderInfoBar(trans);
    }
    else {
        renderCarousel(trans);
    }

    renderExtras(trans, minMax.first, minMax.second);

    if (mCarousel.zIndex > mSystemInfo.getZIndex()) {
        renderCarousel(trans);
    }
    else {
        renderInfoBar(trans);
    }

    renderExtras(trans, minMax.second, INT16_MAX);
}

std::vector<HelpPrompt> SystemView::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL)
        prompts.push_back(HelpPrompt("up/down", "choose"));
    else
        prompts.push_back(HelpPrompt("left/right", "choose"));
    prompts.push_back(HelpPrompt("a", "select"));
    prompts.push_back(HelpPrompt("x", "random"));

    if (!UIModeController::getInstance()->isUIModeKid() &&
            Settings::getInstance()->getBool("ScreensaverControls"))
        prompts.push_back(HelpPrompt("select", "toggle screensaver"));

    return prompts;
}

HelpStyle SystemView::getHelpStyle()
{
    HelpStyle style;
    style.applyTheme(mEntries.at(mCursor).object->getTheme(), "system");
    return style;
}

void  SystemView::onThemeChanged(const std::shared_ptr<ThemeData>& /*theme*/)
{
    LOG(LogDebug) << "SystemView::onThemeChanged()";
    mViewNeedsReload = true;
    populate();
}

//  Get the ThemeElements that make up the SystemView.
void  SystemView::getViewElements(const std::shared_ptr<ThemeData>& theme)
{
    LOG(LogDebug) << "SystemView::getViewElements()";

    getDefaultElements();

    if (!theme->hasView("system"))
        return;

    const ThemeData::ThemeElement* carouselElem = theme->
            getElement("system", "systemcarousel", "carousel");
    if (carouselElem)
        getCarouselFromTheme(carouselElem);

    const ThemeData::ThemeElement* sysInfoElem = theme->
            getElement("system", "systemInfo", "text");
    if (sysInfoElem)
        mSystemInfo.applyTheme(theme, "system", "systemInfo", ThemeFlags::ALL);

    mViewNeedsReload = false;
}

//  Render system carousel.
void SystemView::renderCarousel(const Transform4x4f& trans)
{
    // Background box behind logos.
    Transform4x4f carouselTrans = trans;
    carouselTrans.translate(Vector3f(mCarousel.pos.x(), mCarousel.pos.y(), 0.0));
    carouselTrans.translate(Vector3f(mCarousel.origin.x() * mCarousel.size.x() * -1,
            mCarousel.origin.y() * mCarousel.size.y() * -1, 0.0f));

    Vector2f clipPos(carouselTrans.translation().x(), carouselTrans.translation().y());
    Renderer::pushClipRect(Vector2i(static_cast<int>(clipPos.x()), static_cast<int>(clipPos.y())),
            Vector2i(static_cast<int>(mCarousel.size.x()), static_cast<int>(mCarousel.size.y())));

    Renderer::setMatrix(carouselTrans);
    Renderer::drawRect(0.0f, 0.0f, mCarousel.size.x(), mCarousel.size.y(),
            mCarousel.color, mCarousel.colorEnd, mCarousel.colorGradientHorizontal);

    // Draw logos.
    // NB: logoSpacing will also include the size of the logo itself!
    Vector2f logoSpacing(0.0, 0.0);
    float xOff = 0.0;
    float yOff = 0.0;

    switch (mCarousel.type) {
        case VERTICAL_WHEEL:
            yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2.f -
                    (mCamOffset * logoSpacing[1]);
            if (mCarousel.logoAlignment == ALIGN_LEFT)
                xOff = mCarousel.logoSize.x() / 10.f;
            else if (mCarousel.logoAlignment == ALIGN_RIGHT)
                xOff = mCarousel.size.x() - (mCarousel.logoSize.x() * 1.1f);
            else
                xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2.f;
            break;
        case VERTICAL:
            logoSpacing[1] = ((mCarousel.size.y() - (mCarousel.logoSize.y() *
                    mCarousel.maxLogoCount)) / (mCarousel.maxLogoCount)) + mCarousel.logoSize.y();
            yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2.f -
                    (mCamOffset * logoSpacing[1]);

            if (mCarousel.logoAlignment == ALIGN_LEFT)
                xOff = mCarousel.logoSize.x() / 10.f;
            else if (mCarousel.logoAlignment == ALIGN_RIGHT)
                xOff = mCarousel.size.x() - (mCarousel.logoSize.x() * 1.1f);
            else
                xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2;
            break;
        case HORIZONTAL_WHEEL:
            xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2 -
                    (mCamOffset * logoSpacing[1]);
            if (mCarousel.logoAlignment == ALIGN_TOP)
                yOff = mCarousel.logoSize.y() / 10;
            else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
                yOff = mCarousel.size.y() - (mCarousel.logoSize.y() * 1.1f);
            else
                yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2;
            break;
        case HORIZONTAL:
        default:
            logoSpacing[0] = ((mCarousel.size.x() - (mCarousel.logoSize.x() *
                    mCarousel.maxLogoCount)) / (mCarousel.maxLogoCount)) + mCarousel.logoSize.x();
            xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2.f -
                    (mCamOffset * logoSpacing[0]);

            if (mCarousel.logoAlignment == ALIGN_TOP)
                yOff = mCarousel.logoSize.y() / 10.f;
            else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
                yOff = mCarousel.size.y() - (mCarousel.logoSize.y() * 1.1f);
            else
                yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2.f;
            break;
    }

    int center = static_cast<int>(mCamOffset);
    int logoCount = Math::min(mCarousel.maxLogoCount, static_cast<int>(mEntries.size()));

    // Adding texture loading buffers depending on scrolling speed and status.
    int bufferIndex = getScrollingVelocity() + 1;
    int bufferLeft = logoBuffersLeft[bufferIndex];
    int bufferRight = logoBuffersRight[bufferIndex];
    if (logoCount == 1) {
        bufferLeft = 0;
        bufferRight = 0;
    }

    for (int i = center - logoCount / 2 + bufferLeft;
            i <= center + logoCount / 2 + bufferRight; i++) {
        int index = i;
        while (index < 0)
            index += static_cast<int>(mEntries.size());
        while (index >= static_cast<int>(mEntries.size()))
            index -= static_cast<int>(mEntries.size());

        Transform4x4f logoTrans = carouselTrans;
        logoTrans.translate(Vector3f(i * logoSpacing[0] + xOff, i * logoSpacing[1] + yOff, 0));

        float distance = i - mCamOffset;

        float scale = 1.0f + ((mCarousel.logoScale - 1.0f) * (1.0f - fabs(distance)));
        scale = Math::min(mCarousel.logoScale, Math::max(1.0f, scale));
        scale /= mCarousel.logoScale;

        int opacity = static_cast<int>(Math::round(0x80 + ((0xFF - 0x80) *
                (1.0f - fabs(distance)))));
        opacity = Math::max(static_cast<int>(0x80), opacity);

        const std::shared_ptr<GuiComponent> &comp = mEntries.at(index).data.logo;
        if (mCarousel.type == VERTICAL_WHEEL || mCarousel.type == HORIZONTAL_WHEEL) {
            comp->setRotationDegrees(mCarousel.logoRotation * distance);
            comp->setRotationOrigin(mCarousel.logoRotationOrigin);
        }
        comp->setScale(scale);
        comp->setOpacity(static_cast<unsigned char>(opacity));
        comp->render(logoTrans);
    }
    Renderer::popClipRect();
}

void SystemView::renderInfoBar(const Transform4x4f& trans)
{
    Renderer::setMatrix(trans);
    mSystemInfo.render(trans);
}

// Draw background extras.
void SystemView::renderExtras(const Transform4x4f& trans, float lower, float upper)
{
    int extrasCenter = static_cast<int>(mExtrasCamOffset);

    // Adding texture loading buffers depending on scrolling speed and status.
    int bufferIndex = getScrollingVelocity() + 1;

    Renderer::pushClipRect(Vector2i::Zero(), Vector2i(static_cast<int>(mSize.x()),
            static_cast<int>(mSize.y())));

    for (int i = extrasCenter + logoBuffersLeft[bufferIndex]; i <= extrasCenter +
            logoBuffersRight[bufferIndex]; i++) {
        int index = i;
        while (index < 0)
            index += static_cast<int>(mEntries.size());
        while (index >= static_cast<int>(mEntries.size()))
            index -= static_cast<int>(mEntries.size());

        // Only render selected system when not showing.
        if (mShowing || index == mCursor)
        {
            Transform4x4f extrasTrans = trans;
            if (mCarousel.type == HORIZONTAL || mCarousel.type == HORIZONTAL_WHEEL)
                extrasTrans.translate(Vector3f((i - mExtrasCamOffset) * mSize.x(), 0, 0));
            else
                extrasTrans.translate(Vector3f(0, (i - mExtrasCamOffset) * mSize.y(), 0));

            Renderer::pushClipRect(Vector2i(static_cast<int>(extrasTrans.translation()[0]),
                    static_cast<int>(extrasTrans.translation()[1])),
                    Vector2i(static_cast<int>(mSize.x()), static_cast<int>(mSize.y())));
            SystemViewData data = mEntries.at(index).data;
            for (unsigned int j = 0; j < data.backgroundExtras.size(); j++) {
                GuiComponent *extra = data.backgroundExtras[j];
                if (extra->getZIndex() >= lower && extra->getZIndex() < upper) {
                    extra->render(extrasTrans);
                }
            }
            Renderer::popClipRect();
        }
    }
    Renderer::popClipRect();
}

void SystemView::renderFade(const Transform4x4f& trans)
{
    // Fade extras if necessary.
    if (mExtrasFadeOpacity) {
        unsigned int fadeColor = 0x00000000 | static_cast<unsigned char>(mExtrasFadeOpacity * 255);
        Renderer::setMatrix(trans);
        Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), fadeColor, fadeColor);
    }
}

// Populate the system carousel with the legacy values.
void  SystemView::getDefaultElements(void)
{
    // Carousel.
    mCarousel.type = HORIZONTAL;
    mCarousel.logoAlignment = ALIGN_CENTER;
    mCarousel.size.x() = mSize.x();
    mCarousel.size.y() = 0.2325f * mSize.y();
    mCarousel.pos.x() = 0.0f;
    mCarousel.pos.y() = 0.5f * (mSize.y() - mCarousel.size.y());
    mCarousel.origin.x() = 0.0f;
    mCarousel.origin.y() = 0.0f;
    mCarousel.color = 0xFFFFFFD8;
    mCarousel.colorEnd = 0xFFFFFFD8;
    mCarousel.colorGradientHorizontal = true;
    mCarousel.logoScale = 1.2f;
    mCarousel.logoRotation = 7.5;
    mCarousel.logoRotationOrigin.x() = -5;
    mCarousel.logoRotationOrigin.y() = 0.5;
    mCarousel.logoSize.x() = 0.25f * mSize.x();
    mCarousel.logoSize.y() = 0.155f * mSize.y();
    mCarousel.maxLogoCount = 3;
    mCarousel.zIndex = 40;

    // System Info Bar.
    mSystemInfo.setSize(mSize.x(), mSystemInfo.getFont()->getLetterHeight()*2.2f);
    mSystemInfo.setPosition(0, (mCarousel.pos.y() + mCarousel.size.y() - 0.2f));
    mSystemInfo.setBackgroundColor(0xDDDDDDD8);
    mSystemInfo.setRenderBackground(true);
    mSystemInfo.setFont(Font::get(static_cast<int>(0.035f * mSize.y()), Font::getDefaultPath()));
    mSystemInfo.setColor(0x000000FF);
    mSystemInfo.setZIndex(50);
    mSystemInfo.setDefaultZIndex(50);
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
        mCarousel.size = elem->get<Vector2f>("size") * mSize;
    if (elem->has("pos"))
        mCarousel.pos = elem->get<Vector2f>("pos") * mSize;
    if (elem->has("origin"))
        mCarousel.origin = elem->get<Vector2f>("origin");
    if (elem->has("color")) {
        mCarousel.color = elem->get<unsigned int>("color");
        mCarousel.colorEnd = mCarousel.color;
    }
    if (elem->has("colorEnd"))
        mCarousel.colorEnd = elem->get<unsigned int>("colorEnd");
    if (elem->has("gradientType"))
        mCarousel.colorGradientHorizontal = !(elem->get<std::string>("gradientType").compare("horizontal"));
    if (elem->has("logoScale"))
        mCarousel.logoScale = elem->get<float>("logoScale");
    if (elem->has("logoSize"))
        mCarousel.logoSize = elem->get<Vector2f>("logoSize") * mSize;
    if (elem->has("maxLogoCount"))
        mCarousel.maxLogoCount = static_cast<int>(Math::round(elem->get<float>("maxLogoCount")));
    if (elem->has("zIndex"))
        mCarousel.zIndex = elem->get<float>("zIndex");
    if (elem->has("logoRotation"))
        mCarousel.logoRotation = elem->get<float>("logoRotation");
    if (elem->has("logoRotationOrigin"))
        mCarousel.logoRotationOrigin = elem->get<Vector2f>("logoRotationOrigin");
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
}

void SystemView::onShow()
{
    mShowing = true;
}

void SystemView::onHide()
{
    mShowing = false;
}

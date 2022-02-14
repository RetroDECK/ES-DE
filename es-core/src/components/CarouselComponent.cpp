//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CarouselComponent.cpp
//
//  Carousel.
//

#include "components/CarouselComponent.h"

#include "Log.h"
#include "animations/LambdaAnimation.h"

namespace
{
    // Buffer values for scrolling velocity (left, stopped, right).
    const int logoBuffersLeft[] {-5, -2, -1};
    const int logoBuffersRight[] {1, 2, 5};

} // namespace

CarouselComponent::CarouselComponent()
    : IList<CarouselElement, SystemData*> {LIST_SCROLL_STYLE_SLOW, LIST_ALWAYS_LOOP}
    , mCamOffset {0.0f}
    , mPreviousScrollVelocity {0}
    , mType {HORIZONTAL}
    , mFont {Font::get(FONT_SIZE_LARGE)}
    , mTextColor {0x000000FF}
    , mTextBackgroundColor {0xFFFFFF00}
    , mLineSpacing {1.5f}
    , mLogoHorizontalAlignment {ALIGN_CENTER}
    , mLogoVerticalAlignment {ALIGN_CENTER}
    , mMaxLogoCount {3}
    , mLogoSize {Renderer::getScreenWidth() * 0.25f, Renderer::getScreenHeight() * 0.155f}
    , mLogoScale {1.2f}
    , mLogoRotation {7.5f}
    , mLogoRotationOrigin {-3.0f, 0.5f}
    , mCarouselColor {0}
    , mCarouselColorEnd {0}
    , mColorGradientHorizontal {true}
{
}

void CarouselComponent::addEntry(const std::shared_ptr<ThemeData>& theme,
                                 Entry& entry,
                                 bool legacyMode)
{
    // Make logo.
    if (legacyMode) {
        const ThemeData::ThemeElement* logoElem {
            theme->getElement("system", "image_logo", "image")};

        if (logoElem) {
            std::string path;
            if (logoElem->has("path"))
                path = logoElem->get<std::string>("path");
            std::string defaultPath {
                logoElem->has("default") ? logoElem->get<std::string>("default") : ""};
            if ((!path.empty() && ResourceManager::getInstance().fileExists(path)) ||
                (!defaultPath.empty() && ResourceManager::getInstance().fileExists(defaultPath))) {
                auto logo = std::make_shared<ImageComponent>(false, false);
                logo->setMaxSize(glm::round(mLogoSize * mLogoScale));
                logo->applyTheme(theme, "system", "image_logo",
                                 ThemeFlags::PATH | ThemeFlags::COLOR);
                logo->setRotateByTargetSize(true);
                entry.data.logo = logo;
            }
        }
    }
    else {
        if (entry.data.logoPath != "" &&
            ResourceManager::getInstance().fileExists(entry.data.logoPath)) {
            auto logo = std::make_shared<ImageComponent>(false, false);
            logo->setImage(entry.data.logoPath);
            logo->setMaxSize(glm::round(mLogoSize * mLogoScale));
            logo->applyTheme(theme, "system", "", ThemeFlags::ALL);
            logo->setRotateByTargetSize(true);
            entry.data.logo = logo;
        }
        else if (entry.data.defaultLogoPath != "" &&
                 ResourceManager::getInstance().fileExists(entry.data.defaultLogoPath)) {
            auto defaultLogo = std::make_shared<ImageComponent>(false, false);
            defaultLogo->setImage(entry.data.defaultLogoPath);
            defaultLogo->setMaxSize(glm::round(mLogoSize * mLogoScale));
            defaultLogo->applyTheme(theme, "system", "", ThemeFlags::ALL);
            defaultLogo->setRotateByTargetSize(true);
            entry.data.logo = defaultLogo;
        }
    }

    if (!entry.data.logo) {
        // If no logo image is present, add logo text as fallback.
        auto text = std::make_shared<TextComponent>(entry.name, mFont, 0x000000FF, ALIGN_CENTER);
        text->setSize(mLogoSize * mLogoScale);
        if (legacyMode) {
            text->applyTheme(theme, "system", "text_logoText",
                             ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE | ThemeFlags::COLOR |
                                 ThemeFlags::LETTER_CASE | ThemeFlags::FORCE_UPPERCASE |
                                 ThemeFlags::LINE_SPACING | ThemeFlags::TEXT);
        }
        if (!legacyMode) {
            text->setLineSpacing(mLineSpacing);
            if (mText != "")
                text->setValue(mText);
            text->setColor(mTextColor);
            text->setBackgroundColor(mTextBackgroundColor);
            text->setRenderBackground(true);
        }
        entry.data.logo = text;

        text->setHorizontalAlignment(mLogoHorizontalAlignment);
        text->setVerticalAlignment(mLogoVerticalAlignment);
    }

    // Set origin for the logos based on their alignment so they line up properly.
    if (mLogoHorizontalAlignment == ALIGN_LEFT)
        entry.data.logo->setOrigin(0, 0.5);
    else if (mLogoHorizontalAlignment == ALIGN_RIGHT)
        entry.data.logo->setOrigin(1.0, 0.5);
    else
        entry.data.logo->setOrigin(0.5, 0.5);

    if (mLogoVerticalAlignment == ALIGN_TOP)
        entry.data.logo->setOrigin(entry.data.logo->getOrigin().x, 0);
    else if (mLogoVerticalAlignment == ALIGN_BOTTOM)
        entry.data.logo->setOrigin(entry.data.logo->getOrigin().x, 1);
    else
        entry.data.logo->setOrigin(entry.data.logo->getOrigin().x, 0.5);

    glm::vec2 denormalized {mLogoSize * entry.data.logo->getOrigin()};
    entry.data.logo->setPosition(glm::vec3 {denormalized.x, denormalized.y, 0.0f});

    add(entry);
}

bool CarouselComponent::input(InputConfig* config, Input input)
{
    if (input.value != 0) {
        switch (mType) {
            case VERTICAL:
            case VERTICAL_WHEEL:
                if (config->isMappedLike("up", input)) {
                    if (mCancelTransitionsCallback)
                        mCancelTransitionsCallback();
                    listInput(-1);
                    return true;
                }
                if (config->isMappedLike("down", input)) {
                    if (mCancelTransitionsCallback)
                        mCancelTransitionsCallback();
                    listInput(1);
                    return true;
                }
                break;
            case HORIZONTAL:
            case HORIZONTAL_WHEEL:
            default:
                if (config->isMappedLike("left", input)) {
                    if (mCancelTransitionsCallback)
                        mCancelTransitionsCallback();
                    listInput(-1);
                    return true;
                }
                if (config->isMappedLike("right", input)) {
                    if (mCancelTransitionsCallback)
                        mCancelTransitionsCallback();
                    listInput(1);
                    return true;
                }
                break;
        }
    }
    else {
        if (config->isMappedLike("left", input) || config->isMappedLike("right", input) ||
            config->isMappedLike("up", input) || config->isMappedLike("down", input)) {
            listInput(0);
        }
    }

    return GuiComponent::input(config, input);
}

void CarouselComponent::update(int deltaTime)
{
    listUpdate(deltaTime);
    GuiComponent::update(deltaTime);
}

void CarouselComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 carouselTrans {parentTrans};
    carouselTrans = glm::translate(carouselTrans, glm::vec3 {mPosition.x, mPosition.y, 0.0f});
    carouselTrans = glm::translate(
        carouselTrans, glm::vec3 {mOrigin.x * mSize.x * -1.0f, mOrigin.y * mSize.y * -1.0f, 0.0f});

    glm::vec2 clipPos {carouselTrans[3].x, carouselTrans[3].y};
    Renderer::setMatrix(carouselTrans);

    // Background box behind logos.
    Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, mCarouselColor, mCarouselColorEnd,
                       mColorGradientHorizontal);

    // Draw logos.
    // logoSpacing will also include the size of the logo itself.
    glm::vec2 logoSpacing {};
    float xOff {0.0f};
    float yOff {0.0f};

    switch (mType) {
        case HORIZONTAL_WHEEL:
        case VERTICAL_WHEEL:
            xOff = std::round((mSize.x - mLogoSize.x) / 2.0f - (mCamOffset * logoSpacing.y));
            yOff = (mSize.y - mLogoSize.y) / 2.0f;
            break;
        case VERTICAL:
            logoSpacing.y =
                ((mSize.y - (mLogoSize.y * mMaxLogoCount)) / (mMaxLogoCount)) + mLogoSize.y;
            yOff = (mSize.y - mLogoSize.y) / 2.0f - (mCamOffset * logoSpacing.y);
            if (mLogoHorizontalAlignment == ALIGN_LEFT)
                xOff = mLogoSize.x / 10.0f;
            else if (mLogoHorizontalAlignment == ALIGN_RIGHT)
                xOff = mSize.x - (mLogoSize.x * 1.1f);
            else
                xOff = (mSize.x - mLogoSize.x) / 2.0f;
            break;
        case HORIZONTAL:
        default:
            logoSpacing.x =
                ((mSize.x - (mLogoSize.x * mMaxLogoCount)) / (mMaxLogoCount)) + mLogoSize.x;
            xOff = std::round((mSize.x - mLogoSize.x) / 2.0f - (mCamOffset * logoSpacing.x));
            if (mLogoVerticalAlignment == ALIGN_TOP)
                yOff = mLogoSize.y / 10.0f;
            else if (mLogoVerticalAlignment == ALIGN_BOTTOM)
                yOff = mSize.y - (mLogoSize.y * 1.1f);
            else
                yOff = (mSize.y - mLogoSize.y) / 2.0f;
            break;
    }

    int center {static_cast<int>(mCamOffset)};
    int logoCount {std::min(mMaxLogoCount, static_cast<int>(mEntries.size()))};

    // Adding texture loading buffers depending on scrolling speed and status.
    int bufferIndex {getScrollingVelocity() + 1};
    int bufferLeft {logoBuffersLeft[bufferIndex]};
    int bufferRight {logoBuffersRight[bufferIndex]};
    if (logoCount == 1) {
        bufferLeft = 0;
        bufferRight = 0;
    }

    for (int i = center - logoCount / 2 + bufferLeft; // Line break.
         i <= center + logoCount / 2 + bufferRight; ++i) {
        int index {i};

        while (index < 0)
            index += static_cast<int>(mEntries.size());
        while (index >= static_cast<int>(mEntries.size()))
            index -= static_cast<int>(mEntries.size());

        glm::mat4 logoTrans {carouselTrans};
        logoTrans = glm::translate(
            logoTrans, glm::vec3 {i * logoSpacing.x + xOff, i * logoSpacing.y + yOff, 0.0f});

        float distance = i - mCamOffset;

        float scale {1.0f + ((mLogoScale - 1.0f) * (1.0f - fabs(distance)))};
        scale = std::min(mLogoScale, std::max(1.0f, scale));
        scale /= mLogoScale;

        int opacity {
            static_cast<int>(std::round(0x80 + ((0xFF - 0x80) * (1.0f - fabs(distance)))))};
        opacity = std::max(static_cast<int>(0x80), opacity);

        const std::shared_ptr<GuiComponent>& comp = mEntries.at(index).data.logo;

        if (comp == nullptr)
            continue;

        if (mType == VERTICAL_WHEEL || mType == HORIZONTAL_WHEEL) {
            comp->setRotationDegrees(mLogoRotation * distance);
            comp->setRotationOrigin(mLogoRotationOrigin);
        }

        // When running at lower resolutions, prevent the scale-down to go all the way to the
        // minimum value. This avoids potential single-pixel alignment issues when the logo
        // can't be vertically placed exactly in the middle of the carousel. Although the
        // problem theoretically exists at all resolutions, it's not visble at around 1080p
        // and above.
        if (std::min(Renderer::getScreenWidth(), Renderer::getScreenHeight()) < 1080.0f)
            scale = glm::clamp(scale, 1.0f / mLogoScale + 0.01f, 1.0f);

        comp->setScale(scale);
        comp->setOpacity(static_cast<float>(opacity) / 255.0f);
        comp->render(logoTrans);
    }
}

void CarouselComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                   const std::string& view,
                                   const std::string& element,
                                   unsigned int properties)
{
    using namespace ThemeFlags;
    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "carousel")};

    mSize.x = Renderer::getScreenWidth();
    mSize.y = Renderer::getScreenHeight() * 0.2325f;
    mPosition.x = 0.0f;
    mPosition.y = floorf(0.5f * (Renderer::getScreenHeight() - mSize.y));
    mCarouselColor = 0xFFFFFFD8;
    mCarouselColorEnd = 0xFFFFFFD8;
    mDefaultZIndex = 50.0f;
    mText = "";

    if (!elem)
        return;

    if (elem->has("type")) {
        const std::string type {elem->get<std::string>("type")};
        if (type == "horizontal") {
            mType = HORIZONTAL;
        }
        else if (type == "horizontal_wheel") {
            mType = HORIZONTAL_WHEEL;
        }
        else if (type == "vertical") {
            mType = VERTICAL;
        }
        else if (type == "vertical_wheel") {
            mType = VERTICAL_WHEEL;
        }
        else {
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "<type> set to \""
                            << type << "\"";
            mType = HORIZONTAL;
        }
    }

    if (elem->has("color")) {
        mCarouselColor = elem->get<unsigned int>("color");
        mCarouselColorEnd = mCarouselColor;
    }
    if (elem->has("colorEnd"))
        mCarouselColorEnd = elem->get<unsigned int>("colorEnd");

    if (elem->has("gradientType")) {
        const std::string gradientType {elem->get<std::string>("gradientType")};
        if (gradientType == "horizontal") {
            mColorGradientHorizontal = true;
        }
        else if (gradientType == "vertical") {
            mColorGradientHorizontal = false;
        }
        else {
            mColorGradientHorizontal = true;
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "<gradientType> set to \""
                            << gradientType << "\"";
        }
    }

    if (elem->has("logoScale"))
        mLogoScale = glm::clamp(elem->get<float>("logoScale"), 0.5f, 3.0f);
    if (elem->has("logoSize")) {
        // Keep size within a 0.05 and 1.0 multiple of the screen size.
        glm::vec2 logoSize {elem->get<glm::vec2>("logoSize")};
        if (std::max(logoSize.x, logoSize.y) > 1.0f) {
            logoSize /= std::max(logoSize.x, logoSize.y);
        }
        else if (std::min(logoSize.x, logoSize.y) < 0.005f) {
            float ratio {std::min(logoSize.x, logoSize.y) / 0.005f};
            logoSize /= ratio;
            // Just an extra precaution if a crazy ratio was used.
            logoSize.x = glm::clamp(logoSize.x, 0.005f, 1.0f);
            logoSize.y = glm::clamp(logoSize.y, 0.005f, 1.0f);
        }
        mLogoSize = logoSize * glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    }
    if (elem->has("maxLogoCount"))
        mMaxLogoCount =
            glm::clamp(static_cast<int>(elem->get<unsigned int>("maxLogoCount")), 2, 30);

    if (elem->has("logoRotation"))
        mLogoRotation = elem->get<float>("logoRotation");
    if (elem->has("logoRotationOrigin"))
        mLogoRotationOrigin = elem->get<glm::vec2>("logoRotationOrigin");

    if (elem->has("logoHorizontalAlignment")) {
        const std::string alignment {elem->get<std::string>("logoHorizontalAlignment")};
        if (alignment == "left" && mType != HORIZONTAL) {
            mLogoHorizontalAlignment = ALIGN_LEFT;
        }
        else if (alignment == "right" && mType != HORIZONTAL) {
            mLogoHorizontalAlignment = ALIGN_RIGHT;
        }
        else if (alignment == "center") {
            mLogoHorizontalAlignment = ALIGN_CENTER;
        }
        else {
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "<logoHorizontalAlignment> set to \""
                            << alignment << "\"";
            mLogoHorizontalAlignment = ALIGN_CENTER;
        }
    }

    if (elem->has("logoVerticalAlignment")) {
        const std::string alignment {elem->get<std::string>("logoVerticalAlignment")};
        if (alignment == "top" && mType != VERTICAL) {
            mLogoVerticalAlignment = ALIGN_TOP;
        }
        else if (alignment == "bottom" && mType != VERTICAL) {
            mLogoVerticalAlignment = ALIGN_BOTTOM;
        }
        else if (alignment == "center") {
            mLogoVerticalAlignment = ALIGN_CENTER;
        }
        else {
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "<logoVerticalAlignment> set to \""
                            << alignment << "\"";
            mLogoVerticalAlignment = ALIGN_CENTER;
        }
    }

    // Legacy themes only.
    if (elem->has("logoAlignment")) {
        const std::string alignment {elem->get<std::string>("logoAlignment")};
        if (alignment == "left" && mType != HORIZONTAL) {
            mLogoHorizontalAlignment = ALIGN_LEFT;
            mLogoVerticalAlignment = ALIGN_CENTER;
        }
        else if (alignment == "right" && mType != HORIZONTAL) {
            mLogoHorizontalAlignment = ALIGN_RIGHT;
            mLogoVerticalAlignment = ALIGN_CENTER;
        }
        else if (alignment == "top" && mType != VERTICAL) {
            mLogoVerticalAlignment = ALIGN_TOP;
            mLogoHorizontalAlignment = ALIGN_CENTER;
        }
        else if (alignment == "bottom" && mType != VERTICAL) {
            mLogoVerticalAlignment = ALIGN_BOTTOM;
            mLogoHorizontalAlignment = ALIGN_CENTER;
        }
        else if (alignment == "center") {
            mLogoHorizontalAlignment = ALIGN_CENTER;
            mLogoVerticalAlignment = ALIGN_CENTER;
        }
        else {
            LOG(LogWarning) << "CarouselComponent: Invalid theme configuration, property "
                               "<logoAlignment> set to \""
                            << alignment << "\"";
            mLogoHorizontalAlignment = ALIGN_CENTER;
            mLogoVerticalAlignment = ALIGN_CENTER;
        }
    }

    mFont = Font::getFromTheme(elem, properties, mFont);

    if (elem->has("textColor"))
        mTextColor = elem->get<unsigned int>("textColor");
    if (elem->has("textBackgroundColor"))
        mTextBackgroundColor = elem->get<unsigned int>("textBackgroundColor");

    if (elem->has("lineSpacing"))
        mLineSpacing = glm::clamp(elem->get<float>("lineSpacing"), 0.5f, 3.0f);

    std::string letterCase;

    if (elem->has("letterCase"))
        letterCase = elem->get<std::string>("letterCase");

    if (elem->has("text")) {
        if (letterCase == "uppercase") {
            mText = Utils::String::toUpper(elem->get<std::string>("text"));
        }
        else if (letterCase == "lowercase") {
            mText = Utils::String::toLower(elem->get<std::string>("text"));
        }
        else if (letterCase == "capitalize") {
            mText = Utils::String::toCapitalized(elem->get<std::string>("text"));
        }
        else if (letterCase == "none") {
            mText = elem->get<std::string>("text");
        }
        else {
            LOG(LogWarning)
                << "CarouselComponent: Invalid theme configuration, property <letterCase> set to \""
                << letterCase << "\"";
            mText = elem->get<std::string>("text");
        }
    }

    GuiComponent::applyTheme(theme, view, element, ALL);
}

void CarouselComponent::onCursorChanged(const CursorState& state)
{
    float startPos {mCamOffset};
    float posMax {static_cast<float>(mEntries.size())};
    float target {static_cast<float>(mCursor)};

    // Find the shortest path to the target.
    float endPos {target}; // Directly.
    float dist {fabs(endPos - startPos)};

    if (fabs(target + posMax - startPos - mScrollVelocity) < dist)
        endPos = target + posMax; // Loop around the end (0 -> max).
    if (fabs(target - posMax - startPos - mScrollVelocity) < dist)
        endPos = target - posMax; // Loop around the start (max - 1 -> -1).

    // Make sure there are no reverse jumps between logos.
    bool changedDirection {false};
    if (mPreviousScrollVelocity != 0 && mPreviousScrollVelocity != mScrollVelocity)
        changedDirection = true;

    if (!changedDirection && mScrollVelocity > 0 && endPos < startPos)
        endPos = endPos + posMax;

    if (!changedDirection && mScrollVelocity < 0 && endPos > startPos)
        endPos = endPos - posMax;

    if (mScrollVelocity != 0)
        mPreviousScrollVelocity = mScrollVelocity;

    // No need to animate transition, we're not going anywhere (probably mEntries.size() == 1).
    if (endPos == mCamOffset)
        return;

    Animation* anim = new LambdaAnimation(
        [this, startPos, endPos, posMax](float t) {
            t -= 1;
            float f {glm::mix(startPos, endPos, t * t * t + 1)};
            if (f < 0)
                f += posMax;
            if (f >= posMax)
                f -= posMax;

            mCamOffset = f;
        },
        500);

    setAnimation(anim, 0, nullptr, false, 0);

    if (mCursorChangedCallback)
        mCursorChangedCallback(state);
}

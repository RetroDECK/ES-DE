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
    const int logoBuffersLeft[] = {-5, -2, -1};
    const int logoBuffersRight[] = {1, 2, 5};

} // namespace

CarouselComponent::CarouselComponent()
    : IList<CarouselElement, SystemData*> {LIST_SCROLL_STYLE_SLOW, LIST_ALWAYS_LOOP}
    , mCamOffset {0.0f}
    , mPreviousScrollVelocity {0}
    , mType {HORIZONTAL}
    , mLogoAlignment {ALIGN_CENTER}
    , mMaxLogoCount {3}
    , mLogoSize {Renderer::getScreenWidth() * 0.25f, Renderer::getScreenHeight() * 0.155f}
    , mLogoScale {1.2f}
    , mLogoRotation {7.5f}
    , mLogoRotationOrigin {-5.0f, 0.5f}
    , mCarouselColor {0}
    , mCarouselColorEnd {0}
    , mColorGradientHorizontal {true}
{
}

void CarouselComponent::addEntry(const std::shared_ptr<ThemeData>& theme, Entry& entry)
{
    // Make logo.
    const ThemeData::ThemeElement* logoElem {theme->getElement("system", "image_logo", "image")};

    if (logoElem) {
        std::string path;
        if (logoElem->has("path"))
            path = logoElem->get<std::string>("path");
        std::string defaultPath {logoElem->has("default") ? logoElem->get<std::string>("default") :
                                                            ""};
        if ((!path.empty() && ResourceManager::getInstance().fileExists(path)) ||
            (!defaultPath.empty() && ResourceManager::getInstance().fileExists(defaultPath))) {
            auto logo = std::make_shared<ImageComponent>(false, false);
            logo->setMaxSize(glm::round(mLogoSize * mLogoScale));
            logo->applyTheme(theme, "system", "image_logo", ThemeFlags::PATH | ThemeFlags::COLOR);
            logo->setRotateByTargetSize(true);
            entry.data.logo = logo;
        }
    }

    if (!entry.data.logo) {
        glm::vec2 resolution {static_cast<float>(Renderer::getScreenWidth()),
                              static_cast<float>(Renderer::getScreenHeight())};
        glm::vec3 center {resolution.x / 2.0f, resolution.y / 2.0f, 1.0f};

        // Placeholder Image.
        logoElem = theme->getElement("system", "image_logoPlaceholderImage", "image");
        if (logoElem) {
            auto path = logoElem->get<std::string>("path");
            std::string defaultPath {
                logoElem->has("default") ? logoElem->get<std::string>("default") : ""};
            if ((!path.empty() && ResourceManager::getInstance().fileExists(path)) ||
                (!defaultPath.empty() && ResourceManager::getInstance().fileExists(defaultPath))) {
                auto logo = std::make_shared<ImageComponent>(false, false);
                logo->applyTheme(theme, "system", "image_logoPlaceholderImage", ThemeFlags::ALL);
                if (!logoElem->has("size"))
                    logo->setMaxSize(mLogoSize * mLogoScale);
                logo->setRotateByTargetSize(true);
                entry.data.logo = logo;
            }
        }

        // Placeholder Text.
        const ThemeData::ThemeElement* logoPlaceholderText =
            theme->getElement("system", "text_logoPlaceholderText", "text");
        if (logoPlaceholderText) {
            // Element 'logoPlaceholderText' found in theme configuration.
            auto text = std::make_shared<TextComponent>(entry.name, Font::get(FONT_SIZE_LARGE),
                                                        0x000000FF, ALIGN_CENTER);
            text->setSize(mLogoSize * mLogoScale);
            if (mType == VERTICAL || mType == VERTICAL_WHEEL) {
                text->setHorizontalAlignment(mLogoAlignment);
                text->setVerticalAlignment(ALIGN_CENTER);
            }
            else {
                text->setHorizontalAlignment(ALIGN_CENTER);
                text->setVerticalAlignment(mLogoAlignment);
            }
            text->applyTheme(theme, "system", "text_logoPlaceholderText",
                             ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE | ThemeFlags::COLOR |
                                 ThemeFlags::FORCE_UPPERCASE | ThemeFlags::LINE_SPACING |
                                 ThemeFlags::TEXT);
            if (!entry.data.logo)
                entry.data.logo = text;
        }
        else {
            // Fallback to legacy centered placeholder text.
            auto text = std::make_shared<TextComponent>(entry.name, Font::get(FONT_SIZE_LARGE),
                                                        0x000000FF, ALIGN_CENTER);
            text->setSize(mLogoSize * mLogoScale);
            text->applyTheme(theme, "system", "text_logoText",
                             ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE | ThemeFlags::COLOR |
                                 ThemeFlags::FORCE_UPPERCASE | ThemeFlags::LINE_SPACING |
                                 ThemeFlags::TEXT);
            entry.data.logo = text;

            if (mType == VERTICAL || mType == VERTICAL_WHEEL) {
                text->setHorizontalAlignment(mLogoAlignment);
                text->setVerticalAlignment(ALIGN_CENTER);
            }
            else {
                text->setHorizontalAlignment(ALIGN_CENTER);
                text->setVerticalAlignment(mLogoAlignment);
            }
        }
    }

    if (mType == VERTICAL || mType == VERTICAL_WHEEL) {
        if (mLogoAlignment == ALIGN_LEFT)
            entry.data.logo->setOrigin(0, 0.5);
        else if (mLogoAlignment == ALIGN_RIGHT)
            entry.data.logo->setOrigin(1.0, 0.5);
        else
            entry.data.logo->setOrigin(0.5, 0.5);
    }
    else {
        if (mLogoAlignment == ALIGN_TOP)
            entry.data.logo->setOrigin(0.5, 0);
        else if (mLogoAlignment == ALIGN_BOTTOM)
            entry.data.logo->setOrigin(0.5, 1);
        else
            entry.data.logo->setOrigin(0.5, 0.5);
    }

    glm::vec2 denormalized {mLogoSize * entry.data.logo->getOrigin()};
    entry.data.logo->setPosition(glm::vec3 {denormalized.x, denormalized.y, 0.0f});

    add(entry);
}

void CarouselComponent::update(int deltaTime)
{
    listUpdate(deltaTime);
    GuiComponent::update(deltaTime);
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

void CarouselComponent::render(const glm::mat4& parentTrans)
{
    // Background box behind logos.
    glm::mat4 carouselTrans {parentTrans};
    carouselTrans = glm::translate(carouselTrans, glm::vec3 {mPosition.x, mPosition.y, 0.0f});
    carouselTrans = glm::translate(
        carouselTrans, glm::vec3 {mOrigin.x * mSize.x * -1.0f, mOrigin.y * mSize.y * -1.0f, 0.0f});

    glm::vec2 clipPos {carouselTrans[3].x, carouselTrans[3].y};
    Renderer::pushClipRect(
        glm::ivec2 {static_cast<int>(std::round(clipPos.x)),
                    static_cast<int>(std::round(clipPos.y))},
        glm::ivec2 {static_cast<int>(std::round(mSize.x)), static_cast<int>(std::round(mSize.y))});

    Renderer::setMatrix(carouselTrans);
    Renderer::drawRect(0.0f, 0.0f, mSize.x, mSize.y, mCarouselColor, mCarouselColorEnd,
                       mColorGradientHorizontal);

    // Draw logos.
    // logoSpacing will also include the size of the logo itself.
    glm::vec2 logoSpacing {};
    float xOff {0.0f};
    float yOff {0.0f};

    switch (mType) {
        case VERTICAL_WHEEL: {
            yOff = (mSize.y - mLogoSize.y) / 2.0f - (mCamOffset * logoSpacing.y);
            if (mLogoAlignment == ALIGN_LEFT)
                xOff = mLogoSize.x / 10.0f;
            else if (mLogoAlignment == ALIGN_RIGHT)
                xOff = mSize.x - (mLogoSize.x * 1.1f);
            else
                xOff = (mSize.x - mLogoSize.x) / 2.0f;
            break;
        }
        case VERTICAL: {
            logoSpacing.y =
                ((mSize.y - (mLogoSize.y * mMaxLogoCount)) / (mMaxLogoCount)) + mLogoSize.y;
            yOff = (mSize.y - mLogoSize.y) / 2.0f - (mCamOffset * logoSpacing.y);
            if (mLogoAlignment == ALIGN_LEFT)
                xOff = mLogoSize.x / 10.0f;
            else if (mLogoAlignment == ALIGN_RIGHT)
                xOff = mSize.x - (mLogoSize.x * 1.1f);
            else
                xOff = (mSize.x - mLogoSize.x) / 2.0f;
            break;
        }
        case HORIZONTAL_WHEEL: {
            xOff = std::round((mSize.x - mLogoSize.x) / 2.0f - (mCamOffset * logoSpacing.y));
            if (mLogoAlignment == ALIGN_TOP)
                yOff = mLogoSize.y / 10.0f;
            else if (mLogoAlignment == ALIGN_BOTTOM)
                yOff = mSize.y - (mLogoSize.y * 1.1f);
            else
                yOff = (mSize.y - mLogoSize.y) / 2.0f;
            break;
        }
        case HORIZONTAL: {
        }
        default: {
            logoSpacing.x =
                ((mSize.x - (mLogoSize.x * mMaxLogoCount)) / (mMaxLogoCount)) + mLogoSize.x;
            xOff = std::round((mSize.x - mLogoSize.x) / 2.0f - (mCamOffset * logoSpacing.x));
            if (mLogoAlignment == ALIGN_TOP)
                yOff = mLogoSize.y / 10.0f;
            else if (mLogoAlignment == ALIGN_BOTTOM)
                yOff = mSize.y - (mLogoSize.y * 1.1f);
            else
                yOff = (mSize.y - mLogoSize.y) / 2.0f;
            break;
        }
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
        comp->setOpacity(static_cast<unsigned char>(opacity));
        comp->render(logoTrans);
    }

    Renderer::popClipRect();
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

    if (!elem)
        return;

    if (elem->has("type")) {
        if (!(elem->get<std::string>("type").compare("vertical")))
            mType = VERTICAL;
        else if (!(elem->get<std::string>("type").compare("vertical_wheel")))
            mType = VERTICAL_WHEEL;
        else if (!(elem->get<std::string>("type").compare("horizontal_wheel")))
            mType = HORIZONTAL_WHEEL;
        else
            mType = HORIZONTAL;
    }

    if (elem->has("color")) {
        mCarouselColor = elem->get<unsigned int>("color");
        mCarouselColorEnd = mCarouselColor;
    }
    if (elem->has("colorEnd"))
        mCarouselColorEnd = elem->get<unsigned int>("colorEnd");
    if (elem->has("gradientType"))
        mColorGradientHorizontal = !(elem->get<std::string>("gradientType").compare("horizontal"));

    if (elem->has("logoScale"))
        mLogoScale = elem->get<float>("logoScale");
    if (elem->has("logoSize"))
        mLogoSize = elem->get<glm::vec2>("logoSize") *
                    glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    if (elem->has("maxLogoCount"))
        mMaxLogoCount = static_cast<int>(std::round(elem->get<float>("maxLogoCount")));

    if (elem->has("logoRotation"))
        mLogoRotation = elem->get<float>("logoRotation");
    if (elem->has("logoRotationOrigin"))
        mLogoRotationOrigin = elem->get<glm::vec2>("logoRotationOrigin");
    if (elem->has("logoAlignment")) {
        if (!(elem->get<std::string>("logoAlignment").compare("left")))
            mLogoAlignment = ALIGN_LEFT;
        else if (!(elem->get<std::string>("logoAlignment").compare("right")))
            mLogoAlignment = ALIGN_RIGHT;
        else if (!(elem->get<std::string>("logoAlignment").compare("top")))
            mLogoAlignment = ALIGN_TOP;
        else if (!(elem->get<std::string>("logoAlignment").compare("bottom")))
            mLogoAlignment = ALIGN_BOTTOM;
        else
            mLogoAlignment = ALIGN_CENTER;
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
    if (mPreviousScrollVelocity != 0 && posMax == 2 && mScrollVelocity == mPreviousScrollVelocity) {
        if (fabs(endPos - startPos) < 0.5 || fabs(endPos - startPos) > 1.5) {
            (mScrollVelocity < 0) ? endPos -= 1 : endPos += 1;
            (mCursor == 0) ? mCursor = 1 : mCursor = 0;
            return;
        }
    }

    // No need to animate transition, we're not going anywhere (probably mEntries.size() == 1).
    if (endPos == mCamOffset)
        return;

    Animation* anim = new LambdaAnimation(
        [this, startPos, endPos, posMax](float t) {
            t -= 1;
            float f = glm::mix(startPos, endPos, t * t * t + 1);
            if (f < 0)
                f += posMax;
            if (f >= posMax)
                f -= posMax;

            this->mCamOffset = f;
        },
        500);

    setAnimation(anim, 0, nullptr, false, 0);

    if (mCursorChangedCallback)
        mCursorChangedCallback(state);
}

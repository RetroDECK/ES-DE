//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  RatingComponent.cpp
//
//  Game rating icons.
//  Used by gamelist views, metadata editor and scraper.
//

#include "components/RatingComponent.h"

#include "Settings.h"
#include "ThemeData.h"
#include "resources/TextureResource.h"

RatingComponent::RatingComponent(bool colorizeChanges)
    : mRenderer {Renderer::getInstance()}
    , mColorOriginalValue {DEFAULT_COLORSHIFT}
    , mColorChangedValue {DEFAULT_COLORSHIFT}
    , mColorShift {DEFAULT_COLORSHIFT}
    , mColorShiftEnd {DEFAULT_COLORSHIFT}
    , mUnfilledColor {DEFAULT_COLORSHIFT}
    , mColorizeChanges {colorizeChanges}
{
    mFilledTexture = TextureResource::get(":/graphics/star_filled.svg", true);
    mUnfilledTexture = TextureResource::get(":/graphics/star_unfilled.svg", true);
    mValue = 0.5f;
    mSize = glm::vec2 {64.0f * NUM_RATING_STARS, 64.0f};
    updateVertices();
    updateColors();
}

void RatingComponent::setValue(const std::string& value)
{
    if (value.empty()) {
        mValue = 0.0f;
    }
    else {
        // Round to the closest .1 value, i.e. to the closest half-icon.
        mValue = std::round(stof(value) / 0.1f) / 10.0f;
        mOriginalValue = static_cast<int>(mValue * 10.0f);

        // If the argument to colorize the rating icons has been passed, set the
        // color shift accordingly.
        if (mColorizeChanges) {
            if (static_cast<int>(mValue * 10.0f) == mOriginalValue)
                setColorShift(mColorOriginalValue);
            else
                setColorShift(mColorChangedValue);
        }

        // For the special situation where there is a fractional rating in the gamelist.xml
        // file that has been rounded to a half-star rating, render the rating icons green.
        // This should only happen if an external scraper has been used or if the file has
        // been manually edited.
        if (mColorizeChanges && mValue != stof(value)) {
            mOriginalValue = ICONCOLOR_USERMARKED;
            setColorShift(0x449944FF);
        }

        if (mValue > 1.0f)
            mValue = 1.0f;
        else if (mValue < 0.0f)
            mValue = 0.0f;
    }

    updateVertices();
}

std::string RatingComponent::getValue() const
{
    // Do not use std::to_string here as it will use the current locale
    // and that sometimes encodes decimals as commas.
    std::stringstream ss;
    ss << mValue;
    return ss.str();
}

std::string RatingComponent::getRatingValue(const std::string& rating)
{
    std::stringstream ss;
    ss << (std::round(stof(rating) / 0.1f) / 10.0f) * NUM_RATING_STARS;
    return ss.str();
}

void RatingComponent::setOpacity(float opacity)
{
    mOpacity = opacity;
    mColorShift =
        (mColorShift >> 8 << 8) | static_cast<unsigned char>(mOpacity * mThemeOpacity * 255.0f);
    updateColors();
}

void RatingComponent::setDimming(float dimming)
{
    mDimming = dimming;
    mVertices[0].dimming = mDimming;
    mVertices[4].dimming = mDimming;
}

void RatingComponent::setColorShift(unsigned int color)
{
    mColorShift = color;
    mColorShiftEnd = color;

    // Grab the opacity from the color shift because we may need
    // to apply it if fading in textures.
    mOpacity = static_cast<float>(color & 0xff) / 255.0f;
    updateColors();
}

void RatingComponent::onSizeChanged()
{
    // Make sure the size is not unreasonably large (which may be caused by a mistake in
    // the theme configuration).
    mSize.x = glm::clamp(mSize.x, 0.0f, mRenderer->getScreenWidth() / 2.0f);
    mSize.y = glm::clamp(mSize.y, 0.0f, mRenderer->getScreenHeight() / 2.0f);

    if (mSize.y == 0.0f)
        mSize.y = mSize.x / NUM_RATING_STARS;
    else if (mSize.x == 0.0f)
        mSize.x = mSize.y * NUM_RATING_STARS;

    if (mSize.y > 0.0f) {
        if (mFilledTexture)
            mFilledTexture->rasterizeAt(mSize.y, mSize.y);
        if (mUnfilledTexture)
            mUnfilledTexture->rasterizeAt(mSize.y, mSize.y);
    }

    updateVertices();
}

void RatingComponent::updateVertices()
{
    const float numStars {NUM_RATING_STARS};
    const float h {getSize().y}; // Ss the same as a single star's width.
    const float w {getSize().y * mValue * numStars};
    const float fw {getSize().y * numStars};

    // clang-format off
    mVertices[0] = {{0.0f, 0.0f}, {0.0f,              1.0f}, mColorShift};
    mVertices[1] = {{0.0f, h   }, {0.0f,              0.0f}, mColorShift};
    mVertices[2] = {{w,    0.0f}, {mValue * numStars, 1.0f}, mColorShift};
    mVertices[3] = {{w,    h   }, {mValue * numStars, 0.0f}, mColorShift};

    mVertices[4] = {{0.0f, 0.0f}, {0.0f,              1.0f}, mColorShift};
    mVertices[5] = {{0.0f, h   }, {0.0f,              0.0f}, mColorShift};
    mVertices[6] = {{fw,   0.0f}, {numStars,          1.0f}, mColorShift};
    mVertices[7] = {{fw,   h   }, {numStars,          0.0f}, mColorShift};
    // clang-format on
}

void RatingComponent::updateColors()
{
    for (int i = 0; i < 8; ++i)
        mVertices[i].color = mColorShift;
}

void RatingComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible() || mFilledTexture == nullptr || mUnfilledTexture == nullptr ||
        mThemeOpacity == 0.0f)
        return;

    glm::mat4 trans {parentTrans * getTransform()};

    mRenderer->setMatrix(trans);

    if (mOpacity > 0.0f) {
        if (Settings::getInstance()->getBool("DebugImage")) {
            mRenderer->drawRect(0.0f, 0.0f, mSize.y * NUM_RATING_STARS, mSize.y, 0xFF000033,
                                0xFF000033);
        }

        if (mUnfilledTexture->bind()) {
            if (mUnfilledColor != mColorShift) {
                for (int i = 0; i < 8; ++i)
                    mVertices[i].color =
                        (mUnfilledColor & 0xFFFFFF00) + (mVertices[i].color & 0x000000FF);
            }

            mRenderer->drawTriangleStrips(&mVertices[4], 4);
            mRenderer->bindTexture(0);

            if (mUnfilledColor != mColorShift)
                updateColors();
        }

        if (mFilledTexture->bind()) {
            mRenderer->drawTriangleStrips(&mVertices[0], 4);
            mRenderer->bindTexture(0);
        }
    }

    renderChildren(trans);
}

bool RatingComponent::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("a", input) && input.value != 0) {
        mValue += (1.0f / 2.0f) / NUM_RATING_STARS;
        if (mValue > 1.05f)
            mValue = 0.0f;

        // If the argument to colorize the rating icons has been passed,
        // set the color shift accordingly.
        if (mColorizeChanges) {
            if (static_cast<int>(mValue * 10.0f) == mOriginalValue)
                setColorShift(mColorOriginalValue);
            else
                setColorShift(mColorChangedValue);
        }
        updateVertices();
    }

    return GuiComponent::input(config, input);
}

void RatingComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                 const std::string& view,
                                 const std::string& element,
                                 unsigned int properties)
{
    using namespace ThemeFlags;
    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "rating")};

    if (!elem)
        return;

    // Make sure the size is not unreasonably large (which may be caused by a mistake in
    // the theme configuration).
    mSize.x = glm::clamp(mSize.x, 0.0f, mRenderer->getScreenWidth() / 2.0f);
    mSize.y = glm::clamp(mSize.y, 0.0f, mRenderer->getScreenHeight() / 2.0f);

    if (mSize.y == 0.0f)
        mSize.y = mSize.x / NUM_RATING_STARS;
    else if (mSize.x == 0.0f)
        mSize.x = mSize.y * NUM_RATING_STARS;

    bool imgChanged {false};
    if (properties & PATH && elem->has("filledPath")) {
        mFilledTexture = TextureResource::get(elem->get<std::string>("filledPath"), true, false,
                                              true, false, false, mSize.y, mSize.y);
        imgChanged = true;
    }
    if (properties & PATH && elem->has("unfilledPath")) {
        mUnfilledTexture = TextureResource::get(elem->get<std::string>("unfilledPath"), true, false,
                                                true, false, false, mSize.y, mSize.y);
        imgChanged = true;
    }

    if (properties & COLOR) {
        if (elem->has("color"))
            setColorShift(elem->get<unsigned int>("color"));

        if (elem->has("unfilledColor"))
            mUnfilledColor = elem->get<unsigned int>("unfilledColor");
        else
            mUnfilledColor = mColorShift;
    }

    GuiComponent::applyTheme(theme, view, element, properties);

    if (imgChanged)
        onSizeChanged();
}

std::vector<HelpPrompt> RatingComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("a", "add half star"));
    return prompts;
}

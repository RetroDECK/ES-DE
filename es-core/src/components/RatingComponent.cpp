//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  RatingComponent.cpp
//
//  Game rating icons.
//  Used by gamelist views, metadata editor and scraper.
//

#include "components/RatingComponent.h"

#include "resources/TextureResource.h"
#include "Settings.h"
#include "ThemeData.h"

RatingComponent::RatingComponent(
        Window* window,
        bool colorizeChanges)
        : GuiComponent(window),
        mColorShift(DEFAULT_COLORSHIFT),
        mColorShiftEnd(DEFAULT_COLORSHIFT),
        mUnfilledColor(DEFAULT_COLORSHIFT),
        mColorizeChanges(colorizeChanges),
        mColorOriginalValue(DEFAULT_COLORSHIFT),
        mColorChangedValue(DEFAULT_COLORSHIFT)
{
    mFilledTexture = TextureResource::get(":/graphics/star_filled.svg", true);
    mUnfilledTexture = TextureResource::get(":/graphics/star_unfilled.svg", true);
    mValue = 0.5f;
    mSize = Vector2f(64 * NUM_RATING_STARS, 64);
    updateVertices();
    updateColors();
}

void RatingComponent::setValue(const std::string& value)
{
    if (value.empty()) {
        mValue = 0.0f;
    }
    else {
        // Round up to the closest .1 value, i.e. to the closest half-icon.
        mValue = ceilf(stof(value) / 0.1f) / 10;
        mOriginalValue = static_cast<int>(mValue * 10);

        // If the argument to colorize the rating icons has been passed, set the
        // color shift accordingly.
        if (mColorizeChanges) {
            if (static_cast<int>(mValue * 10) == mOriginalValue)
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

void RatingComponent::setOpacity(unsigned char opacity)
{
    mOpacity = opacity;
    mColorShift = (mColorShift >> 8 << 8) | mOpacity;
    updateColors();
}

void RatingComponent::setColorShift(unsigned int color)
{
    mColorShift = color;
    mColorShiftEnd = color;

    // Grab the opacity from the color shift because we may need
    // to apply it if fading in textures.
    mOpacity = color & 0xff;
    updateColors();
}

void RatingComponent::onSizeChanged()
{
    if (mSize.y() == 0)
        mSize[1] = mSize.x() / NUM_RATING_STARS;
    else if (mSize.x() == 0)
        mSize[0] = mSize.y() * NUM_RATING_STARS;

    if (mSize.y() > 0) {
        size_t heightPx = static_cast<size_t>(std::round(mSize.y()));
        if (mFilledTexture)
            mFilledTexture->rasterizeAt(heightPx, heightPx);
        if (mUnfilledTexture)
            mUnfilledTexture->rasterizeAt(heightPx, heightPx);
    }

    updateVertices();
}

void RatingComponent::updateVertices()
{
    const float numStars = NUM_RATING_STARS;
    const float h = getSize().y(); // Ss the same as a single star's width.
    const float w = getSize().y() * mValue * numStars;
    const float fw = getSize().y() * numStars;
    const unsigned int color = Renderer::convertRGBAToABGR(mColorShift);

    mVertices[0] = { { 0.0f, 0.0f }, { 0.0f,              1.0f }, color };
    mVertices[1] = { { 0.0f, h    }, { 0.0f,              0.0f }, color };
    mVertices[2] = { { w,    0.0f }, { mValue * numStars, 1.0f }, color };
    mVertices[3] = { { w,    h    }, { mValue * numStars, 0.0f }, color };

    mVertices[4] = { { 0.0f, 0.0f }, { 0.0f,              1.0f }, color };
    mVertices[5] = { { 0.0f, h    }, { 0.0f,              0.0f }, color };
    mVertices[6] = { { fw,   0.0f }, { numStars,          1.0f }, color };
    mVertices[7] = { { fw,   h    }, { numStars,          0.0f }, color };


//    Disabled this code as it caused subtle but strange rendering errors
//    where the icons changed size slightly when changing rating scores.
//    // Round vertices.
//    for (int i = 0; i < 8; i++)
//        mVertices[i].pos.round();
}

void RatingComponent::updateColors()
{
    const unsigned int color = Renderer::convertRGBAToABGR(mColorShift);

    for (int i = 0; i < 8; i++)
        mVertices[i].col = color;
}

void RatingComponent::render(const Transform4x4f& parentTrans)
{
    if (!isVisible() || mFilledTexture == nullptr || mUnfilledTexture == nullptr)
        return;

    Transform4x4f trans = parentTrans * getTransform();

    Renderer::setMatrix(trans);

    if (mOpacity > 0) {
        if (Settings::getInstance()->getBool("DebugImage")) {
            Renderer::drawRect(0.0f, 0.0f, mSize.y() * NUM_RATING_STARS,
                    mSize.y(), 0xFF000033, 0xFF000033);
        }

        if (mUnfilledTexture->bind()) {
            if (mUnfilledColor != mColorShift) {
                const unsigned int color = Renderer::convertRGBAToABGR(mUnfilledColor);
                for (int i = 0; i < 8; i++)
                    mVertices[i].col = color;
            }

            Renderer::drawTriangleStrips(&mVertices[4], 4);
            Renderer::bindTexture(0);

            if (mUnfilledColor != mColorShift)
                updateColors();
        }

        if (mFilledTexture->bind()) {
            Renderer::drawTriangleStrips(&mVertices[0], 4);
            Renderer::bindTexture(0);
        }
    }

    renderChildren(trans);
}

bool RatingComponent::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("a", input) && input.value != 0) {
        mValue += (1.f/2) / NUM_RATING_STARS;
        if (mValue > 1.05f)
            mValue = 0.0f;

        // If the argument to colorize the rating icons has been passed,
        // set the color shift accordingly.
        if (mColorizeChanges) {
            if (static_cast<int>(mValue * 10) == mOriginalValue)
                setColorShift(mColorOriginalValue);
            else
                setColorShift(mColorChangedValue);
        }
        updateVertices();
    }

    return GuiComponent::input(config, input);
}

void RatingComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
        const std::string& view, const std::string& element, unsigned int properties)
{
    using namespace ThemeFlags;

    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "rating");
    if (!elem)
        return;

    bool imgChanged = false;
    if (properties & PATH && elem->has("filledPath")) {
        mFilledTexture = TextureResource::get(elem->get<std::string>("filledPath"), true);
        imgChanged = true;
    }
    if (properties & PATH && elem->has("unfilledPath")) {
        mUnfilledTexture = TextureResource::get(elem->get<std::string>("unfilledPath"), true);
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

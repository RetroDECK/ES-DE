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
    , mColorizeChanges {colorizeChanges}
{
    mSize = glm::vec2 {std::round(mRenderer->getScreenHeight() * 0.06f) * NUM_RATING_STARS,
                       std::round(mRenderer->getScreenHeight() * 0.06f)};

    mIconFilled.setResize(mSize, false);
    mIconFilled.setTileSize(mSize.y, mSize.y);

    mIconUnfilled.setResize(mSize, false);
    mIconUnfilled.setTileSize(mSize.y, mSize.y);

    mIconFilled.setImage(std::string(":/graphics/star_filled.svg"), true);
    mIconUnfilled.setImage(std::string(":/graphics/star_unfilled.svg"), true);

    mValue = 0.5f;
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
                mIconFilled.setColorShift(mColorOriginalValue);
            else
                mIconFilled.setColorShift(mColorChangedValue);
        }

        // For the special situation where there is a fractional rating in the gamelist.xml
        // file that has been rounded to a half-star rating, render the rating icons green.
        // This should only happen if an external scraper has been used or if the file has
        // been manually edited.
        if (mColorizeChanges && mValue != stof(value)) {
            mOriginalValue = ICONCOLOR_USERMARKED;
            mIconFilled.setColorShift(0x449944FF);
        }

        if (mValue > 1.0f)
            mValue = 1.0f;
        else if (mValue < 0.0f)
            mValue = 0.0f;
    }

    mIconFilled.setClipRegion(
        glm::vec4 {0.0f, 0.0f, std::round(mIconFilled.getSize().x * mValue), mSize.y});
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

void RatingComponent::setDimming(float dimming)
{
    mDimming = dimming;
    mIconFilled.setDimming(dimming);
    mIconUnfilled.setDimming(dimming);
}

void RatingComponent::onSizeChanged()
{
    mSize = glm::round(mSize);

    if (mSize.x == 0.0f)
        mSize.x = mSize.y * NUM_RATING_STARS;

    mIconFilled.getTexture()->setSize(mSize.y, mSize.y);
    mIconFilled.setResize(glm::vec2 {mSize.y * NUM_RATING_STARS, mSize.y}, true);

    mIconUnfilled.getTexture()->setSize(mSize.y, mSize.y);
    mIconUnfilled.setResize(glm::vec2 {mSize.y * NUM_RATING_STARS, mSize.y}, true);
}

void RatingComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible() || mThemeOpacity == 0.0f || mOpacity == 0.0f)
        return;

    glm::mat4 trans {parentTrans * getTransform()};

    mIconUnfilled.setOpacity(mOpacity * mThemeOpacity);
    mIconFilled.setOpacity(mOpacity * mThemeOpacity);

    mIconUnfilled.render(trans);
    mIconFilled.render(trans);
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
                mIconFilled.setColorShift(mColorOriginalValue);
            else
                mIconFilled.setColorShift(mColorChangedValue);
        }

        mIconFilled.setClipRegion(
            glm::vec4 {0.0f, 0.0f, std::round(mIconFilled.getSize().x * mValue), mSize.y});
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

    GuiComponent::applyTheme(theme, view, element, properties ^ ThemeFlags::SIZE);

    glm::vec2 scale {getParent() ?
                         getParent()->getSize() :
                         glm::vec2(Renderer::getScreenWidth(), Renderer::getScreenHeight())};

    if (elem->has("size")) {
        glm::vec2 ratingSize {elem->get<glm::vec2>("size")};
        if (ratingSize == glm::vec2 {0.0f, 0.0f}) {
            LOG(LogWarning) << "RatingComponent: Invalid theme configuration, property <size> "
                               "for element \""
                            << element.substr(7) << "\" is set to zero";
            ratingSize.y = 0.06f;
        }
        if (ratingSize.x > 0.0f)
            ratingSize.x = glm::clamp(ratingSize.x, 0.01f, 1.0f);
        if (ratingSize.y > 0.0f)
            ratingSize.y = glm::clamp(ratingSize.y, 0.01f, 0.5f);
        mSize = glm::round(ratingSize * scale);
        if (mSize.y == 0.0f)
            mSize.y = mSize.x / NUM_RATING_STARS;
        else
            mSize.x = mSize.y * NUM_RATING_STARS;
    }

    bool linearInterpolation {false};

    if (elem->has("interpolation")) {
        const std::string interpolation {elem->get<std::string>("interpolation")};
        if (interpolation == "linear") {
            linearInterpolation = true;
        }
        else if (interpolation == "nearest") {
            linearInterpolation = false;
        }
        else {
            linearInterpolation = false;

            LOG(LogWarning)
                << "RatingComponent: Invalid theme configuration, property <interpolation> "
                   "for element \""
                << element.substr(7) << "\" defined as \"" << interpolation << "\"";
        }
    }

    mIconFilled.setTileSize(mSize.y, mSize.y);
    mIconFilled.setResize(glm::vec2 {mSize}, false);

    if (properties & PATH && elem->has("filledPath")) {
        mIconFilled.setDynamic(true);
        mIconFilled.setLinearInterpolation(linearInterpolation);
        mIconFilled.setImage(std::string(elem->get<std::string>("filledPath")), true);
        mIconFilled.getTexture()->setSize(mSize.y, mSize.y);
        if (!mIconFilled.getTexture()->getScalable())
            mIconFilled.onSizeChanged();
    }
    else {
        mIconFilled.setImage(std::string(":/graphics/star_filled.svg"), true);
    }

    mIconUnfilled.setTileSize(mSize.y, mSize.y);
    mIconUnfilled.setResize(glm::vec2 {mSize}, false);

    if (properties & PATH && elem->has("unfilledPath")) {
        mIconUnfilled.setDynamic(true);
        mIconUnfilled.setLinearInterpolation(linearInterpolation);
        mIconUnfilled.setImage(std::string(elem->get<std::string>("unfilledPath")), true);
        mIconUnfilled.getTexture()->setSize(mSize.y, mSize.y);
        if (!mIconUnfilled.getTexture()->getScalable())
            mIconUnfilled.onSizeChanged();
    }
    else {
        mIconUnfilled.setImage(std::string(":/graphics/star_unfilled.svg"), true);
    }

    if (properties & COLOR) {
        if (elem->has("color")) {
            mIconFilled.setColorShift(elem->get<unsigned int>("color"));
            mIconUnfilled.setColorShift(elem->get<unsigned int>("color"));
        }
    }
}

std::vector<HelpPrompt> RatingComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("a", "add half star"));
    return prompts;
}

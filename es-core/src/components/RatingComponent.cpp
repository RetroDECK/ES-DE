//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RatingComponent.cpp
//
//  Game rating icons.
//  Used by gamelist views, metadata editor and scraper.
//

#include "components/RatingComponent.h"

#include "Settings.h"
#include "ThemeData.h"
#include "resources/TextureResource.h"
#include "utils/LocalizationUtil.h"

RatingComponent::RatingComponent(bool colorizeChanges, bool linearInterpolation)
    : mRenderer {Renderer::getInstance()}
    , mValue {0.5f}
    , mImageRatio {1.0f}
    , mColorOriginalValue {mMenuColorPrimary}
    , mColorChangedValue {mMenuColorPrimary}
    , mColorizeChanges {colorizeChanges}
    , mOverlay {true}
    , mHideIfZero {false}
{
    mSize = glm::vec2 {std::round(mRenderer->getScreenHeight() * 0.06f) * NUM_RATING_STARS,
                       std::round(mRenderer->getScreenHeight() * 0.06f)};

    mIconFilled.setResize(mSize, false);
    mIconFilled.setTileSize(mSize.y, mSize.y);
    mIconFilled.setDynamic(false);
    mIconFilled.setLinearInterpolation(linearInterpolation);
    mIconFilled.setColorShift(mMenuColorPrimary);

    mIconUnfilled.setResize(mSize, false);
    mIconUnfilled.setTileSize(mSize.y, mSize.y);
    mIconUnfilled.setDynamic(false);
    mIconUnfilled.setLinearInterpolation(linearInterpolation);
    mIconUnfilled.setColorShift(mMenuColorPrimary);

    mIconFilled.setImage(std::string(":/graphics/star_filled.svg"), true);
    mIconUnfilled.setImage(std::string(":/graphics/star_unfilled.svg"), true);
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
            mOriginalValue = mMenuColorBlue;
            mIconFilled.setColorShift(0x449944FF);
        }

        if (mValue > 1.0f)
            mValue = 1.0f;
        else if (mValue < 0.0f)
            mValue = 0.0f;
    }

    const float clipValue {std::round(mIconUnfilled.getSize().x * mValue)};
    if (!mOverlay)
        mIconUnfilled.setClipRegion(glm::vec4 {clipValue, 0.0f, mSize.x, mSize.y});
    mIconFilled.setClipRegion(glm::vec4 {0.0f, 0.0f, clipValue, mSize.y});
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
    mIconFilled.setTileSize(mSize.y, mSize.y);
    mIconFilled.setResize(glm::vec2 {std::round(mSize.y * mImageRatio) * NUM_RATING_STARS, mSize.y},
                          true);

    mIconUnfilled.getTexture()->setSize(mSize.y, mSize.y);
    mIconUnfilled.setTileSize(mSize.y, mSize.y);
    mIconUnfilled.setResize(
        glm::vec2 {std::round(mSize.y * mImageRatio) * NUM_RATING_STARS, mSize.y}, true);
}

void RatingComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible() || mThemeOpacity == 0.0f || mOpacity == 0.0f)
        return;

    if (mHideIfZero && mValue == 0.0f)
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

        const float clipValue {std::round(mIconUnfilled.getSize().x * mValue)};
        if (!mOverlay)
            mIconUnfilled.setClipRegion(glm::vec4 {clipValue, 0.0f, mSize.x, mSize.y});
        mIconFilled.setClipRegion(glm::vec4 {0.0f, 0.0f, clipValue, mSize.y});
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

    {
        // Read the image file in order to retrieve the image dimensions needed to calculate
        // the aspect ratio constant.
        if (properties & PATH && elem->has("filledPath")) {
            const std::string& path {std::string(elem->get<std::string>("filledPath"))};
            if (Utils::FileSystem::isRegularFile(path) || Utils::FileSystem::isSymlink(path)) {
                auto tempImage =
                    TextureResource::get(path, false, false, false, false, false, 0, 0, 0.0f, 0.0f);
                mImageRatio = static_cast<float>(tempImage->getSize().x) /
                              static_cast<float>(tempImage->getSize().y);
            }
        }
    }

    if (elem->has("size")) {
        glm::vec2 ratingSize {elem->get<glm::vec2>("size")};
        if (ratingSize == glm::vec2 {0.0f, 0.0f}) {
            LOG(LogWarning) << "RatingComponent: Invalid theme configuration, property \"size\" "
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
            mSize.y = std::round(mSize.x / mImageRatio) / NUM_RATING_STARS;
        else
            mSize.x = std::round(mSize.y * mImageRatio) * NUM_RATING_STARS;
    }

    if (properties & ThemeFlags::POSITION && elem->has("stationary")) {
        const std::string& stationary {elem->get<std::string>("stationary")};
        if (stationary == "never")
            mStationary = Stationary::NEVER;
        else if (stationary == "always")
            mStationary = Stationary::ALWAYS;
        else if (stationary == "withinView")
            mStationary = Stationary::WITHIN_VIEW;
        else if (stationary == "betweenViews")
            mStationary = Stationary::BETWEEN_VIEWS;
        else
            LOG(LogWarning) << "RatingComponent: Invalid theme configuration, property "
                               "\"stationary\" for element \""
                            << element.substr(7) << "\" defined as \"" << stationary << "\"";
    }

    if (elem->has("hideIfZero"))
        mHideIfZero = elem->get<bool>("hideIfZero");

    bool linearInterpolation {false};

    // Enable linear interpolation by default if element is arbitrarily rotated.
    if (properties & ThemeFlags::ROTATION && elem->has("rotation")) {
        const float rotation {std::abs(elem->get<float>("rotation"))};
        if (rotation != 0.0f &&
            (std::round(rotation) != rotation || static_cast<int>(rotation) % 90 != 0))
            linearInterpolation = true;
    }

    if (elem->has("interpolation")) {
        const std::string& interpolation {elem->get<std::string>("interpolation")};
        if (interpolation == "linear") {
            linearInterpolation = true;
        }
        else if (interpolation == "nearest") {
            linearInterpolation = false;
        }
        else {
            LOG(LogWarning)
                << "RatingComponent: Invalid theme configuration, property \"interpolation\" "
                   "for element \""
                << element.substr(7) << "\" defined as \"" << interpolation << "\"";
        }
    }

    mIconFilled.setTileSize(std::round(mSize.y * mImageRatio), mSize.y);
    mIconFilled.setResize(glm::vec2 {mSize}, false);

    if (properties & PATH && elem->has("filledPath") &&
        (Utils::FileSystem::isRegularFile(elem->get<std::string>("filledPath")) ||
         Utils::FileSystem::isSymlink(elem->get<std::string>("filledPath")))) {
        mIconFilled.setLinearInterpolation(linearInterpolation);
        mIconFilled.setImage(std::string(elem->get<std::string>("filledPath")), true);
        mIconFilled.getTexture()->setSize(std::round(mSize.y * mImageRatio), mSize.y);
        mIconFilled.onSizeChanged();
    }
    else {
        mIconFilled.setImage(std::string(":/graphics/star_filled.svg"), true);
    }

    mIconUnfilled.setTileSize(std::round(mSize.y * mImageRatio), mSize.y);
    mIconUnfilled.setResize(glm::vec2 {mSize}, false);

    if (properties & PATH && elem->has("unfilledPath") &&
        (Utils::FileSystem::isRegularFile(elem->get<std::string>("unfilledPath")) ||
         Utils::FileSystem::isSymlink(elem->get<std::string>("unfilledPath")))) {
        mIconUnfilled.setLinearInterpolation(linearInterpolation);
        mIconUnfilled.setImage(std::string(elem->get<std::string>("unfilledPath")), true);
        mIconUnfilled.getTexture()->setSize(std::round(mSize.y * mImageRatio), mSize.y);
        mIconUnfilled.onSizeChanged();
    }
    else {
        mIconUnfilled.setImage(std::string(":/graphics/star_unfilled.svg"), true);
    }

    if (elem->has("overlay") && !elem->get<bool>("overlay"))
        mOverlay = false;

    if (properties & COLOR) {
        if (elem->has("color")) {
            mIconFilled.setColorShift(elem->get<unsigned int>("color"));
            mIconUnfilled.setColorShift(elem->get<unsigned int>("color"));
        }
        else {
            mIconFilled.setColorShift(0xFFFFFFFF);
            mIconUnfilled.setColorShift(0xFFFFFFFF);
        }
    }
}

std::vector<HelpPrompt> RatingComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("a", _("add half star")));
    return prompts;
}

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BadgesComponent.cpp
//
//  Game badges icons.
//  Used by the gamelist views.
//

#define SLOT_FAVORITE "favorite"
#define SLOT_COMPLETED "completed"
#define SLOT_KIDGAME "kidgame"
#define SLOT_BROKEN "broken"
#define SLOT_ALTEMULATOR "altemulator"

#include "components/BadgesComponent.h"

#include "Log.h"
#include "ThemeData.h"
#include "utils/StringUtil.h"

BadgesComponent::BadgesComponent(Window* window)
    : GuiComponent{window}
    , mFlexboxComponent{window, mBadgeImages}
    , mBadgeTypes{{SLOT_FAVORITE, SLOT_COMPLETED, SLOT_KIDGAME, SLOT_BROKEN, SLOT_ALTEMULATOR}}
{
    mBadgeIcons[SLOT_FAVORITE] = ":/graphics/badge_favorite.svg";
    mBadgeIcons[SLOT_COMPLETED] = ":/graphics/badge_completed.svg";
    mBadgeIcons[SLOT_KIDGAME] = ":/graphics/badge_kidgame.svg";
    mBadgeIcons[SLOT_BROKEN] = ":/graphics/badge_broken.svg";
    mBadgeIcons[SLOT_ALTEMULATOR] = ":/graphics/badge_altemulator.svg";
}

void BadgesComponent::setBadges(const std::vector<std::string>& badges)
{
    std::map<std::string, bool> prevVisibility;

    // Save the visibility status to know whether any badges changed.
    for (auto& image : mBadgeImages) {
        prevVisibility[image.first] = image.second.isVisible();
        image.second.setVisible(false);
    }

    for (auto& badge : badges) {
        auto it = std::find_if(
            mBadgeImages.begin(), mBadgeImages.end(),
            [badge](std::pair<std::string, ImageComponent> image) { return image.first == badge; });

        if (it != mBadgeImages.cend())
            it->second.setVisible(true);
    }

    // Only recalculate the flexbox if any badges changed.
    for (auto& image : mBadgeImages) {
        if (prevVisibility[image.first] != image.second.isVisible()) {
            mFlexboxComponent.onSizeChanged();
            break;
        }
    }
}

void BadgesComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    if (mOpacity == 255) {
        mFlexboxComponent.render(parentTrans);
    }
    else {
        mFlexboxComponent.setOpacity(mOpacity);
        mFlexboxComponent.render(parentTrans);
        mFlexboxComponent.setOpacity(255);
    }
}

void BadgesComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                 const std::string& view,
                                 const std::string& element,
                                 unsigned int properties)
{
    using namespace ThemeFlags;

    const ThemeData::ThemeElement* elem{theme->getElement(view, element, "badges")};
    if (!elem)
        return;

    if (elem->has("alignment")) {
        const std::string alignment{elem->get<std::string>("alignment")};
        if (alignment != "left" && alignment != "right") {
            LOG(LogWarning) << "BadgesComponent: Invalid theme configuration, <alignment> set to \""
                            << alignment << "\"";
        }
        else {
            mFlexboxComponent.setAlignment(alignment);
        }
    }

    if (elem->has("itemsPerRow")) {
        const float itemsPerRow{elem->get<float>("itemsPerRow")};
        if (itemsPerRow < 1.0f || itemsPerRow > 10.0f) {
            LOG(LogWarning)
                << "BadgesComponent: Invalid theme configuration, <itemsPerRow> set to \""
                << itemsPerRow << "\"";
        }
        else {
            mFlexboxComponent.setItemsPerLine(static_cast<unsigned int>(itemsPerRow));
        }
    }

    if (elem->has("rows")) {
        const float rows{elem->get<float>("rows")};
        if (rows < 1.0f || rows > 10.0f) {
            LOG(LogWarning) << "BadgesComponent: Invalid theme configuration, <rows> set to \""
                            << rows << "\"";
        }
        else {
            mFlexboxComponent.setLines(static_cast<unsigned int>(rows));
        }
    }

    if (elem->has("itemPlacement")) {
        std::string itemPlacement{elem->get<std::string>("itemPlacement")};
        if (itemPlacement != "top" && itemPlacement != "center" && itemPlacement != "bottom" &&
            itemPlacement != "stretch") {
            LOG(LogWarning)
                << "BadgesComponent: Invalid theme configuration, <itemPlacement> set to \""
                << itemPlacement << "\"";
        }
        else {
            if (itemPlacement == "top")
                itemPlacement = "start";
            else if (itemPlacement == "bottom")
                itemPlacement = "end";
            mFlexboxComponent.setItemPlacement(itemPlacement);
        }
    }

    if (elem->has("itemMargin")) {
        const glm::vec2 itemMargin = elem->get<glm::vec2>("itemMargin");
        if (itemMargin.x < 0.0f || itemMargin.x > 0.2f || itemMargin.y < 0.0f ||
            itemMargin.y > 0.2f) {
            LOG(LogWarning)
                << "BadgesComponent: Invalid theme configuration, <itemMargin> set to \""
                << itemMargin.x << " " << itemMargin.y << "\"";
        }
        else {
            mFlexboxComponent.setItemMargin(itemMargin);
        }
    }

    if (elem->has("slots")) {
        std::vector<std::string> slots = Utils::String::delimitedStringToVector(
            Utils::String::toLower(elem->get<std::string>("slots")), " ");

        for (auto slot : slots) {
            if (std::find(mBadgeTypes.cbegin(), mBadgeTypes.cend(), slot) != mBadgeTypes.end()) {
                if (properties & PATH && elem->has(slot))
                    mBadgeIcons[slot] = elem->get<std::string>(slot);

                ImageComponent badgeImage{mWindow};

                badgeImage.setImage(mBadgeIcons[slot]);
                badgeImage.setVisible(false);
                mBadgeImages.push_back(std::make_pair(slot, badgeImage));
            }
            else {
                LOG(LogError) << "Invalid badge slot \"" << slot << "\" defined";
            }
        }

        GuiComponent::applyTheme(theme, view, element, properties);

        mFlexboxComponent.setPosition(mPosition);
        mFlexboxComponent.setSize(mSize);
        mFlexboxComponent.setOrigin(mOrigin);
        mFlexboxComponent.setRotation(mRotation);
        mFlexboxComponent.setRotationOrigin(mRotationOrigin);
        mFlexboxComponent.setVisible(mVisible);
        mFlexboxComponent.setDefaultZIndex(mDefaultZIndex);
        mFlexboxComponent.setZIndex(mZIndex);
    }
}

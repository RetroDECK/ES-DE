//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BadgesComponent.cpp
//
//  Game badges icons.
//  Used by gamelist views.
//

#define SLOT_FAVORITE "favorite"
#define SLOT_COMPLETED "completed"
#define SLOT_KIDGAME "kidgame"
#define SLOT_BROKEN "broken"
#define SLOT_ALTERNATIVE_EMULATOR "altemulator"

#include "components/BadgesComponent.h"

#include "ThemeData.h"
#include "utils/StringUtil.h"

BadgesComponent::BadgesComponent(Window* window)
    : FlexboxComponent{window, mBadgeImages}
    , mBadgeTypes{
          {SLOT_FAVORITE, SLOT_COMPLETED, SLOT_KIDGAME, SLOT_BROKEN, SLOT_ALTERNATIVE_EMULATOR}}
{
    mBadgeIcons[SLOT_FAVORITE] = ":/graphics/badge_favorite.svg";
    mBadgeIcons[SLOT_COMPLETED] = ":/graphics/badge_completed.svg";
    mBadgeIcons[SLOT_KIDGAME] = ":/graphics/badge_kidgame.svg";
    mBadgeIcons[SLOT_BROKEN] = ":/graphics/badge_broken.svg";
    mBadgeIcons[SLOT_ALTERNATIVE_EMULATOR] = ":/graphics/badge_altemulator.svg";
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
            onSizeChanged();
            break;
        }
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

        // Apply theme on the flexbox component parent.
        FlexboxComponent::applyTheme(theme, view, element, properties);
    }
}

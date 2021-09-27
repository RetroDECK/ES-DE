//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BadgesComponent.cpp
//
//  Game badges icons.
//  Used by gamelist views.
//

#include "components/BadgesComponent.h"

#include "Settings.h"
#include "ThemeData.h"
#include "resources/TextureResource.h"

// Available slot definitions.
std::vector<std::string> BadgesComponent::mSlots = {SLOT_FAVORITE, SLOT_COMPLETED, SLOT_KIDS,
                                                    SLOT_BROKEN, SLOT_ALTERNATIVE_EMULATOR};

BadgesComponent::BadgesComponent(Window* window)
    : FlexboxComponent(window)
{
    mBadgeIcons[SLOT_FAVORITE] = ":/graphics/badge_favorite.svg";
    mBadgeIcons[SLOT_COMPLETED] = ":/graphics/badge_completed.svg";
    mBadgeIcons[SLOT_KIDS] = ":/graphics/badge_kidgame.svg";
    mBadgeIcons[SLOT_BROKEN] = ":/graphics/badge_broken.svg";
    mBadgeIcons[SLOT_ALTERNATIVE_EMULATOR] = ":/graphics/badge_altemulator.svg";

    ImageComponent mImageFavorite = ImageComponent(window);
    mImageComponents.insert({SLOT_FAVORITE, mImageFavorite});
    ImageComponent mImageCompleted = ImageComponent(window);
    mImageComponents.insert({SLOT_COMPLETED, mImageCompleted});
    ImageComponent mImageKids = ImageComponent(window);
    mImageComponents.insert({SLOT_KIDS, mImageKids});
    ImageComponent mImageBroken = ImageComponent(window);
    mImageComponents.insert({SLOT_BROKEN, mImageBroken});
    ImageComponent mImageAltEmulator = ImageComponent(window);
    mImageComponents.insert({SLOT_ALTERNATIVE_EMULATOR, mImageAltEmulator});
}

BadgesComponent::~BadgesComponent()
{
    for (GuiComponent* c : mChildren)
        c->clearChildren();
    clearChildren();
    mBadgeIcons.clear();
    mImageComponents.clear();
}

void BadgesComponent::setValue(const std::string& value)
{
    mChildren.clear();
    if (!value.empty()) {
        std::string temp;
        std::istringstream ss(value);
        while (std::getline(ss, temp, ' ')) {
            if (!(temp == SLOT_FAVORITE || temp == SLOT_COMPLETED || temp == SLOT_KIDS ||
                  temp == SLOT_BROKEN || temp == SLOT_ALTERNATIVE_EMULATOR))
                LOG(LogError) << "Badge slot '" << temp << "' is invalid.";
            else if (std::find(mSlots.begin(), mSlots.end(), temp) != mSlots.end())
                mChildren.push_back(&mImageComponents.find(temp)->second);
        }
    }

    onSizeChanged();
}

std::string BadgesComponent::getValue() const
{
    std::stringstream ss;
    for (auto& slot : mSlots)
        ss << slot << ' ';
    std::string r = ss.str();
    r.pop_back();
    return r;
}

void BadgesComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                 const std::string& view,
                                 const std::string& element,
                                 unsigned int properties)
{
    using namespace ThemeFlags;

    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "badges");
    if (!elem)
        return;

    for (auto& slot : mSlots) {
        if (properties & PATH && elem->has(slot)) {
            mBadgeIcons[slot] = elem->get<std::string>(slot);
            mImageComponents.find(slot)->second.setImage(mBadgeIcons[slot]);
        }
        else {
            mImageComponents.find(slot)->second.setImage(mBadgeIcons[slot]);
            std::string teststring;
        }
    }

    if (elem->has("slots")) {
        auto value = elem->get<std::string>("slots");
        mSlots = {};
        if (!value.empty()) {
            std::string temp;
            std::istringstream ss(value);
            while (std::getline(ss, temp, ' ')) {
                if (!(temp == SLOT_FAVORITE || temp == SLOT_COMPLETED || temp == SLOT_KIDS ||
                      temp == SLOT_BROKEN || temp == SLOT_ALTERNATIVE_EMULATOR))
                    LOG(LogError) << "Badge slot '" << temp << "' is invalid.";
                else
                    mSlots.push_back(temp);
            }
        }
    }

    // Apply theme on the flexbox component parent.
    FlexboxComponent::applyTheme(theme, view, element, properties);

    onSizeChanged();
}

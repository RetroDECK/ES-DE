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

BadgesComponent::BadgesComponent(Window* window)
    : FlexboxComponent(window, NUM_SLOTS)
{
    // Define the slots.
    setSlots({SLOT_FAVORITE, SLOT_COMPLETED, SLOT_KIDS, SLOT_BROKEN});

    mBadgeIcons = std::map<std::string, std::string>();
    mBadgeIcons[SLOT_FAVORITE] = ":/graphics/badge_favorite.png";
    mBadgeIcons[SLOT_COMPLETED] = ":/graphics/badge_completed.png";
    mBadgeIcons[SLOT_KIDS] = ":/graphics/badge_kidgame.png";
    mBadgeIcons[SLOT_BROKEN] = ":/graphics/badge_broken.png";

    // Create the child ImageComponent for every badge.
    mImageComponents = std::map<std::string, ImageComponent>();
    ImageComponent mImageFavorite = ImageComponent(window);
    mImageFavorite.setImage(mBadgeIcons[SLOT_FAVORITE], false, false);
    mImageComponents.insert({SLOT_FAVORITE, mImageFavorite});
    ImageComponent mImageCompleted = ImageComponent(window);
    mImageCompleted.setImage(mBadgeIcons[SLOT_COMPLETED], false, false);
    mImageComponents.insert({SLOT_COMPLETED, mImageCompleted});
    ImageComponent mImageKids = ImageComponent(window);
    mImageKids.setImage(mBadgeIcons[SLOT_KIDS], false, false);
    mImageComponents.insert({SLOT_KIDS, mImageKids});
    ImageComponent mImageBroken = ImageComponent(window);
    mImageBroken.setImage(mBadgeIcons[SLOT_BROKEN], false, false);
    mImageComponents.insert({SLOT_BROKEN, mImageBroken});

    // TODO: Should be dependent on the direction property.
    mSize = glm::vec2{64.0f * NUM_SLOTS, 64.0f};

    // Trigger initial layout computation.
    onSizeChanged();
}

void BadgesComponent::setValue(const std::string& value)
{
    std::vector<std::string> slots = {};

    if (!value.empty()) {
        // Interpret the value and iteratively fill slots. The value is a space separated list of
        // strings.
        std::string temp;
        std::istringstream ss(value);
        while (std::getline(ss, temp, ' ')) {
            if (!(temp == SLOT_FAVORITE || temp == SLOT_COMPLETED || temp == SLOT_KIDS ||
                  temp == SLOT_BROKEN))
                LOG(LogError) << "Badge slot '" << temp << "' is invalid.";
            else
                slots.push_back(temp);
        }
    }

    setSlots(slots);
    onSizeChanged();
}

std::string BadgesComponent::getValue() const
{
    const std::vector<std::string> slots = getSlots();
    std::stringstream ss;
    for (auto& slot : slots)
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

    bool imgChanged = false;
    const std::vector<std::string> slots = getSlots();
    for (auto& slot : slots) {
        if (properties & PATH && elem->has(slot)) {
            mBadgeIcons[slot] = elem->get<std::string>(slot);
            mImageComponents.find(slot)->second.setImage(mBadgeIcons[slot]);
            imgChanged = true;
        }
    }

    if (elem->has("slots"))
        setValue(elem->get<std::string>("slots"));

    // Apply theme on the flexbox component parent.
    FlexboxComponent::applyTheme(theme, view, element, properties);

    if (imgChanged)
        onSizeChanged();
}

std::vector<HelpPrompt> BadgesComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    return prompts;
}

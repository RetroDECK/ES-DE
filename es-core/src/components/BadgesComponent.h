//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BadgesComponent.h
//
//  Game badges icons.
//  Used by gamelist views.
//

#ifndef ES_CORE_COMPONENTS_BADGES_COMPONENT_H
#define ES_CORE_COMPONENTS_BADGES_COMPONENT_H

#include "FlexboxComponent.h"

class BadgesComponent : public FlexboxComponent
{
public:
    BadgesComponent(Window* window);

    std::vector<std::string> getBadgeTypes() { return mBadgeTypes; }
    void setBadges(const std::vector<std::string>& badges);

    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties) override;

private:
    std::vector<std::string> mBadgeTypes;
    std::map<std::string, std::string> mBadgeIcons;
    std::vector<std::pair<std::string, ImageComponent>> mBadgeImages;
};

#endif // ES_CORE_COMPONENTS_BADGES_COMPONENT_H

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BadgesComponent.h
//
//  Game badges icons.
//  Used by the gamelist views.
//

#ifndef ES_CORE_COMPONENTS_BADGES_COMPONENT_H
#define ES_CORE_COMPONENTS_BADGES_COMPONENT_H

#include "FlexboxComponent.h"
#include "GuiComponent.h"

class BadgesComponent : public GuiComponent
{
public:
    BadgesComponent(Window* window);

    std::vector<std::string> getBadgeTypes() { return mBadgeTypes; }
    void setBadges(const std::vector<std::string>& badges);

    void render(const glm::mat4& parentTrans) override;
    void onSizeChanged() override { mFlexboxComponent.onSizeChanged(); }

    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties) override;

private:
    FlexboxComponent mFlexboxComponent;

    std::vector<std::string> mBadgeTypes;
    std::map<std::string, std::string> mBadgeIcons;
    std::vector<std::pair<std::string, ImageComponent>> mBadgeImages;
};

#endif // ES_CORE_COMPONENTS_BADGES_COMPONENT_H

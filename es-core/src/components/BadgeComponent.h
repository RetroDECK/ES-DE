//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BadgeComponent.h
//
//  Game badges icons.
//  Used by the gamelist views.
//

#ifndef ES_CORE_COMPONENTS_BADGE_COMPONENT_H
#define ES_CORE_COMPONENTS_BADGE_COMPONENT_H

#include "FlexboxComponent.h"
#include "GuiComponent.h"

struct GameControllers {
    std::string shortName;
    std::string displayName;
    std::string fileName;
};

class BadgeComponent : public GuiComponent
{
public:
    BadgeComponent();

    struct BadgeInfo {
        std::string badgeType;
        std::string gameController;
        bool folderLink {false};
    };

    static void populateGameControllers();
    const std::vector<std::string>& getBadgeTypes() const { return mBadgeTypes; }
    void setBadges(const std::vector<BadgeInfo>& badges);
    static const std::vector<GameControllers>& getGameControllers()
    {
        if (sGameControllers.empty())
            populateGameControllers();
        return sGameControllers;
    }

    static const std::string getShortName(const std::string& displayName);
    static const std::string getDisplayName(const std::string& shortName);

    void render(const glm::mat4& parentTrans) override;
    void onSizeChanged() override { mFlexboxComponent.onSizeChanged(); }

    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties) override;

private:
    static inline std::vector<GameControllers> sGameControllers;

    // Used to keep the overlay texture cache entries from expiring.
    std::map<std::string, std::unique_ptr<ImageComponent>> mOverlayMap;

    std::vector<FlexboxComponent::FlexboxItem> mFlexboxItems;
    FlexboxComponent mFlexboxComponent;

    std::vector<std::string> mBadgeTypes;
    std::map<std::string, std::string> mBadgeIcons;
};

#endif // ES_CORE_COMPONENTS_BADGE_COMPONENT_H

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SystemView.h
//
//  Main system view.
//

#ifndef ES_APP_VIEWS_SYSTEM_VIEW_H
#define ES_APP_VIEWS_SYSTEM_VIEW_H

#include "GuiComponent.h"
#include "Sound.h"
#include "components/IList.h"
#include "components/TextComponent.h"
#include "resources/Font.h"

#include <memory>

class AnimatedImageComponent;
class SystemData;

enum CarouselType : unsigned int {
    HORIZONTAL = 0,
    VERTICAL = 1,
    VERTICAL_WHEEL = 2,
    HORIZONTAL_WHEEL = 3
};

struct SystemViewData {
    std::shared_ptr<GuiComponent> logo;
    std::vector<GuiComponent*> backgroundExtras;
};

struct SystemViewCarousel {
    CarouselType type;
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec2 origin;
    float logoScale;
    float logoRotation;
    glm::vec2 logoRotationOrigin;
    Alignment logoAlignment;
    unsigned int color;
    unsigned int colorEnd;
    bool colorGradientHorizontal;
    int maxLogoCount; // Number of logos shown on the carousel.
    glm::vec2 logoSize;
    float zIndex;
    bool legacyZIndexMode;
};

class SystemView : public IList<SystemViewData, SystemData*>
{
public:
    SystemView(Window* window);
    ~SystemView();

    virtual void onShow() override { mShowing = true; }
    virtual void onHide() override { mShowing = false; }

    void goToSystem(SystemData* system, bool animate);

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    void onThemeChanged(const std::shared_ptr<ThemeData>& theme);

    std::vector<HelpPrompt> getHelpPrompts() override;
    virtual HelpStyle getHelpStyle() override;

    CarouselType getCarouselType() { return mCarousel.type; }

protected:
    void onCursorChanged(const CursorState& state) override;
    virtual void onScroll() override
    {
        NavigationSounds::getInstance()->playThemeNavigationSound(SYSTEMBROWSESOUND);
    }

private:
    void populate();
    void updateGameCount();
    //  Get the ThemeElements that make up the SystemView.
    void getViewElements(const std::shared_ptr<ThemeData>& theme);
    // Populate the system carousel with the legacy values.
    void getDefaultElements(void);
    void getCarouselFromTheme(const ThemeData::ThemeElement* elem);

    //  Render system carousel.
    void renderCarousel(const glm::mat4& parentTrans);
    // Draw background extras.
    void renderExtras(const glm::mat4& parentTrans, float lower, float upper);
    void renderFade(const glm::mat4& trans);

    SystemViewCarousel mCarousel;
    TextComponent mSystemInfo;

    // Unit is list index.
    float mCamOffset;
    float mExtrasCamOffset;
    float mExtrasFadeOpacity;

    int mPreviousScrollVelocity;
    bool mUpdatedGameCount;
    bool mViewNeedsReload;
    bool mShowing;
};

#endif // ES_APP_VIEWS_SYSTEM_VIEW_H

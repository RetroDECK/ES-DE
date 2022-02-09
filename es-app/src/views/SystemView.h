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
#include "SystemData.h"
#include "components/CarouselComponent.h"
#include "components/DateTimeComponent.h"
#include "components/LottieComponent.h"
#include "components/ScrollableContainer.h"
#include "components/TextComponent.h"
#include "components/TextListComponent.h"
#include "components/VideoFFmpegComponent.h"
#include "resources/Font.h"

#include <memory>

class SystemData;

struct SystemViewElements {
    std::string name;
    std::string fullName;
    std::vector<GuiComponent*> legacyExtras;
    std::vector<GuiComponent*> children;

    std::vector<std::unique_ptr<TextComponent>> gameCountComponents;
    std::vector<std::unique_ptr<TextComponent>> textComponents;
    std::vector<std::unique_ptr<ImageComponent>> imageComponents;
    std::vector<std::unique_ptr<VideoFFmpegComponent>> videoComponents;
    std::vector<std::unique_ptr<LottieComponent>> lottieAnimComponents;
    std::vector<std::unique_ptr<ScrollableContainer>> containerComponents;
    std::vector<std::unique_ptr<TextComponent>> containerTextComponents;
};

class SystemView : public GuiComponent
{
public:
    SystemView();
    ~SystemView();

    void goToSystem(SystemData* system, bool animate);

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    bool isScrolling() { return mCarousel->isScrolling(); }
    void stopScrolling() { mCarousel->stopScrolling(); }
    bool isSystemAnimationPlaying(unsigned char slot)
    {
        return mCarousel->isAnimationPlaying(slot);
    }
    void finishSystemAnimation(unsigned char slot)
    {
        finishAnimation(slot);
        mCarousel->finishAnimation(slot);
    }

    CarouselComponent::CarouselType getCarouselType() { return mCarousel->getType(); }
    SystemData* getFirstSystem() { return mCarousel->getFirst(); }

    void onThemeChanged(const std::shared_ptr<ThemeData>& theme);

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override;

protected:
    void onCursorChanged(const CursorState& state);

private:
    void populate();
    void updateGameCount();
    void legacyApplyTheme(const std::shared_ptr<ThemeData>& theme);
    void renderElements(const glm::mat4& parentTrans, bool abovePrimary);

    std::unique_ptr<CarouselComponent> mCarousel;
    std::unique_ptr<TextComponent> mLegacySystemInfo;
    std::vector<SystemViewElements> mSystemElements;

    float mCamOffset;
    float mFadeOpacity;
    int mPreviousScrollVelocity;

    bool mUpdatedGameCount;
    bool mViewNeedsReload;
    bool mLegacyMode;
};

#endif // ES_APP_VIEWS_SYSTEM_VIEW_H

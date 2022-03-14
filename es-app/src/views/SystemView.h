//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SystemView.h
//
//  Main system view.
//

#ifndef ES_APP_VIEWS_SYSTEM_VIEW_H
#define ES_APP_VIEWS_SYSTEM_VIEW_H

#include "FileData.h"
#include "GuiComponent.h"
#include "Sound.h"
#include "SystemData.h"
#include "components/CarouselComponent.h"
#include "components/DateTimeComponent.h"
#include "components/GIFAnimComponent.h"
#include "components/GameSelectorComponent.h"
#include "components/LottieAnimComponent.h"
#include "components/TextComponent.h"
#include "components/TextListComponent.h"
#include "components/VideoFFmpegComponent.h"
#include "resources/Font.h"

#include <memory>

class SystemData;

struct SystemViewElements {
    std::string name;
    std::string fullName;
    std::vector<std::unique_ptr<GameSelectorComponent>> gameSelectors;
    std::vector<GuiComponent*> legacyExtras;
    std::vector<GuiComponent*> children;

    std::vector<std::unique_ptr<TextComponent>> gameCountComponents;
    std::vector<std::unique_ptr<TextComponent>> textComponents;
    std::vector<std::unique_ptr<DateTimeComponent>> dateTimeComponents;
    std::vector<std::unique_ptr<ImageComponent>> imageComponents;
    std::vector<std::unique_ptr<VideoFFmpegComponent>> videoComponents;
    std::vector<std::unique_ptr<LottieAnimComponent>> lottieAnimComponents;
    std::vector<std::unique_ptr<GIFAnimComponent>> GIFAnimComponents;
};

class SystemView : public GuiComponent
{
public:
    SystemView();
    ~SystemView();

    void onTransition() override;
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

    void startViewVideos() override
    {
        for (auto& video : mSystemElements[mCarousel->getCursor()].videoComponents)
            video->startVideoPlayer();
    }
    void stopViewVideos() override
    {
        for (auto& video : mSystemElements[mCarousel->getCursor()].videoComponents)
            video->stopVideoPlayer();
    }
    void pauseViewVideos() override
    {
        for (auto& video : mSystemElements[mCarousel->getCursor()].videoComponents) {
            video->pauseVideoPlayer();
        }
    }
    void muteViewVideos() override
    {
        for (auto& video : mSystemElements[mCarousel->getCursor()].videoComponents)
            video->muteVideoPlayer();
    }

    void onThemeChanged(const std::shared_ptr<ThemeData>& theme);

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override;

protected:
    void onCursorChanged(const CursorState& state);

private:
    void populate();
    void updateGameCount();
    void updateGameSelectors();
    void legacyApplyTheme(const std::shared_ptr<ThemeData>& theme);
    void renderElements(const glm::mat4& parentTrans, bool abovePrimary);

    Renderer* mRenderer;
    std::unique_ptr<CarouselComponent> mCarousel;
    std::unique_ptr<TextComponent> mLegacySystemInfo;
    std::vector<SystemViewElements> mSystemElements;

    float mCamOffset;
    float mFadeOpacity;
    int mPreviousScrollVelocity;

    bool mUpdatedGameCount;
    bool mViewNeedsReload;
    bool mLegacyMode;
    bool mNavigated;
    bool mMaxFade;
    bool mFadeTransitions;
};

#endif // ES_APP_VIEWS_SYSTEM_VIEW_H

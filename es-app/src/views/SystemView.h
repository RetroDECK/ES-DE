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
#include "components/DateTimeComponent.h"
#include "components/GIFAnimComponent.h"
#include "components/GameSelectorComponent.h"
#include "components/LottieAnimComponent.h"
#include "components/RatingComponent.h"
#include "components/TextComponent.h"
#include "components/VideoFFmpegComponent.h"
#include "components/primary/CarouselComponent.h"
#include "components/primary/GridComponent.h"
#include "components/primary/TextListComponent.h"
#include "resources/Font.h"

#include <memory>

class SystemData;

class SystemView : public GuiComponent
{
public:
    using PrimaryType = PrimaryComponent<SystemData*>::PrimaryType;

    SystemView();
    ~SystemView();

    void onShow() override;
    void onTransition() override;
    void goToSystem(SystemData* system, bool animate);

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    bool isScrolling() { return mPrimary->isScrolling(); }
    void stopScrolling()
    {
        mPrimary->stopScrolling();
        mCamOffset = static_cast<float>(mPrimary->getCursor());
    }
    bool isSystemAnimationPlaying(unsigned char slot) { return mPrimary->isAnimationPlaying(slot); }
    void finishSystemAnimation(unsigned char slot)
    {
        if (mFadeTransitions && mTransitionAnim) {
            mPrimary->finishAnimation(slot);
            mTransitionAnim = false;
        }
        else {
            finishAnimation(slot);
            mPrimary->finishAnimation(slot);
            mMaxFade = false;
        }
    }

    PrimaryComponent<SystemData*>::PrimaryType getPrimaryType() { return mPrimaryType; }
    CarouselComponent<SystemData*>::CarouselType getCarouselType()
    {
        return (mCarousel != nullptr) ? mCarousel->getType() :
                                        CarouselComponent<SystemData*>::CarouselType::NO_CAROUSEL;
    }
    SystemData* getFirstSystem() { return mPrimary->getFirst(); }

    void startViewVideos() override
    {
        for (auto& video : mSystemElements[mPrimary->getCursor()].videoComponents)
            video->startVideoPlayer();
    }
    void stopViewVideos() override
    {
        for (auto& video : mSystemElements[mPrimary->getCursor()].videoComponents)
            video->stopVideoPlayer();
    }
    void pauseViewVideos() override
    {
        for (auto& video : mSystemElements[mPrimary->getCursor()].videoComponents) {
            video->pauseVideoPlayer();
        }
    }
    void muteViewVideos() override
    {
        for (auto& video : mSystemElements[mPrimary->getCursor()].videoComponents)
            video->muteVideoPlayer();
    }

    void onThemeChanged(const std::shared_ptr<ThemeData>& theme);

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return mSystemElements[mPrimary->getCursor()].helpStyle; }

protected:
    void onCursorChanged(const CursorState& state);

private:
    void populate();
    void updateGameCount(SystemData* system = nullptr);
    void updateGameSelectors();
    void legacyApplyTheme(const std::shared_ptr<ThemeData>& theme);
    void renderElements(const glm::mat4& parentTrans, bool abovePrimary);

    struct SystemViewElements {
        SystemData* system;
        HelpStyle helpStyle;
        std::string name;
        std::string fullName;
        std::vector<std::unique_ptr<GameSelectorComponent>> gameSelectors;
        std::vector<GuiComponent*> legacyExtras;
        std::vector<GuiComponent*> children;

        std::vector<std::unique_ptr<ImageComponent>> imageComponents;
        std::vector<std::unique_ptr<VideoFFmpegComponent>> videoComponents;
        std::vector<std::unique_ptr<LottieAnimComponent>> lottieAnimComponents;
        std::vector<std::unique_ptr<GIFAnimComponent>> GIFAnimComponents;
        std::vector<std::unique_ptr<TextComponent>> gameCountComponents;
        std::vector<std::unique_ptr<TextComponent>> textComponents;
        std::vector<std::unique_ptr<DateTimeComponent>> dateTimeComponents;
        std::vector<std::unique_ptr<RatingComponent>> ratingComponents;
    };

    Renderer* mRenderer;
    std::unique_ptr<CarouselComponent<SystemData*>> mCarousel;
    std::unique_ptr<GridComponent<SystemData*>> mGrid;
    std::unique_ptr<TextListComponent<SystemData*>> mTextList;
    std::unique_ptr<TextComponent> mLegacySystemInfo;
    std::vector<SystemViewElements> mSystemElements;
    PrimaryComponent<SystemData*>* mPrimary;
    PrimaryType mPrimaryType;
    int mLastCursor;

    // Dummy entry to keep the default SVG rating images in the texture cache so they don't
    // need to be re-rasterized for each gamelist that is loaded.
    RatingComponent mRatingDummy;

    float mCamOffset;
    float mFadeOpacity;
    int mPreviousScrollVelocity;

    bool mUpdatedGameCount;
    bool mViewNeedsReload;
    bool mLegacyMode;
    bool mNavigated;
    bool mMaxFade;
    bool mFadeTransitions;
    bool mTransitionAnim;
};

#endif // ES_APP_VIEWS_SYSTEM_VIEW_H

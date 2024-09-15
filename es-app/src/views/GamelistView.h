//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GamelistView.h
//
//  Main gamelist logic.
//

#ifndef ES_APP_VIEWS_GAMELIST_VIEW_H
#define ES_APP_VIEWS_GAMELIST_VIEW_H

#include "views/GamelistBase.h"
#include "views/ViewController.h"

class GamelistView : public GamelistBase
{
public:
    GamelistView(FileData* root);
    ~GamelistView();

    const std::pair<bool, LetterCase> getDescriptionSystemNameSuffix() const;

    // Called when a FileData* is added, has its metadata changed, or is removed.
    void onFileChanged(FileData* file, bool reloadGamelist) override;
    void onShow() override;
    void onHide() override;
    void onTransition() override;

    void preloadGamelist() { updateView(CursorState::CURSOR_STOPPED); }
    void launch(FileData* game) override { ViewController::getInstance()->triggerGameLaunch(game); }

    void startViewVideos() override
    {
        for (auto& video : mVideoComponents)
            video->startVideoPlayer();
        for (auto& video : mStaticVideoComponents)
            video->startVideoPlayer();
    }
    void stopViewVideos() override
    {
        for (auto& video : mVideoComponents)
            video->stopVideoPlayer();
        for (auto& video : mStaticVideoComponents)
            video->stopVideoPlayer();
    }
    void pauseViewVideos() override
    {
        for (auto& video : mVideoComponents)
            video->pauseVideoPlayer();
        for (auto& video : mStaticVideoComponents)
            video->pauseVideoPlayer();
    }
    void muteViewVideos() override
    {
        for (auto& video : mVideoComponents)
            video->muteVideoPlayer();
        for (auto& video : mStaticVideoComponents)
            video->muteVideoPlayer();
    }
    void resetViewVideosTimer() override
    {
        for (auto& video : mVideoComponents)
            video->resetVideoPlayerTimer();
        for (auto& video : mStaticVideoComponents)
            video->resetVideoPlayerTimer();
    }

    void stopGamelistFadeAnimations() override
    {
        for (auto& comp : mTextComponents)
            comp->finishAnimation(0);
        for (auto& comp : mDateTimeComponents)
            comp->finishAnimation(0);
        for (auto& comp : mImageComponents)
            comp->finishAnimation(0);
        for (auto& comp : mStaticVideoComponents)
            comp->finishAnimation(0);
        for (auto& comp : mVideoComponents)
            comp->finishAnimation(0);
        for (auto& comp : mLottieAnimComponents)
            comp->finishAnimation(0);
        for (auto& comp : mGIFAnimComponents)
            comp->finishAnimation(0);
        for (auto& comp : mBadgeComponents)
            comp->finishAnimation(0);
        for (auto& comp : mRatingComponents)
            comp->finishAnimation(0);
        for (auto& comp : mContainerComponents)
            comp->finishAnimation(0);
        for (auto& comp : mContainerTextComponents)
            comp->finishAnimation(0);
        for (auto& comp : mGamelistInfoComponents)
            comp->finishAnimation(0);
    }

    const std::shared_ptr<ThemeData> getTheme() const { return mTheme; }
    void setTheme(const std::shared_ptr<ThemeData>& theme)
    {
        mTheme = theme;
        onThemeChanged(theme);
    }
    void onThemeChanged(const std::shared_ptr<ThemeData>& theme);

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    HelpStyle getHelpStyle() override { return mHelpStyle; }
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void updateView(const CursorState& state);
    void setGameImage(FileData* file, GuiComponent* comp);

    Renderer* mRenderer;
    HelpStyle mHelpStyle;
    bool mStaticVideoAudio;

    std::shared_ptr<ThemeData> mTheme;
    std::vector<GuiComponent*> mThemeExtras;

    std::vector<std::unique_ptr<TextComponent>> mTextComponents;
    std::vector<std::unique_ptr<DateTimeComponent>> mDateTimeComponents;
    std::vector<std::unique_ptr<ImageComponent>> mImageComponents;
    std::vector<std::unique_ptr<VideoFFmpegComponent>> mStaticVideoComponents;
    std::vector<std::unique_ptr<VideoFFmpegComponent>> mVideoComponents;
    std::vector<std::unique_ptr<LottieAnimComponent>> mLottieAnimComponents;
    std::vector<std::unique_ptr<GIFAnimComponent>> mGIFAnimComponents;
    std::vector<std::unique_ptr<BadgeComponent>> mBadgeComponents;
    std::vector<std::unique_ptr<RatingComponent>> mRatingComponents;
    std::vector<std::unique_ptr<ScrollableContainer>> mContainerComponents;
    std::vector<std::unique_ptr<TextComponent>> mContainerTextComponents;
    std::vector<std::unique_ptr<TextComponent>> mGamelistInfoComponents;
};

#endif // ES_APP_VIEWS_GAMELIST_VIEW_H

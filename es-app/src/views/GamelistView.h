//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
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
    void onTransition() override;

    void preloadGamelist() { updateView(CursorState::CURSOR_STOPPED); }
    void launch(FileData* game) override { ViewController::getInstance()->triggerGameLaunch(game); }

    std::string getName() const
    {
        auto selectedViewStyle = ViewController::getInstance()->getState();
        switch (selectedViewStyle.viewstyle) {
            case ViewController::VIDEO:
                return "video";
            case ViewController::DETAILED:
                return "detailed";
            case ViewController::BASIC:
            default:
                return "basic";
        }
    }

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

    // Legacy (backward compatibility) functions.
    void legacyPopulateFields();
    void legacyOnThemeChanged(const std::shared_ptr<ThemeData>& theme);
    void legacyUpdateView(const CursorState& state);
    void legacyUpdate(int deltaTime);
    void legacyInitMDLabels();
    void legacyInitMDValues();

    Renderer* mRenderer;
    HelpStyle mHelpStyle;
    ViewController::GamelistViewStyle mViewStyle;
    bool mLegacyMode;
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

    enum LegacyText {
        LOGOTEXT = 0,
        MD_LBL_RATING = 1,
        MD_LBL_RELEASEDATE = 2,
        MD_LBL_DEVELOPER = 3,
        MD_LBL_PUBLISHER = 4,
        MD_LBL_GENRE = 5,
        MD_LBL_PLAYERS = 6,
        MD_LBL_LASTPLAYED = 7,
        MD_LBL_PLAYCOUNT = 8,
        MD_DEVELOPER = 9,
        MD_PUBLISHER = 10,
        MD_GENRE = 11,
        MD_PLAYERS = 12,
        MD_PLAYCOUNT = 13,
        MD_NAME = 14,
        MD_DESCRIPTION = 15,
        END
    };

    enum LegacyDateTime {
        MD_RELEASEDATE = 0, //
        MD_LASTPLAYED = 1
    };

    enum LegacyImage {
        LOGO = 0, //
        BACKGROUND = 1,
        MD_THUMBNAIL = 2,
        MD_MARQUEE = 3,
        MD_IMAGE = 4
    };
};

#endif // ES_APP_VIEWS_GAMELIST_VIEW_H

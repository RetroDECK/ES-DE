//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoGamelistView.h
//
//  Interface that defines a GamelistView of the type 'video'.
//

#ifndef ES_APP_VIEWS_GAMELIST_VIDEO_GAMELIST_VIEW_H
#define ES_APP_VIEWS_GAMELIST_VIDEO_GAMELIST_VIEW_H

#include "components/BadgeComponent.h"
#include "components/DateTimeComponent.h"
#include "components/RatingComponent.h"
#include "components/ScrollableContainer.h"
#include "views/gamelist/BasicGamelistView.h"

class VideoComponent;

class VideoGamelistView : public BasicGamelistView
{
public:
    VideoGamelistView(Window* window, FileData* root);
    virtual ~VideoGamelistView();

    virtual void onShow() override;
    virtual void onThemeChanged(const std::shared_ptr<ThemeData>& theme) override;
    virtual std::string getName() const override { return "video"; }
    virtual void launch(FileData* game) override;

    virtual void preloadGamelist() override { updateInfoPanel(); }

protected:
    virtual void update(int deltaTime) override;

private:
    void updateInfoPanel();

    void initMDLabels();
    void initMDValues();

    ImageComponent mThumbnail;
    ImageComponent mMarquee;
    ImageComponent mImage;
    VideoComponent* mVideo;

    TextComponent mLblRating;
    TextComponent mLblReleaseDate;
    TextComponent mLblDeveloper;
    TextComponent mLblPublisher;
    TextComponent mLblGenre;
    TextComponent mLblPlayers;
    TextComponent mLblLastPlayed;
    TextComponent mLblPlayCount;

    RatingComponent mRating;
    DateTimeComponent mReleaseDate;
    TextComponent mDeveloper;
    TextComponent mPublisher;
    TextComponent mGenre;
    TextComponent mPlayers;
    DateTimeComponent mLastPlayed;
    TextComponent mPlayCount;
    TextComponent mName;
    BadgeComponent mBadges;

    std::vector<TextComponent*> getMDLabels();
    std::vector<GuiComponent*> getMDValues();

    ScrollableContainer mDescContainer;
    TextComponent mDescription;
    TextComponent mGamelistInfo;

    bool mVideoPlaying;
    FileData* mLastUpdated;
};

#endif // ES_APP_VIEWS_GAMELIST_VIDEO_GAMELIST_VIEW_H

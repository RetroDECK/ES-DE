//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  DetailedGamelistView.h
//
//  Interface that defines a GamelistView of the type 'detailed'.
//

#ifndef ES_APP_VIEWS_GAMELIST_DETAILED_GAMELIST_VIEW_H
#define ES_APP_VIEWS_GAMELIST_DETAILED_GAMELIST_VIEW_H

#include "components/BadgeComponent.h"
#include "components/DateTimeComponent.h"
#include "components/RatingComponent.h"
#include "components/ScrollableContainer.h"
#include "views/gamelist/BasicGamelistView.h"

class DetailedGamelistView : public BasicGamelistView
{
public:
    DetailedGamelistView(Window* window, FileData* root);

    void onShow() override;
    void onThemeChanged(const std::shared_ptr<ThemeData>& theme) override;
    std::string getName() const override { return "detailed"; }
    void launch(FileData* game) override;

    void preloadGamelist() override { updateInfoPanel(); }

protected:
    void update(int deltaTime) override;

private:
    void updateInfoPanel();

    void initMDLabels();
    void initMDValues();

    ImageComponent mThumbnail;
    ImageComponent mMarquee;
    ImageComponent mImage;

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

    FileData* mLastUpdated;
};

#endif // ES_APP_VIEWS_GAMELIST_DETAILED_GAMELIST_VIEW_H

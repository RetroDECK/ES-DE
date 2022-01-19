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

#include "renderers/Renderer.h"
#include "views/ViewController.h"

class GamelistView : public GamelistBase
{
public:
    GamelistView(FileData* root);
    ~GamelistView();

    // Called when a FileData* is added, has its metadata changed, or is removed.
    void onFileChanged(FileData* file, bool reloadGamelist) override;
    void onShow() override;

    void preloadGamelist() { updateInfoPanel(); }
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
                return "basic";
            default:
                return "basic";
        }
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

    HelpStyle getHelpStyle() override;
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void updateInfoPanel();

    void initMDLabels();
    void initMDValues();

    ViewController::GamelistViewStyle mViewStyle;

    std::vector<TextComponent*> getMDLabels();
    std::vector<GuiComponent*> getMDValues();

    std::shared_ptr<ThemeData> mTheme;
    std::vector<GuiComponent*> mThemeExtras;

    TextComponent mHeaderText;
    ImageComponent mHeaderImage;
    ImageComponent mBackground;

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

    ScrollableContainer mDescContainer;
    TextComponent mDescription;
    TextComponent mGamelistInfo;
};

#endif // ES_APP_VIEWS_GAMELIST_VIEW_H

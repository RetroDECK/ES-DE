//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiScraperMulti.h
//
//  Multiple game scraping user interface.
//  Shows the progress for the scraping as it's running.
//  This interface is triggered from GuiScraperMenu.
//  GuiScraperSearch is called from here.
//

#ifndef ES_APP_GUIS_GUI_SCRAPER_MULTI_H
#define ES_APP_GUIS_GUI_SCRAPER_MULTI_H

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "scrapers/Scraper.h"
#include "GuiComponent.h"
#include "MetaData.h"

class GuiScraperSearch;
class TextComponent;

class GuiScraperMulti : public GuiComponent
{
public:
    GuiScraperMulti(
            Window* window,
            const std::queue<ScraperSearchParams>& searches,
            bool approveResults);

    virtual ~GuiScraperMulti();

    void onSizeChanged() override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override;

private:
    void acceptResult(const ScraperSearchResult& result);
    void skip();
    void doNextSearch();

    void finish();
    unsigned int mTotalGames;
    unsigned int mCurrentGame;
    unsigned int mTotalSuccessful;
    unsigned int mTotalSkipped;
    std::queue<ScraperSearchParams> mSearchQueue;
    std::vector<MetaDataDecl> mMetaDataDecl;
    bool mApproveResults;

    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mSystem;
    std::shared_ptr<TextComponent> mSubtitle;
    std::shared_ptr<GuiScraperSearch> mSearchComp;
    std::shared_ptr<ComponentGrid> mButtonGrid;
};

#endif // ES_APP_GUIS_GUI_SCRAPER_MULTI_H

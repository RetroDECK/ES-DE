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

#include "GuiComponent.h"
#include "MetaData.h"
#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "scrapers/Scraper.h"

class GuiScraperSearch;
class TextComponent;

class GuiScraperMulti : public GuiComponent
{
public:
    GuiScraperMulti(Window* window,
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

    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mSystem;
    std::shared_ptr<TextComponent> mSubtitle;
    std::shared_ptr<GuiScraperSearch> mSearchComp;
    std::shared_ptr<ComponentGrid> mButtonGrid;

    std::queue<ScraperSearchParams> mSearchQueue;
    std::vector<MetaDataDecl> mMetaDataDecl;
    unsigned int mTotalGames;
    unsigned int mCurrentGame;
    unsigned int mTotalSuccessful;
    unsigned int mTotalSkipped;
    bool mApproveResults;
};

#endif // ES_APP_GUIS_GUI_SCRAPER_MULTI_H

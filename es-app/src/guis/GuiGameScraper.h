//
//	GuiGameScraper.h
//
//	Single game scraping user interface.
//	This interface is triggered from GuiMetaDataEd.
//	GuiScraperSearch is called from here.
//

#pragma once
#ifndef ES_APP_GUIS_GUI_GAME_SCRAPER_H
#define ES_APP_GUIS_GUI_GAME_SCRAPER_H

#include "components/NinePatchComponent.h"
#include "guis/GuiScraperSearch.h"
#include "GuiComponent.h"

class GuiGameScraper : public GuiComponent
{
public:
	GuiGameScraper(
			Window* window,
			ScraperSearchParams params,
			std::function<void(const ScraperSearchResult&)> doneFunc);

	void onSizeChanged() override;

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime);

	virtual std::vector<HelpPrompt> getHelpPrompts() override;
	HelpStyle getHelpStyle() override;

private:
	bool mClose;
	void close();

	ComponentGrid mGrid;
	NinePatchComponent mBox;

	std::shared_ptr<TextComponent> mGameName;
	std::shared_ptr<TextComponent> mSystemName;
	std::shared_ptr<GuiScraperSearch> mSearch;
	std::shared_ptr<ComponentGrid> mButtonGrid;

	ScraperSearchParams mSearchParams;

	std::function<void()> mCancelFunc;
};

#endif // ES_APP_GUIS_GUI_GAME_SCRAPER_H

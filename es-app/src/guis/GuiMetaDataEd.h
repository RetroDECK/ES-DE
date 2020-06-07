//
//	GuiMetaDataEd.h
//
//	Game metadata edit user interface.
//	This interface is triggered from the GuiGamelistOptions menu.
//	The scraping interface is handled by GuiGameScraper which calls
//	GuiScraperSearch.
//

#pragma once
#ifndef ES_APP_GUIS_GUI_META_DATA_ED_H
#define ES_APP_GUIS_GUI_META_DATA_ED_H

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "scrapers/Scraper.h"
#include "GuiComponent.h"
#include "MetaData.h"

class ComponentList;
class TextComponent;

class GuiMetaDataEd : public GuiComponent
{
public:
	GuiMetaDataEd(
			Window* window,
			MetaDataList* md, const std::vector<MetaDataDecl>&mdd,
			ScraperSearchParams params,
			const std::string& header,
			std::function<void()> savedCallback,
			std::function<void()> deleteFunc);

	bool input(InputConfig* config, Input input) override;
	void onSizeChanged() override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;
	HelpStyle getHelpStyle() override;

private:
	void save();
	void fetch();
	void fetchDone(const ScraperSearchResult& result);
	void close(bool closeAllWindows);

	NinePatchComponent mBackground;
	ComponentGrid mGrid;

	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextComponent> mSubtitle;
	std::shared_ptr<ComponentGrid> mHeaderGrid;
	std::shared_ptr<ComponentList> mList;
	std::shared_ptr<ComponentGrid> mButtons;

	ScraperSearchParams mScraperParams;

	std::vector< std::shared_ptr<GuiComponent> > mEditors;

	std::vector<MetaDataDecl> mMetaDataDecl;
	MetaDataList* mMetaData;
	std::function<void()> mSavedCallback;
	std::function<void()> mDeleteFunc;

	bool mMetadataUpdated;
};

#endif // ES_APP_GUIS_GUI_META_DATA_ED_H

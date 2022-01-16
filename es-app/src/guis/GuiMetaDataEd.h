//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiMetaDataEd.h
//
//  Game metadata edit user interface.
//  This interface is triggered from the GuiGamelistOptions menu.
//  The scraping interface is handled by GuiScraperSingle which calls GuiScraperSearch.
//

#ifndef ES_APP_GUIS_GUI_META_DATA_ED_H
#define ES_APP_GUIS_GUI_META_DATA_ED_H

#include "GuiComponent.h"
#include "MetaData.h"
#include "components/BadgeComponent.h"
#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "components/ScrollIndicatorComponent.h"
#include "guis/GuiSettings.h"
#include "scrapers/Scraper.h"

class ComponentList;
class TextComponent;

class GuiMetaDataEd : public GuiComponent
{
public:
    GuiMetaDataEd(Window* window,
                  MetaDataList* md,
                  const std::vector<MetaDataDecl>& mdd,
                  ScraperSearchParams params,
                  std::function<void()> savedCallback,
                  std::function<void()> clearGameFunc,
                  std::function<void()> deleteGameFunc);

    bool input(InputConfig* config, Input input) override;
    void onSizeChanged() override;

    virtual std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override;

private:
    void save();
    void fetch();
    void fetchDone(const ScraperSearchResult& result);
    void close();

    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<ImageComponent> mScrollUp;
    std::shared_ptr<ImageComponent> mScrollDown;
    std::shared_ptr<ScrollIndicatorComponent> mScrollIndicator;
    std::shared_ptr<TextComponent> mSubtitle;
    std::shared_ptr<ComponentGrid> mHeaderGrid;
    std::shared_ptr<ComponentList> mList;
    std::shared_ptr<ComponentGrid> mButtons;

    ScraperSearchParams mScraperParams;

    std::vector<GameControllers> mControllerBadges;
    std::vector<std::shared_ptr<GuiComponent>> mEditors;

    std::vector<MetaDataDecl> mMetaDataDecl;
    MetaDataList* mMetaData;
    std::function<void()> mSavedCallback;
    std::function<void()> mClearGameFunc;
    std::function<void()> mDeleteGameFunc;

    bool mIsCustomCollection;
    bool mMediaFilesUpdated;
    bool mSavedMediaAndAborted;
    bool mInvalidEmulatorEntry;
};

#endif // ES_APP_GUIS_GUI_META_DATA_ED_H

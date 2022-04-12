//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiGamelistOptions.h
//
//  Gamelist options menu for the 'Jump to...' quick selector,
//  game sorting, game filters, and metadata edit.
//
//  The filter interface is covered by GuiGamelistFilter and the
//  metadata edit interface is covered by GuiMetaDataEd.
//

#ifndef ES_APP_GUIS_GUI_GAMELIST_OPTIONS_H
#define ES_APP_GUIS_GUI_GAMELIST_OPTIONS_H

#include "FileData.h"
#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/OptionListComponent.h"
#include "utils/StringUtil.h"
#include "views/GamelistView.h"

class SystemData;

class GuiGamelistOptions : public GuiComponent
{
public:
    GuiGamelistOptions(SystemData* system);
    virtual ~GuiGamelistOptions();

    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return ViewController::getInstance()->getViewHelpStyle(); }

private:
    void openGamelistFilter();
    void openMetaDataEd();
    void startEditMode();
    void exitEditMode();

    void jumpToLetter();
    void jumpToFirstRow();

    MenuComponent mMenu;

    using LetterList = OptionListComponent<std::string>;
    std::shared_ptr<LetterList> mJumpToLetterList;

    using SortList = OptionListComponent<const FileData::SortType*>;
    std::shared_ptr<SortList> mListSort;

    SystemData* mSystem;
    GamelistView* getGamelist();
    bool mFoldersOnTop;
    bool mFavoritesSorting;
    bool mOnlyHasFolders;
    bool mFromPlaceholder;
    bool mFiltersChanged;
    bool mCancelled;
    bool mIsCustomCollection;
    bool mIsCustomCollectionGroup;
    bool mFolderLinkOverride;
    SystemData* mCustomCollectionSystem;
    std::vector<std::string> mFirstLetterIndex;
    std::string mCurrentFirstCharacter;
};

#endif // ES_APP_GUIS_GUI_GAMELIST_OPTIONS_H

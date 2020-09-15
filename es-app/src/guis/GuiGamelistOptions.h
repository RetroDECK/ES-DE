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

#ifndef ES_APP_GUIS_GUI_GAME_LIST_OPTIONS_H
#define ES_APP_GUIS_GUI_GAME_LIST_OPTIONS_H

#include "components/MenuComponent.h"
#include "components/OptionListComponent.h"
#include "utils/StringUtil.h"
#include "FileData.h"
#include "GuiComponent.h"

class IGameListView;
class SystemData;

class GuiGamelistOptions : public GuiComponent
{
public:
    GuiGamelistOptions(Window* window, SystemData* system);
    virtual ~GuiGamelistOptions();

    virtual bool input(InputConfig* config, Input input) override;
    virtual std::vector<HelpPrompt> getHelpPrompts() override;
    virtual HelpStyle getHelpStyle() override;

private:
    void openGamelistFilter();
    void openMetaDataEd();
    void startEditMode();
    void exitEditMode();

    void jumpToLetter();
    void jumpToFirstRow();

    MenuComponent mMenu;

    typedef OptionListComponent<std::string> LetterList;
    std::shared_ptr<LetterList> mJumpToLetterList;

    typedef OptionListComponent<const FileData::SortType*> SortList;
    std::shared_ptr<SortList> mListSort;

    SystemData* mSystem;
    IGameListView* getGamelist();
    bool mFoldersOnTop;
    bool mFavoritesSorting;
    bool mOnlyHasFolders;
    bool fromPlaceholder;
    bool mFiltersChanged;
    bool mCancelled;
    std::vector<std::string> mFirstLetterIndex;
    std::string mCurrentFirstCharacter;
    const std::string FAVORITE_CHAR = "\uF005";

};

#endif // ES_APP_GUIS_GUI_GAME_LIST_OPTIONS_H

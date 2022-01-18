//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BasicGamelistView.h
//
//  Interface that defines a GamelistView of the type 'basic'.
//

#ifndef ES_APP_VIEWS_GAMELIST_BASIC_GAMELIST_VIEW_H
#define ES_APP_VIEWS_GAMELIST_BASIC_GAMELIST_VIEW_H

#include "components/TextListComponent.h"
#include "views/gamelist/ISimpleGamelistView.h"

class BasicGamelistView : public ISimpleGamelistView
{
public:
    BasicGamelistView(Window* window, FileData* root);

    // Called when a FileData* is added, has its metadata changed, or is removed.
    void onFileChanged(FileData* file, bool reloadGamelist) override;

    void onThemeChanged(const std::shared_ptr<ThemeData>& theme) override;
    void setCursor(FileData* cursor) override;

    FileData* getCursor() override { return mList.getSelected(); }
    FileData* getNextEntry() override { return mList.getNext(); }
    FileData* getPreviousEntry() override { return mList.getPrevious(); }
    FileData* getFirstEntry() override { return mList.getFirst(); }
    FileData* getLastEntry() override { return mList.getLast(); }
    FileData* getFirstGameEntry() override { return mFirstGameEntry; }

    std::string getName() const override { return "basic"; }

    std::vector<HelpPrompt> getHelpPrompts() override;

    bool isListScrolling() override { return mList.isScrolling(); }
    void stopListScrolling() override { mList.stopScrolling(); }

    const std::vector<std::string>& getFirstLetterIndex() override { return mFirstLetterIndex; }

    void addPlaceholder(FileData* firstEntry = nullptr) override;
    void launch(FileData* game) override;

protected:
    std::string getQuickSystemSelectRightButton() override { return "right"; }
    std::string getQuickSystemSelectLeftButton() override { return "left"; }
    void populateList(const std::vector<FileData*>& files, FileData* firstEntry) override;
    void remove(FileData* game, bool deleteFile) override;
    void removeMedia(FileData* game) override;

    TextListComponent<FileData*> mList;
    // Points to the first game in the list, i.e. the first entry which is of the type 'GAME'.
    FileData* mFirstGameEntry;

    std::string FAVORITE_CHAR;
    std::string FOLDER_CHAR;
};

#endif // ES_APP_VIEWS_GAMELIST_BASIC_GAMELIST_VIEW_H

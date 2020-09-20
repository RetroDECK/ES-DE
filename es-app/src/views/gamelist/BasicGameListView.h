//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  BasicGameListView.h
//
//  Interface that defines a GameListView of the type 'basic'.
//

#ifndef ES_APP_VIEWS_GAME_LIST_BASIC_GAME_LIST_VIEW_H
#define ES_APP_VIEWS_GAME_LIST_BASIC_GAME_LIST_VIEW_H

#include "components/TextListComponent.h"
#include "views/gamelist/ISimpleGameListView.h"

class BasicGameListView : public ISimpleGameListView
{
public:
    BasicGameListView(Window* window, FileData* root);

    // Called when a FileData* is added, has its metadata changed, or is removed.
    virtual void onFileChanged(FileData* file, FileChangeType change) override;

    virtual void onThemeChanged(const std::shared_ptr<ThemeData>& theme) override;

    virtual FileData* getCursor() override;
    virtual void setCursor(FileData* file) override;
    virtual FileData* getNextEntry() override;
    virtual FileData* getPreviousEntry() override;
    virtual FileData* getFirstEntry() override;
    virtual FileData* getLastEntry() override;
    virtual FileData* getFirstGameEntry() override;

    virtual const char* getName() const override { return "basic"; }

    virtual std::vector<HelpPrompt> getHelpPrompts() override;
    virtual void launch(FileData* game) override;

    virtual bool isListScrolling() override { return mList.isScrolling(); };
    virtual void stopListScrolling() override { mList.stopScrolling(); };

protected:
    virtual std::string getQuickSystemSelectRightButton() override;
    virtual std::string getQuickSystemSelectLeftButton() override;
    virtual void populateList(const std::vector<FileData*>& files) override;
    virtual void remove(FileData* game, bool deleteFile) override;
    virtual void removeMedia(FileData* game) override;
    virtual void addPlaceholder();

    TextListComponent<FileData*> mList;
    // Points to the first game in the list, i.e. the first entry which is of the type 'GAME'.
    FileData* firstGameEntry;

    std::string FAVORITE_CHAR;
    std::string FOLDER_CHAR;
};

#endif // ES_APP_VIEWS_GAME_LIST_BASIC_GAME_LIST_VIEW_H

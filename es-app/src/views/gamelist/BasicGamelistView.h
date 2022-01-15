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
    virtual void onFileChanged(FileData* file, bool reloadGamelist) override;

    virtual void onThemeChanged(const std::shared_ptr<ThemeData>& theme) override;
    virtual void setCursor(FileData* cursor) override;

    virtual FileData* getCursor() override { return mList.getSelected(); }
    virtual FileData* getNextEntry() override { return mList.getNext(); }
    virtual FileData* getPreviousEntry() override { return mList.getPrevious(); }
    virtual FileData* getFirstEntry() override { return mList.getFirst(); }
    virtual FileData* getLastEntry() override { return mList.getLast(); }
    virtual FileData* getFirstGameEntry() override { return mFirstGameEntry; }

    virtual std::string getName() const override { return "basic"; }

    virtual std::vector<HelpPrompt> getHelpPrompts() override;

    virtual bool isListScrolling() override { return mList.isScrolling(); }
    virtual void stopListScrolling() override { mList.stopScrolling(); }

    virtual const std::vector<std::string>& getFirstLetterIndex() override
    {
        return mFirstLetterIndex;
    }

    virtual void addPlaceholder(FileData* firstEntry = nullptr) override;
    virtual void launch(FileData* game) override;

protected:
    virtual std::string getQuickSystemSelectRightButton() override { return "right"; }
    virtual std::string getQuickSystemSelectLeftButton() override { return "left"; }
    virtual void populateList(const std::vector<FileData*>& files, FileData* firstEntry) override;
    virtual void remove(FileData* game, bool deleteFile) override;
    virtual void removeMedia(FileData* game) override;

    TextListComponent<FileData*> mList;
    // Points to the first game in the list, i.e. the first entry which is of the type 'GAME'.
    FileData* mFirstGameEntry;

    std::string FAVORITE_CHAR;
    std::string FOLDER_CHAR;
};

#endif // ES_APP_VIEWS_GAMELIST_BASIC_GAMELIST_VIEW_H

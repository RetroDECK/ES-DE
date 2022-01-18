//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ISimpleGamelistView.h
//
//  Interface that defines a simple gamelist view.
//

#ifndef ES_APP_VIEWS_GAMELIST_ISIMPLE_GAMELIST_VIEW_H
#define ES_APP_VIEWS_GAMELIST_ISIMPLE_GAMELIST_VIEW_H

#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "views/gamelist/IGamelistView.h"

#include <stack>

class ISimpleGamelistView : public IGamelistView
{
public:
    ISimpleGamelistView(Window* window, FileData* root);
    virtual ~ISimpleGamelistView();

    // Called when a FileData* is added, has its metadata changed, or is removed.
    void onFileChanged(FileData* file, bool reloadGamelist) override;

    // Called whenever the theme changes.
    void onThemeChanged(const std::shared_ptr<ThemeData>& theme) override;

    virtual FileData* getCursor() override = 0;
    virtual void setCursor(FileData*) override = 0;
    virtual void addPlaceholder(FileData*) override = 0;

    bool input(InputConfig* config, Input input) override;
    virtual void launch(FileData* game) override = 0;

    virtual const std::vector<std::string>& getFirstLetterIndex() override = 0;

    // These functions are used to retain the folder cursor history, for instance
    // during a view reload. The calling function stores the history temporarily.
    void copyCursorHistory(std::vector<FileData*>& cursorHistory) override
    {
        cursorHistory = mCursorStackHistory;
    };
    void populateCursorHistory(std::vector<FileData*>& cursorHistory) override
    {
        mCursorStackHistory = cursorHistory;
    };

protected:
    virtual std::string getQuickSystemSelectRightButton() = 0;
    virtual std::string getQuickSystemSelectLeftButton() = 0;
    virtual void populateList(const std::vector<FileData*>& files, FileData* firstEntry) = 0;

    void generateGamelistInfo(FileData* cursor, FileData* firstEntry);
    void generateFirstLetterIndex(const std::vector<FileData*>& files);

    TextComponent mHeaderText;
    ImageComponent mHeaderImage;
    ImageComponent mBackground;

    std::vector<GuiComponent*> mThemeExtras;
    std::stack<FileData*> mCursorStack;
    std::vector<FileData*> mCursorStackHistory;
    // This game is randomly selected in the grouped custom collections view.
    FileData* mRandomGame;

    std::vector<std::string> mFirstLetterIndex;

    unsigned int mGameCount;
    unsigned int mFavoritesGameCount;
    unsigned int mFilteredGameCount;
    unsigned int mFilteredGameCountAll;
    bool mIsFiltered;
    bool mIsFolder;
};

#endif // ES_APP_VIEWS_GAMELIST_ISIMPLE_GAMELIST_VIEW_H

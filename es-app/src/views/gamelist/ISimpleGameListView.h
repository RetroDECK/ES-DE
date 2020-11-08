//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ISimpleGameListView.h
//
//  Interface that defines a simple gamelist view.
//

#ifndef ES_APP_VIEWS_GAME_LIST_ISIMPLE_GAME_LIST_VIEW_H
#define ES_APP_VIEWS_GAME_LIST_ISIMPLE_GAME_LIST_VIEW_H

#include "views/gamelist/IGameListView.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"

#include <stack>

class ISimpleGameListView : public IGameListView
{
public:
    ISimpleGameListView(Window* window, FileData* root);
    virtual ~ISimpleGameListView();

    // Called when a FileData* is added, has its metadata changed, or is removed.
    virtual void onFileChanged(FileData* file, bool reloadGameList) override;

    // Called whenever the theme changes.
    virtual void onThemeChanged(const std::shared_ptr<ThemeData>& theme) override;

    virtual FileData* getCursor() override = 0;
    virtual void setCursor(FileData*) override = 0;

    virtual bool input(InputConfig* config, Input input) override;
    virtual void launch(FileData* game) override = 0;

    virtual const std::vector<std::string>& getFirstLetterIndex() override = 0;

protected:
    virtual std::string getQuickSystemSelectRightButton() = 0;
    virtual std::string getQuickSystemSelectLeftButton() = 0;
    virtual void populateList(const std::vector<FileData*>& files) = 0;

    void generateGamelistInfo(const std::vector<FileData*>& files);
    void generateFirstLetterIndex(const std::vector<FileData*>& files);

    TextComponent mHeaderText;
    ImageComponent mHeaderImage;
    ImageComponent mBackground;

    std::vector<GuiComponent*> mThemeExtras;
    std::stack<FileData*> mCursorStack;

    std::vector<std::string> mFirstLetterIndex;

    unsigned int mGameCount;
    unsigned int mFavoritesGameCount;
    unsigned int mFilteredGameCount;
    unsigned int mFilteredGameCountAll;
    bool mIsFiltered;
    bool mIsFolder;
};

#endif // ES_APP_VIEWS_GAME_LIST_ISIMPLE_GAME_LIST_VIEW_H

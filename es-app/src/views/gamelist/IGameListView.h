//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  IGameListView.h
//
//  Interface that defines the minimum for a GameListView.
//

#ifndef ES_APP_VIEWS_GAME_LIST_IGAME_LIST_VIEW_H
#define ES_APP_VIEWS_GAME_LIST_IGAME_LIST_VIEW_H

#include "renderers/Renderer.h"
#include "FileData.h"
#include "GuiComponent.h"

class ThemeData;
class Window;

// This is an interface that defines the minimum for a GameListView.
class IGameListView : public GuiComponent
{
public:
    IGameListView(
            Window* window,
            FileData* root)
            : GuiComponent(window),
            mRoot(root)
            { setSize(static_cast<float>(Renderer::getScreenWidth()),
            static_cast<float>(Renderer::getScreenHeight())); }

    virtual ~IGameListView() {}

    // Called when a FileData* is added, has its metadata changed, or is removed.
    virtual void onFileChanged(FileData* file, bool reloadGameList) = 0;

    // Called whenever the theme changes.
    virtual void onThemeChanged(const std::shared_ptr<ThemeData>& theme) = 0;

    void setTheme(const std::shared_ptr<ThemeData>& theme);
    inline const std::shared_ptr<ThemeData>& getTheme() const { return mTheme; }

    virtual FileData* getCursor() = 0;
    virtual void setCursor(FileData*) = 0;
    virtual FileData* getNextEntry() = 0;
    virtual FileData* getPreviousEntry() = 0;
    virtual FileData* getFirstEntry() = 0;
    virtual FileData* getLastEntry() = 0;
    virtual FileData* getFirstGameEntry() = 0;
    virtual const std::vector<std::string>& getFirstLetterIndex() = 0;

    virtual bool input(InputConfig* config, Input input) override;
    virtual void remove(FileData* game, bool deleteFile) = 0;
    virtual void removeMedia(FileData* game) = 0;

    virtual const char* getName() const = 0;
    virtual void launch(FileData* game) = 0;

    virtual HelpStyle getHelpStyle() override;

    void render(const Transform4x4f& parentTrans) override;

protected:
    FileData* mRoot;
    std::shared_ptr<ThemeData> mTheme;
};

#endif // ES_APP_VIEWS_GAME_LIST_IGAME_LIST_VIEW_H

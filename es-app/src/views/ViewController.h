//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ViewController.h
//
//  Handles overall system navigation including animations and transitions.
//  Also creates the gamelist views and handles refresh and reloads of these when needed
//  (for example when metadata has been changed or when a list sorting has taken place).
//  Initiates the launching of games, calling FileData to do the actual launch.
//

#ifndef ES_APP_VIEWS_VIEW_CONTROLLER_H
#define ES_APP_VIEWS_VIEW_CONTROLLER_H

#include "renderers/Renderer.h"
#include "FileData.h"
#include "GuiComponent.h"

#include <vector>

class IGameListView;
class SystemData;
class SystemView;

// Handles transitions between views, e.g. from system to system and from gamelist to gamelist.
// Also sets up the initial gamelists and refreshes and reloads them as required.
class ViewController : public GuiComponent
{
public:
    static void init(Window* window);
    static ViewController* get();

    virtual ~ViewController();

    // Try to completely populate the GameListView map.
    // Caches things so there's no pauses during transitions.
    void preload();

    // If a basic view detected a metadata change, it can request to recreate
    // the current gamelist view (as it may change to be detailed).
    void reloadGameListView(IGameListView* gamelist, bool reloadTheme = false);
    inline void reloadGameListView(SystemData* system, bool reloadTheme = false)
            { reloadGameListView(getGameListView(system).get(), reloadTheme); }
    // Reload everything with a theme.
    // Used when the "ThemeSet" setting changes.
    void reloadAll();

    // Navigation.
    void goToNextGameList();
    void goToPrevGameList();
    void goToGameList(SystemData* system);
    void goToSystemView(SystemData* system);
    void goToStart();
    void ReloadAndGoToStart();

    // Functions to make the GUI behave properly.
    bool isCameraMoving();
    void resetMovingCamera();
    void stopScrolling();

    void onFileChanged(FileData* file, FileChangeType change);

    void launch(FileData* game, Vector3f centerCameraOn =
            Vector3f(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f, 0));

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const Transform4x4f& parentTrans) override;

    enum ViewMode {
        NOTHING,
        START_SCREEN,
        SYSTEM_SELECT,
        GAME_LIST
    };

    enum GameListViewType {
        AUTOMATIC,
        BASIC,
        DETAILED,
        GRID,
        VIDEO
    };

    struct State {
        ViewMode viewing;

        inline SystemData* getSystem() const
        {
            assert(viewing == GAME_LIST || viewing == SYSTEM_SELECT);
            return system;
        }

        private:
            friend ViewController;
            SystemData* system;
    };

    inline const State& getState() const { return mState; }

    virtual std::vector<HelpPrompt> getHelpPrompts() override;
    virtual HelpStyle getHelpStyle() override;

    std::shared_ptr<IGameListView> getGameListView(SystemData* system);
    std::shared_ptr<SystemView> getSystemListView();
    void removeGameListView(SystemData* system);

private:
    ViewController(Window* window);
    static ViewController* sInstance;

    void playViewTransition();
    int getSystemId(SystemData* system);

    std::shared_ptr<GuiComponent> mCurrentView;
    std::map<SystemData*, std::shared_ptr<IGameListView>> mGameListViews;
    std::shared_ptr<SystemView> mSystemListView;

    Transform4x4f mCamera;
    float mFadeOpacity;
    bool mLockInput;

    State mState;
};

#endif // ES_APP_VIEWS_VIEW_CONTROLLER_H

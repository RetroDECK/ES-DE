//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ViewController.h
//
//  Handles overall system navigation including animations and transitions.
//  Creates the gamelist views and handles refresh and reloads of these when needed
//  (for example when metadata has been changed or when a list sorting has taken place).
//  Initiates the launching of games, calling FileData to do the actual launch.
//  Displays a dialog when there are no games found on startup.
//

#ifndef ES_APP_VIEWS_VIEW_CONTROLLER_H
#define ES_APP_VIEWS_VIEW_CONTROLLER_H

#include "FileData.h"
#include "GuiComponent.h"
#include "guis/GuiMsgBox.h"
#include "renderers/Renderer.h"

#include <vector>

class IGameListView;
class SystemData;
class SystemView;

// Handles transitions between views, e.g. from system to system and from gamelist to gamelist.
// Also sets up the initial gamelists and refreshes and reloads them as required.
class ViewController : public GuiComponent
{
public:
    static ViewController* getInstance();

    // These functions are called from main().
    void invalidSystemsFileDialog();
    void noGamesDialog();
    void invalidAlternativeEmulatorDialog();

    // Try to completely populate the GameListView map.
    // Caches things so there's no pauses during transitions.
    void preload();

    // If a basic view detected a metadata change, it can request to recreate
    // the current gamelist view (as it may change to be detailed).
    void reloadGameListView(IGameListView* gamelist, bool reloadTheme = false);
    void reloadGameListView(SystemData* system, bool reloadTheme = false)
    {
        reloadGameListView(getGameListView(system).get(), reloadTheme);
    }
    // Reload everything with a theme.
    // Used when the "ThemeSet" setting changes.
    void reloadAll();

    // Navigation.
    void goToNextGameList();
    void goToPrevGameList();
    void goToGameList(SystemData* system);
    void goToSystemView(SystemData* system, bool playTransition);
    void goToSystem(SystemData* system, bool animate);
    void goToStart(bool playTransition);
    void ReloadAndGoToStart();

    // Functions to make the GUI behave properly.
    bool isCameraMoving();
    void cancelViewTransitions();
    void stopScrolling();

    void onFileChanged(FileData* file, bool reloadGameList);
    void triggerGameLaunch(FileData* game)
    {
        mGameToLaunch = game;
        mLockInput = true;
    };
    bool getGameLaunchTriggered() { return (mGameToLaunch != nullptr); }

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    enum ViewMode {
        NOTHING, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
        START_SCREEN,
        SYSTEM_SELECT,
        GAME_LIST
    };

    enum GameListViewStyle {
        AUTOMATIC, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
        BASIC,
        DETAILED,
        GRID,
        VIDEO
    };

    struct State {
        ViewMode viewing;
        GameListViewStyle viewstyle;

        SystemData* getSystem() const
        {
            assert(viewing == GAME_LIST || viewing == SYSTEM_SELECT);
            return system;
        }

    private:
        friend ViewController;
        SystemData* system;
    };

    const State& getState() const { return mState; }

    virtual std::vector<HelpPrompt> getHelpPrompts() override;
    virtual HelpStyle getHelpStyle() override;

    std::shared_ptr<IGameListView> getGameListView(SystemData* system);
    std::shared_ptr<SystemView> getSystemListView();
    void removeGameListView(SystemData* system);

    // Font Awesome symbols.
    static const std::string CONTROLLER_CHAR;
    static const std::string CROSSEDCIRCLE_CHAR;
    static const std::string EXCLAMATION_CHAR;
    static const std::string FAVORITE_CHAR;
    static const std::string FILTER_CHAR;
    static const std::string FOLDER_CHAR;
    static const std::string GEAR_CHAR;
    static const std::string KEYBOARD_CHAR;
    static const std::string TICKMARK_CHAR;

private:
    ViewController() noexcept;

    void launch(FileData* game);

    std::string mNoGamesErrorMessage;
    std::string mRomDirectory;
    GuiMsgBox* mNoGamesMessageBox;

    void playViewTransition(bool instant = false);
    int getSystemId(SystemData* system);
    // Restore view position if it was moved during wrap around.
    void restoreViewPosition();

    std::shared_ptr<GuiComponent> mCurrentView;
    std::shared_ptr<GuiComponent> mPreviousView;
    std::shared_ptr<GuiComponent> mSkipView;
    std::map<SystemData*, std::shared_ptr<IGameListView>> mGameListViews;
    std::shared_ptr<SystemView> mSystemListView;

    FileData* mGameToLaunch;
    State mState;

    glm::mat4 mCamera;
    bool mSystemViewTransition;
    bool mWrappedViews;
    float mWrapPreviousPositionX;
    float mFadeOpacity;
    bool mCancelledTransition; // Needed only for the Fade transition style.
    bool mLockInput;
    bool mNextSystem;
};

#endif // ES_APP_VIEWS_VIEW_CONTROLLER_H

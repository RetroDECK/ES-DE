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
#include "utils/StringUtil.h"

#include <vector>

class GamelistView;
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
    void updateAvailableDialog();

    // Try to completely populate the GamelistView map.
    // Caches things so there's no pauses during transitions.
    void preload();

    // If a basic view detected a metadata change, it can request to recreate
    // the current gamelist view (as it may change to be detailed).
    void reloadGamelistView(GamelistView* gamelist, bool reloadTheme = false);
    void reloadGamelistView(SystemData* system, bool reloadTheme = false)
    {
        reloadGamelistView(getGamelistView(system).get(), reloadTheme);
    }
    // Reload everything with a theme.
    // Used when the "ThemeSet" setting changes.
    void reloadAll();

    // Navigation.
    void goToNextGamelist();
    void goToPrevGamelist();
    void goToGamelist(SystemData* system);
    void goToSystemView(SystemData* system, bool playTransition);
    void goToSystem(SystemData* system, bool animate);
    void goToStart(bool playTransition);
    void ReloadAndGoToStart();

    // Functions to make the GUI behave properly.
    bool isCameraMoving();
    void cancelViewTransitions();
    void stopScrolling();
    void resetCamera()
    {
        if (mCurrentView != nullptr) {
            mCamera[3].x = -mCurrentView->getPosition().x;
            mCamera[3].y = -mCurrentView->getPosition().y;
        }
    }

    // Basic video controls.
    void startViewVideos() override { mCurrentView->startViewVideos(); }
    void stopViewVideos() override { mCurrentView->stopViewVideos(); }
    void pauseViewVideos() override { mCurrentView->pauseViewVideos(); }
    void muteViewVideos() override { mCurrentView->muteViewVideos(); }

    void onFileChanged(FileData* file, bool reloadGamelist);
    void triggerGameLaunch(FileData* game)
    {
        mGameToLaunch = game;
        mLockInput = true;
    };
    bool getGameLaunchTriggered() { return (mGameToLaunch != nullptr); }
    std::vector<std::string>& getGameEndEventParams() { return mGameEndEventParams; }

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    enum ViewMode {
        NOTHING,
        START_SCREEN,
        SYSTEM_SELECT,
        GAMELIST
    };

    enum GamelistViewStyle {
        AUTOMATIC,
        BASIC,
        DETAILED,
        VIDEO
    };

    struct State {
        ViewMode viewing;
        GamelistViewStyle viewstyle;

        SystemData* getSystem() const
        {
            assert(viewing == GAMELIST || viewing == SYSTEM_SELECT);
            return system;
        }

    private:
        friend ViewController;
        SystemData* system;
    };

    const State& getState() const { return mState; }

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override;
    HelpStyle getViewHelpStyle();

    std::shared_ptr<GamelistView> getGamelistView(SystemData* system);
    std::shared_ptr<SystemView> getSystemListView();
    void removeGamelistView(SystemData* system);

    // Font Awesome symbols.
#if defined(_MSC_VER) // MSVC compiler.
    static inline const std::string CONTROLLER_CHAR {Utils::String::wideStringToString(L"\uf11b")};
    static inline const std::string CROSSEDCIRCLE_CHAR {
        Utils::String::wideStringToString(L"\uf05e")};
    static inline const std::string EXCLAMATION_CHAR {Utils::String::wideStringToString(L"\uf06a")};
    static inline const std::string FAVORITE_CHAR {Utils::String::wideStringToString(L"\uf005")};
    static inline const std::string FILTER_CHAR {Utils::String::wideStringToString(L"\uf0b0")};
    static inline const std::string FOLDER_CHAR {Utils::String::wideStringToString(L"\uf07C")};
    static inline const std::string FOLDERLINK_CHAR {Utils::String::wideStringToString(L"\uf090")};
    static inline const std::string GEAR_CHAR {Utils::String::wideStringToString(L"\uf013")};
    static inline const std::string KEYBOARD_CHAR {Utils::String::wideStringToString(L"\uf11c")};
    static inline const std::string TICKMARK_CHAR {Utils::String::wideStringToString(L"\uf14A")};
#else
    static inline const std::string CONTROLLER_CHAR {"\uf11b"};
    static inline const std::string CROSSEDCIRCLE_CHAR {"\uf05e"};
    static inline const std::string EXCLAMATION_CHAR {"\uf06a"};
    static inline const std::string FAVORITE_CHAR {"\uf005"};
    static inline const std::string FILTER_CHAR {"\uf0b0"};
    static inline const std::string FOLDER_CHAR {"\uf07C"};
    static inline const std::string FOLDERLINK_CHAR {"\uf090"};
    static inline const std::string GEAR_CHAR {"\uf013"};
    static inline const std::string KEYBOARD_CHAR {"\uf11c"};
    static inline const std::string TICKMARK_CHAR {"\uf14a"};
#endif

private:
    ViewController() noexcept;

    void launch(FileData* game);

    Renderer* mRenderer;
    std::string mNoGamesErrorMessage;
    std::string mRomDirectory;
    GuiMsgBox* mNoGamesMessageBox;

    void playViewTransition(ViewTransition transitionType, bool instant = false);
    int getSystemId(SystemData* system);
    // Restore view position if it was moved during wrap around.
    void restoreViewPosition();

    std::shared_ptr<GuiComponent> mCurrentView;
    std::shared_ptr<GuiComponent> mPreviousView;
    std::shared_ptr<GuiComponent> mSkipView;
    std::map<SystemData*, std::shared_ptr<GamelistView>> mGamelistViews;
    std::shared_ptr<SystemView> mSystemListView;
    ViewTransitionAnimation mLastTransitionAnim;

    std::vector<std::string> mGameEndEventParams;
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

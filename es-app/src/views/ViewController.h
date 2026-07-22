//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
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

#include <memory>
#include <vector>

class GamelistView;
class RomMLibrarySync;
class SystemData;
class SystemView;

// Handles transitions between views, e.g. from system to system and from gamelist to gamelist.
// Also sets up the initial gamelists and refreshes and reloads them as required.
class ViewController : public GuiComponent
{
public:
    static ViewController* getInstance();

    // These functions are called from main().
    void setMenuColors();
    void migratedAppDataFilesDialog();
    void unsafeUpgradeDialog();
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
    // Reload everything with a theme, used when the "Theme" setting changes.
    void reloadAll();

    // On window size changes we need to deinit/init the application, reload the systems etc.
    void setWindowSizeChanged(const int width, const int height);
    void checkWindowSizeChanged();

    // Rescan the ROM directory for any changes to games and systems.
    void rescanROMDirectory();

    // Kicks off a silent, non-blocking RomM library sync in the background (used on startup).
    // No-op if RomM integration or the startup-sync setting is disabled, or if no system is
    // opted into sync.
    void startRomMBackgroundSync();
    // Whether the background sync is currently in progress, so the manual "FORCE FULL RESYNC"
    // menu action and the post-pairing sync can avoid starting a second sync that would race it
    // over RomMCache (see RomMCache's class comment). runRomMSyncWithSplashScreen() isn't
    // tracked here - it blocks all input while running (see its own comment), so it can't be
    // re-triggered concurrently.
    //
    // Not const: also finalizes a just-completed sync if update() hasn't noticed it yet, since
    // update() only runs on this object while it's the topmost GUI stack entry - a menu sitting
    // on top of it would otherwise see a stale "still syncing" answer indefinitely.
    bool isRomMSyncing();

    // Runs a RomM library sync synchronously with Window::SplashScreenState::SYNCING, blocking
    // input for the duration; results are logged rather than shown in a message box.
    // forceFullResync is forwarded to RomMLibrarySync - see its constructor for details.
    void runRomMSyncWithSplashScreen(bool forceFullResync = false);

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
    void resetViewVideosTimer() override
    {
        if (mCurrentView != nullptr)
            mCurrentView->resetViewVideosTimer();
    }

    void onFileChanged(FileData* file, bool reloadGamelist);
    void triggerGameLaunch(FileData* game)
    {
        mGameToLaunch = game;
        mWindow->setBlockInput(true);
    };
    const bool getGameLaunchTriggered() { return (mGameToLaunch != nullptr); }

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    enum class ViewMode {
        NOTHING,
        SYSTEM_SELECT,
        GAMELIST
    };

    struct State {
        ViewMode viewing;
        ViewMode previouslyViewed;

        SystemData* getSystem() const
        {
            assert(viewing == ViewMode::GAMELIST || viewing == ViewMode::SYSTEM_SELECT);
            return system;
        }

    private:
        friend ViewController;
        SystemData* system;
    };

    const State& getState() const { return mState; }

    std::vector<HelpPrompt> getHelpPrompts() override;

    void setHelpComponentsVisibility(const bool state) override
    {
        if (mCurrentView != nullptr)
            mCurrentView->setHelpComponentsVisibility(state);
    }

    std::shared_ptr<GamelistView> getGamelistView(SystemData* system);
    std::shared_ptr<SystemView> getSystemListView();
    void removeGamelistView(SystemData* system);

    // Font Awesome symbols.
#if defined(_MSC_VER) // MSVC compiler.
    static inline const std::string ARROW_LEFT_CHAR {Utils::String::wideStringToString(L"\uf104")};
    static inline const std::string ARROW_RIGHT_CHAR {Utils::String::wideStringToString(L"\uf105")};
    static inline const std::string BRANCH_CHAR {Utils::String::wideStringToString(L"\uf18c")};
    static inline const std::string CONTROLLER_CHAR {Utils::String::wideStringToString(L"\uf11b")};
    static inline const std::string CROSSEDCIRCLE_CHAR {
        Utils::String::wideStringToString(L"\uf05e")};
    static inline const std::string DOWNLOAD_CHAR {Utils::String::wideStringToString(L"\uf0ed")};
    static inline const std::string EXCLAMATION_CHAR {Utils::String::wideStringToString(L"\uf06a")};
    static inline const std::string FAVORITE_CHAR {Utils::String::wideStringToString(L"\uf005")};
    static inline const std::string FILTER_CHAR {Utils::String::wideStringToString(L"\uf0b0")};
    static inline const std::string FOLDER_CHAR {Utils::String::wideStringToString(L"\uf07C")};
    static inline const std::string FOLDERLINK_CHAR {Utils::String::wideStringToString(L"\uf090")};
    static inline const std::string GEAR_CHAR {Utils::String::wideStringToString(L"\uf013")};
    static inline const std::string KEYBOARD_CHAR {Utils::String::wideStringToString(L"\uf11c")};
    static inline const std::string TICKMARK_CHAR {Utils::String::wideStringToString(L"\uf14A")};
#else
    static inline const std::string ARROW_LEFT_CHAR {"\uf104"};
    static inline const std::string ARROW_RIGHT_CHAR {"\uf105"};
    static inline const std::string BRANCH_CHAR {"\uf18c"};
    static inline const std::string CONTROLLER_CHAR {"\uf11b"};
    static inline const std::string CROSSEDCIRCLE_CHAR {"\uf05e"};
    static inline const std::string DOWNLOAD_CHAR {"\uf0ed"};
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

    FileData* mGameToLaunch;
    State mState;

    std::unique_ptr<RomMLibrarySync> mRomMBackgroundSync;
    void finalizeRomMBackgroundSyncIfDone(); // shared by isRomMSyncing() and update()

    glm::mat4 mCamera;
    bool mSystemViewTransition;
    bool mWrappedViews;
    float mWrapPreviousPositionX;
    float mFadeOpacity;
    bool mCancelledTransition; // Needed only for the Fade transition style.
    bool mNextSystem;
    int mWindowChangedWidth;
    int mWindowChangedHeight;
};

#endif // ES_APP_VIEWS_VIEW_CONTROLLER_H

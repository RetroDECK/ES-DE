//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ViewController.cpp
//
//  Handles overall system navigation including animations and transitions.
//  Also creates the gamelist views and handles refresh and reloads of these when needed
//  (for example when metadata has been changed or when a list sorting has taken place).
//  Initiates the launching of games, calling FileData to do the actual launch.
//

#include "views/ViewController.h"

#include "animations/Animation.h"
#include "animations/LambdaAnimation.h"
#include "animations/MoveCameraAnimation.h"
#include "guis/GuiInfoPopup.h"
#include "guis/GuiMenu.h"
#include "guis/GuiMsgBox.h"
#include "views/gamelist/DetailedGameListView.h"
#include "views/gamelist/GridGameListView.h"
#include "views/gamelist/IGameListView.h"
#include "views/gamelist/VideoGameListView.h"
#include "views/SystemView.h"
#include "views/UIModeController.h"
#include "FileFilterIndex.h"
#include "InputManager.h"
#include "Log.h"
#include "Settings.h"
#include "Sound.h"
#include "SystemData.h"
#include "SystemView.h"
#include "Window.h"

ViewController* ViewController::sInstance = nullptr;

ViewController* ViewController::get()
{
    assert(sInstance);
    return sInstance;
}

void ViewController::init(Window* window)
{
    assert(!sInstance);
    sInstance = new ViewController(window);
}

ViewController::ViewController(
        Window* window)
        : GuiComponent(window),
        mCurrentView(nullptr),
        mPreviousView(nullptr),
        mSkipView(nullptr),
        mCamera(Transform4x4f::Identity()),
        mSystemViewTransition(false),
        mWrappedViews(false),
        mFadeOpacity(0),
        mCancelledTransition(false),
        mLockInput(false),
        mGameToLaunch(nullptr)
{
    mState.viewing = NOTHING;
}

ViewController::~ViewController()
{
    assert(sInstance == this);
    sInstance = nullptr;
}

void ViewController::goToStart()
{
    // Check if the keyboard config is set as application default, meaning no user
    // configuration has been performed.
    if (InputManager::getInstance()->
            getInputConfigByDevice(DEVICE_KEYBOARD)->getDefaultConfigFlag()) {

        LOG(LogInfo) << "Applying default keyboard mappings.";

        if (Settings::getInstance()->getBool("ShowDefaultKeyboardWarning")) {
            std::string message = "NO KEYBOARD CONFIGURATION COULD BE\n"
                    "FOUND IN ES_INPUT.CFG, SO APPLYING THE\n"
                    "DEFAULT KEYBOARD MAPPINGS. IT'S HOWEVER\n"
                    "RECOMMENDED TO SETUP YOUR OWN KEYBOARD\n"
                    "CONFIGURATION. TO DO SO, CHOOSE THE ENTRY\n"
                    "'CONFIGURE INPUT' ON THE MAIN MENU.";

        mWindow->pushGui(new GuiMsgBox(mWindow, HelpStyle(), message.c_str(),
            "OK", nullptr, "DON'T SHOW AGAIN", [] {
                Settings::getInstance()->setBool("ShowDefaultKeyboardWarning", false);
                Settings::getInstance()->saveFile();
            }));
        }
    }

    // If a specific system is requested, go directly to its game list.
    auto requestedSystem = Settings::getInstance()->getString("StartupSystem");
    if ("" != requestedSystem && "retropie" != requestedSystem) {
        for (auto it = SystemData::sSystemVector.cbegin();
                it != SystemData::sSystemVector.cend(); it++) {
            if ((*it)->getName() == requestedSystem) {
                goToGameList(*it);
                return;
            }
        }

        // Requested system doesn't exist.
        Settings::getInstance()->setString("StartupSystem", "");
    }
    // Get the first system entry.
    goToSystemView(getSystemListView()->getFirst(), false);
}

void ViewController::ReloadAndGoToStart()
{
    mWindow->renderLoadingScreen("Loading...");
    ViewController::get()->reloadAll();
    ViewController::get()->goToStart();
}

bool ViewController::isCameraMoving()
{
    if (mCurrentView) {
        if (mCamera.r3().x() != -mCurrentView->getPosition().x() ||
                mCamera.r3().y() != -mCurrentView->getPosition().y())
            return true;
    }
    return false;
}

void ViewController::cancelViewTransitions()
{
    if (Settings::getInstance()->getString("TransitionStyle") == "slide") {
        if (isCameraMoving()) {
            mCamera.r3().x() = -mCurrentView->getPosition().x();
            mCamera.r3().y() = -mCurrentView->getPosition().y();
            stopAllAnimations();
        }
        // mSkipView is used when skipping through the gamelists in quick succession.
        // Without this, the game video (or static image) would not get rendered during
        // the slide transition animation.
        else if (mSkipView) {
            mSkipView.reset();
            mSkipView = nullptr;
        }
    }
    else if (Settings::getInstance()->getString("TransitionStyle") == "fade") {
        if (isAnimationPlaying(0)) {
            finishAnimation(0);
            mCancelledTransition = true;
            mFadeOpacity = 0;
            mWindow->invalidateCachedBackground();
        }
    }
}

void ViewController::stopScrolling()
{
    mSystemListView->stopScrolling();
    mCurrentView->stopListScrolling();

    if (mSystemListView->isAnimationPlaying(0))
        mSystemListView->finishAnimation(0);
}

int ViewController::getSystemId(SystemData* system)
{
    std::vector<SystemData*>& sysVec = SystemData::sSystemVector;
    return static_cast<int>(std::find(sysVec.cbegin(), sysVec.cend(), system) - sysVec.cbegin());
}

void ViewController::restoreViewPosition()
{
    if (mPreviousView) {
        Vector3f restorePosition = mPreviousView->getPosition();
        restorePosition.x() = mWrapPreviousPositionX;
        mPreviousView->setPosition(restorePosition);
        mWrapPreviousPositionX = 0;
        mWrappedViews = false;
    }
}

void ViewController::goToSystemView(SystemData* system, bool playTransition)
{
    bool applicationStartup = false;

    if (mState.viewing == NOTHING)
        applicationStartup = true;

    // Restore the X position for the view, if it was previously moved.
    if (mWrappedViews)
        restoreViewPosition();

    if (mPreviousView) {
        mPreviousView.reset();
        mPreviousView = nullptr;
    }

    mPreviousView = mCurrentView;

    if (system->isGroupedCustomCollection())
        system = system->getRootFolder()->getParent()->getSystem();

    mState.viewing = SYSTEM_SELECT;
    mState.system = system;
    mSystemViewTransition = true;

    auto systemList = getSystemListView();
    systemList->setPosition(getSystemId(system) * static_cast<float>(Renderer::getScreenWidth()),
            systemList->getPosition().y());

    systemList->goToSystem(system, false);
    mCurrentView = systemList;
    mCurrentView->onShow();
    PowerSaver::setState(true);

    // Application startup animation.
    if (applicationStartup) {
        mCamera.translation() = -mCurrentView->getPosition();
        if (Settings::getInstance()->getString("TransitionStyle") == "slide") {
            if (getSystemListView()->getCarouselType() == CarouselType::HORIZONTAL ||
                    getSystemListView()->getCarouselType() == CarouselType::HORIZONTAL_WHEEL)
                mCamera.translation().y() += Renderer::getScreenHeight();
            else
                mCamera.translation().x() -= Renderer::getScreenWidth();
            updateHelpPrompts();
        }
        else if (Settings::getInstance()->getString("TransitionStyle") == "fade") {
            if (getSystemListView()->getCarouselType() == CarouselType::HORIZONTAL ||
                    getSystemListView()->getCarouselType() == CarouselType::HORIZONTAL_WHEEL)
                mCamera.translation().y() += Renderer::getScreenHeight();
            else
                mCamera.translation().x() += Renderer::getScreenWidth();
        }
        else {
            updateHelpPrompts();
        }
    }

    if (playTransition || applicationStartup)
        playViewTransition();
    else
        playViewTransition(true);
}

void ViewController::goToNextGameList()
{
    assert(mState.viewing == GAME_LIST);
    SystemData* system = getState().getSystem();
    assert(system);
    NavigationSounds::getInstance()->playThemeNavigationSound(QUICKSYSSELECTSOUND);
    goToGameList(system->getNext());
}

void ViewController::goToPrevGameList()
{
    assert(mState.viewing == GAME_LIST);
    SystemData* system = getState().getSystem();
    assert(system);
    NavigationSounds::getInstance()->playThemeNavigationSound(QUICKSYSSELECTSOUND);
    goToGameList(system->getPrev());
}

void ViewController::goToGameList(SystemData* system)
{
    bool wrapFirstToLast = false;
    bool wrapLastToFirst = false;
    bool slideTransitions = false;

    if (Settings::getInstance()->getString("TransitionStyle") == "slide")
        slideTransitions = true;

    // Restore the X position for the view, if it was previously moved.
    if (mWrappedViews)
        restoreViewPosition();

    if (mPreviousView && Settings::getInstance()->getString("TransitionStyle") == "fade" &&
            isAnimationPlaying(0))
        mPreviousView->onHide();

    if (mPreviousView) {
        mSkipView = mPreviousView;
        mPreviousView.reset();
        mPreviousView = nullptr;
    }

    if (mState.viewing != SYSTEM_SELECT) {
        mPreviousView = mCurrentView;
        mSystemViewTransition = false;
    }
    else {
        mSystemViewTransition = true;
    }

    // Find if we're wrapping around the first and last systems, which requires the gamelist
    // to be moved in order to avoid weird camera movements. This is only needed for the
    // slide transition style though.
    if (mState.viewing == GAME_LIST && slideTransitions) {
        if (SystemData::sSystemVector.front() == mState.getSystem()) {
            if (SystemData::sSystemVector.back() == system)
                wrapFirstToLast = true;
        }
        else if (SystemData::sSystemVector.back() == mState.getSystem()) {
            if (SystemData::sSystemVector.front() == system)
                wrapLastToFirst = true;
        }
    }

    // Stop any scrolling, animations and camera movements.
    if (mState.viewing == SYSTEM_SELECT) {
        mSystemListView->stopScrolling();
        if (mSystemListView->isAnimationPlaying(0))
            mSystemListView->finishAnimation(0);
    }

    if (slideTransitions)
        cancelViewTransitions();

    if (mState.viewing == SYSTEM_SELECT) {
        // Move the system list.
        auto sysList = getSystemListView();
        float offsetX = sysList->getPosition().x();
        int sysId = getSystemId(system);

        sysList->setPosition(sysId * static_cast<float>(Renderer::getScreenWidth()),
                sysList->getPosition().y());
        offsetX = sysList->getPosition().x() - offsetX;
        mCamera.translation().x() -= offsetX;
    }

    // If we are wrapping around, either from the first to last system, or the other way
    // around, we need to temporarily move the gamelist view location so that the camera
    // movements will be correct. This is accomplished by simply offsetting the X position
    // with the position of the first or last system plus the screen width.
    if (wrapFirstToLast) {
        Vector3f currentPosition = mCurrentView->getPosition();
        mWrapPreviousPositionX = currentPosition.x();
        float offsetX = getGameListView(system)->getPosition().x();
        offsetX += Renderer::getScreenWidth();
        currentPosition.x() = offsetX;
        mCurrentView->setPosition(currentPosition);
        mCamera.translation().x() -= offsetX;
        mWrappedViews = true;
    }
    else if (wrapLastToFirst) {
        Vector3f currentPosition = mCurrentView->getPosition();
        mWrapPreviousPositionX = currentPosition.x();
        float offsetX = getGameListView(system)->getPosition().x();
        offsetX -= Renderer::getScreenWidth();
        currentPosition.x() = offsetX;
        mCurrentView->setPosition(currentPosition);
        mCamera.translation().x() = -offsetX;
        mWrappedViews = true;
    }

    mCurrentView = getGameListView(system);

    // Application startup animation, if starting in a gamelist rather than in the system view.
    if (mState.viewing == NOTHING) {
        mCamera.translation() = -mCurrentView->getPosition();
        if (Settings::getInstance()->getString("TransitionStyle") == "slide") {
            mCamera.translation().y() -= Renderer::getScreenHeight();
            updateHelpPrompts();
        }
        else if (Settings::getInstance()->getString("TransitionStyle") == "fade") {
            mCamera.translation().y() += Renderer::getScreenHeight() * 2;
        }
        else {
            updateHelpPrompts();
        }
    }

    mState.viewing = GAME_LIST;
    mState.system = system;

    if (mCurrentView)
        mCurrentView->onShow();

    playViewTransition();
}

void ViewController::playViewTransition(bool instant)
{
    mCancelledTransition = false;

    Vector3f target(Vector3f::Zero());
    if (mCurrentView)
        target = mCurrentView->getPosition();

    // No need to animate, we're not going anywhere (probably due to goToNextGamelist()
    // or goToPrevGamelist() being called when there's only 1 system).
    if (target == -mCamera.translation() && !isAnimationPlaying(0))
        return;

    std::string transition_style = Settings::getInstance()->getString("TransitionStyle");

    if (instant || transition_style == "instant") {
        setAnimation(new LambdaAnimation([this, target](float /*t*/) {
            this->mCamera.translation() = -target;
            if (mPreviousView)
                mPreviousView->onHide();
        }, 1));
        updateHelpPrompts();
    }
    else if (transition_style == "fade") {
        // Stop whatever's currently playing, leaving mFadeOpacity wherever it is.
        cancelAnimation(0);

        auto fadeFunc = [this](float t) {
            // The flag mCancelledTransition is required only when cancelViewTransitions()
            // cancels the animation, and it's only needed for the Fade transitions.
            // Without this, a (much shorter) fade transition would still play as
            // finishedCallback is calling this function.
            if (!mCancelledTransition)
                mFadeOpacity = Math::lerp(0, 1, t);
        };

        auto fadeCallback = [this]() {
            if (mPreviousView)
                mPreviousView->onHide();
        };

        const static int FADE_DURATION = 120; // Fade in/out time.
        const static int FADE_WAIT = 200; // Time to wait between in/out.
        setAnimation(new LambdaAnimation(fadeFunc, FADE_DURATION), 0,
                [this, fadeFunc, fadeCallback, target] {
            this->mCamera.translation() = -target;
            updateHelpPrompts();
            setAnimation(new LambdaAnimation(fadeFunc, FADE_DURATION),
                    FADE_WAIT, fadeCallback, true);
        });

        // Fast-forward animation if we're partway faded.
        if (target == -mCamera.translation()) {
            // Not changing screens, so cancel the first half entirely.
            advanceAnimation(0, FADE_DURATION);
            advanceAnimation(0, FADE_WAIT);
            advanceAnimation(0, FADE_DURATION - static_cast<int>(mFadeOpacity * FADE_DURATION));
        }
        else {
            advanceAnimation(0, static_cast<int>(mFadeOpacity * FADE_DURATION));
        }
    }
    else if (transition_style == "slide") {
        auto slideCallback = [this]() {
            if (mSkipView) {
                mSkipView->onHide();
                mSkipView.reset();
                mSkipView = nullptr;
            }
            else if (mPreviousView) {
                mPreviousView->onHide();
            }
        };
        setAnimation(new MoveCameraAnimation(mCamera, target), 0, slideCallback);
        updateHelpPrompts(); // Update help prompts immediately.
    }
}

void ViewController::onFileChanged(FileData* file, bool reloadGameList)
{
    auto it = mGameListViews.find(file->getSystem());
    if (it != mGameListViews.cend())
        it->second->onFileChanged(file, reloadGameList);
}

void ViewController::launch(FileData* game)
{
    if (game->getType() != GAME) {
        LOG(LogError) << "tried to launch something that isn't a game.";
        return;
    }

    // If the video view style is used, pause the video currently playing or block the
    // video from starting to play if the static image is still shown.
    if (mCurrentView)
        mCurrentView->onPauseVideo();

    // Disable text scrolling. It will be enabled again in FileData upon returning from the game.
    mWindow->setAllowTextScrolling(false);

    stopAnimation(1); // Make sure the fade in isn't still playing.
    mWindow->stopInfoPopup(); // Make sure we disable any existing info popup.

    // Until a proper game launch screen is implemented, at least this will let the
    // user know that something is actually happening (in addition to the launch sound,
    // if navigation sounds are enabled).
    GuiInfoPopup* s = new GuiInfoPopup(mWindow, "LAUNCHING GAME '" +
            Utils::String::toUpper(game->metadata.get("name") + "'"), 10000);
    mWindow->setInfoPopup(s);

    NavigationSounds::getInstance()->playThemeNavigationSound(LAUNCHSOUND);

    // This is just a dummy animation in order for the launch notification popup to be
    // displayed briefly, and for the navigation sound playing to be able to complete.
    // During this time period, all user input is blocked.
    setAnimation(new LambdaAnimation([](float t){}, 1700), 0, [this, game] {
        while (NavigationSounds::getInstance()->isPlayingThemeNavigationSound(LAUNCHSOUND));
        game->launchGame(mWindow);
        onFileChanged(game, true);
        // This is a workaround so that any key or button presses used for exiting the emulator
        // are not captured upon returning to ES.
        setAnimation(new LambdaAnimation([](float t){}, 1), 0, [this] {
            mLockInput = false;
        });
    });
}

void ViewController::removeGameListView(SystemData* system)
{
    auto exists = mGameListViews.find(system);
    if (exists != mGameListViews.cend()) {
        exists->second.reset();
        mGameListViews.erase(system);
    }
}

std::shared_ptr<IGameListView> ViewController::getGameListView(SystemData* system)
{
    // If we have already created an entry for this system, then return that one.
    auto exists = mGameListViews.find(system);
    if (exists != mGameListViews.cend())
        return exists->second;

    system->getIndex()->setUIModeFilters();
    // If there's no entry, then create it and return it.
    std::shared_ptr<IGameListView> view;

    bool themeHasVideoView = system->getTheme()->hasView("video");

    // Decide which view style to use.
    GameListViewType selectedViewStyle = AUTOMATIC;

    std::string viewPreference = Settings::getInstance()->getString("GamelistViewStyle");
    if (viewPreference.compare("basic") == 0)
        selectedViewStyle = BASIC;
    if (viewPreference.compare("detailed") == 0)
        selectedViewStyle = DETAILED;
    if (viewPreference.compare("grid") == 0)
        selectedViewStyle = GRID;
    if (viewPreference.compare("video") == 0)
        selectedViewStyle = VIDEO;

    if (selectedViewStyle == AUTOMATIC) {
        std::vector<FileData*> files = system->getRootFolder()->getFilesRecursive(GAME | FOLDER);
        for (auto it = files.cbegin(); it != files.cend(); it++) {
            if (themeHasVideoView && !(*it)->getVideoPath().empty()) {
                selectedViewStyle = VIDEO;
                break;
            }
            else if (!(*it)->getImagePath().empty()) {
                selectedViewStyle = DETAILED;
                // Don't break out in case any subsequent files have videos.
            }
        }
    }

    // Create the view.
    switch (selectedViewStyle)
    {
        case VIDEO:
            view = std::shared_ptr<IGameListView>(
                    new VideoGameListView(mWindow, system->getRootFolder()));
            break;
        case DETAILED:
            view = std::shared_ptr<IGameListView>(
                    new DetailedGameListView(mWindow, system->getRootFolder()));
            break;
        case GRID:
            view = std::shared_ptr<IGameListView>(
                    new GridGameListView(mWindow, system->getRootFolder()));
            break;
        case BASIC:
        default:
            view = std::shared_ptr<IGameListView>(
                    new BasicGameListView(mWindow, system->getRootFolder()));
            break;
    }

    view->setTheme(system->getTheme());

    std::vector<SystemData*>& sysVec = SystemData::sSystemVector;
    int id = static_cast<int>(
            std::find(sysVec.cbegin(), sysVec.cend(), system) - sysVec.cbegin());
    view->setPosition(id * static_cast<float>(Renderer::getScreenWidth()),
            static_cast<float>(Renderer::getScreenHeight() * 2));

    addChild(view.get());

    mGameListViews[system] = view;
    return view;
}

std::shared_ptr<SystemView> ViewController::getSystemListView()
{
    // If we have already created a system view entry, then return it.
    if (mSystemListView)
        return mSystemListView;

    mSystemListView = std::shared_ptr<SystemView>(new SystemView(mWindow));
    addChild(mSystemListView.get());
    mSystemListView->setPosition(0, static_cast<float>(Renderer::getScreenHeight()));
    return mSystemListView;
}

bool ViewController::input(InputConfig* config, Input input)
{
    if (mLockInput)
        return true;

    #if defined(_WIN64)
    // This code is only needed for Windows, where we may need to keep ES running while
    // the game/emulator is in use. It's basically used to pause any playing game video
    // and to keep the screensaver from activating.
    if (Settings::getInstance()->getBool("RunInBackground")) {
        // If we have previously launched a game and there is now input registered, it means
        // the user is back in ES, so unset the flag to indicate that a game has been launched
        // and update all the GUI components to reflect this.
        if (mWindow->getGameLaunchedState())
            mWindow->unsetLaunchedGame();
    }
    #endif

    // Open the main menu.
    if (!(UIModeController::getInstance()->isUIModeKid() &&
            !Settings::getInstance()->getBool("ShowKidStartMenu")) &&
            config->isMappedTo("start", input) && input.value != 0) {
        // If we don't stop the scrolling here, it will continue to
        // run after closing the menu.
        if (mSystemListView->isScrolling())
            mSystemListView->stopScrolling();
        // Finish the animation too, so that it doesn't continue
        // to play when we've closed the menu.
        if (mSystemListView->isAnimationPlaying(0))
            mSystemListView->finishAnimation(0);
        // Stop the gamelist scrolling as well as it would otherwise
        // also continue to run after closing the menu.
        mCurrentView->stopListScrolling();
        // Finally, if the camera is currently moving, reset its position.
        cancelViewTransitions();

        mWindow->pushGui(new GuiMenu(mWindow));
        return true;
    }

    // Check if UI mode has changed due to passphrase completion.
    if (UIModeController::getInstance()->listen(config, input))
        return true;

    if (mCurrentView)
        return mCurrentView->input(config, input);

    return false;
}

void ViewController::update(int deltaTime)
{
    if (mCurrentView)
        mCurrentView->update(deltaTime);

    updateSelf(deltaTime);

    if (mGameToLaunch) {
        launch(mGameToLaunch);
        mGameToLaunch = nullptr;
    }
}

void ViewController::render(const Transform4x4f& parentTrans)
{
    Transform4x4f trans = mCamera * parentTrans;
    Transform4x4f transInverse;
    transInverse.invert(trans);

    // Camera position, position + size.
    Vector3f viewStart = transInverse.translation();
    Vector3f viewEnd = transInverse * Vector3f(static_cast<float>(Renderer::getScreenWidth()),
            static_cast<float>(Renderer::getScreenHeight(), 0));

    // Keep track of UI mode changes.
    UIModeController::getInstance()->monitorUIMode();

    // Render the system view if it's the currently displayed view, or if we're in the progress
    // of transitioning to or from this view.
    if (mSystemListView == mCurrentView || (mSystemViewTransition && isCameraMoving()))
        getSystemListView()->render(trans);

    // Draw the gamelists.
    for (auto it = mGameListViews.cbegin(); it != mGameListViews.cend(); it++) {
        // Same thing as for the system view, limit the rendering only to what needs to be drawn.
        if (it->second == mCurrentView || (it->second == mPreviousView && isCameraMoving())) {
            // Clipping.
            Vector3f guiStart = it->second->getPosition();
            Vector3f guiEnd = it->second->getPosition() + Vector3f(it->second->getSize().x(),
                    it->second->getSize().y(), 0);

            if (guiEnd.x() >= viewStart.x() && guiEnd.y() >= viewStart.y() &&
                    guiStart.x() <= viewEnd.x() && guiStart.y() <= viewEnd.y())
                it->second->render(trans);
        }
    }

    if (mWindow->peekGui() == this)
        mWindow->renderHelpPromptsEarly();

    // Fade out.
    if (mFadeOpacity) {
        unsigned int fadeColor = 0x00000000 | static_cast<unsigned char>(mFadeOpacity * 255);
        Renderer::setMatrix(parentTrans);
        Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(),
                Renderer::getScreenHeight(), fadeColor, fadeColor);
    }
}

void ViewController::preload()
{
    unsigned int systemCount = SystemData::sSystemVector.size();

    for (auto it = SystemData::sSystemVector.cbegin();
            it != SystemData::sSystemVector.cend(); it ++) {
        if (Settings::getInstance()->getBool("SplashScreen") &&
                Settings::getInstance()->getBool("SplashScreenProgress")) {
            mWindow->renderLoadingScreen("Loading '" + (*it)->getFullName() + "' (" +
                    std::to_string(std::distance(SystemData::sSystemVector.cbegin(), it)+1) +
                    "/" + std::to_string(systemCount) + ")");
        }
        (*it)->getIndex()->resetFilters();
        getGameListView(*it);
    }
    // Load navigation sounds, but only if at least one system exists.
    if (systemCount > 0)
        NavigationSounds::getInstance()->loadThemeNavigationSounds(
                SystemData::sSystemVector.front()->getTheme());
}

void ViewController::reloadGameListView(IGameListView* view, bool reloadTheme)
{
    for (auto it = mGameListViews.cbegin(); it != mGameListViews.cend(); it++) {
        if (it->second.get() == view) {
            bool isCurrent = (mCurrentView == it->second);
            SystemData* system = it->first;
            FileData* cursor = view->getCursor();
            mGameListViews.erase(it);

            if (reloadTheme)
                system->loadTheme();
            system->getIndex()->setUIModeFilters();
            std::shared_ptr<IGameListView> newView = getGameListView(system);

            // To counter having come from a placeholder.
            if (!cursor->isPlaceHolder()) {
                newView->setCursor(cursor);
            }
            if (isCurrent)
                mCurrentView = newView;

            break;
        }
    }

    #if defined(_WIN64)
    // This code is only needed for Windows, where we may need to keep ES running while
    // the game/emulator is in use. It's basically used to pause any playing game video
    // and to keep the screensaver from activating.
    if (Settings::getInstance()->getBool("RunInBackground")) {
        // If a game has been launched, then update all the GUI components to reflect this.
        if (mWindow->getGameLaunchedState())
            mWindow->setLaunchedGame();
    }
    #endif

    // Redisplay the current view.
    if (mCurrentView)
        mCurrentView->onShow();
}

void ViewController::reloadAll()
{
    // Clear all GameListViews.
    std::map<SystemData*, FileData*> cursorMap;
    for (auto it = mGameListViews.cbegin(); it != mGameListViews.cend(); it++)
        cursorMap[it->first] = it->second->getCursor();

    mGameListViews.clear();

    // Load themes, create GameListViews and reset filters.
    for (auto it = cursorMap.cbegin(); it != cursorMap.cend(); it++) {
        it->first->loadTheme();
        it->first->getIndex()->resetFilters();
        getGameListView(it->first)->setCursor(it->second);
    }

    // Rebuild SystemListView.
    mSystemListView.reset();
    getSystemListView();

    // Update mCurrentView since the pointers changed.
    if (mState.viewing == GAME_LIST) {
        mCurrentView = getGameListView(mState.getSystem());
    }
    else if (mState.viewing == SYSTEM_SELECT) {
        SystemData* system = mState.getSystem();
        goToSystemView(SystemData::sSystemVector.front(), false);
        mSystemListView->goToSystem(system, false);
        mCurrentView = mSystemListView;
        mCamera.r3().x() = 0;
    }
    else {
        goToSystemView(SystemData::sSystemVector.front(), false);
    }

    // Load navigation sounds.
    NavigationSounds::getInstance()->deinit();
    NavigationSounds::getInstance()->loadThemeNavigationSounds(
            SystemData::sSystemVector.front()->getTheme());

    mCurrentView->onShow();
    updateHelpPrompts();
}

std::vector<HelpPrompt> ViewController::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (!mCurrentView)
        return prompts;

    prompts = mCurrentView->getHelpPrompts();
    if (!(UIModeController::getInstance()->isUIModeKid() &&
            !Settings::getInstance()->getBool("ShowKidStartMenu")))
        prompts.push_back(HelpPrompt("start", "menu"));
    return prompts;
}

HelpStyle ViewController::getHelpStyle()
{
    if (!mCurrentView)
        return GuiComponent::getHelpStyle();

    return mCurrentView->getHelpStyle();
}

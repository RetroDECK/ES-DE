//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Window.h
//
//  Window management, screensaver management, and help prompts.
//  The input stack starts here as well, as this is the first instance called by InputManager.
//

#ifndef ES_CORE_WINDOW_H
#define ES_CORE_WINDOW_H

#include "HelpPrompt.h"
#include "InputConfig.h"
#include "Scripting.h"
#include "Settings.h"
#include "resources/TextureResource.h"

#include <atomic>
#include <memory>
#include <queue>

class FileData;
class Font;
class GuiComponent;
class GuiInfoPopup;
class HelpComponent;
class ImageComponent;
class InputConfig;
class TextCache;
struct HelpStyle;

class Window
{
public:
    class Screensaver
    {
    public:
        virtual bool allowSleep() = 0;
        virtual bool isScreensaverActive() = 0;
        virtual bool isFallbackScreensaver() = 0;

        virtual void startScreensaver(bool generateMediaList) = 0;
        virtual void stopScreensaver() = 0;
        virtual void nextGame() = 0;
        virtual void launchGame() = 0;
        virtual void goToGame() = 0;

        virtual void renderScreensaver() = 0;
        virtual void update(int deltaTime) = 0;

        virtual FileData* getCurrentGame() = 0;
        virtual void triggerNextGame() = 0;
    };

    class MediaViewer
    {
    public:
        virtual bool startMediaViewer(FileData* game) = 0;
        virtual void stopMediaViewer() = 0;

        virtual void showNext() = 0;
        virtual void showPrevious() = 0;

        virtual void update(int deltaTime) = 0;
        virtual void render(const glm::mat4& parentTrans) = 0;
    };

    class GuiLaunchScreen
    {
    public:
        virtual void displayLaunchScreen(FileData* game) = 0;
        virtual void closeLaunchScreen() = 0;
        virtual void update(int deltaTime) = 0;
        virtual void render(const glm::mat4& parentTrans) = 0;
    };

    static Window* getInstance();

    void pushGui(GuiComponent* gui);
    void removeGui(GuiComponent* gui);
    GuiComponent* peekGui();
    int getGuiStackSize() { return static_cast<int>(mGuiStack.size()); }
    bool isBackgroundDimmed();

    bool init();
    void deinit();

    void input(InputConfig* config, Input input);
    void textInput(const std::string& text);
    void logInput(InputConfig* config, Input input);
    void update(int deltaTime);
    void render();

    void normalizeNextUpdate() { mNormalizeNextUpdate = true; }

    bool getAllowSleep() { return mAllowSleep; }
    void setAllowSleep(bool sleep) { mAllowSleep = sleep; }
    bool isSleeping() const { return mSleeping; }

    void renderLoadingScreen(std::string text);
    // The list scroll overlay is triggered from IList when the highest scrolling tier is reached.
    void renderListScrollOverlay(unsigned char opacity, const std::string& text);

    void renderHelpPromptsEarly(); // Used to render HelpPrompts before a fade.
    void setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style);
    void reloadHelpPrompts();

    // GuiInfoPopup notifications.
    void queueInfoPopup(const std::string& message, const int& duration)
    {
        mInfoPopupQueue.emplace(std::make_pair(message, duration));
    }
    void stopInfoPopup();

    void startScreensaver();
    bool stopScreensaver();
    void renderScreensaver();
    void screensaverTriggerNextGame() { mScreensaver->triggerNextGame(); }
    void setScreensaver(Screensaver* screensaver) { mScreensaver = screensaver; }
    bool isScreensaverActive() { return mRenderScreensaver; }

    void startMediaViewer(FileData* game);
    void stopMediaViewer();
    void setMediaViewer(MediaViewer* mediaViewer) { mMediaViewer = mediaViewer; }
    bool isMediaViewerActive() { return mRenderMediaViewer; }

    void displayLaunchScreen(FileData* game);
    void closeLaunchScreen();
    void setLaunchScreen(GuiLaunchScreen* launchScreen) { mLaunchScreen = launchScreen; }
    bool isLaunchScreenDisplayed() { return mRenderLaunchScreen; }

    void increaseVideoPlayerCount();
    void decreaseVideoPlayerCount();
    int getVideoPlayerCount();

    void setLaunchedGame();
    void unsetLaunchedGame();
    void invalidateCachedBackground();
    bool isInvalidatingCachedBackground() { return mInvalidateCacheTimer > 0; }

    bool getGameLaunchedState() { return mGameLaunchedState; }
    void setAllowTextScrolling(bool value) { mAllowTextScrolling = value; }
    bool getAllowTextScrolling() { return mAllowTextScrolling; }

    // For Lottie animations.
    void setAllowFileAnimation(bool value) { mAllowFileAnimation = value; }
    bool getAllowFileAnimation() { return mAllowFileAnimation; }

    void setChangedThemeSet() { mChangedThemeSet = true; }
    bool getChangedThemeSet() { return mChangedThemeSet; }

private:
    Window() noexcept;
    ~Window();

    void onSleep() { Scripting::fireEvent("sleep"); }
    void onWake() { Scripting::fireEvent("wake"); }

    // Returns true if at least one component on the stack is processing.
    bool isProcessing();

    HelpComponent* mHelp;
    ImageComponent* mBackgroundOverlay;
    unsigned char mBackgroundOverlayOpacity;
    std::vector<GuiComponent*> mGuiStack;
    std::vector<std::shared_ptr<Font>> mDefaultFonts;
    std::unique_ptr<TextCache> mFrameDataText;

    Screensaver* mScreensaver;
    MediaViewer* mMediaViewer;
    GuiLaunchScreen* mLaunchScreen;
    GuiInfoPopup* mInfoPopup;

    std::queue<std::pair<std::string, int>> mInfoPopupQueue;

#if defined(USE_OPENGL_21)
    std::shared_ptr<TextureResource> mPostprocessedBackground;
#endif

    std::string mListScrollText;
    std::shared_ptr<Font> mListScrollFont;
    unsigned char mListScrollOpacity;

    int mFrameTimeElapsed;
    int mFrameCountElapsed;
    int mAverageDeltaTime;
    unsigned int mTimeSinceLastInput;

    bool mNormalizeNextUpdate;
    bool mAllowSleep;
    bool mSleeping;

    bool mRenderScreensaver;
    bool mRenderMediaViewer;
    bool mRenderLaunchScreen;
    bool mGameLaunchedState;
    bool mAllowTextScrolling;
    bool mAllowFileAnimation;
    bool mCachedBackground;
    bool mInvalidatedCachedBackground;
    bool mInitiateCacheTimer;
    int mInvalidateCacheTimer;

    std::atomic<int> mVideoPlayerCount;

    float mTopScale;
    bool mRenderedHelpPrompts;
    bool mChangedThemeSet;
};

#endif // ES_CORE_WINDOW_H

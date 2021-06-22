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

#include "resources/TextureResource.h"
#include "HelpPrompt.h"
#include "InputConfig.h"
#include "Settings.h"

#include <memory>
#include <mutex>

class FileData;
class Font;
class GuiComponent;
class HelpComponent;
class ImageComponent;
class InputConfig;
class TextCache;
class Transform4x4f;
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
        virtual void render() = 0;
    };

    class GuiLaunchScreen
    {
    public:
        virtual void displayLaunchScreen(FileData* game) = 0;
        virtual void closeLaunchScreen() = 0;
        virtual void update(int deltaTime) = 0;
        virtual void render() = 0;
    };

    class InfoPopup
    {
    public:
        virtual void render(const Transform4x4f& parentTrans) = 0;
        virtual void stop() = 0;
        virtual ~InfoPopup() {};
    };

    Window();
    ~Window();

    void pushGui(GuiComponent* gui);
    void removeGui(GuiComponent* gui);
    GuiComponent* peekGui();
    inline int getGuiStackSize() { return static_cast<int>(mGuiStack.size()); }

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
    inline bool isSleeping() const { return mSleeping; }

    void renderLoadingScreen(std::string text);
    // The list scroll overlay is triggered from IList when the highest scrolling tier is reached.
    void renderListScrollOverlay(unsigned char opacity, const std::string& text);

    void renderHelpPromptsEarly(); // Used to render HelpPrompts before a fade.
    void setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style);
    void reloadHelpPrompts();

    void setInfoPopup(InfoPopup* infoPopup);
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

    bool getGameLaunchedState() { return mGameLaunchedState; }
    void setAllowTextScrolling(bool setting) { mAllowTextScrolling = setting; }
    bool getAllowTextScrolling() { return mAllowTextScrolling; }

    void setChangedThemeSet() { mChangedThemeSet = true; }
    bool getChangedThemeSet() { return mChangedThemeSet; }

private:
    void onSleep();
    void onWake();

    // Returns true if at least one component on the stack is processing.
    bool isProcessing();

    HelpComponent* mHelp;
    ImageComponent* mBackgroundOverlay;
    unsigned char mBackgroundOverlayOpacity;
    Screensaver* mScreensaver;
    InfoPopup* mInfoPopup;
    std::vector<GuiComponent*> mGuiStack;
    std::vector<std::shared_ptr<Font>> mDefaultFonts;
    std::unique_ptr<TextCache> mFrameDataText;

    MediaViewer* mMediaViewer;
    bool mRenderMediaViewer;

    GuiLaunchScreen* mLaunchScreen;
    bool mRenderLaunchScreen;

    std::string mListScrollText;
    std::shared_ptr<Font> mListScrollFont;
    unsigned char mListScrollOpacity;

    bool mNormalizeNextUpdate;
    int mFrameTimeElapsed;
    int mFrameCountElapsed;
    int mAverageDeltaTime;
    bool mAllowSleep;
    bool mSleeping;
    unsigned int mTimeSinceLastInput;

    bool mRenderScreensaver;
    bool mGameLaunchedState;
    bool mAllowTextScrolling;
    bool mCachedBackground;
    bool mInvalidatedCachedBackground;

    int mVideoPlayerCount;
    std::mutex mVideoCountMutex;

    unsigned char mTopOpacity;
    float mTopScale;
    bool mRenderedHelpPrompts;
    bool mChangedThemeSet;
};

#endif // ES_CORE_WINDOW_H

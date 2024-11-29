//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  Window.h
//
//  Window management, screensaver management, help prompts and splash screen.
//  The input stack starts here as well, as this is the first instance called by InputManager.
//

#ifndef ES_CORE_WINDOW_H
#define ES_CORE_WINDOW_H

#include "GuiComponent.h"
#include "HelpPrompt.h"
#include "HelpStyle.h"
#include "InputConfig.h"
#include "Settings.h"
#include "components/HelpComponent.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "guis/GuiInfoPopup.h"
#include "resources/Font.h"
#include "resources/TextureResource.h"

#include <atomic>
#include <memory>
#include <queue>

class FileData;

class Window
{
public:
    class Screensaver
    {
    public:
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
        virtual void launchPDFViewer() = 0;

        virtual void input(InputConfig* config, Input input) = 0;
        virtual void update(int deltaTime) = 0;
        virtual void render(const glm::mat4& parentTrans) = 0;
        virtual std::vector<HelpPrompt> getHelpPrompts() = 0;
    };

    class PDFViewer
    {
    public:
        virtual bool startPDFViewer(FileData* game) = 0;
        virtual void stopPDFViewer() = 0;
        virtual void launchMediaViewer() = 0;

        virtual void input(InputConfig* config, Input input) = 0;
        virtual void update(int deltaTime) = 0;
        virtual void render(const glm::mat4& parentTrans) = 0;
        virtual std::vector<HelpPrompt> getHelpPrompts() = 0;
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

    bool init(bool resized = false);
    void deinit();

    void input(InputConfig* config, Input input);
    void textInput(const std::string& text, const bool pasting = false);
    void logInput(InputConfig* config, Input input);
    void update(int deltaTime);
    void render();

    void setBlockInput(const bool state) { mBlockInput = state; }
    void normalizeNextUpdate() { mNormalizeNextUpdate = true; }

    enum class SplashScreenState {
        SCANNING,
        POPULATING,
        RELOADING,
        RESOURCE_COPY,
        DIR_CREATION
    };

    void updateSplashScreenText();
    void renderSplashScreen(SplashScreenState state, float progress);
    // The list scroll overlay is triggered from IList when the highest scrolling tier is reached.
    void renderListScrollOverlay(const float opacity, const std::string& text);

    void renderHelpPromptsEarly(); // Used to render HelpPrompts before a fade.
    void setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style);

    // GuiInfoPopup notifications.
    void queueInfoPopup(const std::string& message, const int& duration)
    {
        mInfoPopupQueue.emplace(std::make_pair(message, duration));
    }
    void stopInfoPopup();

    void startScreensaver(bool onTimer);
    bool stopScreensaver();
    void screensaverTriggerNextGame() { mScreensaver->triggerNextGame(); }
    void setScreensaver(Screensaver* screensaver) { mScreensaver = screensaver; }
    bool isScreensaverActive() { return mRenderScreensaver; }

    void startMediaViewer(FileData* game);
    void stopMediaViewer();
    void setMediaViewer(MediaViewer* mediaViewer) { mMediaViewer = mediaViewer; }
    bool isMediaViewerActive() { return mRenderMediaViewer; }

    void startPDFViewer(FileData* game);
    void stopPDFViewer();
    void setPDFViewer(PDFViewer* pdfViewer) { mPDFViewer = pdfViewer; }
    bool isPDFViewerActive() { return mRenderPDFViewer; }

    void displayLaunchScreen(FileData* game);
    void closeLaunchScreen();
    void setLaunchScreen(GuiLaunchScreen* launchScreen) { mLaunchScreen = launchScreen; }
    bool isLaunchScreenDisplayed() { return mRenderLaunchScreen; }

    void increaseVideoPlayerCount() { ++mVideoPlayerCount; }
    void decreaseVideoPlayerCount() { --mVideoPlayerCount; }
    int getVideoPlayerCount();

    void setLaunchedGame(bool state) { mGameLaunchedState = state; }
    void invalidateCachedBackground();
    bool isInvalidatingCachedBackground() { return mInvalidateCacheTimer > 0; }

    std::vector<std::string>& getGameEndEventParams() { return mGameEndEventParams; }
    bool getGameLaunchedState() { return mGameLaunchedState; }
    void setAllowTextScrolling(bool value) { mAllowTextScrolling = value; }
    bool getAllowTextScrolling() { return mAllowTextScrolling; }

    // For GIF and Lottie animations.
    void setAllowFileAnimation(bool value) { mAllowFileAnimation = value; }
    bool getAllowFileAnimation() { return mAllowFileAnimation; }

    void setChangedTheme() { mChangedTheme = true; }
    bool getChangedTheme() { return mChangedTheme; }

private:
    Window() noexcept;
    ~Window();

    // Returns true if at least one component on the stack is processing.
    bool isProcessing();

    struct ProgressBarRectangle {
        float barWidth;
        float barHeight;
        float barPosX;
        float barPosY;
        unsigned int color;
    };

    Renderer* mRenderer;
    std::unique_ptr<HelpComponent> mHelp;
    std::unique_ptr<ImageComponent> mBackgroundOverlay;
    std::unique_ptr<ImageComponent> mSplash;
    std::unique_ptr<TextComponent> mSplashTextScanning;
    std::unique_ptr<TextComponent> mSplashTextPopulating;
    std::unique_ptr<TextComponent> mSplashTextReloading;
    std::unique_ptr<TextComponent> mSplashTextResourceCopy;
    std::unique_ptr<TextComponent> mSplashTextDirCreation;

    glm::vec4 mSplashTextPositions;
    std::vector<ProgressBarRectangle> mProgressBarRectangles;

    float mBackgroundOverlayOpacity;
    std::vector<GuiComponent*> mGuiStack;
    std::vector<std::shared_ptr<Font>> mDefaultFonts;
    std::unique_ptr<TextComponent> mGPUStatisticsText;

    Screensaver* mScreensaver;
    MediaViewer* mMediaViewer;
    PDFViewer* mPDFViewer;
    GuiLaunchScreen* mLaunchScreen;
    GuiInfoPopup* mInfoPopup;

    std::queue<std::pair<std::string, int>> mInfoPopupQueue;
    std::shared_ptr<TextureResource> mPostprocessedBackground;

    std::vector<std::string> mGameEndEventParams;
    std::unique_ptr<TextComponent> mListScrollText;
    float mListScrollOpacity;

    int mFrameTimeElapsed;
    int mFrameCountElapsed;
    int mAverageDeltaTime;
    unsigned int mTimeSinceLastInput;

    bool mBlockInput;
    bool mNormalizeNextUpdate;

    bool mRenderScreensaver;
    bool mRenderMediaViewer;
    bool mRenderLaunchScreen;
    bool mRenderPDFViewer;
    std::atomic<bool> mGameLaunchedState;
    std::atomic<bool> mAllowTextScrolling;
    std::atomic<bool> mAllowFileAnimation;
    bool mCachedBackground;
    bool mInvalidatedCachedBackground;
    bool mInitiateCacheTimer;
    int mInvalidateCacheTimer;

    std::atomic<int> mVideoPlayerCount;

    float mTopScale;
    bool mRenderedHelpPrompts;
    bool mChangedTheme;
};

#endif // ES_CORE_WINDOW_H

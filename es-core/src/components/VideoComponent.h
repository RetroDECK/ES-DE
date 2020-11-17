//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoComponent.h
//
//  Base class for playing videos.
//

#ifndef ES_CORE_COMPONENTS_VIDEO_COMPONENT_H
#define ES_CORE_COMPONENTS_VIDEO_COMPONENT_H

#include "components/ImageComponent.h"
#include "GuiComponent.h"

#include <string>

class TextureResource;

class VideoComponent : public GuiComponent
{
    // Structure that groups together the configuration of the video component.
    struct Configuration {
        unsigned startDelay;
        bool showSnapshotNoVideo;
        bool showSnapshotDelay;
        std::string defaultVideoPath;
    };

public:
    VideoComponent(Window* window);
    virtual ~VideoComponent();

    // Loads the video at the given filepath.
    bool setVideo(std::string path);
    // Configures the component to show the default video.
    void setDefaultVideo();
    // Loads a static image that is displayed if the video cannot be played.
    void setImage(std::string path);
    // Sets whether it's going to render in screensaver mode.
    void setScreensaverMode(bool isScreensaver);
    // Set the opacity for the embedded static image.
    void setOpacity(unsigned char opacity) override;

    virtual void onShow() override;
    virtual void onHide() override;
    virtual void onPauseVideo() override;
    virtual void onUnpauseVideo() override;
    virtual bool isVideoPaused() override { return mPause; }
    virtual void onScreensaverActivate() override;
    virtual void onScreensaverDeactivate() override;
    virtual void onGameLaunchedActivate() override;
    virtual void onGameLaunchedDeactivate() override;
    virtual void topWindow(bool isTop) override;

    // These functions update the embedded static image.
    void onOriginChanged() override;
    void onPositionChanged() override;
    void onSizeChanged() override;

    void render(const Transform4x4f& parentTrans) override;
    void renderSnapshot(const Transform4x4f& parentTrans);

    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view,
            const std::string& element, unsigned int properties) override;

    virtual std::vector<HelpPrompt> getHelpPrompts() override;

    virtual void update(int deltaTime) override;

    // Resize the video to fit this size. If one axis is zero, scale that axis to maintain
    // aspect ratio. If both are non-zero, potentially break the aspect ratio. If both are
    // zero, no resizing. This can be set before or after a video is loaded.
    // setMaxSize() and setResize() are mutually exclusive.
    virtual void setResize(float width, float height) = 0;
    inline void setResize(const Vector2f& size) { setResize(size.x(), size.y()); }

    // Resize the video to be as large as possible but fit within a box of this size.
    // This can be set before or after a video is loaded.
    // Never breaks the aspect ratio. setMaxSize() and setResize() are mutually exclusive.
    virtual void setMaxSize(float width, float height) = 0;
    inline void setMaxSize(const Vector2f& size) { setMaxSize(size.x(), size.y()); }

private:
    // Start the video immediately.
    virtual void startVideo() {};
    // Stop the video.
    virtual void stopVideo() {};
    // Pause the video when a game has been launched.
    virtual void pauseVideo() {};
    // Handle looping of the video. Must be called periodically.
    virtual void handleLooping() {};

    // Start the video after any configured delay.
    void startVideoWithDelay();
    // Handle any delay to the start of playing the video clip. Must be called periodically.
    void handleStartDelay();
    // Manage the playing state of the component.
    void manageState();

protected:
    Window* mWindow;
    unsigned mVideoWidth;
    unsigned mVideoHeight;
    Vector2f mTargetSize;
    Vector2f mVideoAreaPos;
    Vector2f mVideoAreaSize;
    std::shared_ptr<TextureResource> mTexture;
    std::string mStaticImagePath;
    ImageComponent mStaticImage;

    std::string mVideoPath;
    std::string mPlayingVideoPath;
    unsigned mStartTime;
    bool mStartDelayed;
    bool mIsPlaying;
    bool mIsActuallyPlaying;
    bool mPause;
    bool mShowing;
    bool mDisable;
    bool mScreensaverActive;
    bool mScreensaverMode;
    bool mGameLaunched;
    bool mBlockPlayer;
    bool mTargetIsMax;
    float mFadeIn; // Used for fading in the video screensaver.

    Configuration mConfig;
};

#endif // ES_CORE_COMPONENTS_VIDEO_COMPONENT_H

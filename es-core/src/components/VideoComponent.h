//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoComponent.h
//
//  Base class for playing videos.
//

#ifndef ES_CORE_COMPONENTS_VIDEO_COMPONENT_H
#define ES_CORE_COMPONENTS_VIDEO_COMPONENT_H

#include "GuiComponent.h"
#include "components/ImageComponent.h"

#include <atomic>
#include <string>

class MediaViewer;
class TextureResource;

class VideoComponent : public GuiComponent
{
    // Structure that groups together the configuration of the video component.
    struct Configuration {
        unsigned startDelay;
        bool showSnapshotNoVideo;
        bool showSnapshotDelay;
        std::string defaultVideoPath;
        std::string staticVideoPath;
    };

public:
    VideoComponent();
    virtual ~VideoComponent();

    // Loads the video at the given filepath.
    bool setVideo(std::string path);
    // Configures the component to show the default video.
    void setDefaultVideo() { setVideo(mConfig.defaultVideoPath); }
    // Configures the component to show the static video.
    void setStaticVideo() { setVideo(mConfig.staticVideoPath); }
    // Loads a static image that is displayed if the video cannot be played.
    void setImage(const std::string& path, bool tile = false, bool linearMagnify = false);
    // Sets whether we're in media viewer mode.
    void setMediaViewerMode(bool isMediaViewer) { mMediaViewerMode = isMediaViewer; }
    // Sets whether we're in screensaver mode.
    void setScreensaverMode(bool isScreensaver) { mScreensaverMode = isScreensaver; }
    // Set the opacity for the embedded static image.
    void setOpacity(unsigned char opacity) override { mOpacity = opacity; }

    bool hasStaticVideo() { return !mConfig.staticVideoPath.empty(); }
    bool hasStaticImage() { return mStaticImage.getTextureSize() != glm::ivec2 {0, 0}; }

    void onShow() override;
    void onHide() override;
    void onStopVideo() override;
    void onPauseVideo() override;
    void onUnpauseVideo() override;
    bool isVideoPaused() override { return mPause; }
    void onScreensaverActivate() override;
    void onScreensaverDeactivate() override;
    void onGameLaunchedActivate() override;
    void onGameLaunchedDeactivate() override;
    void topWindow(bool isTop) override;

    // These functions update the embedded static image.
    void onOriginChanged() override { mStaticImage.setOrigin(mOrigin); }
    void onPositionChanged() override { mStaticImage.setPosition(mPosition); }
    void onSizeChanged() override { mStaticImage.onSizeChanged(); }

    void render(const glm::mat4& parentTrans) override;
    void renderSnapshot(const glm::mat4& parentTrans);

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    std::vector<HelpPrompt> getHelpPrompts() override;

    void update(int deltaTime) override;

    // Resize the video to fit this size. If one axis is zero, scale that axis to maintain
    // aspect ratio. If both are non-zero, potentially break the aspect ratio. If both are
    // zero, no resizing. This can be set before or after a video is loaded.
    // setMaxSize() and setResize() are mutually exclusive.
    virtual void setResize(float width, float height) override = 0;
    void setResize(const glm::vec2& size) { setResize(size.x, size.y); }

    // Resize the video to be as large as possible but fit within a box of this size.
    // This can be set before or after a video is loaded.
    // Never breaks the aspect ratio. setMaxSize() and setResize() are mutually exclusive.
    virtual void setMaxSize(float width, float height) = 0;
    void setMaxSize(const glm::vec2& size) { setMaxSize(size.x, size.y); }

private:
    // Start the video immediately.
    virtual void startVideo() {}
    // Stop the video.
    virtual void stopVideo() {}
    // Pause the video when a game has been launched.
    virtual void pauseVideo() {}
    // Handle looping of the video. Must be called periodically.
    virtual void handleLooping() {}
    virtual void updatePlayer() {}

    // Start the video after any configured delay.
    void startVideoWithDelay();
    // Handle any delay to the start of playing the video clip. Must be called periodically.
    void handleStartDelay();
    // Manage the playing state of the component.
    void manageState();

    friend MediaViewer;

protected:
    ImageComponent mStaticImage;

    unsigned mVideoWidth;
    unsigned mVideoHeight;
    glm::vec2 mTargetSize;
    glm::vec2 mVideoAreaPos;
    glm::vec2 mVideoAreaSize;
    std::shared_ptr<TextureResource> mTexture;
    std::string mStaticImagePath;

    std::string mVideoPath;
    std::string mPlayingVideoPath;
    unsigned mStartTime;
    bool mStartDelayed;
    std::atomic<bool> mIsPlaying;
    std::atomic<bool> mIsActuallyPlaying;
    std::atomic<bool> mPause;
    bool mShowing;
    bool mDisable;
    bool mMediaViewerMode;
    bool mScreensaverActive;
    bool mScreensaverMode;
    bool mGameLaunched;
    bool mBlockPlayer;
    bool mTargetIsMax;
    float mFadeIn; // Used for fading in the video screensaver.

    Configuration mConfig;
};

#endif // ES_CORE_COMPONENTS_VIDEO_COMPONENT_H

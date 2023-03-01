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
    void setImage(const std::string& path, bool tile = false) override;
    // Same as setImage() but does not set the default image if the path argument is empty.
    void setImageNoDefault(const std::string& path);
    // Sets whether we're in media viewer mode.
    void setMediaViewerMode(bool isMediaViewer) { mMediaViewerMode = isMediaViewer; }
    // Sets whether we're in screensaver mode.
    void setScreensaverMode(bool isScreensaver) { mScreensaverMode = isScreensaver; }
    // Set the opacity for the embedded static image.
    void setOpacity(float opacity) override { mOpacity = opacity; }
    // Set whether to draw black pillarboxes/letterboxes behind videos.
    void setDrawPillarboxes(bool state) { mDrawPillarboxes = state; }
    // Whether to fade out the entire video surface including the black rectangle.
    void setGeneralFade(bool state) { mGeneralFade = state; }

    bool hasStaticVideo() { return !mConfig.staticVideoPath.empty(); }
    bool hasStaticImage() { return mStaticImage.getTextureSize() != glm::ivec2 {0, 0}; }
    bool hasStartDelay()
    {
        if (mLegacyTheme)
            return mConfig.showSnapshotDelay && mConfig.startDelay > 0;
        else
            return mConfig.startDelay > 0;
    }

    // These functions update the embedded static image.
    void onOriginChanged() override { mStaticImage.setOrigin(mOrigin); }
    void onPositionChanged() override { mStaticImage.setPosition(mPosition); }
    void onSizeChanged() override { mStaticImage.onSizeChanged(); }

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

    std::vector<HelpPrompt> getHelpPrompts() override;

    void update(int deltaTime) override;

    // Resize the video to be as large as possible but fit within a box of this size.
    // This can be set before or after a video is loaded.
    // Never breaks the aspect ratio. setMaxSize() and setResize() are mutually exclusive.
    virtual void setMaxSize(float width, float height) = 0;
    void setMaxSize(const glm::vec2& size) { setMaxSize(size.x, size.y); }
    // Resize and crop the video so it fills the entire area.
    virtual void setCroppedSize(const glm::vec2& size) = 0;

    // Basic video controls.
    void startVideoPlayer();
    virtual void stopVideoPlayer(bool muteAudio = true) {}
    virtual void pauseVideoPlayer() {}

    // Handle looping of the video. Must be called periodically.
    virtual void handleLooping() {}
    // Used to immediately mute audio even if there are still samples to play in the buffer.
    virtual void muteVideoPlayer() {}
    virtual void updatePlayer() {}

protected:
    virtual void startVideoStream() {}
    void renderSnapshot(const glm::mat4& parentTrans);

    ImageComponent mStaticImage;

    unsigned mVideoWidth;
    unsigned mVideoHeight;
    unsigned int mColorShift;
    unsigned int mColorShiftEnd;
    bool mColorGradientHorizontal;
    glm::vec2 mTargetSize;
    glm::vec2 mVideoAreaPos;
    glm::vec2 mVideoAreaSize;
    glm::vec2 mTopLeftCrop;
    glm::vec2 mBottomRightCrop;
    glm::vec2 mPillarboxThreshold;
    std::shared_ptr<TextureResource> mTexture;
    std::string mStaticImagePath;
    std::string mDefaultImagePath;

    static inline std::vector<std::string> supportedImageTypes {
        "image", "miximage",  "marquee", "screenshot",    "titlescreen",
        "cover", "backcover", "3dbox",   "physicalmedia", "fanart"};

    std::string mVideoPath;
    unsigned mStartTime;
    std::atomic<bool> mIsPlaying;
    std::atomic<bool> mIsActuallyPlaying;
    std::atomic<bool> mPaused;
    bool mMediaViewerMode;
    bool mScreensaverMode;
    bool mTargetIsMax;
    bool mTargetIsCrop;
    bool mPlayAudio;
    bool mDrawPillarboxes;
    bool mRenderScanlines;
    bool mLegacyTheme;
    bool mHasVideo;
    bool mGeneralFade;
    float mFadeIn;
    float mFadeInTime;

    Configuration mConfig;
};

#endif // ES_CORE_COMPONENTS_VIDEO_COMPONENT_H

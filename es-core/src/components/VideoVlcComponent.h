//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoVlcComponent.h
//
//  Video playing using libVLC.
//

#ifndef ES_CORE_COMPONENTS_VIDEO_VLC_COMPONENT_H
#define ES_CORE_COMPONENTS_VIDEO_VLC_COMPONENT_H

#include "VideoComponent.h"

#include <vlc/vlc.h>

struct SDL_mutex;
struct SDL_Surface;
struct libvlc_instance_t;
struct libvlc_media_t;
struct libvlc_media_player_t;

struct VideoContext {
    SDL_Surface* surface;
    SDL_mutex* mutex;
    bool valid;
};

class VideoVlcComponent : public VideoComponent
{
public:
    VideoVlcComponent(Window* window);
    virtual ~VideoVlcComponent();

    // Resize the video to fit this size. If one axis is zero, scale that axis to maintain
    // aspect ratio. If both are non-zero, potentially break the aspect ratio. If both are
    // zero, no resizing. This can be set before or after a video is loaded.
    // setMaxSize() and setResize() are mutually exclusive.
    void setResize(float width, float height) override;

    // Resize the video to be as large as possible but fit within a box of this size.
    // This can be set before or after a video is loaded.
    // Never breaks the aspect ratio. setMaxSize() and setResize() are mutually exclusive.
    void setMaxSize(float width, float height) override;

private:
    static void setupVLC();
    void setupContext();
    void freeContext();

    // Calculates the correct mSize from our resizing information (set by setResize/setMaxSize).
    // Used internally whenever the resizing parameters or texture change.
    void resize();

    void render(const Transform4x4f& parentTrans) override;
    void calculateBlackRectangle();

    // Start the video immediately.
    virtual void startVideo() override;
    // Stop the video.
    virtual void stopVideo() override;
    // Pause the video when a game has been launched.
    virtual void pauseVideo() override;
    // Handle looping the video. Must be called periodically.
    virtual void handleLooping() override;

    static void VlcMediaParseCallback(const libvlc_event_t *event, void *user_data) {};

    static VideoVlcComponent* sInstance;
    static libvlc_instance_t* mVLC;
    libvlc_media_t* mMedia;
    libvlc_media_player_t* mMediaPlayer;
    VideoContext mContext;
    std::shared_ptr<TextureResource> mTexture;
    std::vector<float> mVideoRectangleCoords;
};

#endif // ES_CORE_COMPONENTS_VIDEO_VLC_COMPONENT_H

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoFFmpegComponent.h
//
//  Video player based on FFmpeg.
//

#ifndef ES_CORE_COMPONENTS_VIDEO_FFMPEG_COMPONENT_H
#define ES_CORE_COMPONENTS_VIDEO_FFMPEG_COMPONENT_H

#define VIDEO_FRAME_QUEUE_SIZE 3

#include "VideoComponent.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <chrono>
#include <queue>

class VideoFFmpegComponent : public VideoComponent
{
public:
    VideoFFmpegComponent(Window* window);
    virtual ~VideoFFmpegComponent();

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
    // Calculates the correct mSize from our resizing information (set by setResize/setMaxSize).
    // Used internally whenever the resizing parameters or texture change.
    void resize();

    void render(const Transform4x4f& parentTrans) override;
    void update(int deltaTime) override;

    void readFrames();
    void processFrames();

    void calculateBlackRectangle();

    // Start the video immediately.
    virtual void startVideo() override;
    // Stop the video.
    virtual void stopVideo() override;
    // Pause the video when a game has been launched.
    virtual void pauseVideo() override;
    // Handle looping the video. Must be called periodically.
    virtual void handleLooping() override;

    std::shared_ptr<TextureResource> mTexture;
    std::vector<float> mVideoRectangleCoords;

    AVFormatContext* mFormatContext;
    AVStream* mVideoStream;
    AVStream* mAudioStream;
    AVCodec *mVideoCodec;
    AVCodec *mAudioCodec;
    AVCodecContext* mVideoCodecContext;
    AVCodecContext* mAudioCodecContext;
    int mVideoStreamIndex;
    int mAudioStreamIndex;

    AVPacket* mPacket;
    AVFrame* mVideoFrame;
    AVFrame* mAudioFrame;

    struct VideoFrame {
        std::vector<uint8_t> frameRGBA;
        int width;
        int height;
        double pts;
    };

    struct AudioFrame {
        std::vector<uint8_t> resampledData;
        int resampledDataSize;
        double pts;
    };

    std::queue<VideoFrame> mVideoFrameQueue;
    std::queue<AudioFrame> mAudioFrameQueue;

    int mVideoMinQueueSize;
    int mAudioMinQueueSize;
    double mVideoTimeBase;

    // Used for audio and video synchronization.
    std::chrono::high_resolution_clock::time_point mTimeReference;

    double mAccumulatedTime;
    bool mStartTimeAccumulation;
    bool mDecodedFrame;
    bool mEndOfVideo;

    // These are only used for debugging.
    int mAudioFrameCount;
    int mVideoFrameCount;
};

#endif // ES_CORE_COMPONENTS_VIDEO_FFMPEG_COMPONENT_H

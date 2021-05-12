//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoFFmpegComponent.h
//
//  Video player based on FFmpeg.
//

#ifndef ES_CORE_COMPONENTS_VIDEO_FFMPEG_COMPONENT_H
#define ES_CORE_COMPONENTS_VIDEO_FFMPEG_COMPONENT_H

// Audio buffer in seconds.
#define AUDIO_BUFFER 0.2l

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
#include <mutex>
#include <queue>
#include <thread>

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

    // This will run the frame processing in a separate thread.
    void frameProcessing();
    // Read frames from the video file and perform format conversion.
    void readFrames();
    // Output frames to AudioManager and to the video surface.
    void outputFrames();

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

    std::unique_ptr<std::thread> mFrameProcessingThread;
    std::mutex mPictureMutex;

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

    struct OutputPicture {
        std::vector<uint8_t> pictureRGBA;
        int width = 0;
        int height = 0;
    };

    std::queue<VideoFrame> mVideoFrameQueue;
    std::queue<AudioFrame> mAudioFrameQueue;
    OutputPicture mOutputPicture;

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

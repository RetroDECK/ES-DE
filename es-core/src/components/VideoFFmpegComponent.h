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
#define AUDIO_BUFFER 0.1l

#include "VideoComponent.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
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

    void render(const glm::mat4& parentTrans) override;
    virtual void updatePlayer() override;

    // This will run the frame processing in a separate thread.
    void frameProcessing();
    // Setup libavfilter.
    bool setupVideoFilters();
    bool setupAudioFilters();

    // Read frames from the video file and add them to the filter source.
    void readFrames();
    // Get the frames that have been processed by the filters.
    void getProcessedFrames();
    // Output frames to AudioManager and to the video surface (via the main thread).
    void outputFrames();

    // Calculate the black rectangle that is shown behind videos with non-standard aspect ratios.
    void calculateBlackRectangle();

    // Detect and initialize the hardware decoder.
    static void detectHWDecoder();
    bool decoderInitHW();

    // Start the video immediately.
    virtual void startVideo() override;
    // Stop the video.
    virtual void stopVideo() override;
    // Pause the video when a game has been launched.
    virtual void pauseVideo() override;
    // Handle looping the video. Must be called periodically.
    virtual void handleLooping() override;

    static enum AVHWDeviceType sDeviceType;
    static enum AVPixelFormat sPixelFormat;
    static std::vector<std::string> sSWDecodedVideos;
    static std::vector<std::string> sHWDecodedVideos;

    std::shared_ptr<TextureResource> mTexture;
    std::vector<float> mVideoRectangleCoords;

    std::unique_ptr<std::thread> mFrameProcessingThread;
    std::mutex mPictureMutex;
    std::mutex mAudioMutex;

    AVFormatContext* mFormatContext;
    AVStream* mVideoStream;
    AVStream* mAudioStream;
    AVCodec* mVideoCodec;
    AVCodec* mAudioCodec;
    AVCodec* mHardwareCodec;
    AVBufferRef* mHwContext;
    AVCodecContext* mVideoCodecContext;
    AVCodecContext* mAudioCodecContext;
    int mVideoStreamIndex;
    int mAudioStreamIndex;

    AVPacket* mPacket;
    AVFrame* mVideoFrame;
    AVFrame* mVideoFrameResampled;
    AVFrame* mAudioFrame;
    AVFrame* mAudioFrameResampled;

    struct VideoFrame {
        std::vector<uint8_t> frameRGBA;
        int width;
        int height;
        double pts;
        double frameDuration;
    };

    struct AudioFrame {
        std::vector<uint8_t> resampledData;
        double pts;
    };

    struct OutputPicture {
        std::vector<uint8_t> pictureRGBA;
        bool hasBeenRendered;
        int width;
        int height;
    };

    std::queue<VideoFrame> mVideoFrameQueue;
    std::queue<AudioFrame> mAudioFrameQueue;
    OutputPicture mOutputPicture;
    std::vector<uint8_t> mOutputAudio;

    AVFilterContext* mVBufferSrcContext;
    AVFilterContext* mVBufferSinkContext;
    AVFilterGraph* mVFilterGraph;
    AVFilterInOut* mVFilterInputs;
    AVFilterInOut* mVFilterOutputs;

    AVFilterContext* mABufferSrcContext;
    AVFilterContext* mABufferSinkContext;
    AVFilterGraph* mAFilterGraph;
    AVFilterInOut* mAFilterInputs;
    AVFilterInOut* mAFilterOutputs;

    int mVideoTargetQueueSize;
    int mAudioTargetQueueSize;
    double mVideoTimeBase;

    // Used for audio and video synchronization.
    std::chrono::high_resolution_clock::time_point mTimeReference;

    int mAudioFrameCount;
    int mVideoFrameCount;

    double mAccumulatedTime;
    bool mStartTimeAccumulation;
    bool mDecodedFrame;
    bool mEndOfVideo;
    bool mSWDecoder;
};

#endif // ES_CORE_COMPONENTS_VIDEO_FFMPEG_COMPONENT_H

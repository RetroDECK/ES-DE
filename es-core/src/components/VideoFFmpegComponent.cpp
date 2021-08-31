//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoFFmpegComponent.cpp
//
//  Video player based on FFmpeg.
//

#define DEBUG_VIDEO false

#include "components/VideoFFmpegComponent.h"

#include "AudioManager.h"
#include "Settings.h"
#include "Window.h"
#include "resources/TextureResource.h"

#include <algorithm>

enum AVHWDeviceType VideoFFmpegComponent::sDeviceType = AV_HWDEVICE_TYPE_NONE;
enum AVPixelFormat VideoFFmpegComponent::sPixelFormat = AV_PIX_FMT_NONE;
std::vector<std::string> VideoFFmpegComponent::sHWDecodedVideos;
std::vector<std::string> VideoFFmpegComponent::sSWDecodedVideos;

VideoFFmpegComponent::VideoFFmpegComponent(Window* window)
    : VideoComponent(window)
    , mFrameProcessingThread(nullptr)
    , mFormatContext(nullptr)
    , mVideoStream(nullptr)
    , mAudioStream(nullptr)
    , mVideoCodec(nullptr)
    , mAudioCodec(nullptr)
    , mHardwareCodec(nullptr)
    , mHwContext(nullptr)
    , mVideoCodecContext(nullptr)
    , mAudioCodecContext(nullptr)
    , mVBufferSrcContext(nullptr)
    , mVBufferSinkContext(nullptr)
    , mVFilterGraph(nullptr)
    , mVFilterInputs(nullptr)
    , mVFilterOutputs(nullptr)
    , mABufferSrcContext(nullptr)
    , mABufferSinkContext(nullptr)
    , mAFilterGraph(nullptr)
    , mAFilterInputs(nullptr)
    , mAFilterOutputs(nullptr)
    , mVideoTimeBase(0.0l)
    , mVideoTargetQueueSize(0)
    , mAudioTargetQueueSize(0)
    , mAccumulatedTime(0)
    , mStartTimeAccumulation(false)
    , mDecodedFrame(false)
    , mEndOfVideo(false)
{
}

VideoFFmpegComponent::~VideoFFmpegComponent() { stopVideo(); }

void VideoFFmpegComponent::setResize(float width, float height)
{
    // This resize function is used when stretching videos to full screen in the video screensaver.
    mTargetSize = glm::vec2{width, height};
    mTargetIsMax = false;
    mStaticImage.setResize(width, height);
    resize();
}

void VideoFFmpegComponent::setMaxSize(float width, float height)
{
    // This resize function is used in most instances, such as non-stretched video screensaver
    // and the gamelist videos.
    mTargetSize = glm::vec2{width, height};
    mTargetIsMax = true;
    mStaticImage.setMaxSize(width, height);
    resize();
}

void VideoFFmpegComponent::resize()
{
    if (!mTexture)
        return;

    const glm::vec2 textureSize{static_cast<float>(mVideoWidth), static_cast<float>(mVideoHeight)};

    if (textureSize == glm::vec2{})
        return;

    if (mTargetIsMax) {
        mSize = textureSize;

        glm::vec2 resizeScale{(mTargetSize.x / mSize.x), (mTargetSize.y / mSize.y)};

        if (resizeScale.x < resizeScale.y) {
            mSize.x *= resizeScale.x;
            mSize.y *= resizeScale.x;
        }
        else {
            mSize.x *= resizeScale.y;
            mSize.y *= resizeScale.y;
        }

        mSize.y = std::round(mSize.y);
        mSize.x = (mSize.y / textureSize.y) * textureSize.x;
    }
    else {
        // If both components are set, we just stretch.
        // If no components are set, we don't resize at all.
        mSize = mTargetSize == glm::vec2{} ? textureSize : mTargetSize;

        // If only one component is set, we resize in a way that maintains aspect ratio.
        if (!mTargetSize.x && mTargetSize.y) {
            mSize.y = std::round(mTargetSize.y);
            mSize.x = (mSize.y / textureSize.y) * textureSize.x;
        }
        else if (mTargetSize.x && !mTargetSize.y) {
            mSize.y = std::round((mTargetSize.x / textureSize.x) * textureSize.y);
            mSize.x = (mSize.y / textureSize.y) * textureSize.x;
        }
    }

    onSizeChanged();
}

void VideoFFmpegComponent::render(const glm::mat4& parentTrans)
{
    VideoComponent::render(parentTrans);
    glm::mat4 trans{parentTrans * getTransform()};
    GuiComponent::renderChildren(trans);

    if (mIsPlaying && mFormatContext) {
        unsigned int color;
        if (mDecodedFrame && mFadeIn < 1) {
            const unsigned int fadeIn = static_cast<int>(mFadeIn * 255.0f);
            color =
                Renderer::convertRGBAToABGR((fadeIn << 24) | (fadeIn << 16) | (fadeIn << 8) | 255);
        }
        else {
            color = 0xFFFFFFFF;
        }
        Renderer::Vertex vertices[4];
        Renderer::setMatrix(parentTrans);

        // Render the black rectangle behind the video.
        if (mVideoRectangleCoords.size() == 4) {
            Renderer::drawRect(mVideoRectangleCoords[0], mVideoRectangleCoords[1],
                               mVideoRectangleCoords[2], mVideoRectangleCoords[3], // Line break.
                               0x000000FF, 0x000000FF);
        }

        // clang-format off
        vertices[0] = {{0.0f,    0.0f   }, {0.0f, 0.0f}, color};
        vertices[1] = {{0.0f,    mSize.y}, {0.0f, 1.0f}, color};
        vertices[2] = {{mSize.x, 0.0f   }, {1.0f, 0.0f}, color};
        vertices[3] = {{mSize.x, mSize.y}, {1.0f, 1.0f}, color};
        // clang-format on

        // Round vertices.
        for (int i = 0; i < 4; i++)
            vertices[i].pos = glm::round(vertices[i].pos);

        // This is needed to avoid a slight gap before the video starts playing.
        if (!mDecodedFrame)
            return;

        mPictureMutex.lock();

        if (!mOutputPicture.hasBeenRendered) {
            // Copy the contents of mOutputPicture to a temporary vector in order to call
            // initFromPixels() only after the mutex unlock. This significantly reduces the
            // lock waits in outputFrames().
            size_t pictureSize = mOutputPicture.pictureRGBA.size();
            std::vector<uint8_t> tempPictureRGBA(pictureSize);
            int pictureWidth = 0;
            int pictureHeight = 0;

            if (pictureSize > 0) {
                tempPictureRGBA.insert(tempPictureRGBA.begin(), mOutputPicture.pictureRGBA.begin(),
                                       mOutputPicture.pictureRGBA.end());
                pictureWidth = mOutputPicture.width;
                pictureHeight = mOutputPicture.height;

                mOutputPicture.hasBeenRendered = true;
            }

            mPictureMutex.unlock();

            if (pictureSize > 0) {
                // Build a texture for the video frame.
                mTexture->initFromPixels(&tempPictureRGBA.at(0), pictureWidth, pictureHeight);
            }
        }
        else {
            mPictureMutex.unlock();
        }

        mTexture->bind();

#if defined(USE_OPENGL_21)
        // Render scanlines if this option is enabled. However, if this is the media viewer
        // or the video screensaver, then skip this as the scanline rendering is then handled
        // in those modules as a postprocessing step.
        if ((!mScreensaverMode && !mMediaViewerMode) &&
            Settings::getInstance()->getBool("GamelistVideoScanlines"))
            vertices[0].shaders = Renderer::SHADER_SCANLINES;
#endif

        // Render it.
        Renderer::setMatrix(trans);
        Renderer::drawTriangleStrips(&vertices[0], 4, trans);
    }
    else {
        VideoComponent::renderSnapshot(parentTrans);
    }
}

void VideoFFmpegComponent::updatePlayer()
{
    if (mPause || !mFormatContext)
        return;

    // Output any audio that has been added by the processing thread.
    mAudioMutex.lock();
    if (mOutputAudio.size()) {
        AudioManager::getInstance()->processStream(&mOutputAudio.at(0),
                                                   static_cast<unsigned int>(mOutputAudio.size()));
        mOutputAudio.clear();
    }
    mAudioMutex.unlock();

    if (mIsActuallyPlaying && mStartTimeAccumulation) {
        mAccumulatedTime +=
            static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                    std::chrono::high_resolution_clock::now() - mTimeReference)
                                    .count()) /
            1000000000.0l;
    }

    mTimeReference = std::chrono::high_resolution_clock::now();

    if (!mFrameProcessingThread) {
        AudioManager::getInstance()->unmuteStream();
        mFrameProcessingThread =
            std::make_unique<std::thread>(&VideoFFmpegComponent::frameProcessing, this);
    }
}

void VideoFFmpegComponent::frameProcessing()
{
    mWindow->increaseVideoPlayerCount();

    bool videoFilter = false;
    bool audioFilter = false;

    videoFilter = setupVideoFilters();

    if (mAudioCodecContext)
        audioFilter = setupAudioFilters();

    while (mIsPlaying && !mPause && videoFilter && (!mAudioCodecContext || audioFilter)) {
        readFrames();
        getProcessedFrames();
        outputFrames();

        // This 1 ms wait makes sure that the thread does not consume all available CPU cycles.
        SDL_Delay(1);
    }

    if (videoFilter) {
        avfilter_inout_free(&mVFilterInputs);
        avfilter_inout_free(&mVFilterOutputs);
        avfilter_free(mVBufferSrcContext);
        avfilter_free(mVBufferSinkContext);
        avfilter_graph_free(&mVFilterGraph);
        mVBufferSrcContext = nullptr;
        mVBufferSinkContext = nullptr;
    }

    if (audioFilter) {
        avfilter_inout_free(&mAFilterInputs);
        avfilter_inout_free(&mAFilterOutputs);
        avfilter_free(mABufferSrcContext);
        avfilter_free(mABufferSinkContext);
        avfilter_graph_free(&mAFilterGraph);
        mABufferSrcContext = nullptr;
        mABufferSinkContext = nullptr;
    }

    mWindow->decreaseVideoPlayerCount();
}

bool VideoFFmpegComponent::setupVideoFilters()
{
    int returnValue = 0;
    char errorMessage[512];
    const enum AVPixelFormat outPixFormats[] = {AV_PIX_FMT_RGBA, AV_PIX_FMT_NONE};

    mVFilterInputs = avfilter_inout_alloc();
    mVFilterOutputs = avfilter_inout_alloc();

    if (!(mVFilterGraph = avfilter_graph_alloc())) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't allocate filter graph";
        return false;
    }

    // Limit the libavfilter video processing to two additional threads.
    // Not sure why the actual thread count is one less than specified.
    mVFilterGraph->nb_threads = 3;

    const AVFilter* bufferSrc = avfilter_get_by_name("buffer");
    if (!bufferSrc) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't find \"buffer\" filter";
        return false;
    }

    const AVFilter* bufferSink = avfilter_get_by_name("buffersink");
    if (!bufferSink) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't find \"buffersink\" filter";
        return false;
    }

    // Some codecs such as H.264 need the width to be in increments of 16 pixels.
    int width = mVideoCodecContext->width;
    int height = mVideoCodecContext->height;
    int modulo = mVideoCodecContext->width % 16;

    if (modulo > 0)
        width += 16 - modulo;

    std::string filterArguments =
        "width=" + std::to_string(width) + ":" + "height=" + std::to_string(height) +
        ":pix_fmt=" + av_get_pix_fmt_name(mVideoCodecContext->pix_fmt) +
        ":time_base=" + std::to_string(mVideoStream->time_base.num) + "/" +
        std::to_string(mVideoStream->time_base.den) +
        ":sar=" + std::to_string(mVideoCodecContext->sample_aspect_ratio.num) + "/" +
        std::to_string(mVideoCodecContext->sample_aspect_ratio.den);

    returnValue = avfilter_graph_create_filter(&mVBufferSrcContext, bufferSrc, "in",
                                               filterArguments.c_str(), nullptr, mVFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't create filter instance for buffer source: "
                      << av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_create_filter(&mVBufferSinkContext, bufferSink, "out", nullptr,
                                               nullptr, mVFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't create filter instance for buffer sink: "
                      << av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    // Endpoints for the filter graph.
    mVFilterInputs->name = av_strdup("out");
    mVFilterInputs->filter_ctx = mVBufferSinkContext;
    mVFilterInputs->pad_idx = 0;
    mVFilterInputs->next = nullptr;

    mVFilterOutputs->name = av_strdup("in");
    mVFilterOutputs->filter_ctx = mVBufferSrcContext;
    mVFilterOutputs->pad_idx = 0;
    mVFilterOutputs->next = nullptr;

    std::string filterDescription;

    // Whether to upscale the frame rate to 60 FPS.
    if (Settings::getInstance()->getBool("VideoUpscaleFrameRate")) {
        if (modulo > 0)
            filterDescription = "scale=width=" + std::to_string(width) +
                                ":height=" + std::to_string(height) + ",fps=fps=60,";
        else
            filterDescription = "fps=fps=60,";

        // The "framerate" filter is a more advanced way to upscale the frame rate using
        // interpolation. However I have not been able to get this to work with slice
        // threading so the performance is poor. As such it's disabled for now.
        //        if (modulo > 0)
        //            filterDescription =
        //                    "scale=width=" + std::to_string(width) +
        //                    ":height=" + std::to_string(height) +
        //                    ",framerate=fps=60,";
        //        else
        //            filterDescription = "framerate=fps=60,";
    }

    filterDescription += "format=pix_fmts=" + std::string(av_get_pix_fmt_name(outPixFormats[0]));

    returnValue = avfilter_graph_parse_ptr(mVFilterGraph, filterDescription.c_str(),
                                           &mVFilterInputs, &mVFilterOutputs, nullptr);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't add graph filter: "
                      << av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_config(mVFilterGraph, nullptr);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't configure graph: "
                      << av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    return true;
}

bool VideoFFmpegComponent::setupAudioFilters()
{
    int returnValue = 0;
    char errorMessage[512];
    const int outSampleRates[] = {AudioManager::getInstance()->sAudioFormat.freq, -1};
    const enum AVSampleFormat outSampleFormats[] = {AV_SAMPLE_FMT_FLT, AV_SAMPLE_FMT_NONE};

    mAFilterInputs = avfilter_inout_alloc();
    mAFilterOutputs = avfilter_inout_alloc();

    if (!(mAFilterGraph = avfilter_graph_alloc())) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't allocate filter graph";
        return false;
    }

    // Limit the libavfilter audio processing to one additional thread.
    // Not sure why the actual thread count is one less than specified.
    mAFilterGraph->nb_threads = 2;

    const AVFilter* bufferSrc = avfilter_get_by_name("abuffer");
    if (!bufferSrc) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't find \"abuffer\" filter";
        return false;
    }

    const AVFilter* bufferSink = avfilter_get_by_name("abuffersink");
    if (!bufferSink) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't find \"abuffersink\" filter";
        return false;
    }

    char channelLayout[512];
    av_get_channel_layout_string(channelLayout, sizeof(channelLayout), mAudioCodecContext->channels,
                                 mAudioCodecContext->channel_layout);

    std::string filterArguments =
        "time_base=" + std::to_string(mAudioStream->time_base.num) + "/" +
        std::to_string(mAudioStream->time_base.den) +
        ":sample_rate=" + std::to_string(mAudioCodecContext->sample_rate) +
        ":sample_fmt=" + av_get_sample_fmt_name(mAudioCodecContext->sample_fmt) +
        ":channel_layout=" + channelLayout;

    returnValue = avfilter_graph_create_filter(&mABufferSrcContext, bufferSrc, "in",
                                               filterArguments.c_str(), nullptr, mAFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't create filter instance for buffer source: "
                      << av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_create_filter(&mABufferSinkContext, bufferSink, "out", nullptr,
                                               nullptr, mAFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't create filter instance for buffer sink: "
                      << av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    // Endpoints for the filter graph.
    mAFilterInputs->name = av_strdup("out");
    mAFilterInputs->filter_ctx = mABufferSinkContext;
    mAFilterInputs->pad_idx = 0;
    mAFilterInputs->next = nullptr;

    mAFilterOutputs->name = av_strdup("in");
    mAFilterOutputs->filter_ctx = mABufferSrcContext;
    mAFilterOutputs->pad_idx = 0;
    mAFilterOutputs->next = nullptr;

    std::string filterDescription =
        "aresample=" + std::to_string(outSampleRates[0]) + "," +
        "aformat=sample_fmts=" + av_get_sample_fmt_name(outSampleFormats[0]) +
        ":channel_layouts=stereo,"
        "asetnsamples=n=1024:p=0";

    returnValue = avfilter_graph_parse_ptr(mAFilterGraph, filterDescription.c_str(),
                                           &mAFilterInputs, &mAFilterOutputs, nullptr);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't add graph filter: "
                      << av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_config(mAFilterGraph, nullptr);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't configure graph: "
                      << av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    return true;
}

void VideoFFmpegComponent::readFrames()
{
    int readFrameReturn = 0;

    // It's not clear if this can actually happen in practise, but in theory we could
    // continue to load frames indefinitely and run out of memory if invalid PTS values
    // are presented by FFmpeg.
    if (mVideoFrameQueue.size() > 300 || mAudioFrameQueue.size() > 600)
        return;

    if (mVideoCodecContext && mFormatContext) {
        if (mVideoFrameQueue.size() < mVideoTargetQueueSize ||
            (mAudioStreamIndex >= 0 && mAudioFrameQueue.size() < mAudioTargetQueueSize)) {
            while ((readFrameReturn = av_read_frame(mFormatContext, mPacket)) >= 0) {
                if (mPacket->stream_index == mVideoStreamIndex) {
                    if (!avcodec_send_packet(mVideoCodecContext, mPacket) &&
                        !avcodec_receive_frame(mVideoCodecContext, mVideoFrame)) {

                        int returnValue = 0;

                        if (mSWDecoder) {
                            returnValue = av_buffersrc_add_frame_flags(
                                mVBufferSrcContext, mVideoFrame, AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT);
                        }
                        else {
                            AVFrame* destFrame = nullptr;
                            destFrame = av_frame_alloc();

                            if (mVideoFrame->format == sPixelFormat) {
                                if (av_hwframe_transfer_data(destFrame, mVideoFrame, 0) < 0) {
                                    LOG(LogError) << "VideoFFmpegComponent::readFrames(): "
                                                     "Couldn't transfer decoded video frame to "
                                                     "system memory";
                                    av_frame_free(&destFrame);
                                    av_packet_unref(mPacket);
                                    break;
                                }
                                else {
                                    destFrame->pts = mVideoFrame->pts;
                                    destFrame->pkt_dts = mVideoFrame->pkt_dts;
                                    destFrame->pict_type = mVideoFrame->pict_type;
                                    destFrame->chroma_location = mVideoFrame->chroma_location;
                                    destFrame->pkt_pos = mVideoFrame->pkt_pos;
                                    destFrame->pkt_duration = mVideoFrame->pkt_duration;
                                    destFrame->pkt_size = mVideoFrame->pkt_size;
                                }
                            }
                            else {
                                LOG(LogError) << "VideoFFmpegComponent::readFrames(): "
                                                 "Couldn't decode video frame";
                            }

                            returnValue = av_buffersrc_add_frame_flags(
                                mVBufferSrcContext, destFrame, AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT);
                            av_frame_free(&destFrame);
                        }

                        if (returnValue < 0) {
                            LOG(LogError) << "VideoFFmpegComponent::readFrames(): "
                                             "Couldn't add video frame to buffer source";
                        }

                        av_packet_unref(mPacket);
                        break;
                    }
                    else {
                        av_packet_unref(mPacket);
                    }
                }
                else if (mPacket->stream_index == mAudioStreamIndex) {
                    if (!avcodec_send_packet(mAudioCodecContext, mPacket) &&
                        !avcodec_receive_frame(mAudioCodecContext, mAudioFrame)) {

                        // We have an audio frame that needs conversion and resampling.
                        int returnValue = av_buffersrc_add_frame_flags(
                            mABufferSrcContext, mAudioFrame, AV_BUFFERSRC_FLAG_KEEP_REF);

                        if (returnValue < 0) {
                            LOG(LogError) << "VideoFFmpegComponent::readFrames(): "
                                             "Couldn't add audio frame to buffer source";
                        }

                        av_packet_unref(mPacket);
                        continue;
                    }
                    else {
                        av_packet_unref(mPacket);
                    }
                }
                else {
                    // Ignore any stream that is not video or audio.
                    av_packet_unref(mPacket);
                }
            }
        }
    }

    if (readFrameReturn < 0)
        mEndOfVideo = true;
}

void VideoFFmpegComponent::getProcessedFrames()
{
    // Video frames.
    while (av_buffersink_get_frame(mVBufferSinkContext, mVideoFrameResampled) >= 0) {

        // Save the frame into the queue for later processing.
        VideoFrame currFrame;

        currFrame.width = mVideoFrameResampled->width;
        currFrame.height = mVideoFrameResampled->height;

        mVideoFrameResampled->best_effort_timestamp = mVideoFrameResampled->pkt_dts;

        // The PTS value is the presentation time, i.e. the time stamp when the frame
        // (picture) should be displayed. The packet DTS value is used for the basis of
        // the calculation as per the recommendation in the FFmpeg documentation for
        // the av_read_frame function.
        double pts =
            static_cast<double>(mVideoFrameResampled->pkt_dts) * av_q2d(mVideoStream->time_base);

        // Needs to be adjusted if changing the rate?
        double frameDuration = static_cast<double>(mVideoFrameResampled->pkt_duration) *
                               av_q2d(mVideoStream->time_base);

        currFrame.pts = pts;
        currFrame.frameDuration = frameDuration;

        int bufferSize = mVideoFrameResampled->width * mVideoFrameResampled->height * 4;

        currFrame.frameRGBA.insert(currFrame.frameRGBA.begin(), &mVideoFrameResampled->data[0][0],
                                   &mVideoFrameResampled->data[0][bufferSize]);

        mVideoFrameQueue.push(currFrame);
        av_frame_unref(mVideoFrameResampled);
    }

    // Audio frames.
    // When resampling, we may not always get a frame returned from the sink as there may not
    // have been enough data available to the filter.
    while (mAudioCodecContext &&
           av_buffersink_get_frame(mABufferSinkContext, mAudioFrameResampled) >= 0) {

        AudioFrame currFrame;
        AVRational timeBase;

        mAudioFrameResampled->best_effort_timestamp = mAudioFrameResampled->pts;

        timeBase.num = 1;
        timeBase.den = mAudioFrameResampled->sample_rate;

        double pts = mAudioFrameResampled->pts * av_q2d(timeBase);
        currFrame.pts = pts;

        int bufferSize = mAudioFrameResampled->nb_samples * mAudioFrameResampled->channels *
                         av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);

        currFrame.resampledData.insert(currFrame.resampledData.begin(),
                                       &mAudioFrameResampled->data[0][0],
                                       &mAudioFrameResampled->data[0][bufferSize]);

        mAudioFrameQueue.push(currFrame);
        av_frame_unref(mAudioFrameResampled);
    }
}

void VideoFFmpegComponent::outputFrames()
{
    // Check if we should start counting the time (i.e. start playing the video).
    // The audio stream controls when the playback and time counting starts, assuming
    // there is an audio track.
    if (!mAudioCodecContext || (mAudioCodecContext && !mAudioFrameQueue.empty())) {
        if (!mStartTimeAccumulation) {
            mTimeReference = std::chrono::high_resolution_clock::now();
            mStartTimeAccumulation = true;
            mIsActuallyPlaying = true;
        }
    }

    // Process the audio frames that have a PTS value below mAccumulatedTime (plus a small
    // buffer to avoid underflows).
    while (!mAudioFrameQueue.empty()) {
        if (mAudioFrameQueue.front().pts < mAccumulatedTime + AUDIO_BUFFER) {
            // Enable only when needed, as this generates a lot of debug output.
            if (DEBUG_VIDEO) {
                LOG(LogDebug) << "Processing audio frame with PTS: "
                              << mAudioFrameQueue.front().pts;
                LOG(LogDebug) << "Total audio frames processed / audio frame queue size: "
                              << mAudioFrameCount << " / "
                              << std::to_string(mAudioFrameQueue.size());
            }

            bool outputSound = false;

            if ((!mScreensaverMode && !mMediaViewerMode) &&
                Settings::getInstance()->getBool("GamelistVideoAudio"))
                outputSound = true;
            else if (mScreensaverMode && Settings::getInstance()->getBool("ScreensaverVideoAudio"))
                outputSound = true;
            else if (mMediaViewerMode && Settings::getInstance()->getBool("MediaViewerVideoAudio"))
                outputSound = true;

            if (outputSound) {
                // The audio is output to AudioManager from updatePlayer() in the main thread.
                mAudioMutex.lock();

                mOutputAudio.insert(mOutputAudio.end(),
                                    mAudioFrameQueue.front().resampledData.begin(),
                                    mAudioFrameQueue.front().resampledData.end());

                mAudioMutex.unlock();
            }
            mAudioFrameQueue.pop();
            mAudioFrameCount++;
        }
        else {
            break;
        }
    }

    // Process all available video frames that have a PTS value below mAccumulatedTime.
    // But if more than one frame is processed here, it means that the computer can't
    // keep up for some reason.
    while (mIsActuallyPlaying && !mVideoFrameQueue.empty()) {
        if (mVideoFrameQueue.front().pts < mAccumulatedTime) {
            // Enable only when needed, as this generates a lot of debug output.
            if (DEBUG_VIDEO) {
                LOG(LogDebug) << "Processing video frame with PTS: "
                              << mVideoFrameQueue.front().pts;
                LOG(LogDebug) << "Total video frames processed / video frame queue size: "
                              << mVideoFrameCount << " / "
                              << std::to_string(mVideoFrameQueue.size());
            }

            mPictureMutex.lock();

            // Give some leeway for frames that have not yet been rendered but that have pts
            // values with a time difference relative to the frame duration that is under a
            // certain threshold. In this case, give the renderer an additional chance to output
            // the frames. If the difference exceeds the threshold though, then skip them as
            // otherwise videos would just slow down instead of skipping frames when the computer
            // can't keep up. This approach primarily decreases stuttering for videos with frame
            // rates close to, or at, the rendering frame rate, for example 59.94 and 60 FPS.
            if (mDecodedFrame && !mOutputPicture.hasBeenRendered) {
                double timeDifference = mAccumulatedTime - mVideoFrameQueue.front().pts -
                                        mVideoFrameQueue.front().frameDuration * 2.0l;
                if (timeDifference < mVideoFrameQueue.front().frameDuration) {
                    mPictureMutex.unlock();
                    break;
                }
            }

            mOutputPicture.pictureRGBA.clear();
            mOutputPicture.pictureRGBA.insert(mOutputPicture.pictureRGBA.begin(),
                                              mVideoFrameQueue.front().frameRGBA.begin(),
                                              mVideoFrameQueue.front().frameRGBA.end());

            mOutputPicture.width = mVideoFrameQueue.front().width;
            mOutputPicture.height = mVideoFrameQueue.front().height;
            mOutputPicture.hasBeenRendered = false;

            mPictureMutex.unlock();

            mVideoFrameQueue.pop();
            mVideoFrameCount++;

            mDecodedFrame = true;
        }
        else {
            break;
        }
    }
}

void VideoFFmpegComponent::calculateBlackRectangle()
{
    // Calculate the position and size for the black rectangle that will be rendered behind
    // videos. If the option to display pillarboxes (and letterboxes) is enabled, then this
    // would extend to the entire md_video area (if above the threshold as defined below) or
    // otherwise it will exactly match the video size. The reason to add a black rectangle
    // behind videos in this second instance is that the scanline rendering will make the
    // video partially transparent so this may avoid some unforseen issues with some themes.
    if (mVideoAreaPos != glm::vec2{} && mVideoAreaSize != glm::vec2{}) {
        mVideoRectangleCoords.clear();

        if (Settings::getInstance()->getBool("GamelistVideoPillarbox")) {
            float rectHeight;
            float rectWidth;
            // Video is in landscape orientation.
            if (mSize.x > mSize.y) {
                // Checking the Y size should not normally be required as landscape format
                // should mean the height can't be higher than the max size defined by the
                // theme. But as the height in mSize is provided by FFmpeg in integer format
                // and then scaled, there could be rounding errors that make the video height
                // slightly higher than allowed. It's only a single pixel or a few pixels, but
                // it's still visible for some videos.
                if (mSize.y < mVideoAreaSize.y && mSize.y / mVideoAreaSize.y < 0.90f)
                    rectHeight = mVideoAreaSize.y;
                else
                    rectHeight = mSize.y;
                // Don't add a black border that is too narrow, that's what the 0.85 constant
                // takes care of.
                if (mSize.x < mVideoAreaSize.x && mSize.x / mVideoAreaSize.x < 0.85f)
                    rectWidth = mVideoAreaSize.x;
                else
                    rectWidth = mSize.x;
            }
            // Video is in portrait orientation (or completely square).
            else {
                rectWidth = mVideoAreaSize.x;
                rectHeight = mSize.y;
            }
            // Populate the rectangle coordinates to be used in render().
            mVideoRectangleCoords.push_back(std::round(mVideoAreaPos.x - rectWidth * mOrigin.x));
            mVideoRectangleCoords.push_back(std::round(mVideoAreaPos.y - rectHeight * mOrigin.y));
            mVideoRectangleCoords.push_back(std::round(rectWidth));
            mVideoRectangleCoords.push_back(std::round(rectHeight));
        }
        // If the option to display pillarboxes is disabled, then make the rectangle equivalent
        // to the size of the video.
        else {
            mVideoRectangleCoords.push_back(std::round(mPosition.x - mSize.x * mOrigin.x));
            mVideoRectangleCoords.push_back(std::round(mPosition.y - mSize.y * mOrigin.y));
            mVideoRectangleCoords.push_back(std::round(mSize.x));
            mVideoRectangleCoords.push_back(std::round(mSize.y));
        }
    }
}

void VideoFFmpegComponent::detectHWDecoder()
{
#if defined(__APPLE__)
    LOG(LogDebug) << "VideoFFmpegComponent::detectHWDecoder(): Using hardware decoder VideoToolbox";
    sDeviceType = AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
    return;
#elif defined(_WIN64)
    bool hasDXVA2 = false;
    bool hasD3D11VA = false;

    AVBufferRef* testContext = nullptr;
    AVHWDeviceType tempDevice = AV_HWDEVICE_TYPE_NONE;

    while ((tempDevice = av_hwdevice_iterate_types(tempDevice)) != AV_HWDEVICE_TYPE_NONE) {
        // The Direct3D 11 decoder detection seems to cause stability issues on some machines
        // so disabling it for now.
        if (tempDevice == AV_HWDEVICE_TYPE_DXVA2) {
            // if (tempDevice == AV_HWDEVICE_TYPE_DXVA2 || tempDevice == AV_HWDEVICE_TYPE_D3D11VA) {
            if (av_hwdevice_ctx_create(&testContext, tempDevice, nullptr, nullptr, 0) >= 0) {
                if (tempDevice == AV_HWDEVICE_TYPE_DXVA2)
                    hasDXVA2 = true;
                else
                    hasD3D11VA = true;
            }
            av_buffer_unref(&testContext);
        }
    }

    // Prioritize DXVA2.
    if (hasDXVA2) {
        LOG(LogDebug) << "VideoFFmpegComponent::detectHWDecoder(): Using hardware decoder DXVA2";
        sDeviceType = AV_HWDEVICE_TYPE_DXVA2;
    }
    else if (hasD3D11VA) {
        LOG(LogDebug) << "VideoFFmpegComponent::detectHWDecoder(): Using hardware decoder D3D11VA";
        sDeviceType = AV_HWDEVICE_TYPE_D3D11VA;
    }
    else {
        LOG(LogWarning) << "VideoFFmpegComponent::detectHWDecoder(): Unable to detect any usable "
                           "hardware decoder";
    }
#else
    // This would mostly be Linux, but possibly also BSD Unix.

    bool hasVAAPI = false;
    bool hasVDPAU = false;

    AVBufferRef* testContext = nullptr;
    AVHWDeviceType tempDevice = AV_HWDEVICE_TYPE_NONE;

    while ((tempDevice = av_hwdevice_iterate_types(tempDevice)) != AV_HWDEVICE_TYPE_NONE) {
        if (tempDevice == AV_HWDEVICE_TYPE_VDPAU || tempDevice == AV_HWDEVICE_TYPE_VAAPI) {
            if (av_hwdevice_ctx_create(&testContext, tempDevice, nullptr, nullptr, 0) >= 0) {
                if (tempDevice == AV_HWDEVICE_TYPE_VAAPI)
                    hasVAAPI = true;
                else
                    hasVDPAU = true;
            }
            av_buffer_unref(&testContext);
        }
    }

    // Prioritize VAAPI.
    if (hasVAAPI) {
        LOG(LogDebug) << "VideoFFmpegComponent::detectHWDecoder(): Using hardware decoder VAAPI";
        sDeviceType = AV_HWDEVICE_TYPE_VAAPI;
    }
    else if (hasVDPAU) {
        LOG(LogDebug) << "VideoFFmpegComponent::detectHWDecoder(): Using hardware decoder VDPAU";
        sDeviceType = AV_HWDEVICE_TYPE_VDPAU;
    }
    else {
        LOG(LogWarning) << "VideoFFmpegComponent::detectHWDecoder(): Unable to detect any "
                           "usable hardware decoder";
    }
#endif
}

bool VideoFFmpegComponent::decoderInitHW()
{
    // This should only be required the first time any video is played.
    if (sDeviceType == AV_HWDEVICE_TYPE_NONE)
        detectHWDecoder();

    // If there is no device, the detection failed.
    if (sDeviceType == AV_HWDEVICE_TYPE_NONE)
        return true;

    // If the hardware decoding of the file was previously unsuccessful during the program
    // session, then don't attempt it again.
    if (std::find(sSWDecodedVideos.begin(), sSWDecodedVideos.end(), mVideoPath) !=
        sSWDecodedVideos.end()) {
        return true;
    }

    // 50 is just an arbitrary number so we don't potentially get stuck in an endless loop.
    for (int i = 0; i < 50; i++) {
        const AVCodecHWConfig* config = avcodec_get_hw_config(mHardwareCodec, i);
        if (!config) {
            LOG(LogDebug) << "VideoFFmpegComponent::decoderInitHW(): Hardware decoder \""
                          << av_hwdevice_get_type_name(sDeviceType)
                          << "\" does not seem to support codec \"" << mHardwareCodec->name << "\"";
        }
        else if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                 config->device_type == sDeviceType) {
            sPixelFormat = config->pix_fmt;
            break;
        }
    }

    // If the pixel format is not set properly, then hardware decoding won't work for the file.
    if (sPixelFormat == AV_PIX_FMT_NONE)
        return true;

    if (av_hwdevice_ctx_create(&mHwContext, sDeviceType, nullptr, nullptr, 0) < 0) {
        LOG(LogDebug) << "VideoFFmpegComponent::decoderInitHW(): Unable to open hardware device \""
                      << av_hwdevice_get_type_name(sDeviceType) << "\"";
        av_buffer_unref(&mHwContext);
        return true;
    }

    // Callback function for AVCodecContext.
    // clang-format off
    auto formatFunc =
        [](AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) -> enum AVPixelFormat {

        const enum AVPixelFormat* pixelFormats;

        for (pixelFormats = pix_fmts; *pixelFormats != -1; pixelFormats++)
            if (*pixelFormats == sPixelFormat)
                return static_cast<enum AVPixelFormat>(sPixelFormat);

        return AV_PIX_FMT_NONE;
    };

    // Check if the video can actually be hardware decoded (unless this has already been done).
    if (std::find(sHWDecodedVideos.begin(), sHWDecodedVideos.end(), mVideoPath) ==
        sHWDecodedVideos.end()) {

        // clang-format on
        AVCodecContext* checkCodecContext = avcodec_alloc_context3(mHardwareCodec);

        if (avcodec_parameters_to_context(checkCodecContext, mVideoStream->codecpar)) {
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                             "Couldn't fill the video codec context parameters for file \""
                          << mVideoPath << "\"";
            avcodec_free_context(&checkCodecContext);
            return true;
        }
        else {
            bool onlySWDecode = false;

            checkCodecContext->get_format = formatFunc;
            checkCodecContext->hw_device_ctx = av_buffer_ref(mHwContext);

            if (avcodec_open2(checkCodecContext, mHardwareCodec, nullptr)) {
                LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                                 "Couldn't initialize the video codec context for file \""
                              << mVideoPath << "\"";
            }

            AVPacket* checkPacket = av_packet_alloc();
            int readFrameReturn = 0;

            while ((readFrameReturn = av_read_frame(mFormatContext, checkPacket)) == 0) {
                if (checkPacket->stream_index != mVideoStreamIndex)
                    av_packet_unref(checkPacket);
                else
                    break;
            }

            // Supplying a packet to the decoder will cause an immediate error for some videos
            // while others will require that one or several frame receive attempts are performed
            // before we get a definitive result. On error we fall back to the software decoder.
            if (readFrameReturn == 0 && checkPacket->stream_index == mVideoStreamIndex) {
                if (avcodec_send_packet(checkCodecContext, checkPacket) < 0) {
                    // Save the file path to the list of videos that require software decoding
                    // so we don't have to check it again during the program session.
                    sSWDecodedVideos.emplace_back(mVideoPath);
                    onlySWDecode = true;
                }
                else {
                    AVFrame* checkFrame;
                    checkFrame = av_frame_alloc();

                    onlySWDecode = true;

                    // For some videos we need to process at least one extra frame to verify
                    // that the hardware encoder can actually be used, otherwise the fallback
                    // to software decoding would take place when it's not necessary.
                    for (int i = 0; i < 3; i++) {
                        if (avcodec_receive_frame(checkCodecContext, checkFrame) < 0) {
                            av_packet_unref(checkPacket);
                            while (av_read_frame(mFormatContext, checkPacket) == 0) {
                                if (checkPacket->stream_index != mVideoStreamIndex)
                                    av_packet_unref(checkPacket);
                                else
                                    break;
                            }

                            avcodec_send_packet(checkCodecContext, checkPacket);
                            av_packet_unref(checkPacket);

                            if (avcodec_receive_frame(checkCodecContext, checkFrame) == 0) {
                                onlySWDecode = false;
                                break;
                            }
                            else {
                                onlySWDecode = true;
                            }
                        }
                        else {
                            onlySWDecode = false;
                        }
                        av_packet_unref(checkPacket);
                        av_frame_unref(checkFrame);
                    }

                    av_frame_free(&checkFrame);

                    if (onlySWDecode == false) {
                        // Save the file path to the list of videos that work with hardware
                        // decoding so we don't have to check it again during the program session.
                        sHWDecodedVideos.emplace_back(mVideoPath);
                    }
                }

                av_packet_free(&checkPacket);
                avcodec_free_context(&checkCodecContext);

                // Seek back to the start position of the file.
                av_seek_frame(mFormatContext, -1, 0, AVSEEK_FLAG_ANY);

                if (onlySWDecode)
                    return true;
            }
        }
    }

    // The hardware decoding check passed successfully or it was done previously for the file.
    // Now perform the real setup.
    mVideoCodecContext = avcodec_alloc_context3(mHardwareCodec);

    if (!mVideoCodecContext) {
        LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                         "Couldn't allocate video codec context for file \""
                      << mVideoPath << "\"";
        avcodec_free_context(&mVideoCodecContext);
        return true;
    }

    if (avcodec_parameters_to_context(mVideoCodecContext, mVideoStream->codecpar)) {
        LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                         "Couldn't fill the video codec context parameters for file \""
                      << mVideoPath << "\"";
        avcodec_free_context(&mVideoCodecContext);
        return true;
    }

    mVideoCodecContext->get_format = formatFunc;
    mVideoCodecContext->hw_device_ctx = av_buffer_ref(mHwContext);

    if (avcodec_open2(mVideoCodecContext, mHardwareCodec, nullptr)) {
        LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                         "Couldn't initialize the video codec context for file \""
                      << mVideoPath << "\"";
        avcodec_free_context(&mVideoCodecContext);
        return true;
    }

    return false;
}

void VideoFFmpegComponent::startVideo()
{
    if (!mFormatContext) {
        mHardwareCodec = nullptr;
        mHwContext = nullptr;
        mFrameProcessingThread = nullptr;
        mVideoWidth = 0;
        mVideoHeight = 0;
        mAccumulatedTime = 0;
        mStartTimeAccumulation = false;
        mSWDecoder = true;
        mDecodedFrame = false;
        mEndOfVideo = false;
        mVideoFrameCount = 0;
        mAudioFrameCount = 0;
        mOutputPicture = {};

        // Get an empty texture for rendering the video.
        mTexture = TextureResource::get("");

        // This is used for the audio and video synchronization.
        mTimeReference = std::chrono::high_resolution_clock::now();

        // Clear the video and audio frame queues.
        std::queue<VideoFrame>().swap(mVideoFrameQueue);
        std::queue<AudioFrame>().swap(mAudioFrameQueue);

        std::string filePath = "file:" + mVideoPath;

        // This will disable the FFmpeg logging, so comment this out if debug info is needed.
        av_log_set_callback(nullptr);

        // File operations and basic setup.

        if (avformat_open_input(&mFormatContext, filePath.c_str(), nullptr, nullptr)) {
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                             "Couldn't open video file \""
                          << mVideoPath << "\"";
            return;
        }

        if (avformat_find_stream_info(mFormatContext, nullptr)) {
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                             "Couldn't read stream information from video file \""
                          << mVideoPath << "\"";
            return;
        }

        mVideoStreamIndex = -1;
        mAudioStreamIndex = -1;

        // Video stream setup.

#if defined(_RPI_)
        bool hwDecoding = false;
#else
        bool hwDecoding = Settings::getInstance()->getBool("VideoHardwareDecoding");
#endif

        mVideoStreamIndex =
            av_find_best_stream(mFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &mHardwareCodec, 0);

        if (mVideoStreamIndex < 0) {
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                             "Couldn't retrieve video stream for file \""
                          << mVideoPath << "\"";
            avformat_close_input(&mFormatContext);
            avformat_free_context(mFormatContext);
            return;
        }

        mVideoStream = mFormatContext->streams[mVideoStreamIndex];
        mVideoWidth = mFormatContext->streams[mVideoStreamIndex]->codecpar->width;
        mVideoHeight = mFormatContext->streams[mVideoStreamIndex]->codecpar->height;

        LOG(LogDebug) << "VideoFFmpegComponent::startVideo(): "
                      << "Playing video \"" << mVideoPath << "\" (codec: "
                      << avcodec_get_name(
                             mFormatContext->streams[mVideoStreamIndex]->codecpar->codec_id)
                      << ", decoder: " << (hwDecoding ? "hardware" : "software") << ")";

        if (hwDecoding)
            mSWDecoder = decoderInitHW();
        else
            mSWDecoder = true;

        if (mSWDecoder) {
            // The hardware decoder initialization failed, which can happen for a number of reasons.
            if (hwDecoding) {
                LOG(LogDebug) << "VideoFFmpegComponent::startVideo(): Hardware decoding failed, "
                                 "falling back to software decoder";
            }

            mVideoCodec =
                const_cast<AVCodec*>(avcodec_find_decoder(mVideoStream->codecpar->codec_id));

            if (!mVideoCodec) {
                LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                                 "Couldn't find a suitable video codec for file \""
                              << mVideoPath << "\"";
                return;
            }

            mVideoCodecContext = avcodec_alloc_context3(mVideoCodec);

            if (!mVideoCodecContext) {
                LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                                 "Couldn't allocate video codec context for file \""
                              << mVideoPath << "\"";
                return;
            }

            if (mVideoCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
                mVideoCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED;

            if (avcodec_parameters_to_context(mVideoCodecContext, mVideoStream->codecpar)) {
                LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                                 "Couldn't fill the video codec context parameters for file \""
                              << mVideoPath << "\"";
                return;
            }

            if (avcodec_open2(mVideoCodecContext, mVideoCodec, nullptr)) {
                LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                                 "Couldn't initialize the video codec context for file \""
                              << mVideoPath << "\"";
                return;
            }
        }

        // Audio stream setup, optional as some videos do not have any audio tracks.

        mAudioStreamIndex =
            av_find_best_stream(mFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

        if (mAudioStreamIndex < 0) {
            LOG(LogDebug) << "VideoFFmpegComponent::startVideo(): "
                             "File does not seem to contain any audio streams";
        }

        if (mAudioStreamIndex >= 0) {
            mAudioStream = mFormatContext->streams[mAudioStreamIndex];
            mAudioCodec =
                const_cast<AVCodec*>(avcodec_find_decoder(mAudioStream->codecpar->codec_id));

            if (!mAudioCodec) {
                LOG(LogError) << "Couldn't find a suitable audio codec for file \"" << mVideoPath
                              << "\"";
                return;
            }

            mAudioCodecContext = avcodec_alloc_context3(mAudioCodec);

            if (mAudioCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
                mAudioCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED;

            // Some formats want separate stream headers.
            if (mAudioCodecContext->flags & AVFMT_GLOBALHEADER)
                mAudioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            if (avcodec_parameters_to_context(mAudioCodecContext, mAudioStream->codecpar)) {
                LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                                 "Couldn't fill the audio codec context parameters for file \""
                              << mVideoPath << "\"";
                return;
            }

            if (avcodec_open2(mAudioCodecContext, mAudioCodec, nullptr)) {
                LOG(LogError) << "VideoFFmpegComponent::startVideo(): "
                                 "Couldn't initialize the audio codec context for file \""
                              << mVideoPath << "\"";
                return;
            }
        }

        mVideoTimeBase = 1.0l / av_q2d(mVideoStream->avg_frame_rate);

        // Set some reasonable target queue sizes (buffers).
        mVideoTargetQueueSize = static_cast<int>(av_q2d(mVideoStream->avg_frame_rate) / 2.0l);
        if (mAudioStreamIndex >= 0)
            mAudioTargetQueueSize = mAudioStream->codecpar->channels * 15;
        else
            mAudioTargetQueueSize = 30;

        mPacket = av_packet_alloc();
        mVideoFrame = av_frame_alloc();
        mVideoFrameResampled = av_frame_alloc();
        mAudioFrame = av_frame_alloc();
        mAudioFrameResampled = av_frame_alloc();

        // Resize the video surface, which is needed both for the gamelist view and for
        // the video screeensaver.
        resize();

        // Calculate pillarbox/letterbox sizes.
        calculateBlackRectangle();

        mIsPlaying = true;
        mFadeIn = 0.0f;
    }
}

void VideoFFmpegComponent::stopVideo()
{
    mIsPlaying = false;
    mIsActuallyPlaying = false;
    mStartDelayed = false;
    mPause = false;
    mEndOfVideo = false;
    mTexture.reset();

    if (mFrameProcessingThread) {
        if (mWindow->getVideoPlayerCount() == 0)
            AudioManager::getInstance()->muteStream();
        // Wait for the thread execution to complete.
        mFrameProcessingThread->join();
        mFrameProcessingThread.reset();
        mOutputAudio.clear();
    }

    // Clear the video and audio frame queues.
    std::queue<VideoFrame>().swap(mVideoFrameQueue);
    std::queue<AudioFrame>().swap(mAudioFrameQueue);

    // Clear the audio buffer.
    AudioManager::getInstance()->clearStream();

    if (mFormatContext) {
        av_frame_free(&mVideoFrame);
        av_frame_free(&mVideoFrameResampled);
        av_frame_free(&mAudioFrame);
        av_frame_free(&mAudioFrameResampled);
        av_packet_unref(mPacket);
        av_packet_free(&mPacket);
        av_buffer_unref(&mHwContext);
        avcodec_free_context(&mVideoCodecContext);
        avcodec_free_context(&mAudioCodecContext);
        avformat_close_input(&mFormatContext);
        avformat_free_context(mFormatContext);
        mVideoCodecContext = nullptr;
        mAudioCodecContext = nullptr;
        mFormatContext = nullptr;
    }
}

void VideoFFmpegComponent::pauseVideo()
{
    if (mPause && mWindow->getVideoPlayerCount() == 0)
        AudioManager::getInstance()->muteStream();
}

void VideoFFmpegComponent::handleLooping()
{
    if (mIsPlaying && mEndOfVideo) {
        // If the screensaver video swap time is set to 0, it means we should
        // skip to the next game when the video has finished playing.
        if (mScreensaverMode &&
            Settings::getInstance()->getInt("ScreensaverSwapVideoTimeout") == 0) {
            mWindow->screensaverTriggerNextGame();
        }
        else {
            stopVideo();
            startVideo();
        }
    }
}

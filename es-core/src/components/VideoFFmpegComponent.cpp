//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoFFmpegComponent.cpp
//
//  Video player based on FFmpeg.
//

#include "components/VideoFFmpegComponent.h"

#include "resources/TextureResource.h"
#include "AudioManager.h"
#include "Settings.h"
#include "Window.h"

#define DEBUG_VIDEO false

VideoFFmpegComponent::VideoFFmpegComponent(
        Window* window)
        : VideoComponent(window),
        mFrameProcessingThread(nullptr),
        mFormatContext(nullptr),
        mVideoStream(nullptr),
        mAudioStream(nullptr),
        mVideoCodec(nullptr),
        mAudioCodec(nullptr),
        mVideoCodecContext(nullptr),
        mAudioCodecContext(nullptr),
        mVBufferSrcContext(nullptr),
        mVBufferSinkContext(nullptr),
        mVFilterGraph(nullptr),
        mVFilterInputs(nullptr),
        mVFilterOutputs(nullptr),
        mABufferSrcContext(nullptr),
        mABufferSinkContext(nullptr),
        mAFilterGraph(nullptr),
        mAFilterInputs(nullptr),
        mAFilterOutputs(nullptr),
        mVideoTimeBase(0.0l),
        mVideoTargetQueueSize(0),
        mAudioTargetQueueSize(0),
        mAccumulatedTime(0),
        mStartTimeAccumulation(false),
        mDecodedFrame(false),
        mEndOfVideo(false)
{
}

VideoFFmpegComponent::~VideoFFmpegComponent()
{
    stopVideo();
}

void VideoFFmpegComponent::setResize(float width, float height)
{
    // This resize function is used when stretching videos to full screen in the video screensaver.
    mTargetSize = Vector2f(width, height);
    mTargetIsMax = false;
    mStaticImage.setResize(width, height);
    resize();
}

void VideoFFmpegComponent::setMaxSize(float width, float height)
{
    // This resize function is used in most instances, such as non-stretched video screensaver
    // and the gamelist videos.
    mTargetSize = Vector2f(width, height);
    mTargetIsMax = true;
    mStaticImage.setMaxSize(width, height);
    resize();
}

void VideoFFmpegComponent::resize()
{
    if (!mTexture)
        return;

    const Vector2f textureSize(static_cast<float>(mVideoWidth), static_cast<float>(mVideoHeight));

    if (textureSize == Vector2f::Zero())
        return;

    if (mTargetIsMax) {
        mSize = textureSize;

        Vector2f resizeScale((mTargetSize.x() / mSize.x()), (mTargetSize.y() / mSize.y()));

        if (resizeScale.x() < resizeScale.y()) {
            mSize[0] *= resizeScale.x();
            mSize[1] *= resizeScale.x();
        }
        else {
            mSize[0] *= resizeScale.y();
            mSize[1] *= resizeScale.y();
        }

        mSize[1] = std::round(mSize[1]);
        mSize[0] = (mSize[1] / textureSize.y()) * textureSize.x();

    }
    else {
        // If both components are set, we just stretch.
        // If no components are set, we don't resize at all.
        mSize = mTargetSize == Vector2f::Zero() ? textureSize : mTargetSize;

        // If only one component is set, we resize in a way that maintains aspect ratio.
        if (!mTargetSize.x() && mTargetSize.y()) {
            mSize[1] = std::round(mTargetSize.y());
            mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
        }
        else if (mTargetSize.x() && !mTargetSize.y()) {
            mSize[1] = std::round((mTargetSize.x() / textureSize.x()) * textureSize.y());
            mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
        }
    }

    onSizeChanged();
}

void VideoFFmpegComponent::render(const Transform4x4f& parentTrans)
{
    VideoComponent::render(parentTrans);
    Transform4x4f trans = parentTrans * getTransform();
    GuiComponent::renderChildren(trans);

    if (mIsPlaying && mFormatContext) {
        unsigned int color;
        if (mDecodedFrame && mFadeIn < 1) {
            const unsigned int fadeIn = static_cast<int>(mFadeIn * 255.0f);
            color = Renderer::convertRGBAToABGR((fadeIn << 24) |
                    (fadeIn << 16) | (fadeIn << 8) | 255);
        }
        else {
            color = 0xFFFFFFFF;
        }
        Renderer::Vertex vertices[4];
        Renderer::setMatrix(parentTrans);

        // Render the black rectangle behind the video.
        if (mVideoRectangleCoords.size() == 4) {
            Renderer::drawRect(mVideoRectangleCoords[0], mVideoRectangleCoords[1],
                    mVideoRectangleCoords[2], mVideoRectangleCoords[3], 0x000000FF, 0x000000FF);
        }

        vertices[0] = { { 0.0f     , 0.0f      }, { 0.0f, 0.0f }, color };
        vertices[1] = { { 0.0f     , mSize.y() }, { 0.0f, 1.0f }, color };
        vertices[2] = { { mSize.x(), 0.0f      }, { 1.0f, 0.0f }, color };
        vertices[3] = { { mSize.x(), mSize.y() }, { 1.0f, 1.0f }, color };

        // Round vertices.
        for (int i = 0; i < 4; i++)
            vertices[i].pos.round();

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
                tempPictureRGBA.insert(tempPictureRGBA.begin(),
                        mOutputPicture.pictureRGBA.begin(), mOutputPicture.pictureRGBA.end());
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
        mAccumulatedTime += static_cast<double>(
                std::chrono::duration_cast<std::chrono::nanoseconds>
                (std::chrono::high_resolution_clock::now() -
                mTimeReference).count()) / 1000000000.0l;
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

    bool videoFilter;
    bool audioFilter;

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
    const enum AVPixelFormat outPixFormats[] = { AV_PIX_FMT_RGBA, AV_PIX_FMT_NONE };

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
            "width=" + std::to_string(width) + ":" +
            "height=" + std::to_string(height) +
            ":pix_fmt=" + av_get_pix_fmt_name(mVideoCodecContext->pix_fmt) +
            ":time_base=" + std::to_string(mVideoStream->time_base.num) + "/" +
            std::to_string(mVideoStream->time_base.den) +
            ":sar=" + std::to_string(mVideoCodecContext->sample_aspect_ratio.num) + "/" +
            std::to_string(mVideoCodecContext->sample_aspect_ratio.den);

    returnValue = avfilter_graph_create_filter(
            &mVBufferSrcContext,
            bufferSrc,
            "in",
            filterArguments.c_str(),
            nullptr,
            mVFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                "Couldn't create filter instance for buffer source: " <<
                av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_create_filter(
            &mVBufferSinkContext,
            bufferSink,
            "out",
            nullptr,
            nullptr,
            mVFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                "Couldn't create filter instance for buffer sink: " <<
                av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
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
            filterDescription =
                    "scale=width=" + std::to_string(width) +
                    ":height=" + std::to_string(height) +
                    ",fps=fps=60,";
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
                "Couldn't add graph filter: " <<
                av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_config(mVFilterGraph, nullptr);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                "Couldn't configure graph: " <<
                av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    return true;
}

bool VideoFFmpegComponent::setupAudioFilters()
{
    int returnValue = 0;
    char errorMessage[512];
    const int outSampleRates[] = { AudioManager::getInstance()->sAudioFormat.freq, -1 };
    const enum AVSampleFormat outSampleFormats[] = { AV_SAMPLE_FMT_FLT, AV_SAMPLE_FMT_NONE };

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
    av_get_channel_layout_string(channelLayout, sizeof(channelLayout),
            mAudioCodecContext->channels, mAudioCodecContext->channel_layout);

    std::string filterArguments =
            "time_base=" + std::to_string(mAudioStream->time_base.num) + "/" +
            std::to_string(mAudioStream->time_base.den) +
            ":sample_rate=" + std::to_string(mAudioCodecContext->sample_rate) +
            ":sample_fmt=" + av_get_sample_fmt_name(mAudioCodecContext->sample_fmt) +
            ":channel_layout=" + channelLayout;

    returnValue = avfilter_graph_create_filter(
            &mABufferSrcContext,
            bufferSrc,
            "in",
            filterArguments.c_str(),
            nullptr,
            mAFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                "Couldn't create filter instance for buffer source: " <<
                av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_create_filter(
            &mABufferSinkContext,
            bufferSink,
            "out",
            nullptr,
            nullptr,
            mAFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                "Couldn't create filter instance for buffer sink: " <<
                av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
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
                "Couldn't add graph filter: " <<
                av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_config(mAFilterGraph, nullptr);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                "Couldn't configure graph: " <<
                av_make_error_string(errorMessage, sizeof(errorMessage), returnValue);
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
        if (mVideoFrameQueue.size() < mVideoTargetQueueSize || (mAudioStreamIndex >= 0 &&
                mAudioFrameQueue.size() < mAudioTargetQueueSize)) {
            while ((readFrameReturn = av_read_frame(mFormatContext, mPacket)) >= 0) {
                if (mPacket->stream_index == mVideoStreamIndex) {
                    if (!avcodec_send_packet(mVideoCodecContext, mPacket) &&
                            !avcodec_receive_frame(mVideoCodecContext, mVideoFrame)) {

                        // We have a video frame that needs conversion to RGBA format.
                        int returnValue = av_buffersrc_add_frame_flags(mVBufferSrcContext,
                                mVideoFrame, AV_BUFFERSRC_FLAG_KEEP_REF);

                        if (returnValue < 0) {
                            LOG(LogError) << "VideoFFmpegComponent::readFrames(): "
                                    "Couldn't add video frame to buffer source";
                        }

                        av_packet_unref(mPacket);
                        break;
                    }
                }
                else if (mPacket->stream_index == mAudioStreamIndex) {
                    if (!avcodec_send_packet(mAudioCodecContext, mPacket) &&
                            !avcodec_receive_frame(mAudioCodecContext, mAudioFrame)) {

                        // We have an audio frame that needs conversion and resampling.
                        int returnValue = av_buffersrc_add_frame_flags(mABufferSrcContext,
                                mAudioFrame, AV_BUFFERSRC_FLAG_KEEP_REF);

                        if (returnValue < 0) {
                            LOG(LogError) << "VideoFFmpegComponent::readFrames(): "
                                    "Couldn't add audio frame to buffer source";
                        }

                        av_packet_unref(mPacket);
                        continue;
                    }
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
        double pts = static_cast<double>(mVideoFrameResampled->pkt_dts) *
                av_q2d(mVideoStream->time_base);

        // Needs to be adjusted if changing the rate?
        double frameDuration = static_cast<double>(mVideoFrameResampled->pkt_duration) *
                av_q2d(mVideoStream->time_base);

        currFrame.pts = pts;
        currFrame.frameDuration = frameDuration;

        int bufferSize = mVideoFrameResampled->width * mVideoFrameResampled->height * 4;

        currFrame.frameRGBA.insert(currFrame.frameRGBA.begin(),
                &mVideoFrameResampled->data[0][0],
                &mVideoFrameResampled->data[0][bufferSize]);

        mVideoFrameQueue.push(currFrame);
        av_frame_unref(mVideoFrameResampled);
    }

    // Audio frames.
    // When resampling, we may not always get a frame returned from the sink as there may not
    // have been enough data available to the filter.
    while (mAudioCodecContext && av_buffersink_get_frame(mABufferSinkContext,
            mAudioFrameResampled) >= 0) {

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
                LOG(LogDebug) << "Processing audio frame with PTS: " <<
                mAudioFrameQueue.front().pts;
                LOG(LogDebug) << "Total audio frames processed / audio frame queue size: " <<
                mAudioFrameCount << " / " << std::to_string(mAudioFrameQueue.size());
            }

            bool outputSound = false;

            if ((!mScreensaverMode && !mMediaViewerMode) &&
                    Settings::getInstance()->getBool("GamelistVideoAudio"))
                outputSound = true;
            else if (mScreensaverMode && Settings::getInstance()->
                    getBool("ScreensaverVideoAudio"))
                outputSound = true;
            else if (mMediaViewerMode && Settings::getInstance()->
                    getBool("MediaViewerVideoAudio"))
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
                LOG(LogDebug) << "Processing video frame with PTS: " <<
                        mVideoFrameQueue.front().pts;
                LOG(LogDebug) << "Total video frames processed / video frame queue size: " <<
                        mVideoFrameCount << " / " << std::to_string(mVideoFrameQueue.size());
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
    if (mVideoAreaPos != 0 && mVideoAreaSize != 0) {
        mVideoRectangleCoords.clear();

        if (Settings::getInstance()->getBool("GamelistVideoPillarbox")) {
            float rectHeight;
            float rectWidth;
            // Video is in landscape orientation.
            if (mSize.x() > mSize.y()) {
                // Checking the Y size should not normally be required as landscape format
                // should mean the height can't be higher than the max size defined by the
                // theme. But as the height in mSize is provided by FFmpeg in integer format
                // and then scaled, there could be rounding errors that make the video height
                // slightly higher than allowed. It's only a single pixel or a few pixels, but
                // it's still visible for some videos.
                if (mSize.y() < mVideoAreaSize.y() && mSize.y() / mVideoAreaSize.y() < 0.90)
                    rectHeight = mVideoAreaSize.y();
                else
                    rectHeight = mSize.y();
                // Don't add a black border that is too narrow, that's what the 0.85 constant
                // takes care of.
                if (mSize.x() < mVideoAreaSize.x() && mSize.x() / mVideoAreaSize.x() < 0.85)
                    rectWidth = mVideoAreaSize.x();
                else
                    rectWidth = mSize.x();
            }
            // Video is in portrait orientation (or completely square).
            else {
                rectWidth = mVideoAreaSize.x();
                rectHeight = mSize.y();
            }
            // Populate the rectangle coordinates to be used in render().
            mVideoRectangleCoords.push_back(std::round(mVideoAreaPos.x() -
                    rectWidth * mOrigin.x()));
            mVideoRectangleCoords.push_back(std::round(mVideoAreaPos.y() -
                    rectHeight * mOrigin.y()));
            mVideoRectangleCoords.push_back(std::round(rectWidth));
            mVideoRectangleCoords.push_back(std::round(rectHeight));
        }
        // If the option to display pillarboxes is disabled, then make the rectangle equivalent
        // to the size of the video.
        else {
            mVideoRectangleCoords.push_back(std::round(mPosition.x() - mSize.x() * mOrigin.x()));
            mVideoRectangleCoords.push_back(std::round(mPosition.y() - mSize.y() * mOrigin.y()));
            mVideoRectangleCoords.push_back(std::round(mSize.x()));
            mVideoRectangleCoords.push_back(std::round(mSize.y()));
        }
    }
}

void VideoFFmpegComponent::startVideo()
{
    if (!mFormatContext) {
        mFrameProcessingThread = nullptr;
        mVideoWidth = 0;
        mVideoHeight = 0;
        mAccumulatedTime = 0;
        mStartTimeAccumulation = false;
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
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): Couldn't open video file \"" <<
                    mVideoPath << "\"";
            return;
        }

        if (avformat_find_stream_info(mFormatContext, nullptr)) {
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): Couldn't read stream information"
                    "from video file \"" << mVideoPath << "\"";
            return;
        }

        mVideoStreamIndex = -1;
        mAudioStreamIndex = -1;

        // Video stream setup.

        mVideoStreamIndex = av_find_best_stream(
                mFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

        if (mVideoStreamIndex < 0) {
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): Couldn't retrieve video stream "
                    "for file \"" << mVideoPath << "\"";
            return;
        }

        mVideoStream = mFormatContext->streams[mVideoStreamIndex];
        mVideoWidth = mFormatContext->streams[mVideoStreamIndex]->codecpar->width;
        mVideoHeight = mFormatContext->streams[mVideoStreamIndex]->codecpar->height;

        mVideoCodec = const_cast<AVCodec*>(avcodec_find_decoder(mVideoStream->codecpar->codec_id));

        if (!mVideoCodec) {
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): Couldn't find a suitable video "
                    "codec for file \"" << mVideoPath << "\"";
            return;
        }

        mVideoCodecContext = avcodec_alloc_context3(mVideoCodec);

        if (!mVideoCodec) {
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): Couldn't allocate video "
                    "codec context for file \"" << mVideoPath << "\"";
            return;
        }

        if (mVideoCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
            mVideoCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED;

        if (avcodec_parameters_to_context(mVideoCodecContext, mVideoStream->codecpar)) {
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): Couldn't fill the video "
                    "codec context parameters for file \"" << mVideoPath << "\"";
            return;
        }

        if (avcodec_open2(mVideoCodecContext, mVideoCodec, nullptr)) {
            LOG(LogError) << "VideoFFmpegComponent::startVideo(): Couldn't initialize the "
                    "video codec context for file \"" << mVideoPath << "\"";
            return;
        }

        // Audio stream setup, optional as some videos may not have any audio tracks.

        mAudioStreamIndex = av_find_best_stream(
                mFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

        if (mAudioStreamIndex < 0) {
            LOG(LogDebug) << "VideoFFmpegComponent::startVideo(): Couldn't retrieve audio stream "
                    "for file \"" << mVideoPath << "\"";
        }

        if (mAudioStreamIndex >= 0) {
            mAudioStream = mFormatContext->streams[mAudioStreamIndex];
            mAudioCodec = const_cast<AVCodec*>(
                    avcodec_find_decoder(mAudioStream->codecpar->codec_id));

            if (!mAudioCodec) {
                LOG(LogError) << "Couldn't find a suitable audio codec for file \"" <<
                        mVideoPath << "\"";
                return;
            }

            mAudioCodecContext = avcodec_alloc_context3(mAudioCodec);

            if (mAudioCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
                mAudioCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED;

            // Some formats want separate stream headers.
            if (mAudioCodecContext->flags & AVFMT_GLOBALHEADER)
                mAudioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            if (avcodec_parameters_to_context(mAudioCodecContext, mAudioStream->codecpar)) {
                LOG(LogError) << "VideoFFmpegComponent::startVideo(): Couldn't fill the audio "
                        "codec context parameters for file \"" << mVideoPath << "\"";
                return;
            }

            if (avcodec_open2(mAudioCodecContext, mAudioCodec, nullptr)) {
                LOG(LogError) << "VideoFFmpegComponent::startVideo(): Couldn't initialize the "
                        "audio codec context for file \"" << mVideoPath << "\"";
                return;
            }
        }

        mVideoTimeBase = 1.0l / av_q2d(mVideoStream->avg_frame_rate);

        // Set some reasonable target queue sizes (buffers).
        mVideoTargetQueueSize = static_cast<int>(av_q2d(mVideoStream->avg_frame_rate) / 2.0l);
        if (mAudioStreamIndex >=0)
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
        AudioManager::getInstance()->clearStream();
    }

    // Clear the video and audio frame queues.
    std::queue<VideoFrame>().swap(mVideoFrameQueue);
    std::queue<AudioFrame>().swap(mAudioFrameQueue);

    if (mFormatContext) {
        av_frame_free(&mVideoFrame);
        av_frame_free(&mVideoFrameResampled);
        av_frame_free(&mAudioFrame);
        av_frame_free(&mAudioFrameResampled);
        av_packet_unref(mPacket);
        av_packet_free(&mPacket);

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

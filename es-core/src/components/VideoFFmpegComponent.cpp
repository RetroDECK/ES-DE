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
#include "utils/StringUtil.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <iomanip>

#if LIBAVUTIL_VERSION_MAJOR >= 58 ||                                                               \
    (LIBAVUTIL_VERSION_MAJOR >= 57 && LIBAVUTIL_VERSION_MINOR >= 28)
// FFmpeg 5.1 and above.
#define CHANNELS ch_layout.nb_channels
#else
#define CHANNELS channels
#endif

VideoFFmpegComponent::VideoFFmpegComponent()
    : mRenderer {Renderer::getInstance()}
    , mRectangleOffset {0.0f, 0.0f}
    , mFrameProcessingThread {nullptr}
    , mFormatContext {nullptr}
    , mVideoStream {nullptr}
    , mAudioStream {nullptr}
    , mVideoCodec {nullptr}
    , mAudioCodec {nullptr}
    , mHardwareCodec {nullptr}
    , mHwContext {nullptr}
    , mVideoCodecContext {nullptr}
    , mAudioCodecContext {nullptr}
    , mVBufferSrcContext {nullptr}
    , mVBufferSinkContext {nullptr}
    , mVFilterGraph {nullptr}
    , mVFilterInputs {nullptr}
    , mVFilterOutputs {nullptr}
    , mABufferSrcContext {nullptr}
    , mABufferSinkContext {nullptr}
    , mAFilterGraph {nullptr}
    , mAFilterInputs {nullptr}
    , mAFilterOutputs {nullptr}
    , mVideoTargetQueueSize {0}
    , mAudioTargetQueueSize {0}
    , mVideoTimeBase {0.0l}
    , mAccumulatedTime {0.0l}
    , mStartTimeAccumulation {false}
    , mDecodedFrame {false}
    , mEndOfVideo {false}
{
}

void VideoFFmpegComponent::setResize(const float width, const float height)
{
    // This resize function is used when stretching videos to full screen in the video screensaver.
    mTargetSize = glm::vec2 {width, height};
    mTargetIsMax = false;
    mTargetIsCrop = false;
    mStaticImage.setResize(mTargetSize);
    resize();
}

void VideoFFmpegComponent::setMaxSize(float width, float height)
{
    // This resize function is used in most instances, such as non-stretched video screensaver
    // and the gamelist videos.
    mTargetSize = glm::vec2 {width, height};
    mTargetIsMax = true;
    mTargetIsCrop = false;
    mStaticImage.setMaxSize(width, height);
    resize();
}

void VideoFFmpegComponent::setCroppedSize(const glm::vec2& size)
{
    mTargetSize = size;
    mTargetIsMax = false;
    mTargetIsCrop = true;
    mStaticImage.setCroppedSize(size);
    resize();
}

void VideoFFmpegComponent::resize()
{
    if (!mTexture)
        return;

    const glm::vec2 textureSize {static_cast<float>(mVideoWidth), static_cast<float>(mVideoHeight)};

    if (textureSize == glm::vec2 {0.0f, 0.0f})
        return;

    if (mTargetIsMax) {
        mSize = textureSize;

        glm::vec2 resizeScale {mTargetSize.x / mSize.x, mTargetSize.y / mSize.y};

        if (resizeScale.x < resizeScale.y) {
            mSize.x *= resizeScale.x;
            mSize.y *= resizeScale.x;
        }
        else {
            mSize.x *= resizeScale.y;
            mSize.y *= resizeScale.y;
        }

        mSize.x = (mSize.y / textureSize.y) * textureSize.x;
    }
    else if (mTargetIsCrop) {
        // Size texture to allow for cropped video to fill the entire area.
        const float cropFactor {
            std::max(mTargetSize.x / textureSize.x, mTargetSize.y / textureSize.y)};
        mSize = textureSize * cropFactor;

        if (std::round(mSize.y) > std::round(mTargetSize.y)) {
            const float cropSize {1.0f - (mTargetSize.y / std::round(mSize.y))};
            mTopLeftCrop.y = cropSize / 2.0f;
            mBottomRightCrop.y = 1.0f - (cropSize / 2.0f);
            mSize.y = mSize.y - (mSize.y * cropSize);
        }
        else {
            const float cropSize {1.0f - (mTargetSize.x / std::round(mSize.x))};
            mTopLeftCrop.x = cropSize / 2.0f;
            mBottomRightCrop.x = 1.0f - (cropSize / 2.0f);
            mSize.x = mSize.x - (mSize.x * cropSize);
        }
    }
    else {
        // If both components are set, we just stretch.
        // If no components are set, we don't resize at all.
        mSize = mTargetSize == glm::vec2 {0.0f, 0.0f} ? textureSize : mTargetSize;

        // If only one component is set, we resize in a way that maintains aspect ratio.
        if (!mTargetSize.x && mTargetSize.y) {
            mSize.y = mTargetSize.y;
            mSize.x = (mSize.y / textureSize.y) * textureSize.x;
        }
        else if (mTargetSize.x && !mTargetSize.y) {
            mSize.y = (mTargetSize.x / textureSize.x) * textureSize.y;
            mSize.x = (mSize.y / textureSize.y) * textureSize.x;
        }
    }

    onSizeChanged();
}

void VideoFFmpegComponent::render(const glm::mat4& parentTrans)
{
    if (!mVisible || mOpacity == 0.0f || mThemeOpacity == 0.0f)
        return;

    if (!mHasVideo && mStaticImagePath == "")
        return;

    glm::mat4 trans {parentTrans * getTransform()};
    GuiComponent::renderChildren(trans);

    if (mIsPlaying && mFormatContext) {
        Renderer::Vertex vertices[4];
        mRenderer->setMatrix(trans);

        unsigned int rectColor {0x000000FF};

        if (!mGeneralFade && mThemeOpacity != 1.0f)
            rectColor = static_cast<int>(mThemeOpacity * 255.0f);
        if (mGeneralFade && (mOpacity != 1.0f || mThemeOpacity != 1.0f))
            rectColor = static_cast<int>(mFadeIn * mOpacity * mThemeOpacity * 255.0f);

        // Render the black rectangle behind the video.
        if (mVideoRectangleCoords.size() == 4) {
            mRenderer->drawRect(mVideoRectangleCoords[0], mVideoRectangleCoords[1],
                                mVideoRectangleCoords[2], mVideoRectangleCoords[3], // Line break.
                                rectColor, rectColor);
        }

        // This is needed to avoid a slight gap before the video starts playing.
        if (!mDecodedFrame)
            return;

        // clang-format off
        vertices[0] = {{0.0f + mRectangleOffset.x,    0.0f + mRectangleOffset.y     }, {mTopLeftCrop.x,            1.0f - mBottomRightCrop.y}, 0xFFFFFFFF};
        vertices[1] = {{0.0f + mRectangleOffset.x,    mSize.y + mRectangleOffset.y  }, {mTopLeftCrop.x,            1.0f - mTopLeftCrop.y    }, 0xFFFFFFFF};
        vertices[2] = {{mSize.x + mRectangleOffset.x, 0.0f + + mRectangleOffset.y   }, {mBottomRightCrop.x * 1.0f, 1.0f - mBottomRightCrop.y}, 0xFFFFFFFF};
        vertices[3] = {{mSize.x + mRectangleOffset.x, mSize.y + + mRectangleOffset.y}, {mBottomRightCrop.x * 1.0f, 1.0f - mTopLeftCrop.y    }, 0xFFFFFFFF};
        // clang-format on

        vertices[0].color = mColorShift;
        vertices[1].color = mColorGradientHorizontal ? mColorShift : mColorShiftEnd;
        vertices[2].color = mColorGradientHorizontal ? mColorShiftEnd : mColorShift;
        vertices[3].color = mColorShiftEnd;

        // Round vertices.
        for (int i = 0; i < 4; ++i)
            vertices[i].position = glm::round(vertices[i].position);

        if (mFadeIn < 1.0f || mThemeOpacity < 1.0f)
            vertices->opacity = mFadeIn * mThemeOpacity;

        vertices->brightness = mBrightness;
        vertices->saturation = mSaturation * mThemeSaturation;
        vertices->dimming = mDimming;

        std::unique_lock<std::mutex> pictureLock {mPictureMutex};

        if (!mOutputPicture.hasBeenRendered) {
            // Move the contents of mOutputPicture to a temporary vector in order to call
            // initFromPixels() only after the mutex unlock. This significantly reduces the
            // lock waits in outputFrames().
            size_t pictureSize {mOutputPicture.pictureRGBA.size()};
            std::vector<uint8_t> tempPictureRGBA;
            int pictureWidth {0};
            int pictureHeight {0};

            if (pictureSize > 0) {
                tempPictureRGBA.insert(tempPictureRGBA.begin(),
                                       std::make_move_iterator(mOutputPicture.pictureRGBA.begin()),
                                       std::make_move_iterator(mOutputPicture.pictureRGBA.end()));

                mOutputPicture.pictureRGBA.erase(mOutputPicture.pictureRGBA.begin(),
                                                 mOutputPicture.pictureRGBA.end());

                pictureWidth = mOutputPicture.width;
                pictureHeight = mOutputPicture.height;

                mOutputPicture.hasBeenRendered = true;
            }

            pictureLock.unlock();

            if (pictureSize > 0) {
                // Build a texture for the video frame.
                mTexture->initFromPixels(&tempPictureRGBA.at(0), pictureWidth, pictureHeight);
            }
        }
        else {
            pictureLock.unlock();
        }

        if (mTexture != nullptr)
            mTexture->bind();

        // Render scanlines if this option is enabled. However, if this is the media viewer
        // or the video screensaver, then skip this as the scanline rendering is then handled
        // in those modules as a post-processing step.
        if (!mScreensaverMode && !mMediaViewerMode) {
            vertices[0].opacity = mFadeIn * mOpacity * mThemeOpacity;
            if ((mLegacyTheme && Settings::getInstance()->getBool("GamelistVideoScanlines")) ||
                (!mLegacyTheme && mRenderScanlines)) {
                vertices[0].shaders = Renderer::Shader::SCANLINES;
            }
        }

        mRenderer->drawTriangleStrips(&vertices[0], 4, Renderer::BlendFactor::SRC_ALPHA,
                                      Renderer::BlendFactor::ONE_MINUS_SRC_ALPHA);
    }
    else {
        if (mVisible)
            VideoComponent::renderSnapshot(parentTrans);
    }
}

void VideoFFmpegComponent::updatePlayer()
{
    if (mPaused || !mFormatContext)
        return;

    // Output any audio that has been added by the processing thread.
    std::unique_lock<std::mutex> audioLock {mAudioMutex};
    if (mOutputAudio.size()) {
        AudioManager::getInstance().processStream(&mOutputAudio.at(0),
                                                  static_cast<unsigned int>(mOutputAudio.size()));
        mOutputAudio.clear();
    }

    if (mIsActuallyPlaying && mStartTimeAccumulation) {
        mAccumulatedTime =
            mAccumulatedTime +
            static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                    std::chrono::high_resolution_clock::now() - mTimeReference)
                                    .count()) /
                1000000000.0l;
    }

    mTimeReference = std::chrono::high_resolution_clock::now();

    audioLock.unlock();

    if (!mFrameProcessingThread) {
        AudioManager::getInstance().unmuteStream();
        mFrameProcessingThread =
            std::make_unique<std::thread>(&VideoFFmpegComponent::frameProcessing, this);
    }
}

void VideoFFmpegComponent::frameProcessing()
{
    mWindow->increaseVideoPlayerCount();

    bool videoFilter {false};
    bool audioFilter {false};

    videoFilter = setupVideoFilters();

    if (mAudioCodecContext)
        audioFilter = setupAudioFilters();

    while (mIsPlaying && !mPaused && videoFilter && (!mAudioCodecContext || audioFilter)) {
        readFrames();
        if (!mIsPlaying)
            break;

        getProcessedFrames();
        if (!mIsPlaying)
            break;

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
    int returnValue {0};
    std::string errorMessage(512, '\0');

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

    const AVFilter* bufferSrc {avfilter_get_by_name("buffer")};
    if (!bufferSrc) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't find \"buffer\" filter";
        return false;
    }

    const AVFilter* bufferSink {avfilter_get_by_name("buffersink")};
    if (!bufferSink) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't find \"buffersink\" filter";
        return false;
    }

    // Some codecs such as H.264 need the width to be in increments of 16 pixels.
    int width {mVideoCodecContext->width};
    int height {mVideoCodecContext->height};
    int modulo {mVideoCodecContext->width % 16};

    if (modulo > 0)
        width += 16 - modulo;

    std::string filterArguments;
    filterArguments.append("width=")
        .append(std::to_string(width))
        .append(":")
        .append("height=")
        .append(std::to_string(height))
        .append(":pix_fmt=")
        .append(av_get_pix_fmt_name(mVideoCodecContext->pix_fmt))
        .append(":time_base=")
        .append(std::to_string(mVideoStream->time_base.num))
        .append("/")
        .append(std::to_string(mVideoStream->time_base.den))
        .append(":sar=")
        .append(std::to_string(mVideoCodecContext->sample_aspect_ratio.num))
        .append("/")
        .append(std::to_string(mVideoCodecContext->sample_aspect_ratio.den));

    returnValue = avfilter_graph_create_filter(&mVBufferSrcContext, bufferSrc, "in",
                                               filterArguments.c_str(), nullptr, mVFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't create filter instance for buffer source: "
                      << av_make_error_string(&errorMessage[0], sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_create_filter(&mVBufferSinkContext, bufferSink, "out", nullptr,
                                               nullptr, mVFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't create filter instance for buffer sink: "
                      << av_make_error_string(&errorMessage[0], sizeof(errorMessage), returnValue);
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

        if (modulo > 0) {
            filterDescription.append("scale=width=")
                .append(std::to_string(width))
                .append(":height=")
                .append(std::to_string(height))
                .append(",fps=fps=60,");
        }
        else {
            filterDescription.append("fps=fps=60,");
        }

        // The "framerate" filter is a more advanced way to upscale the frame rate using
        // interpolation. However I have not been able to get this to work with slice
        // threading so the performance is poor. As such it's disabled for now.
        // if (modulo > 0) {
        //    filterDescription.append("scale=width=")
        //        .append(std::to_string(width))
        //        .append(":height=")
        //        .append(std::to_string(height))
        //        .append(",framerate=fps=60,");
        // }
        // else {
        //     filterDescription.append("framerate=fps=60,");
        // }
    }

    filterDescription.append("format=pix_fmts=")
        .append(std::string(av_get_pix_fmt_name(AV_PIX_FMT_BGRA)));

    returnValue = avfilter_graph_parse_ptr(mVFilterGraph, filterDescription.c_str(),
                                           &mVFilterInputs, &mVFilterOutputs, nullptr);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't add graph filter: "
                      << av_make_error_string(&errorMessage[0], sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_config(mVFilterGraph, nullptr);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupVideoFilters(): "
                         "Couldn't configure graph: "
                      << av_make_error_string(&errorMessage[0], sizeof(errorMessage), returnValue);
        return false;
    }

    return true;
}

bool VideoFFmpegComponent::setupAudioFilters()
{
    int returnValue {0};
    std::string errorMessage(512, '\0');
    const int outSampleRates[] {AudioManager::getInstance().sAudioFormat.freq, -1};
    // clang-format off
    const enum AVSampleFormat outSampleFormats[] {
        AV_SAMPLE_FMT_FLT,
        AV_SAMPLE_FMT_NONE
    };
    // clang-format on

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

    const AVFilter* bufferSrc {avfilter_get_by_name("abuffer")};
    if (!bufferSrc) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't find \"abuffer\" filter";
        return false;
    }

    const AVFilter* bufferSink {avfilter_get_by_name("abuffersink")};
    if (!bufferSink) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't find \"abuffersink\" filter";
        return false;
    }

    std::string channelLayout(128, '\0');

#if LIBAVUTIL_VERSION_MAJOR >= 58 ||                                                               \
    (LIBAVUTIL_VERSION_MAJOR >= 57 && LIBAVUTIL_VERSION_MINOR >= 28)
    // FFmpeg 5.1 and above.
    AVChannelLayout chLayout {};
    av_channel_layout_from_mask(&chLayout, mAudioCodecContext->ch_layout.u.mask);
    av_channel_layout_describe(&chLayout, &channelLayout[0], sizeof(channelLayout));
    av_channel_layout_uninit(&chLayout);
#else
    av_get_channel_layout_string(&channelLayout[0], sizeof(channelLayout),
                                 mAudioCodecContext->CHANNELS, mAudioCodecContext->channel_layout);
#endif

    std::string filterArguments;
    filterArguments.append("time_base=")
        .append(std::to_string(mAudioStream->time_base.num))
        .append("/")
        .append(std::to_string(mAudioStream->time_base.den))
        .append(":sample_rate=")
        .append(std::to_string(mAudioCodecContext->sample_rate))
        .append(":sample_fmt=")
        .append(av_get_sample_fmt_name(mAudioCodecContext->sample_fmt))
        .append(":channel_layout=")
        .append(channelLayout);

    returnValue = avfilter_graph_create_filter(&mABufferSrcContext, bufferSrc, "in",
                                               filterArguments.c_str(), nullptr, mAFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't create filter instance for buffer source: "
                      << av_make_error_string(&errorMessage[0], sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_create_filter(&mABufferSinkContext, bufferSink, "out", nullptr,
                                               nullptr, mAFilterGraph);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't create filter instance for buffer sink: "
                      << av_make_error_string(&errorMessage[0], sizeof(errorMessage), returnValue);
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

    std::string filterDescription;
    filterDescription.append("aresample=")
        .append(std::to_string(outSampleRates[0]) + ",")
        .append("aformat=sample_fmts=")
        .append(av_get_sample_fmt_name(outSampleFormats[0]))
        .append(":channel_layouts=stereo,")
        .append("asetnsamples=n=1024:p=0");

    returnValue = avfilter_graph_parse_ptr(mAFilterGraph, filterDescription.c_str(),
                                           &mAFilterInputs, &mAFilterOutputs, nullptr);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't add graph filter: "
                      << av_make_error_string(&errorMessage[0], sizeof(errorMessage), returnValue);
        return false;
    }

    returnValue = avfilter_graph_config(mAFilterGraph, nullptr);

    if (returnValue < 0) {
        LOG(LogError) << "VideoFFmpegComponent::setupAudioFilters(): "
                         "Couldn't configure graph: "
                      << av_make_error_string(&errorMessage[0], sizeof(errorMessage), returnValue);
        return false;
    }

    return true;
}

void VideoFFmpegComponent::readFrames()
{
    int readFrameReturn {0};

    // It's not clear if this can actually happen in practise, but in theory we could
    // continue to load frames indefinitely and run out of memory if invalid PTS values
    // are presented by FFmpeg.
    if (mVideoFrameQueue.size() > 300 || mAudioFrameQueue.size() > 600)
        return;

    int readLoops {1};

    // If we can't keep up the audio processing, then drop video frames as it's much worse
    // to have stuttering audio than a lower video framerate.
    if (mAudioStreamIndex >= 0 && mAudioFrameCount > mAudioTargetQueueSize / 2) {
        if (static_cast<int>(mAudioFrameQueue.size()) < mAudioTargetQueueSize / 6)
            readLoops = 5;
        else if (static_cast<int>(mAudioFrameQueue.size()) < mAudioTargetQueueSize / 4)
            readLoops = 3;
        else if (static_cast<int>(mAudioFrameQueue.size()) < mAudioTargetQueueSize / 2)
            readLoops = 2;
    }

    if (mVideoCodecContext && mFormatContext) {
        for (int i = 0; i < readLoops; ++i) {
            if (static_cast<int>(mVideoFrameQueue.size()) < mVideoTargetQueueSize ||
                (mAudioStreamIndex >= 0 &&
                 static_cast<int>(mAudioFrameQueue.size()) < mAudioTargetQueueSize)) {
                while ((readFrameReturn = av_read_frame(mFormatContext, mPacket)) >= 0) {
                    if (mPacket->stream_index == mVideoStreamIndex) {
                        if (!avcodec_send_packet(mVideoCodecContext, mPacket) &&
                            !avcodec_receive_frame(mVideoCodecContext, mVideoFrame)) {

                            int returnValue {0};
                            ++mVideoFrameReadCount;

                            if (mSWDecoder) {
                                // Drop the frame if necessary.
                                if (i == 0 || mAudioFrameCount == 0) {
                                    returnValue = av_buffersrc_add_frame_flags(
                                        mVBufferSrcContext, mVideoFrame,
                                        AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT);
                                }
                                else {
                                    ++mVideoFrameDroppedCount;
                                }
                            }
                            else {
                                if (i == 0 || mAudioFrameCount == 0) {
                                    AVFrame* destFrame {nullptr};
                                    destFrame = av_frame_alloc();

                                    if (mVideoFrame->format == sPixelFormat) {
                                        if (av_hwframe_transfer_data(destFrame, mVideoFrame, 0) <
                                            0) {
                                            LOG(LogError)
                                                << "VideoFFmpegComponent::readFrames(): "
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
                                            destFrame->chroma_location =
                                                mVideoFrame->chroma_location;
                                            destFrame->pkt_pos = mVideoFrame->pkt_pos;
#if LIBAVUTIL_VERSION_MAJOR < 58
                                            destFrame->pkt_duration = mVideoFrame->pkt_duration;
#else
                                            destFrame->duration = mVideoFrame->duration;
#endif
                                            destFrame->pkt_size = mVideoFrame->pkt_size;
                                        }
                                    }
                                    else {
                                        LOG(LogError) << "VideoFFmpegComponent::readFrames(): "
                                                         "Couldn't decode video frame";
                                    }

                                    returnValue = av_buffersrc_add_frame_flags(
                                        mVBufferSrcContext, destFrame,
                                        AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT);
                                    av_frame_free(&destFrame);
                                }
                                else {
                                    ++mVideoFrameDroppedCount;
                                }
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
                            int returnValue {av_buffersrc_add_frame_flags(
                                mABufferSrcContext, mAudioFrame, AV_BUFFERSRC_FLAG_KEEP_REF)};

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
            else {
                // The target queue sizes have been reached.
                break;
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

        // Save frame into the queue for later processing.
        VideoFrame currFrame;

        // This is likely unnecessary as AV_PIX_FMT_RGBA always uses 4 bytes per pixel.
        // const int bytesPerPixel {
        //    av_get_padded_bits_per_pixel(av_pix_fmt_desc_get(AV_PIX_FMT_RGBA)) / 8};
        const int bytesPerPixel {4};
        const int width {mVideoFrameResampled->linesize[0] / bytesPerPixel};

        currFrame.width = width;
        currFrame.height = mVideoFrameResampled->height;

        mVideoFrameResampled->best_effort_timestamp = mVideoFrameResampled->pkt_dts;

        // The PTS value is the presentation time, i.e. the time stamp when the frame
        // (picture) should be displayed. The packet DTS value is used for the basis of
        // the calculation as per the recommendation in the FFmpeg documentation for
        // the av_read_frame function.
        const double pts {static_cast<double>(mVideoFrameResampled->pkt_dts) *
                          av_q2d(mVideoStream->time_base)};

// Needs to be adjusted if changing the rate?
#if LIBAVUTIL_VERSION_MAJOR < 58
        const double frameDuration {static_cast<double>(mVideoFrameResampled->pkt_duration) *
                                    av_q2d(mVideoStream->time_base)};
#else
        const double frameDuration {static_cast<double>(mVideoFrameResampled->duration) *
                                    av_q2d(mVideoStream->time_base)};
#endif

        currFrame.pts = pts;
        currFrame.frameDuration = frameDuration;

        const int bufferSize {width * mVideoFrameResampled->height * 4};

        currFrame.frameRGBA.insert(
            currFrame.frameRGBA.begin(), std::make_move_iterator(&mVideoFrameResampled->data[0][0]),
            std::make_move_iterator(&mVideoFrameResampled->data[0][bufferSize]));

        mVideoFrameQueue.emplace(std::move(currFrame));
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

        double pts {mAudioFrameResampled->pts * av_q2d(timeBase)};
        currFrame.pts = pts;

        int bufferSize {mAudioFrameResampled->nb_samples * mAudioFrameResampled->CHANNELS *
                        av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT)};

        currFrame.resampledData.insert(currFrame.resampledData.begin(),
                                       &mAudioFrameResampled->data[0][0],
                                       &mAudioFrameResampled->data[0][bufferSize]);

        mAudioFrameQueue.emplace(std::move(currFrame));
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
            std::unique_lock<std::mutex> audioLock {mAudioMutex};
            mTimeReference = std::chrono::high_resolution_clock::now();
            mStartTimeAccumulation = true;
            mIsActuallyPlaying = true;
        }
    }

    // Process the audio frames that have a PTS value below mAccumulatedTime (plus a small
    // buffer to avoid underflows).
    while (!mAudioFrameQueue.empty()) {
        // In very rare instances video files are broken and start with a high PTS value for
        // the first frame. In this case set the accumulated time value to this PTS value if
        // the audio frame queue is filled, otherwise the stream will never start playing.
        if (mAudioFrameCount == 0 &&
            mAudioFrameQueue.size() == static_cast<size_t>(mAudioTargetQueueSize) &&
            mAccumulatedTime < mAudioFrameQueue.front().pts) {
            mAccumulatedTime = mAudioFrameQueue.front().pts;
        }
        if (mAudioFrameQueue.front().pts < mAccumulatedTime + AUDIO_BUFFER) {
            // Enable only when needed, as this generates a lot of debug output.
            if (DEBUG_VIDEO) {
                LOG(LogDebug) << "Processing audio frame with PTS: "
                              << mAudioFrameQueue.front().pts;
                LOG(LogDebug) << "Total audio frames processed / audio frame queue size: "
                              << mAudioFrameCount << " / "
                              << std::to_string(mAudioFrameQueue.size());
            }

            bool outputSound {false};

            if ((!mScreensaverMode && !mMediaViewerMode) &&
                Settings::getInstance()->getBool("ViewsVideoAudio"))
                outputSound = true;
            else if (mScreensaverMode && Settings::getInstance()->getBool("ScreensaverVideoAudio"))
                outputSound = true;
            else if (mMediaViewerMode && Settings::getInstance()->getBool("MediaViewerVideoAudio"))
                outputSound = true;

            if (outputSound) {
                // The audio is output to AudioManager from updatePlayer() in the main thread.
                std::unique_lock<std::mutex> audioLock {mAudioMutex};

                mOutputAudio.insert(
                    mOutputAudio.end(),
                    std::make_move_iterator(mAudioFrameQueue.front().resampledData.begin()),
                    std::make_move_iterator(mAudioFrameQueue.front().resampledData.end()));

                audioLock.unlock();
            }
            mAudioFrameQueue.pop();
            ++mAudioFrameCount;
        }
        else {
            break;
        }
    }

    // Process all available video frames that have a PTS value below mAccumulatedTime.
    // But if more than one frame is processed here, it means that the computer can't
    // keep up for some reason.
    while (mIsActuallyPlaying && !mVideoFrameQueue.empty()) {
        // This workaround for broken files with a high PTS value for the first frame is only
        // applied if there are no audio streams available.
        if (!mAudioCodecContext && !mDecodedFrame &&
            mVideoFrameQueue.size() == static_cast<size_t>(mVideoTargetQueueSize) &&
            mAccumulatedTime < mVideoFrameQueue.front().pts) {
            mAccumulatedTime = mVideoFrameQueue.front().pts;
        }

        if (mVideoFrameQueue.front().pts < mAccumulatedTime) {
            // Enable only when needed, as this generates a lot of debug output.
            if (DEBUG_VIDEO) {
                LOG(LogDebug) << "Processing video frame with PTS: "
                              << mVideoFrameQueue.front().pts;
                LOG(LogDebug) << "Total video frames processed / video frame queue size: "
                              << mVideoFrameCount << " / "
                              << std::to_string(mVideoFrameQueue.size());
                if (mVideoFrameDroppedCount > 0) {
                    LOG(LogDebug) << "Video frames dropped: " << mVideoFrameDroppedCount << " of "
                                  << mVideoFrameReadCount << " (" << std::setprecision(2)
                                  << (static_cast<float>(mVideoFrameDroppedCount) /
                                      static_cast<float>(mVideoFrameReadCount)) *
                                         100.0f
                                  << "%)";
                }
            }

            std::unique_lock<std::mutex> pictureLock {mPictureMutex};

            // Give some leeway for frames that have not yet been rendered but that have pts
            // values with a time difference relative to the frame duration that is under a
            // certain threshold. In this case, give the renderer an additional chance to output
            // the frames. If the difference exceeds the threshold though, then skip them as
            // otherwise videos would just slow down instead of skipping frames when the computer
            // can't keep up. This approach primarily decreases stuttering for videos with frame
            // rates close to, or at, the rendering frame rate, for example 59.94 and 60 FPS.
            if (mDecodedFrame && !mOutputPicture.hasBeenRendered) {
                double timeDifference {mAccumulatedTime - mVideoFrameQueue.front().pts -
                                       mVideoFrameQueue.front().frameDuration * 2.0};
                if (timeDifference < mVideoFrameQueue.front().frameDuration) {
                    pictureLock.unlock();
                    break;
                }
            }

            mOutputPicture.pictureRGBA.clear();
            mOutputPicture.pictureRGBA.insert(
                mOutputPicture.pictureRGBA.begin(),
                std::make_move_iterator(mVideoFrameQueue.front().frameRGBA.begin()),
                std::make_move_iterator(mVideoFrameQueue.front().frameRGBA.end()));

            mOutputPicture.width = mVideoFrameQueue.front().width;
            mOutputPicture.height = mVideoFrameQueue.front().height;
            mOutputPicture.hasBeenRendered = false;

            mDecodedFrame = true;

            pictureLock.unlock();

            mVideoFrameQueue.pop();
            ++mVideoFrameCount;
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
    // would extend to the entire video area (if above the threshold as defined below) or
    // otherwise it will exactly match the video size. The reason to add a black rectangle
    // behind videos in this second instance is that the scanline rendering will make the
    // video partially transparent so this may avoid some unforseen issues with some themes.
    // In general, adding very narrow pillarboxes or letterboxes doesn't look good, so by
    // default this is not done unless the size of the video vs the overall video area is
    // above the threshold defined by mPillarboxThreshold. By default this is set to 0.85
    // for the X axis and 0.90 for the Y axis, but this is theme-controllable via the
    // pillarboxThreshold property.
    if (mVideoAreaPos != glm::vec2 {0.0f, 0.0f} && mVideoAreaSize != glm::vec2 {0.0f, 0.0f}) {
        mVideoRectangleCoords.clear();
        mRectangleOffset = {0.0f, 0.0f};

        if ((mLegacyTheme && Settings::getInstance()->getBool("GamelistVideoPillarbox")) ||
            (!mLegacyTheme && mDrawPillarboxes)) {
            float rectHeight {0.0f};
            float rectWidth {0.0f};
            // Video is in landscape orientation.
            if (mSize.x > mSize.y) {
                // Checking the Y size should not normally be required as landscape format
                // should mean the height can't be higher than the max size defined by the
                // theme. But as the height in mSize is provided by FFmpeg in integer format
                // and then scaled, there could be rounding errors that make the video height
                // slightly higher than allowed. It's only a single pixel or a few pixels, but
                // it's still visible for some videos.
                if (mSize.y < mVideoAreaSize.y &&
                    mSize.y / mVideoAreaSize.y < mPillarboxThreshold.y)
                    rectHeight = mVideoAreaSize.y;
                else
                    rectHeight = mSize.y;
                if (mSize.x < mVideoAreaSize.x &&
                    mSize.x / mVideoAreaSize.x < mPillarboxThreshold.x)
                    rectWidth = mVideoAreaSize.x;
                else
                    rectWidth = mSize.x;
            }
            // Video is in portrait orientation (or completely square).
            else {
                if (mSize.x <= mVideoAreaSize.x &&
                    mSize.x / mVideoAreaSize.x < mPillarboxThreshold.x)
                    rectWidth = mVideoAreaSize.x;
                else
                    rectWidth = mSize.x;
                rectHeight = mSize.y;
            }
            // If an origin value other than 0.5 is used, then create an offset for centering
            // the video correctly.
            if (mOrigin != glm::vec2 {0.5f, 0.5f}) {
                if (rectWidth > mSize.x)
                    mRectangleOffset.x -= (rectWidth - mSize.x) * (mOrigin.x - 0.5f);
                else if (rectHeight > mSize.y)
                    mRectangleOffset.y -= (rectHeight - mSize.y) * (mOrigin.y - 0.5f);
            }

            // Populate the rectangle coordinates to be used in render().
            const float offsetX {rectWidth - mSize.x};
            const float offsetY {rectHeight - mSize.y};
            mVideoRectangleCoords.emplace_back(std::round((-offsetX / 2.0f) + mRectangleOffset.x));
            mVideoRectangleCoords.emplace_back(std::round((-offsetY / 2.0f) + mRectangleOffset.y));
            mVideoRectangleCoords.emplace_back(std::round(rectWidth));
            mVideoRectangleCoords.emplace_back(std::round(rectHeight));
        }
        // If the option to display pillarboxes is disabled, then make the rectangle equivalent
        // to the size of the video.
        else {
            mVideoRectangleCoords.emplace_back(0.0f);
            mVideoRectangleCoords.emplace_back(0.0f);
            mVideoRectangleCoords.emplace_back(std::round(mSize.x));
            mVideoRectangleCoords.emplace_back(std::round(mSize.y));
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
    bool hasDXVA2 {false};
    bool hasD3D11VA {false};

    AVBufferRef* testContext {nullptr};
    AVHWDeviceType tempDevice {AV_HWDEVICE_TYPE_NONE};

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

    bool hasVAAPI {false};
    bool hasVDPAU {false};

    AVBufferRef* testContext {nullptr};
    AVHWDeviceType tempDevice {AV_HWDEVICE_TYPE_NONE};

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
    for (int i = 0; i < 50; ++i) {
        const AVCodecHWConfig* config {avcodec_get_hw_config(mHardwareCodec, i)};
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

        for (pixelFormats = pix_fmts; *pixelFormats != -1; ++pixelFormats)
            if (*pixelFormats == sPixelFormat)
                return static_cast<enum AVPixelFormat>(sPixelFormat);

        return AV_PIX_FMT_NONE;
    };

    // Check if the video can actually be hardware decoded (unless this has already been done).
    if (std::find(sHWDecodedVideos.begin(), sHWDecodedVideos.end(), mVideoPath) ==
        sHWDecodedVideos.end()) {

        // clang-format on
        AVCodecContext* checkCodecContext {avcodec_alloc_context3(mHardwareCodec)};

        if (avcodec_parameters_to_context(checkCodecContext, mVideoStream->codecpar)) {
            LOG(LogError) << "VideoFFmpegComponent::decoderInitHW(): "
                             "Couldn't fill the video codec context parameters for file \""
                          << mVideoPath << "\"";
            avcodec_free_context(&checkCodecContext);
            return true;
        }
        else {
            bool onlySWDecode {false};

            checkCodecContext->get_format = formatFunc;
            checkCodecContext->hw_device_ctx = av_buffer_ref(mHwContext);

            if (avcodec_open2(checkCodecContext, mHardwareCodec, nullptr)) {
                LOG(LogError) << "VideoFFmpegComponent::decoderInitHW(): "
                                 "Couldn't initialize the video codec context for file \""
                              << mVideoPath << "\"";
            }

            AVPacket* checkPacket {av_packet_alloc()};
            int readFrameReturn {0};

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
                    AVFrame* checkFrame {av_frame_alloc()};

                    onlySWDecode = true;

                    // For some videos we need to process at least one extra frame to verify
                    // that the hardware encoder can actually be used, otherwise the fallback
                    // to software decoding would take place when it's not necessary.
                    for (int i = 0; i < 3; ++i) {
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
        LOG(LogError) << "VideoFFmpegComponent::decoderInitHW(): "
                         "Couldn't allocate video codec context for file \""
                      << mVideoPath << "\"";
        avcodec_free_context(&mVideoCodecContext);
        return true;
    }

    if (avcodec_parameters_to_context(mVideoCodecContext, mVideoStream->codecpar)) {
        LOG(LogError) << "VideoFFmpegComponent::decoderInitHW(): "
                         "Couldn't fill the video codec context parameters for file \""
                      << mVideoPath << "\"";
        avcodec_free_context(&mVideoCodecContext);
        return true;
    }

    mVideoCodecContext->get_format = formatFunc;
    mVideoCodecContext->hw_device_ctx = av_buffer_ref(mHwContext);

    if (avcodec_open2(mVideoCodecContext, mHardwareCodec, nullptr)) {
        LOG(LogError) << "VideoFFmpegComponent::decoderInitHW(): "
                         "Couldn't initialize the video codec context for file \""
                      << mVideoPath << "\"";
        avcodec_free_context(&mVideoCodecContext);
        return true;
    }

    return false;
}

void VideoFFmpegComponent::startVideoStream()
{
    if (!mVisible || mThemeOpacity == 0.0f)
        return;

    mIsPlaying = true;

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
        mVideoFrameReadCount = 0;
        mVideoFrameDroppedCount = 0;
        mOutputPicture = {};

        // Get an empty texture for rendering the video.
        mTexture = TextureResource::get("");

        // This is used for the audio and video synchronization.
        mTimeReference = std::chrono::high_resolution_clock::now();

        // Clear the video and audio frame queues.
        std::queue<VideoFrame>().swap(mVideoFrameQueue);
        std::queue<AudioFrame>().swap(mAudioFrameQueue);

        std::string filePath {"file:" + mVideoPath};

        // This will disable the FFmpeg logging, so comment this out if debug info is needed.
        av_log_set_callback(nullptr);

        // File operations and basic setup.

        if (avformat_open_input(&mFormatContext, filePath.c_str(), nullptr, nullptr)) {
            LOG(LogError) << "VideoFFmpegComponent::startVideoStream(): "
                             "Couldn't open video file \""
                          << mVideoPath << "\"";
            return;
        }

        if (avformat_find_stream_info(mFormatContext, nullptr)) {
            LOG(LogError) << "VideoFFmpegComponent::startVideoStream(): "
                             "Couldn't read stream information from video file \""
                          << mVideoPath << "\"";
            return;
        }

        mVideoStreamIndex = -1;
        mAudioStreamIndex = -1;

        // Video stream setup.

#if defined(VIDEO_HW_DECODING)
        bool hwDecoding {Settings::getInstance()->getBool("VideoHardwareDecoding")};
#else
        bool hwDecoding {false};
#endif

#if LIBAVUTIL_VERSION_MAJOR > 56
        mVideoStreamIndex = av_find_best_stream(mFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1,
                                                const_cast<const AVCodec**>(&mHardwareCodec), 0);
#else
        mVideoStreamIndex =
            av_find_best_stream(mFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &mHardwareCodec, 0);
#endif

        if (mVideoStreamIndex < 0) {
            LOG(LogError) << "VideoFFmpegComponent::startVideoStream(): "
                             "Couldn't retrieve video stream for file \""
                          << mVideoPath << "\"";
            avformat_close_input(&mFormatContext);
            avformat_free_context(mFormatContext);
            return;
        }

        mVideoStream = mFormatContext->streams[mVideoStreamIndex];
        mVideoWidth = mFormatContext->streams[mVideoStreamIndex]->codecpar->width;
        mVideoHeight = mFormatContext->streams[mVideoStreamIndex]->codecpar->height;

        LOG(LogDebug) << "VideoFFmpegComponent::startVideoStream(): "
#if defined(_WIN64)
                      << "Playing video \"" << Utils::String::replace(mVideoPath, "/", "\\")
                      << "\" (codec: "
#else
                      << "Playing video \"" << mVideoPath << "\" (codec: "
#endif
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
                LOG(LogDebug)
                    << "VideoFFmpegComponent::startVideoStream(): Hardware decoding failed, "
                       "falling back to software decoder";
            }

            mVideoCodec =
                const_cast<AVCodec*>(avcodec_find_decoder(mVideoStream->codecpar->codec_id));

            if (!mVideoCodec) {
                LOG(LogError) << "VideoFFmpegComponent::startVideoStream(): "
                                 "Couldn't find a suitable video codec for file \""
                              << mVideoPath << "\"";
                return;
            }

            mVideoCodecContext = avcodec_alloc_context3(mVideoCodec);

            if (!mVideoCodecContext) {
                LOG(LogError) << "VideoFFmpegComponent::startVideoStream(): "
                                 "Couldn't allocate video codec context for file \""
                              << mVideoPath << "\"";
                return;
            }

#if LIBAVUTIL_VERSION_MAJOR < 58
            if (mVideoCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
                mVideoCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED;
#endif

            if (avcodec_parameters_to_context(mVideoCodecContext, mVideoStream->codecpar)) {
                LOG(LogError) << "VideoFFmpegComponent::startVideoStream(): "
                                 "Couldn't fill the video codec context parameters for file \""
                              << mVideoPath << "\"";
                return;
            }

            if (avcodec_open2(mVideoCodecContext, mVideoCodec, nullptr)) {
                LOG(LogError) << "VideoFFmpegComponent::startVideoStream(): "
                                 "Couldn't initialize the video codec context for file \""
                              << mVideoPath << "\"";
                return;
            }
        }

        // Audio stream setup, optional as some videos do not have any audio tracks.
        // Audio can also be disabled per video via the theme configuration.

        if (mPlayAudio) {
            mAudioStreamIndex =
                av_find_best_stream(mFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

            if (mAudioStreamIndex < 0) {
                LOG(LogDebug) << "VideoFFmpegComponent::startVideoStream(): "
                                 "File does not seem to contain any audio streams";
            }

            if (mAudioStreamIndex >= 0) {
                mAudioStream = mFormatContext->streams[mAudioStreamIndex];
                mAudioCodec =
                    const_cast<AVCodec*>(avcodec_find_decoder(mAudioStream->codecpar->codec_id));

                if (!mAudioCodec) {
                    LOG(LogError) << "Couldn't find a suitable audio codec for file \""
                                  << mVideoPath << "\"";
                    return;
                }

                mAudioCodecContext = avcodec_alloc_context3(mAudioCodec);

#if LIBAVUTIL_VERSION_MAJOR < 58
                if (mAudioCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
                    mAudioCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED;
#endif

                // Some formats want separate stream headers.
                if (mAudioCodecContext->flags & AVFMT_GLOBALHEADER)
                    mAudioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

                if (avcodec_parameters_to_context(mAudioCodecContext, mAudioStream->codecpar)) {
                    LOG(LogError) << "VideoFFmpegComponent::startVideoStream(): "
                                     "Couldn't fill the audio codec context parameters for file \""
                                  << mVideoPath << "\"";
                    return;
                }

                if (avcodec_open2(mAudioCodecContext, mAudioCodec, nullptr)) {
                    LOG(LogError) << "VideoFFmpegComponent::startVideoStream(): "
                                     "Couldn't initialize the audio codec context for file \""
                                  << mVideoPath << "\"";
                    return;
                }
            }
        }

        mVideoTimeBase = 1.0l / av_q2d(mVideoStream->avg_frame_rate);

        // Set some reasonable target queue sizes (buffers).
        mVideoTargetQueueSize = static_cast<int>(av_q2d(mVideoStream->avg_frame_rate) / 2.0l);
        if (mAudioStreamIndex >= 0)
            mAudioTargetQueueSize = mAudioStream->codecpar->CHANNELS * 15;
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

        mFadeIn = 0.0f;
    }
}

void VideoFFmpegComponent::stopVideoPlayer(bool muteAudio)
{
    if (muteAudio)
        muteVideoPlayer();

    mIsPlaying = false;
    mIsActuallyPlaying = false;
    mPaused = false;
    mEndOfVideo = false;
    mTexture.reset();

    if (mFrameProcessingThread) {
        if (mWindow->getVideoPlayerCount() == 0)
            AudioManager::getInstance().muteStream();
        // Wait for the thread execution to complete.
        mFrameProcessingThread->join();
        mFrameProcessingThread.reset();
        mOutputAudio.clear();
    }

    // Clear the video and audio frame queues.
    std::queue<VideoFrame>().swap(mVideoFrameQueue);
    std::queue<AudioFrame>().swap(mAudioFrameQueue);

    // Clear the audio buffer.
    if (AudioManager::sAudioDevice != 0)
        AudioManager::getInstance().clearStream();

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

void VideoFFmpegComponent::pauseVideoPlayer()
{
    muteVideoPlayer();
    mPaused = true;
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
            stopVideoPlayer();
            startVideoStream();
        }
    }
}

void VideoFFmpegComponent::muteVideoPlayer()
{
    if (AudioManager::sAudioDevice != 0) {
        AudioManager::getInstance().clearStream();
        AudioManager::getInstance().muteStream();
    }
}

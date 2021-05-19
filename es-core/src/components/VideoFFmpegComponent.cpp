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
        mVideoTimeBase(0.0l),
        mVideoMinQueueSize(0),
        mAccumulatedTime(0),
        mStartTimeAccumulation(false),
        mDecodedFrame(false),
        mEndOfVideo(false)
{
    // Get an empty texture for rendering the video.
    mTexture = TextureResource::get("");
}

VideoFFmpegComponent::~VideoFFmpegComponent()
{
    stopVideo();
    mTexture.reset();
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
        if (mFadeIn < 1) {
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

void VideoFFmpegComponent::update(int deltaTime)
{
    if (mPause || !mFormatContext)
        return;

    if (mIsActuallyPlaying && mStartTimeAccumulation) {
        mAccumulatedTime += static_cast<double>(
                std::chrono::duration_cast<std::chrono::nanoseconds>
                (std::chrono::high_resolution_clock::now() -
                mTimeReference).count()) / 1000000000.0l;
    }

    mTimeReference = std::chrono::high_resolution_clock::now();

    if (!mFrameProcessingThread)
        mFrameProcessingThread =
                std::make_unique<std::thread>(&VideoFFmpegComponent::frameProcessing, this);
}

void VideoFFmpegComponent::frameProcessing()
{
    while (mIsPlaying) {
        readFrames();

        if (!mEndOfVideo && mIsActuallyPlaying &&
                mVideoFrameQueue.empty() && mAudioFrameQueue.empty())
            mEndOfVideo = true;

        outputFrames();

        // This 1 ms wait makes sure that the thread does not consume all available CPU cycles.
        SDL_Delay(1);
    }
}

void VideoFFmpegComponent::readFrames()
{
    if (mVideoCodecContext && mFormatContext) {
        if (mVideoFrameQueue.size() < mVideoMinQueueSize || (mAudioStreamIndex >= 0 &&
                mAudioFrameQueue.size() < mAudioMinQueueSize)) {
            while(av_read_frame(mFormatContext, mPacket) >= 0) {
                if (mPacket->stream_index == mVideoStreamIndex) {
                    if (!avcodec_send_packet(mVideoCodecContext, mPacket) &&
                            !avcodec_receive_frame(mVideoCodecContext, mVideoFrame)) {

                        // We have a video frame that needs conversion to RGBA format.
                        uint8_t* frameRGBA[4];
                        int lineSize[4];
                        int allocatedSize = 0;

                        // The pts value is the presentation time, i.e. the time stamp when
                        // the frame (picture) should be displayed. The packet dts value is
                        // used for the basis of the calculation as per the recommendation
                        // in the FFmpeg documentation for the av_read_frame function.
                        double pts = static_cast<double>(mPacket->dts) *
                                av_q2d(mVideoStream->time_base);

                        double frameDuration = static_cast<double>(mPacket->duration) *
                                av_q2d(mVideoStream->time_base);

                        // Due to some unknown reason, attempting to scale frames where
                        // coded_width is larger than width leads to graphics corruption or
                        // crashes. The only workaround I've been able to find is to decrease the
                        // source width by one pixel. Unfortunately this leads to a noticeably
                        // softer picture, but as few videos have this issue it's an acceptable
                        // workaround for now. Possibly this problem is caused by an FFmpeg bug.
                        int sourceWidth = mVideoCodecContext->width;
                        if (mVideoCodecContext->coded_width > mVideoCodecContext->width)
                            sourceWidth--;

                        // Conversion using libswscale.
                        struct SwsContext* conversionContext = sws_getContext(
                                sourceWidth,
                                mVideoCodecContext->height,
                                mVideoCodecContext->pix_fmt,
                                mVideoFrame->width,
                                mVideoFrame->height,
                                AV_PIX_FMT_RGBA,
                                SWS_BILINEAR,
                                nullptr,
                                nullptr,
                                nullptr);

                        allocatedSize = av_image_alloc(
                                frameRGBA,
                                lineSize,
                                mVideoFrame->width,
                                mVideoFrame->height,
                                AV_PIX_FMT_RGB32,
                                1);

                        sws_scale(
                                conversionContext,
                                const_cast<uint8_t const* const*>(mVideoFrame->data),
                                mVideoFrame->linesize,
                                0,
                                mVideoCodecContext->height,
                                frameRGBA,
                                lineSize);

                        VideoFrame currFrame;

                        // Save the frame into the queue for later processing.
                        currFrame.width = mVideoFrame->width;
                        currFrame.height = mVideoFrame->height;
                        currFrame.pts = pts;
                        currFrame.frameDuration = frameDuration;

                        currFrame.frameRGBA.insert(currFrame.frameRGBA.begin(),
                                &frameRGBA[0][0], &frameRGBA[0][allocatedSize]);

                        mVideoFrameQueue.push(currFrame);

                        av_freep(&frameRGBA[0]);
                        sws_freeContext(conversionContext);
                        av_packet_unref(mPacket);
                        break;
                    }
                }
                else if (mPacket->stream_index == mAudioStreamIndex) {
                    if (!avcodec_send_packet(mAudioCodecContext, mPacket) &&
                            !avcodec_receive_frame(mAudioCodecContext, mAudioFrame)) {

                        // We have a audio frame that needs to be converted using libswresample.
                        SwrContext* resampleContext = nullptr;
                        uint8_t** convertedData = nullptr;
                        int numConvertedSamples = 0;
                        int resampledDataSize = 0;

                        enum AVSampleFormat outSampleFormat = AV_SAMPLE_FMT_FLT;
                        int outSampleRate = mAudioCodecContext->sample_rate;

                        int64_t inChannelLayout = mAudioCodecContext->channel_layout;
                        int64_t outChannelLayout = AV_CH_LAYOUT_STEREO;
                        int outNumChannels = 0;
                        int outChannels = 2;
                        int inNumSamples = 0;
                        int outMaxNumSamples = 0;
                        int outLineSize = 0;

                        // The pts value is the presentation time, i.e. the time stamp when
                        // the audio should be played.
                        double timeBase = av_q2d(mAudioStream->time_base);
                        double pts = mAudioFrame->pts * av_q2d(mAudioStream->time_base);

                        // Audio resampler setup. We only perform channel rematrixing and
                        // format conversion here, the sample rate is left untouched.
                        // There is a sample rate conversion in AudioManager and we don't
                        // want to resample twice. And for some files there may not be any
                        // resampling needed at all if the format is the same as the output
                        // format for the application.
                        int outNumSamples = static_cast<int>(av_rescale_rnd(mAudioFrame->nb_samples,
                                outSampleRate, mAudioCodecContext->sample_rate, AV_ROUND_UP));

                        resampleContext = swr_alloc();
                        if (!resampleContext) {
                            LOG(LogError) << "VideoFFmpegComponent::readFrames() Couldn't "
                                    "allocate audio resample context";
                        }

                        inChannelLayout = (mAudioCodecContext->channels ==
                                av_get_channel_layout_nb_channels(
                                mAudioCodecContext->channel_layout)) ?
                                mAudioCodecContext->channel_layout :
                                av_get_default_channel_layout(mAudioCodecContext->channels);

                        if (outChannels == 1)
                            outChannelLayout = AV_CH_LAYOUT_MONO;
                        else if (outChannels == 2)
                            outChannelLayout = AV_CH_LAYOUT_STEREO;
                        else
                            outChannelLayout = AV_CH_LAYOUT_SURROUND;

                        inNumSamples = mAudioFrame->nb_samples;

                        av_opt_set_int(resampleContext, "in_channel_layout", inChannelLayout, 0);
                        av_opt_set_int(resampleContext, "in_sample_rate",
                                mAudioCodecContext->sample_rate, 0);
                        av_opt_set_sample_fmt(resampleContext, "in_sample_fmt",
                                mAudioCodecContext->sample_fmt, 0);

                        av_opt_set_int(resampleContext, "out_channel_layout", outChannelLayout, 0);
                        av_opt_set_int(resampleContext, "out_sample_rate", outSampleRate, 0);
                        av_opt_set_sample_fmt(resampleContext, "out_sample_fmt",
                                outSampleFormat, 0);

                        if (swr_init(resampleContext) < 0) {
                            LOG(LogError) << "VideoFFmpegComponent::readFrames() Couldn't "
                                    "initialize the resampling context";
                        }

                        outMaxNumSamples = outNumSamples =
                                static_cast<int>(av_rescale_rnd(inNumSamples, outSampleRate,
                                mAudioCodecContext->sample_rate, AV_ROUND_UP));

                        outNumChannels = av_get_channel_layout_nb_channels(outChannelLayout);

                        av_samples_alloc_array_and_samples(
                                &convertedData,
                                &outLineSize,
                                outNumChannels,
                                outNumSamples,
                                outSampleFormat,
                                1);

                        outNumSamples = static_cast<int>(av_rescale_rnd(swr_get_delay(
                                resampleContext, mAudioCodecContext->sample_rate) + inNumSamples,
                                outSampleRate, mAudioCodecContext->sample_rate, AV_ROUND_UP));

                        if (outNumSamples > outMaxNumSamples) {
                            av_freep(&convertedData[0]);
                            av_samples_alloc(
                                    convertedData,
                                    &outLineSize,
                                    outNumChannels,
                                    outNumSamples,
                                    outSampleFormat,
                                    1);
                            outMaxNumSamples = outNumSamples;
                        }

                        // Perform the actual conversion.
                        if (resampleContext) {
                            numConvertedSamples = swr_convert(
                                    resampleContext,
                                    convertedData,
                                    outNumSamples,
                                    const_cast<const uint8_t**>(mAudioFrame->data),
                                    mAudioFrame->nb_samples);
                            if (numConvertedSamples < 0) {
                                LOG(LogError) << "VideoFFmpegComponent::readFrames() Audio "
                                    "resampling failed";
                            }

                            resampledDataSize = av_samples_get_buffer_size(
                                    &outLineSize,
                                    outNumChannels,
                                    numConvertedSamples,
                                    outSampleFormat,
                                    1);
                            if (resampledDataSize < 0) {
                                LOG(LogError) << "VideoFFmpegComponent::readFrames() Audio "
                                    "resampling did not generated any output";
                            }
                        }

                        AudioFrame currFrame;

                        // Save the frame into the queue for later processing.
                        currFrame.resampledData.insert(currFrame.resampledData.begin(),
                                &convertedData[0][0], &convertedData[0][resampledDataSize]);
                        currFrame.resampledDataSize = resampledDataSize;
                        currFrame.pts = pts;

                        mAudioFrameQueue.push(currFrame);

                        if (convertedData) {
                            av_freep(&convertedData[0]);
                            av_freep(&convertedData);
                        }

                        if (resampleContext)
                            swr_free(&resampleContext);

                        av_packet_unref(mPacket);
                        continue;
                    }
                }
            }
        }
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

    // Process the audio frames that have a pts value below mAccumulatedTime (plus a small
    // buffer to avoid underflows).
    while (!mAudioFrameQueue.empty()) {
        if (mAudioFrameQueue.front().pts < mAccumulatedTime + AUDIO_BUFFER) {
            // Enable only when needed, as this generates a lot of debug output.
//            LOG(LogDebug) << "Processing audio frame with PTS: " <<
//            mAudioFrameQueue.front().pts;
//            LOG(LogDebug) << "Total audio frames processed / audio frame queue size: " <<
//            mAudioFrameCount << " / " << std::to_string(mAudioFrameQueue.size());

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
                AudioManager::getInstance()->processStream(
                        &mAudioFrameQueue.front().resampledData.at(0),
                        mAudioFrameQueue.front().resampledDataSize);
            }
            mAudioFrameQueue.pop();
            mAudioFrameCount++;
        }
        else {
            break;
        }
    }

    // Process all available video frames that have a pts value below mAccumulatedTime.
    // But if more than one frame is processed here, it means that the computer can't
    // keep up for some reason.
    while (mIsActuallyPlaying && !mVideoFrameQueue.empty()) {
        if (mVideoFrameQueue.front().pts < mAccumulatedTime) {
            // Enable only when needed, as this generates a lot of debug output.
//            LOG(LogDebug) << "Processing video frame with PTS: " <<
//                    mVideoFrameQueue.front().pts;
//            LOG(LogDebug) << "Total video frames processed / video frame queue size: " <<
//                    mVideoFrameCount << " / " << std::to_string(mVideoFrameQueue.size());

            mPictureMutex.lock();

            // Give some leeway for frames that have not yet been rendered but that have pts
            // values with a time difference relative to the frame duration that is under a
            // certain threshold. In this case, give the renderer an additional chance to output
            // the frames. If the difference exceeds the threshold though, then skip them as
            // otherwise videos would just slow down instead of skipping frames when the computer
            // can't keep up. This approach primarily decreases stuttering for videos with frame
            // rates close to, or at, the rendering frame rate, for example 59.94 and 60 fps.
            if (!mOutputPicture.hasBeenRendered) {
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

            AudioManager::getInstance()->setupAudioStream(mAudioCodecContext->sample_rate);
        }

        mVideoTimeBase = 1.0l / av_q2d(mVideoStream->avg_frame_rate);
        // Set some reasonable minimum queue sizes (buffers).
        mVideoMinQueueSize = static_cast<int>(av_q2d(mVideoStream->avg_frame_rate) / 2.0l);
        if (mAudioStreamIndex >=0)
            mAudioMinQueueSize = mAudioStream->codecpar->channels * 15;
        else
            mAudioMinQueueSize = 30;

        mPacket = av_packet_alloc();
        mVideoFrame = av_frame_alloc();
        mAudioFrame = av_frame_alloc();

        // Resize the video surface, which is needed both for the gamelist view and for the
        // video screeensaver.
        resize();

        // Calculate pillarbox/letterbox sizes.
        calculateBlackRectangle();

        mIsPlaying = true;
    }
}

void VideoFFmpegComponent::stopVideo()
{
    mIsPlaying = false;
    mIsActuallyPlaying = false;
    mStartDelayed = false;
    mPause = false;
    mEndOfVideo = false;

    if (mFrameProcessingThread) {
        // Wait for the thread execution to complete.
        mFrameProcessingThread->join();
        mFrameProcessingThread.reset();
    }

    // Clear the video and audio frame queues.
    std::queue<VideoFrame>().swap(mVideoFrameQueue);
    std::queue<AudioFrame>().swap(mAudioFrameQueue);

    if (mFormatContext) {
        av_frame_free(&mVideoFrame);
        av_frame_free(&mAudioFrame);
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

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoVlcComponent.cpp
//
//  Video player based on libVLC.
//

#if defined(BUILD_VLC_PLAYER)

#include "components/VideoVlcComponent.h"

#include "AudioManager.h"
#include "Settings.h"
#include "Window.h"
#include "renderers/Renderer.h"
#include "resources/TextureResource.h"
#include "utils/StringUtil.h"

#if defined(__APPLE__)
#include "utils/FileSystemUtil.h"
#endif

#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_timer.h>
#include <vlc/vlc.h>

#if defined(_WIN64)
#include <codecvt>
#include <cstring>
#endif

libvlc_instance_t* VideoVlcComponent::mVLC = nullptr;

VideoVlcComponent::VideoVlcComponent(Window* window)
    : VideoComponent(window)
    , mMediaPlayer(nullptr)
    , mMedia(nullptr)
    , mContext({})
    , mHasSetAudioVolume(false)
{
    // Get an empty texture for rendering the video.
    mTexture = TextureResource::get("");

    // Make sure VLC has been initialized.
    setupVLC();
}

VideoVlcComponent::~VideoVlcComponent()
{
    stopVideo();
    mTexture.reset();
}

void VideoVlcComponent::deinit()
{
    if (mVLC) {
        libvlc_release(mVLC);
        mVLC = nullptr;
    }
}

void VideoVlcComponent::setResize(float width, float height)
{
    // This resize function is used when stretching videos to full screen in the video screensaver.
    mTargetSize = glm::vec2(width, height);
    mTargetIsMax = false;
    mStaticImage.setResize(width, height);
    resize();
}

void VideoVlcComponent::setMaxSize(float width, float height)
{
    // This resize function is used in most instances, such as non-stretched video screensaver
    // and the gamelist videos.
    mTargetSize = glm::vec2(width, height);
    mTargetIsMax = true;
    mStaticImage.setMaxSize(width, height);
    resize();
}

void VideoVlcComponent::setupVLC()
{
    // If VLC hasn't been initialised yet then do it now.
    if (!mVLC) {
        const char* args[] = { "--quiet" };

#if defined(__APPLE__)
        // It's required to set the VLC_PLUGIN_PATH variable on macOS, or the libVLC
        // initialization will fail (with no error message).
        std::string vlcPluginPath = Utils::FileSystem::getExePath() + "/plugins";
        if (Utils::FileSystem::isDirectory(vlcPluginPath))
            setenv("VLC_PLUGIN_PATH", vlcPluginPath.c_str(), 1);
        else
            setenv("VLC_PLUGIN_PATH", "/Applications/VLC.app/Contents/MacOS/plugins/", 1);
#endif

        mVLC = libvlc_new(1, args);
    }
}

void VideoVlcComponent::setupContext()
{
    if (!mContext.valid) {
        // Create an RGBA surface to render the video into.
        mContext.surface = SDL_CreateRGBSurface(SDL_SWSURFACE, static_cast<int>(mVideoWidth),
                                                static_cast<int>(mVideoHeight), 32, 0xff000000,
                                                0x00ff0000, 0x0000ff00, 0x000000ff);
        mContext.mutex = SDL_CreateMutex();
        mContext.valid = true;
        resize();
        mHasSetAudioVolume = false;
    }
}

void VideoVlcComponent::freeContext()
{
    if (mContext.valid) {
        SDL_FreeSurface(mContext.surface);
        SDL_DestroyMutex(mContext.mutex);
        mContext.valid = false;
    }
}

void VideoVlcComponent::resize()
{
    if (!mTexture)
        return;

    const glm::vec2 textureSize(static_cast<float>(mVideoWidth), static_cast<float>(mVideoHeight));

    if (textureSize == glm::vec2({}))
        return;

    if (mTargetIsMax) {
        mSize = textureSize;

        glm::vec2 resizeScale((mTargetSize.x / mSize.x), (mTargetSize.y / mSize.y));

        if (resizeScale.x < resizeScale.y) {
            mSize.x *= resizeScale.x;
            mSize.y *= resizeScale.x;
        }
        else {
            mSize.x *= resizeScale.y;
            mSize.y *= resizeScale.y;
        }

        mSize.x = (mSize.y / textureSize.y) * textureSize.x;
        mSize.y = std::round(mSize[1]);
    }
    else {
        // If both components are set, we just stretch.
        // If no components are set, we don't resize at all.
        mSize = mTargetSize == glm::vec2({}) ? textureSize : mTargetSize;

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

void VideoVlcComponent::render(const glm::mat4& parentTrans)
{
    // Set the audio volume. As libVLC is very unreliable we need to make an additional
    // attempt here in the render loop in addition to the initialization in startVideo().
    // This is required under some circumstances such as when running on a slow computer
    // or sometimes even on a faster machine when changing to the video view style or
    // when starting the application directly into a gamelist.
    if (!mHasSetAudioVolume && mMediaPlayer)
        setAudioVolume();

    VideoComponent::render(parentTrans);
    glm::mat4 trans = parentTrans * getTransform();
    GuiComponent::renderChildren(trans);

    // Check the actual VLC state, i.e. if the video is really playing rather than
    // still being opened.
    if (mMediaPlayer && mIsPlaying && !mIsActuallyPlaying) {
        libvlc_state_t state;
        state = libvlc_media_player_get_state(mMediaPlayer);
        if (state == libvlc_Playing)
            mIsActuallyPlaying = true;
    }

    if (mIsPlaying && mContext.valid && mIsActuallyPlaying) {
        unsigned int color;
        if (mFadeIn < 1) {
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
        vertices[0] = { { 0.0f   , 0.0f    }, { 0.0f, 0.0f }, color };
        vertices[1] = { { 0.0f   , mSize.y }, { 0.0f, 1.0f }, color };
        vertices[2] = { { mSize.x, 0.0f    }, { 1.0f, 0.0f }, color };
        vertices[3] = { { mSize.x, mSize.y }, { 1.0f, 1.0f }, color };
        // clang-format on

        // Round vertices.
        for (int i = 0; i < 4; i++)
            vertices[i].pos = glm::round(vertices[i].pos);

        // Build a texture for the video frame.
        mTexture->initFromPixels(reinterpret_cast<unsigned char*>(mContext.surface->pixels),
                                 mContext.surface->w, mContext.surface->h);
        mTexture->bind();

#if defined(USE_OPENGL_21)
        // Render scanlines if this option is enabled. However, if this is the media viewer
        // or the video screensaver, then skip this as the scanline rendering is then handled
        // in those modules as a postprocessing step.
        if ((!mScreensaverMode && !mMediaViewerMode) &&
            Settings::getInstance()->getBool("GamelistVideoScanlines")) {
            vertices[0].shaders = Renderer::SHADER_SCANLINES;
        }
#endif

        // Render it.
        Renderer::setMatrix(trans);
        Renderer::drawTriangleStrips(&vertices[0], 4, trans);
    }
    else {
        VideoComponent::renderSnapshot(parentTrans);
    }
}

void VideoVlcComponent::calculateBlackRectangle()
{
    // Calculate the position and size for the black rectangle that will be rendered behind
    // videos. If the option to display pillarboxes (and letterboxes) is enabled, then this
    // would extend to the entire md_video area (if above the threshold as defined below) or
    // otherwise it will exactly match the video size. The reason to add a black rectangle
    // behind videos in this second instance is that the scanline rendering will make the
    // video partially transparent so this may avoid some unforseen issues with some themes.
    if (mVideoAreaPos != glm::vec2({}) && mVideoAreaSize != glm::vec2({})) {
        mVideoRectangleCoords.clear();

        if (Settings::getInstance()->getBool("GamelistVideoPillarbox")) {
            float rectHeight;
            float rectWidth;
            // Video is in landscape orientation.
            if (mSize.x > mSize.y) {
                // Checking the Y size should not normally be required as landscape format
                // should mean the height can't be higher than the max size defined by the
                // theme. But as the height in mSize is provided by libVLC in integer format
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

void VideoVlcComponent::setAudioVolume()
{
    if (mMediaPlayer && libvlc_media_player_get_state(mMediaPlayer) == libvlc_Playing) {
        // This small delay may avoid a race condition in libVLC that could crash the application.
        SDL_Delay(2);

        bool outputSound = false;

        if ((!mScreensaverMode && !mMediaViewerMode) &&
            Settings::getInstance()->getBool("GamelistVideoAudio"))
            outputSound = true;
        else if (mScreensaverMode && Settings::getInstance()->getBool("ScreensaverVideoAudio"))
            outputSound = true;
        else if (mMediaViewerMode && Settings::getInstance()->getBool("MediaViewerVideoAudio"))
            outputSound = true;

        if (outputSound) {
            if (libvlc_audio_get_mute(mMediaPlayer) == 1)
                libvlc_audio_set_mute(mMediaPlayer, 0);
            libvlc_audio_set_volume(mMediaPlayer,
                                    Settings::getInstance()->getInt("SoundVolumeVideos"));
        }
        else {
            libvlc_audio_set_volume(mMediaPlayer, 0);
        }

        mHasSetAudioVolume = true;
    }
}

void VideoVlcComponent::startVideo()
{
    if (!mIsPlaying && !mGameLaunched) {
        mVideoWidth = 0;
        mVideoHeight = 0;

#if defined(_WIN64)
        std::string path(Utils::String::replace(mVideoPath, "/", "\\"));
#else
        std::string path(mVideoPath);
#endif
        // Make sure we have a video path.
        if (mVLC && (path.size() > 0)) {
            // Set the video that we are going to be playing so we don't attempt to restart it.
            mPlayingVideoPath = mVideoPath;

            // Open the media.
            mMedia = libvlc_media_new_path(mVLC, path.c_str());
            if (mMedia) {
                unsigned track_count;
                int parseResult;

                // Asynchronous media parsing.
                libvlc_event_attach(libvlc_media_event_manager(mMedia), libvlc_MediaParsedChanged,
                                    VlcMediaParseCallback, 0);
                parseResult = libvlc_media_parse_with_options(mMedia, libvlc_media_parse_local, -1);

                if (!parseResult) {
                    // Wait for a maximum of 1 second for the media parsing.
                    // This maximum time is quite excessive as this step should normally
                    // be completed in 15 - 30 ms or so.
                    for (int i = 0; i < 200; i++) {
                        if (libvlc_media_get_parsed_status(mMedia))
                            break;
                        SDL_Delay(5);
                    };
                }

                libvlc_media_track_t** tracks;
                track_count = libvlc_media_tracks_get(mMedia, &tracks);
                for (unsigned track = 0; track < track_count; track++) {
                    if (tracks[track]->i_type == libvlc_track_video) {
                        mVideoWidth = tracks[track]->video->i_width;
                        mVideoHeight = tracks[track]->video->i_height;
                        break;
                    }
                }
                libvlc_media_tracks_release(tracks, track_count);
                libvlc_media_parse_stop(mMedia);
                libvlc_event_detach(libvlc_media_event_manager(mMedia), libvlc_MediaParsedChanged,
                                    VlcMediaParseCallback, 0);

                // Make sure we found a valid video track.
                if ((mVideoWidth > 0) && (mVideoHeight > 0)) {
                    setupContext();

                    // Setup the media player.
                    mMediaPlayer = libvlc_media_player_new_from_media(mMedia);

                    // The code below enables the libVLC audio output to be processed inside ES-DE.
                    // Unfortunately this causes excessive stuttering for some reason that I still
                    // don't understand, so at the moment this code is disabled.
                    //                    auto audioFormatCallback = [](void **data, char *format,
                    //                            unsigned *rate, unsigned *channels) -> int {
                    //                        format = const_cast<char*>("F32L");
                    //                        *rate = 48000;
                    //                        *channels = 2;
                    //                        return 0;
                    //                    };
                    //
                    //                    libvlc_audio_set_format_callbacks(mMediaPlayer,
                    //                            audioFormatCallback, nullptr);
                    //
                    //                    auto audioPlayCallback = [](void* data, const void*
                    //                    samples,
                    //                            unsigned count, int64_t pts) {
                    //                        AudioManager::getInstance()->processStream(samples,
                    //                        count);
                    //                    };
                    //
                    //                    libvlc_audio_set_callbacks(mMediaPlayer,
                    //                    audioPlayCallback,
                    //                            nullptr, nullptr, nullptr, nullptr, this);

                    libvlc_video_set_format(mMediaPlayer, "RGBA", static_cast<int>(mVideoWidth),
                                            static_cast<int>(mVideoHeight),
                                            static_cast<int>(mVideoWidth * 4));

                    // Lock video memory as a preparation for rendering a frame.
                    auto videoLockCallback = [](void* data, void** p_pixels) -> void* {
                        struct VideoContext* videoContext =
                            reinterpret_cast<struct VideoContext*>(data);
                        SDL_LockMutex(videoContext->mutex);
                        SDL_LockSurface(videoContext->surface);
                        *p_pixels = videoContext->surface->pixels;
                        return nullptr; // Picture identifier, not needed here.
                    };

                    // Unlock the video memory after rendering a frame.
                    auto videoUnlockCallback = [](void* data, void*, void* const*) {
                        struct VideoContext* videoContext =
                            reinterpret_cast<struct VideoContext*>(data);
                        SDL_UnlockSurface(videoContext->surface);
                        SDL_UnlockMutex(videoContext->mutex);
                    };

                    libvlc_video_set_callbacks(mMediaPlayer, videoLockCallback, videoUnlockCallback,
                                               nullptr, reinterpret_cast<void*>(&mContext));

                    libvlc_media_player_play(mMediaPlayer);

                    // Calculate pillarbox/letterbox sizes.
                    calculateBlackRectangle();

                    libvlc_state_t state;
                    state = libvlc_media_player_get_state(mMediaPlayer);
                    if (state != libvlc_Playing) {
                        // Wait for a maximum of 100 ms for the status of the video to change
                        // to libvlc_Playing as there would otherwise be a brief flicker before
                        // the video starts to play. This is also required to prevent the
                        // application from crashing under some circumstances as changing the
                        // video player audio volume is apparently not properly handled by libVLC.
                        // This maximum time is quite excessive as this step should normally
                        // be completed in 4 - 16 ms or so even on slower machines.
                        for (int i = 0; i < 50; i++) {
                            state = libvlc_media_player_get_state(mMediaPlayer);
                            if (state == libvlc_Playing) {
                                // This additional delay is needed to prevent some kind of race
                                // condition in libVLC which would otherwise crash the application.
                                SDL_Delay(2);
                                break;
                            }
                            SDL_Delay(2);
                        };
                    }

                    // Attempt to set the audio volume. Under some circumstances it could fail
                    // as libVLC may not be correctly initialized. Therefore there is an
                    // additional call to this function in the render() loop.
                    setAudioVolume();

                    // Update the playing state.
                    mIsPlaying = true;
                    mFadeIn = 0.0f;
                }
            }
        }
    }
}

void VideoVlcComponent::stopVideo()
{
    mIsPlaying = false;
    mIsActuallyPlaying = false;
    mStartDelayed = false;
    mPause = false;
    // Release the media player so it stops calling back to us.
    if (mMediaPlayer) {
        libvlc_media_player_stop(mMediaPlayer);
        libvlc_media_player_release(mMediaPlayer);
        libvlc_media_release(mMedia);
        mMediaPlayer = nullptr;
        mMedia = nullptr;
        freeContext();
    }
}

void VideoVlcComponent::pauseVideo()
{
    // If a game has been launched and the flag to pause the video has been
    // set, then rewind and pause.
    if (!mPause || !mMediaPlayer)
        return;

    if (libvlc_media_player_get_state(mMediaPlayer) == libvlc_Playing) {
        libvlc_media_player_set_position(mMediaPlayer, 0.0f);
        libvlc_media_player_pause(mMediaPlayer);
    }
}

void VideoVlcComponent::handleLooping()
{
    if (mIsPlaying && mMediaPlayer) {
        libvlc_state_t state = libvlc_media_player_get_state(mMediaPlayer);
        if (state == libvlc_Ended) {
            // If the screensaver video swap time is set to 0, it means we should
            // skip to the next game when the video has finished playing.
            if (mScreensaverMode &&
                Settings::getInstance()->getInt("ScreensaverSwapVideoTimeout") == 0) {
                mWindow->screensaverTriggerNextGame();
            }
            else {
                libvlc_media_player_set_media(mMediaPlayer, mMedia);
                libvlc_media_player_play(mMediaPlayer);

                bool outputSound = false;

                if ((!mScreensaverMode && !mMediaViewerMode) &&
                    Settings::getInstance()->getBool("GamelistVideoAudio"))
                    outputSound = true;
                else if (mScreensaverMode &&
                         Settings::getInstance()->getBool("ScreensaverVideoAudio"))
                    outputSound = true;
                else if (mMediaViewerMode &&
                         Settings::getInstance()->getBool("MediaViewerVideoAudio"))
                    outputSound = true;

                if (!outputSound)
                    libvlc_audio_set_volume(mMediaPlayer, 0);
            }
        }
    }
}

#endif

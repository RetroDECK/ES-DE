//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoVlcComponent.cpp
//
//  Video playing using libVLC.
//

#include "components/VideoVlcComponent.h"

#include "renderers/Renderer.h"
#include "resources/TextureResource.h"
#include "utils/StringUtil.h"
#include "Settings.h"
#include "Window.h"

#if defined(__APPLE__)
#include "utils/FileSystemUtil.h"
#endif

#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_timer.h>
#include <vlc/vlc.h>

#if defined(_WIN64)
#include <cstring>
#include <codecvt>
#endif

libvlc_instance_t* VideoVlcComponent::mVLC = nullptr;

// VLC prepares to render a video frame.
static void* lock(void* data, void** p_pixels) {
    struct VideoContext* c = reinterpret_cast<struct VideoContext*>(data);
    SDL_LockMutex(c->mutex);
    SDL_LockSurface(c->surface);
    *p_pixels = c->surface->pixels;
    return nullptr; // Picture identifier, not needed here.
}

// VLC just rendered a video frame.
static void unlock(void* data, void* /*id*/, void *const* /*p_pixels*/) {
    struct VideoContext* c = reinterpret_cast<struct VideoContext*>(data);
    SDL_UnlockSurface(c->surface);
    SDL_UnlockMutex(c->mutex);
}

// VLC wants to display a video frame.
static void display(void* /*data*/, void* /*id*/) {
    // Data to be displayed.
}

VideoVlcComponent::VideoVlcComponent(Window* window)
        : VideoComponent(window), mMediaPlayer(nullptr), mContext({})
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

void VideoVlcComponent::setResize(float width, float height)
{
    mTargetSize = Vector2f(width, height);
    mTargetIsMax = false;
    mStaticImage.setResize(width, height);
    resize();
}

void VideoVlcComponent::setMaxSize(float width, float height)
{
    mTargetSize = Vector2f(width, height);
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
                static_cast<int>(mVideoHeight), 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
        mContext.mutex = SDL_CreateMutex();
        mContext.valid = true;
        resize();
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

    const Vector2f textureSize(static_cast<float>(mVideoWidth), static_cast<float>(mVideoHeight));

    if (textureSize == Vector2f::Zero())
        return;

    // SVG rasterization is determined by height and rasterization is done in terms of pixels.
    // If rounding is off enough in the rasterization step (for images with extreme aspect
    // ratios), it can cause cutoff when the aspect ratio breaks.
    // So we always make sure the resultant height is an integer to make sure cutoff doesn't
    // happen, and scale width from that.
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

        // For SVG rasterization, always calculate width from rounded height.
        mSize[1] = Math::round(mSize[1]);
        mSize[0] = (mSize[1] / textureSize.y()) * textureSize.x();

    }
    else {
        // If both components are set, we just stretch.
        // If no components are set, we don't resize at all.
        mSize = mTargetSize == Vector2f::Zero() ? textureSize : mTargetSize;

        // If only one component is set, we resize in a way that maintains aspect ratio.
        if (!mTargetSize.x() && mTargetSize.y()) {
            mSize[1] = Math::round(mTargetSize.y());
            mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
        }
        else if (mTargetSize.x() && !mTargetSize.y()) {
            mSize[1] = Math::round((mTargetSize.x() / textureSize.x()) * textureSize.y());
            mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
        }
    }

    mTexture->rasterizeAt(static_cast<size_t>(Math::round(mSize.x())),
            static_cast<size_t>(Math::round(mSize.y())));

    onSizeChanged();
}

void VideoVlcComponent::render(const Transform4x4f& parentTrans)
{
    VideoComponent::render(parentTrans);
    Transform4x4f trans = parentTrans * getTransform();
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
            const unsigned int fadeIn = mFadeIn * 255.0f;
            color = Renderer::convertColor((fadeIn << 24) | (fadeIn << 16) | (fadeIn << 8) | 255);
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
        for (int i = 0; i < 4; ++i)
            vertices[i].pos.round();

        // Build a texture for the video frame.
        mTexture->initFromPixels(reinterpret_cast<unsigned char*>(mContext.surface->pixels),
                mContext.surface->w, mContext.surface->h);
        mTexture->bind();

        #if defined(USE_OPENGL_21)
        // Render scanlines if this option is enabled. However, if this is the video
        // screensaver, then skip this as screensaver scanline rendering is handled in
        // SystemScreenSaver as a postprocessing step.
        if (!mScreensaverMode && Settings::getInstance()->getBool("GamelistVideoScanlines"))
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

void VideoVlcComponent::calculateBlackRectangle()
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
                // theme. But as the height in mSize is provided by libVLC in integer format
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
            mVideoRectangleCoords.push_back(Math::round(mVideoAreaPos.x() -
                    rectWidth * mOrigin.x()));
            mVideoRectangleCoords.push_back(Math::round(mVideoAreaPos.y() -
                    rectHeight * mOrigin.y()));
            mVideoRectangleCoords.push_back(Math::round(rectWidth));
            mVideoRectangleCoords.push_back(Math::round(rectHeight));
        }
        // If the option to display pillarboxes is disabled, then make the rectangle equivalent
        // to the size of the video.
        else {
            mVideoRectangleCoords.push_back(Math::round(mPosition.x() - mSize.x() * mOrigin.x()));
            mVideoRectangleCoords.push_back(Math::round(mPosition.y() - mSize.y() * mOrigin.y()));
            mVideoRectangleCoords.push_back(Math::round(mSize.x()));
            mVideoRectangleCoords.push_back(Math::round(mSize.y()));
        }
    }
}

void VideoVlcComponent::startVideo()
{
    if (!mIsPlaying) {
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
                libvlc_event_t vlcEvent;

                // Asynchronous media parsing.
                libvlc_event_attach(libvlc_media_event_manager(mMedia),
                        libvlc_MediaParsedChanged, VlcMediaParseCallback, 0);
                parseResult = libvlc_media_parse_with_options(mMedia, libvlc_media_parse_local, -1);

                if (!parseResult) {
                    // Wait for a maximum of 1 second for the media parsing.
                    for (int i = 0; i < 200; i++) {
                        if (libvlc_media_get_parsed_status(mMedia))
                            break;
                        SDL_Delay(5);
                    };
                }

                libvlc_media_track_t** tracks;
                track_count = libvlc_media_tracks_get(mMedia, &tracks);
                for (unsigned track = 0; track < track_count; ++track) {
                    if (tracks[track]->i_type == libvlc_track_video) {
                        mVideoWidth = tracks[track]->video->i_width;
                        mVideoHeight = tracks[track]->video->i_height;
                        break;
                    }
                }
                libvlc_media_tracks_release(tracks, track_count);

                // Make sure we found a valid video track.
                if ((mVideoWidth > 0) && (mVideoHeight > 0)) {
                    setupContext();

                    // Setup the media player.
                    mMediaPlayer = libvlc_media_player_new_from_media(mMedia);

                    if ((!Settings::getInstance()->getBool("GamelistVideoAudio") &&
                            !mScreensaverMode) ||
                            (!Settings::getInstance()->getBool("ScreensaverVideoAudio") &&
                            mScreensaverMode))
                        libvlc_audio_set_mute(mMediaPlayer, 1);

                    libvlc_media_player_play(mMediaPlayer);
                    libvlc_video_set_callbacks(mMediaPlayer, lock, unlock, display,
                            reinterpret_cast<void*>(&mContext));
                    libvlc_video_set_format(mMediaPlayer, "RGBA", static_cast<int>(mVideoWidth),
                            static_cast<int>(mVideoHeight), static_cast<int>(mVideoWidth * 4));

                    // Update the playing state.
                    mIsPlaying = true;
                    mFadeIn = 0.0f;
                }
                if (mIsPlaying) {
                    calculateBlackRectangle();
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

                if ((!Settings::getInstance()->getBool("GamelistVideoAudio") &&
                        !mScreensaverMode) || (!Settings::getInstance()->
                        getBool("ScreensaverVideoAudio") && mScreensaverMode))
                    libvlc_audio_set_mute(mMediaPlayer, 1);

                libvlc_media_player_play(mMediaPlayer);
            }
        }
    }
}

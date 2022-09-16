//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Screensaver.h
//
//  Screensaver, supporting the following types:
//  Dim, black, slideshow, video.
//

#ifndef ES_APP_SCREENSAVER_H
#define ES_APP_SCREENSAVER_H

#include "Window.h"
#include "resources/Font.h"

class ImageComponent;
class VideoComponent;

// Screensaver implementation.
class Screensaver : public Window::Screensaver
{
public:
    Screensaver();

    virtual bool isScreensaverActive() { return (mState != STATE_INACTIVE); }
    virtual bool isFallbackScreensaver() { return mFallbackScreensaver; }

    virtual void startScreensaver(bool generateMediaList);
    virtual void stopScreensaver();
    virtual void nextGame();
    virtual void launchGame();
    virtual void goToGame();

    virtual void renderScreensaver();
    virtual void update(int deltaTime);

    virtual FileData* getCurrentGame() { return mCurrentGame; }
    virtual void triggerNextGame() { mTriggerNextGame = true; }

private:
    void generateImageList();
    void generateVideoList();
    void generateCustomImageList();
    void pickRandomImage(std::string& path);
    void pickRandomVideo(std::string& path);
    void pickRandomCustomImage(std::string& path);
    void generateOverlayInfo();

    enum STATE {
        STATE_INACTIVE,
        STATE_FADE_OUT_WINDOW,
        STATE_FADE_IN_VIDEO,
        STATE_SCREENSAVER_ACTIVE
    };

    Renderer* mRenderer;
    Window* mWindow;
    STATE mState;

    std::vector<FileData*> mImageFiles;
    std::vector<FileData*> mVideoFiles;
    std::vector<std::string> mImageCustomFiles;
    std::unique_ptr<ImageComponent> mImageScreensaver;
    std::unique_ptr<VideoComponent> mVideoScreensaver;

    FileData* mCurrentGame;
    FileData* mPreviousGame;
    std::string mScreensaverType;
    std::string mPreviousCustomImage;
    std::string mGameName;
    std::string mSystemName;

    int mTimer;
    int mMediaSwapTime;
    bool mTriggerNextGame;
    bool mHasMediaFiles;
    bool mFallbackScreensaver;
    float mOpacity;
    float mDimValue;
    unsigned char mRectangleFadeIn;
    unsigned char mTextFadeIn;
    float mSaturationAmount;

    std::unique_ptr<TextCache> mGameOverlay;
    std::vector<std::shared_ptr<Font>> mGameOverlayFont;
    std::vector<float> mGameOverlayRectangleCoords;
};

#endif // ES_APP_SCREENSAVER_H

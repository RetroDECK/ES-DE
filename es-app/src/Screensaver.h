//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  Screensaver.h
//
//  Screensaver, supporting the following types:
//  Dim, black, slideshow, video.
//

#ifndef ES_APP_SCREENSAVER_H
#define ES_APP_SCREENSAVER_H

#include "Window.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "components/VideoComponent.h"

class Screensaver : public Window::Screensaver
{
public:
    Screensaver();

    virtual bool isScreensaverActive() { return mScreensaverActive; }
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

    Renderer* mRenderer;
    Window* mWindow;

    std::vector<FileData*> mImageFiles;
    std::vector<FileData*> mVideoFiles;
    std::vector<FileData*> mFilesInventory;
    std::vector<std::string> mImageCustomFiles;
    std::vector<std::string> mCustomFilesInventory;
    std::unique_ptr<ImageComponent> mImageScreensaver;
    std::unique_ptr<VideoComponent> mVideoScreensaver;
    std::unique_ptr<TextComponent> mGameOverlay;
    std::vector<float> mGameOverlayRectangleCoords;

    FileData* mCurrentGame;
    FileData* mPreviousGame;
    std::string mScreensaverType;
    std::string mPreviousCustomImage;
    std::string mGameName;
    std::string mSystemName;

    int mTimer;
    int mMediaSwapTime;
    bool mScreensaverActive;
    bool mTriggerNextGame;
    bool mHasMediaFiles;
    bool mFallbackScreensaver;
    float mOpacity;
    float mDimValue;
    unsigned char mRectangleFadeIn;
    unsigned char mTextFadeIn;
    float mSaturationAmount;
};

#endif // ES_APP_SCREENSAVER_H

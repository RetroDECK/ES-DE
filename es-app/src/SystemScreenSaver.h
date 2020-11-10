//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SystemScreenSaver.h
//
//  Screensaver, supporting the following types:
//  Dim, black, slideshow, video.
//

#ifndef ES_APP_SYSTEM_SCREEN_SAVER_H
#define ES_APP_SYSTEM_SCREEN_SAVER_H

#include "Window.h"

class ImageComponent;
class Sound;
class VideoComponent;

// Screensaver implementation.
class SystemScreensaver : public Window::Screensaver
{
public:
    SystemScreensaver(Window* window);
    virtual ~SystemScreensaver();

    virtual bool allowSleep();
    virtual bool isScreensaverActive();

    virtual void startScreensaver(bool generateMediaList);
    virtual void stopScreensaver();
    virtual void nextGame();
    virtual void launchGame();

    virtual void renderScreensaver();
    virtual void update(int deltaTime);

    virtual bool getHasMediaFiles() { return mHasMediaFiles; };
    virtual FileData* getCurrentGame() { return mCurrentGame; };

private:
    void generateImageList();
    void generateVideoList();
    void generateCustomImageList();
    void pickRandomImage(std::string& path);
    void pickRandomVideo(std::string& path);
    void pickRandomCustomImage(std::string& path);

    enum STATE {
        STATE_INACTIVE,
        STATE_FADE_OUT_WINDOW,
        STATE_FADE_IN_VIDEO,
        STATE_SCREENSAVER_ACTIVE
    };

    std::vector<FileData*> mImageFiles;
    std::vector<FileData*> mVideoFiles;
    std::vector<std::string> mImageCustomFiles;
    bool mHasMediaFiles;
    VideoComponent* mVideoScreensaver;
    ImageComponent* mImageScreensaver;
    Window* mWindow;
    STATE mState;
    float mOpacity;
    int mTimer;
    FileData* mCurrentGame;
    FileData* mPreviousGame;
    std::string mPreviousCustomImage;
    std::string mGameName;
    std::string mSystemName;
    int mVideoChangeTime;
};

#endif // ES_APP_SYSTEM_SCREEN_SAVER_H

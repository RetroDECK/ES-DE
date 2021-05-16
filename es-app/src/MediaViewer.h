//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MediaViewer.h
//
//  Fullscreen game media viewer.
//

#ifndef ES_APP_MEDIA_VIEWER_H
#define ES_APP_MEDIA_VIEWER_H

#include "components/ImageComponent.h"
#include "components/VideoComponent.h"
#include "FileData.h"
#include "Window.h"

class MediaViewer : public Window::MediaViewer
{
public:
    MediaViewer(Window* window);
    virtual ~MediaViewer();

    virtual bool startMediaViewer(FileData* game);
    virtual void stopMediaViewer();

    virtual void update(int deltaTime);
    virtual void render();

private:
    void initiateViewer();
    void findMedia();

    void playVideo();
    void showImage(int index);

    virtual void showNext();
    virtual void showPrevious();

    Window* mWindow;
    FileData* mGame;

    bool mHasVideo;
    bool mHasImages;
    bool mDisplayingImage;

    int mCurrentImageIndex;
    int mScreenShotIndex;

    std::string mVideoFile;
    std::vector<std::string> mImageFiles;

    VideoComponent* mVideo;
    ImageComponent* mImage;
};

#endif // ES_APP_MEDIA_VIEWER_H

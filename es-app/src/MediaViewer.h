//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MediaViewer.h
//
//  Fullscreen game media viewer.
//

#ifndef ES_APP_MEDIA_VIEWER_H
#define ES_APP_MEDIA_VIEWER_H

#include "FileData.h"
#include "Window.h"
#include "components/ImageComponent.h"
#include "components/VideoComponent.h"

class MediaViewer : public Window::MediaViewer
{
public:
    MediaViewer(Window* window);
    virtual ~MediaViewer();

    virtual bool startMediaViewer(FileData* game) override;
    virtual void stopMediaViewer() override;

    virtual void update(int deltaTime) override;
    virtual void render(const glm::mat4& parentTrans) override;

private:
    void initiateViewer();
    void findMedia();

    void playVideo();
    void showImage(int index);

    virtual void showNext() override;
    virtual void showPrevious() override;

    Window* mWindow;
    FileData* mGame;

    bool mHasVideo;
    bool mHasImages;
    bool mDisplayingImage;

    int mCurrentImageIndex;
    int mScreenshotIndex;
    int mTitleScreenIndex;

    std::string mVideoFile;
    std::vector<std::string> mImageFiles;

    VideoComponent* mVideo;
    ImageComponent* mImage;
};

#endif // ES_APP_MEDIA_VIEWER_H

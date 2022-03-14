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
    MediaViewer();
    virtual ~MediaViewer();

    bool startMediaViewer(FileData* game) override;
    void stopMediaViewer() override;

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

private:
    void initiateViewer();
    void findMedia();

    void playVideo();
    void showImage(int index);

    void showNext() override;
    void showPrevious() override;

    Renderer* mRenderer;
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

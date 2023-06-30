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
#include "components/HelpComponent.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "components/VideoComponent.h"

class MediaViewer : public Window::MediaViewer
{
public:
    MediaViewer();

    bool startMediaViewer(FileData* game) override;
    void stopMediaViewer() override;
    void launchPDFViewer() override;

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

    enum class HelpInfoPosition {
        TOP,
        BOTTOM,
        DISABLED
    };

private:
    void initiateViewer();
    void findMedia();
    void loadImages();

    void playVideo();

    void showNext() override;
    void showPrevious() override;
    void showFirst() override;
    void showLast() override;

    Renderer* mRenderer;
    FileData* mGame;

    bool mHasVideo;
    bool mHasImages;
    bool mDisplayingImage;
    bool mHasManual;

    float mFrameHeight;
    int mCurrentImageIndex;
    int mScreenshotIndex;
    int mTitleScreenIndex;

    std::string mVideoFile;
    std::unique_ptr<VideoComponent> mVideo;
    std::vector<std::pair<std::string, std::string>> mImageFiles;
    std::vector<std::unique_ptr<ImageComponent>> mImages;

    std::unique_ptr<HelpComponent> mHelp;
    std::unique_ptr<TextComponent> mEntryNumText;
    std::string mEntryCount;
    HelpInfoPosition mHelpInfoPosition;
};

#endif // ES_APP_MEDIA_VIEWER_H

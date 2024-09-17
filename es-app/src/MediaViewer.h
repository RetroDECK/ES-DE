//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
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

    void input(InputConfig* config, Input input) override;
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

    void showNext();
    void showPrevious();
    void showFirst();
    void showLast();

    Renderer* mRenderer;
    FileData* mGame;

    bool mHasVideo;
    bool mHasImages;
    bool mDisplayingImage;
    bool mHasManual;
    bool mShowMediaTypes;

    float mFrameHeight;
    int mCurrentImageIndex;
    int mScreenshotIndex;
    int mTitleScreenIndex;

    int mKeyRepeatDir;
    int mKeyRepeatTimer;

    struct ImageInfo {
        std::string mediaType;
        bool linearInterpolation;

        ImageInfo(const std::string& mediaTypeArg, const bool interpolationArg)
            : mediaType {mediaTypeArg}
            , linearInterpolation {interpolationArg} {};
    };

    std::string mVideoFile;
    std::unique_ptr<VideoComponent> mVideo;
    std::vector<std::pair<std::string, ImageInfo>> mImageFiles;
    std::vector<std::unique_ptr<ImageComponent>> mImages;

    std::unique_ptr<HelpComponent> mHelp;
    std::unique_ptr<TextComponent> mMediaType;
    std::string mEntryCount;
    HelpInfoPosition mHelpInfoPosition;
};

#endif // ES_APP_MEDIA_VIEWER_H

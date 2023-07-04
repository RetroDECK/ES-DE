//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  PDFViewer.h
//
//  Parses and renders pages using the Poppler library via the external es-pdf-convert binary.
//

#ifndef ES_APP_PDF_VIEWER_H
#define ES_APP_PDF_VIEWER_H

#include "FileData.h"
#include "Window.h"
#include "components/HelpComponent.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"

class PDFViewer : public Window::PDFViewer
{
public:
    PDFViewer();

    bool startPDFViewer(FileData* game) override;
    void stopPDFViewer() override;
    void launchMediaViewer() override;

    bool getDocumentInfo();
    void convertPage(int pageNum);

    void render(const glm::mat4& parentTrans) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

    enum class HelpInfoPosition {
        TOP,
        BOTTOM,
        DISABLED
    };

private:
    void showNextPage();
    void showPreviousPage();

    void navigateUp() override;
    void navigateDown() override;
    void navigateLeft() override;
    void navigateRight() override;

    void navigateLeftShoulder() override;
    void navigateRightShoulder() override;

    void navigateLeftTrigger() override;
    void navigateRightTrigger() override;

    struct PageEntry {
        float width;
        float height;
        std::string orientation;
        std::vector<char> imageData;
    };

    Renderer* mRenderer;
    FileData* mGame;

    float mFrameHeight;
    float mScaleFactor;
    int mCurrentPage;
    int mPageCount;
    float mZoom;
    float mPanAmount;
    glm::vec3 mPanOffset;

    std::string mESConvertPath;
    std::string mManualPath;

    std::unique_ptr<ImageComponent> mPageImage;
    std::map<int, PageEntry> mPages;

    std::unique_ptr<HelpComponent> mHelp;
    std::unique_ptr<TextComponent> mEntryNumText;
    std::string mEntryCount;
    HelpInfoPosition mHelpInfoPosition;
};

#endif // ES_APP_PDF_VIEWER_H

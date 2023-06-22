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
#include "components/ImageComponent.h"

class PDFViewer : public Window::PDFViewer
{
public:
    PDFViewer();

    bool startPDFViewer(FileData* game) override;
    void stopPDFViewer() override;

    bool getDocumentInfo();
    void convertPage(int pageNum);

    void render(const glm::mat4& parentTrans) override;

private:
    void showNextPage() override;
    void showPreviousPage() override;
    void showFirstPage() override;
    void showLastPage() override;

    struct PageEntry {
        float width;
        float height;
        bool portraitOrientation;
        std::vector<char> imageData;
    };

    Renderer* mRenderer;
    std::shared_ptr<TextureResource> mTexture;
    std::unique_ptr<ImageComponent> mPageImage;
    std::map<int, PageEntry> mPages;

    std::string mESConvertPath;
    std::string mManualPath;

    float mScaleFactor;
    int mCurrentPage;
    int mPageCount;
};

#endif // ES_APP_PDF_VIEWER_H

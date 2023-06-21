//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  PDFViewer.h
//
//  Parses PDF documents using the PoDoFo library and renders pages using the Poppler
//  library via the external es-pdf-convert binary.
//

#ifndef ES_APP_PDF_VIEWER_H
#define ES_APP_PDF_VIEWER_H

#include "FileData.h"
#include "Window.h"
#include "components/ImageComponent.h"

#include <podofo/podofo.h>

class PDFViewer : public Window::PDFViewer
{
public:
    PDFViewer();
    ~PDFViewer() { stopPDFViewer(); }

    bool startPDFViewer(FileData* game) override;
    void stopPDFViewer() override;

    void convertPage(int pageNum);

    void render(const glm::mat4& parentTrans) override;

private:
    void showNextPage() override;
    void showPreviousPage() override;
    void showFirstPage() override;
    void showLastPage() override;

    struct PageEntry {
        int width;
        int height;
        std::vector<char> imageData;
    };

    Renderer* mRenderer;
    std::shared_ptr<TextureResource> mTexture;
    std::unique_ptr<ImageComponent> mPageImage;
    std::map<int, PageEntry> mPages;

    float mScaleFactor;
    int mCurrentPage;
    int mPageCount;

    std::string mManualPath;
};

#endif // ES_APP_PDF_VIEWER_H

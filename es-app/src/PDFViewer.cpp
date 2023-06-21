//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  PDFViewer.cpp
//
//  Parses PDF documents using the PoDoFo library and renders pages using the Poppler
//  library via the external es-pdf-convert binary.
//

#include "PDFViewer.h"

#include "Log.h"
#include "Sound.h"
#include "utils/FileSystemUtil.h"

#define DEBUG_PDF_CONVERSION false

PDFViewer::PDFViewer()
    : mRenderer {Renderer::getInstance()}
{
    Window::getInstance()->setPDFViewer(this);
    mTexture = TextureResource::get("");
    mPages.clear();
    mPageImage.reset();
}

bool PDFViewer::startPDFViewer(FileData* game)
{
    mManualPath = game->getManualPath();

    if (!Utils::FileSystem::exists(mManualPath)) {
        LOG(LogError) << "No PDF manual found for game \"" << game->getName() << "\"";
        return false;
    }

    LOG(LogDebug) << "PDFViewer::startPDFViewer(): Opening document \"" << mManualPath << "\"";

    PoDoFo::PdfMemDocument pdf;
    mPages.clear();
    mPageCount = 0;
    mCurrentPage = 0;
    mScaleFactor = 1.0f;

    try {
        pdf.Load(mManualPath.c_str());
    }
    catch (PoDoFo::PdfError& e) {
        LOG(LogError) << "PDFViewer: Couldn't load file \"" << mManualPath << "\", PoDoFo error \""
                      << e.what() << ": " << e.ErrorMessage(e.GetError()) << "\"";
        return false;
    }

#if (DEBUG_PDF_CONVERSION)
    PoDoFo::EPdfVersion versionEPdf {pdf.GetPdfVersion()};
    std::string version {"unknown"};

    switch (versionEPdf) {
        case 0:
            version = "1.0";
            break;
        case 1:
            version = "1.1";
            break;
        case 2:
            version = "1.2";
            break;
        case 3:
            version = "1.3";
            break;
        case 4:
            version = "1.4";
            break;
        case 5:
            version = "1.5";
            break;
        case 6:
            version = "1.6";
            break;
        case 7:
            version = "1.7";
            break;
        default:
            version = "unknown";
    };

    LOG(LogDebug) << "PDF version: " << version;
    LOG(LogDebug) << "Page count: " << pdf.GetPageCount();
#endif

    mPageCount = static_cast<int>(pdf.GetPageCount());

    for (int i {0}; i < mPageCount; ++i) {
        const int rotation {pdf.GetPage(i)->GetRotation()};
        const PoDoFo::PdfRect cropBox {pdf.GetPage(i)->GetCropBox()};
        float width {static_cast<float>(cropBox.GetWidth())};
        float height {static_cast<float>(cropBox.GetHeight())};

        if (rotation != 0 && rotation != 180)
            std::swap(width, height);

        // Maintain page aspect ratio.
        glm::vec2 textureSize {glm::vec2 {width, height}};
        const glm::vec2 targetSize {glm::vec2 {mRenderer->getScreenWidth() * mScaleFactor,
                                               mRenderer->getScreenHeight() * mScaleFactor}};
        glm::vec2 resizeScale {targetSize.x / textureSize.x, targetSize.y / textureSize.y};

        if (resizeScale.x < resizeScale.y) {
            textureSize.x *= resizeScale.x;
            textureSize.y = std::min(textureSize.y * resizeScale.x, targetSize.y);
        }
        else {
            textureSize.y *= resizeScale.y;
            textureSize.x = std::min((textureSize.y / height) * width, targetSize.x);
        }

        const int textureWidth {static_cast<int>(std::round(textureSize.x))};
        const int textureHeight {static_cast<int>(std::round(textureSize.y))};

#if (DEBUG_PDF_CONVERSION)
        LOG(LogDebug) << "Page " << i + 1 << ": Rotation: " << rotation << " degrees / "
                      << "Crop box width: " << width << " / "
                      << "Crop box height: " << height << " / "
                      << "Size ratio: " << width / height << " / "
                      << "Texture size: " << textureWidth << "x" << textureHeight;
#endif

        mPages[i + 1] = PageEntry {textureWidth, textureHeight, {}};
    }

    mCurrentPage = 1;
    convertPage(mCurrentPage);
    return true;
}

void PDFViewer::stopPDFViewer()
{
    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    mPages.clear();
    mPageImage.reset();
}

void PDFViewer::convertPage(int pageNum)
{
    assert(pageNum <= static_cast<int>(mPages.size()));

    const std::string esConvertPath {Utils::FileSystem::getExePath() + "/es-pdf-convert"};
    if (!Utils::FileSystem::exists(esConvertPath)) {
        LOG(LogError) << "Couldn't find PDF conversion binary es-pdf-convert";
        return;
    }

    std::string command {Utils::FileSystem::getEscapedPath(esConvertPath)};
    command.append(" ")
        .append(Utils::FileSystem::getEscapedPath(mManualPath))
        .append(" ")
        .append(std::to_string(pageNum))
        .append(" ")
        .append(std::to_string(mPages[pageNum].width))
        .append(" ")
        .append(std::to_string(mPages[pageNum].height));

    if (mPages[pageNum].imageData.empty()) {
#if (DEBUG_PDF_CONVERSION)
        LOG(LogDebug) << "Converting page: " << mCurrentPage;
        LOG(LogDebug) << command;
#endif
        FILE* commandPipe;
        std::array<char, 512> buffer {};
        std::string imageData;
        int returnValue;

        if (!(commandPipe = reinterpret_cast<FILE*>(popen(command.c_str(), "r")))) {
            LOG(LogError) << "Couldn't open pipe to es-pdf-convert";
            return;
        }

        while (fread(buffer.data(), 1, 512, commandPipe)) {
            mPages[pageNum].imageData.insert(mPages[pageNum].imageData.end(),
                                             std::make_move_iterator(buffer.begin()),
                                             std::make_move_iterator(buffer.end()));
        }

        returnValue = pclose(commandPipe);
        size_t imageDataSize {mPages[pageNum].imageData.size()};

        if (returnValue != 0 || (static_cast<int>(imageDataSize) <
                                 mPages[pageNum].width * mPages[pageNum].height * 4)) {
            LOG(LogError) << "Error reading PDF file";
            mPages[pageNum].imageData.clear();
            return;
        }
    }
    else {
#if (DEBUG_PDF_CONVERSION)
        LOG(LogDebug) << "Using cached texture for page: " << mCurrentPage;
#endif
    }

    mPageImage.reset();
    mPageImage = std::make_unique<ImageComponent>(false, false);
    mPageImage->setOrigin(0.5f, 0.5f);
    mPageImage->setPosition(mRenderer->getScreenWidth() / 2.0f,
                            mRenderer->getScreenHeight() / 2.0f);

    mPageImage->setFlipY(true);
    mPageImage->setMaxSize(
        glm::vec2 {mPages[pageNum].width / mScaleFactor, mPages[pageNum].height / mScaleFactor});
    mPageImage->setRawImage(reinterpret_cast<const unsigned char*>(&mPages[pageNum].imageData[0]),
                            mPages[pageNum].width, mPages[pageNum].height);

#if (DEBUG_PDF_CONVERSION)
    LOG(LogDebug) << "ABGR32 data stream size: " << mPages[pageNum].imageData.size();
#endif
}

void PDFViewer::render(const glm::mat4& /*parentTrans*/)
{
    glm::mat4 trans {Renderer::getIdentity()};
    mRenderer->setMatrix(trans);

    // Render a black background below the document.
    mRenderer->drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(),
                        0x000000FF, 0x000000FF);

    if (mPageImage != nullptr) {
        mPageImage->render(trans);
    }
}

void PDFViewer::showNextPage()
{
    if (mCurrentPage == mPageCount)
        return;

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    ++mCurrentPage;
    convertPage(mCurrentPage);
}

void PDFViewer::showPreviousPage()
{
    if (mCurrentPage == 1)
        return;

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    --mCurrentPage;
    convertPage(mCurrentPage);
}

void PDFViewer::showFirstPage()
{
    if (mCurrentPage == 1)
        return;

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    mCurrentPage = 1;
    convertPage(mCurrentPage);
}

void PDFViewer::showLastPage()
{
    if (mCurrentPage == mPageCount)
        return;

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    mCurrentPage = mPageCount;
    convertPage(mCurrentPage);
}

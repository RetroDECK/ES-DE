//  SPDX-License-Identifier: GPL-2.0-only
//
//  ES-DE
//  ConvertPDF.cpp
//
//  Converts PDF document pages to raw ARGB32 pixel data for maximum performance.
//  This needs to be separated into its own binary to get around the restrictive GPL
//  license used by the Poppler PDF rendering library.
//

#include "ConvertPDF.h"

#include "poppler-document.h"
#include "poppler-image.h"
#include "poppler-page-renderer.h"
#include "poppler-page.h"

#include <cmath>
#include <fstream>
#include <iostream>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#if defined(_WIN64)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

#if defined(_WIN64)
int ConvertPDF::processFile(
    const std::wstring path, const std::wstring mode, int pageNum, int width, int height)
#elif defined(__ANDROID__)
int ConvertPDF::processFile(const std::string path,
                            const std::string mode,
                            int pageNum,
                            int width,
                            int height,
                            std::string& result)
#else
int ConvertPDF::processFile(
    const std::string path, const std::string mode, int pageNum, int width, int height)
#endif
{
    std::ifstream file;

    file.open(path.c_str(), std::ifstream::binary);
    if (file.fail()) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_ERROR, ANDROID_APPLICATION_ID,
                            "Error: Couldn't open PDF file, permission problems?");
#else
        std::cerr << "Error: Couldn't open PDF file, permission problems?" << std::endl;
#endif
        return (-1);
    }

    file.seekg(0, std::ios::end);
    const long fileLength {static_cast<long>(file.tellg())};
    file.seekg(0, std::ios::beg);
    std::vector<char> fileData(fileLength);
    file.read(&fileData[0], fileLength);
    file.close();

    const poppler::document* document {
        poppler::document::load_from_raw_data(&fileData[0], fileLength)};

    if (document == nullptr) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_ERROR, ANDROID_APPLICATION_ID,
                            "Error: Couldn't open document, invalid PDF file?");
#else
        std::cerr << "Error: Couldn't open document, invalid PDF file?" << std::endl;
#endif
        return (-1);
    }

    const int pageCount {document->pages()};
#if defined(_WIN64)
    if (mode == L"-fileinfo") {
#else
    if (mode == "-fileinfo") {
#endif
        std::vector<std::string> pageInfo;
        for (int i {0}; i < pageCount; ++i) {
            std::string pageRow;
            const poppler::page* page {document->create_page(i)};
            if (page == nullptr) {
                if (page == nullptr) {
#if defined(__ANDROID__)
                    __android_log_print(ANDROID_LOG_ERROR, ANDROID_APPLICATION_ID,
                                        "Error: Couldn't read page %i", i + 1);
#else
                    std::cerr << "Error: Couldn't read page " << i + 1 << std::endl;
#endif
                    return (-1);
                }
            }

            std::string orientation;
            if (page->orientation() == poppler::page::portrait)
                orientation = "portrait";
            else if (page->orientation() == poppler::page::upside_down)
                orientation = "upside_down";
            else if (page->orientation() == poppler::page::seascape)
                orientation = "seascape";
            else
                orientation = "landscape";

            const poppler::rectf pageRect {page->page_rect()};
            pageRow.append(std::to_string(i + 1))
                .append(";")
                .append(orientation)
                .append(";")
                .append(std::to_string(pageRect.width()))
                .append(";")
                .append(std::to_string(pageRect.height()));
            pageInfo.emplace_back(pageRow);
        }
        for (auto& row : pageInfo) {
#if defined(__ANDROID__)
            result.append(row).append("\n");
#else
            std::cout << row << std::endl;
#endif
        }
        return (0);
    }

    if (pageNum < 1 || pageNum > pageCount) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_ERROR, ANDROID_APPLICATION_ID,
                            "Error: Requested page %i does not exist in document", pageNum);
#else
        std::cerr << "Error: Requested page " << pageNum << " does not exist in document"
                  << std::endl;
#endif
        return (-1);
    }

    const poppler::page* page {document->create_page(pageNum - 1)};

    if (page == nullptr) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_ERROR, ANDROID_APPLICATION_ID,
                            "Error: Couldn't read page %i", pageNum);
#else
        std::cerr << "Error: Couldn't read page " << pageNum << std::endl;
#endif
        return (-1);
    }

    poppler::page_renderer pageRenderer;

    pageRenderer.set_render_hint(poppler::page_renderer::text_antialiasing);
    pageRenderer.set_render_hint(poppler::page_renderer::antialiasing);
    //  pageRenderer.set_render_hint(poppler::page_renderer::text_hinting);

    const poppler::rectf pageRect {page->page_rect()};
    const bool rotate {page->orientation() == poppler::page::portrait ||
                       page->orientation() == poppler::page::upside_down};
    const double pageHeight {pageRect.height()};
    const double sizeFactor {static_cast<double>(rotate ? height : width) / pageHeight};

    poppler::image image {
        pageRenderer.render_page(page, 72.0 * sizeFactor, 72.0 * sizeFactor, 0, 0, width, height)};

    if (!image.is_valid()) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_ERROR, ANDROID_APPLICATION_ID, "Rendered image is invalid");
#else
        std::cerr << "Rendered image is invalid" << std::endl;
#endif
        return (-1);
    }

#if defined(__ANDROID__)
    result.insert(0, std::move(image.data()), width * height * 4);
#else
    // Necessary as the image data stream may contain null characters.
    std::string imageARGB32;
    imageARGB32.insert(0, std::move(image.data()), width * height * 4);

    std::cout << imageARGB32;
#endif

    return 0;
}
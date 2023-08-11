//  SPDX-License-Identifier: GPL-2.0-only
//
//  EmulationStation Desktop Edition (ES-DE) PDF converter
//  main.cpp
//
//  Converts PDF document pages to raw ARGB32 pixel data for maximum performance.
//  This needs to be separated into its own binary to get around the restrictive GPL
//  license used by the Poppler PDF rendering library.
//
//  The column limit is 100 characters.
//  All ES-DE C++ source code is formatted using clang-format.
//

#include "poppler-document.h"
#include "poppler-image.h"
#include "poppler-page-renderer.h"
#include "poppler-page.h"

#include <cmath>
#include <fstream>
#include <iostream>

#if defined(_WIN64)
#include <fcntl.h>
#include <io.h>
#include <windows.h>

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    wchar_t** argv {__wargv};
    int argc {__argc};

    HANDLE stdoutHandle {GetStdHandle(STD_OUTPUT_HANDLE)};

    if (stdoutHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Invalid stdout handle" << std::endl;
        exit(-1);
    }

    // This is required as Windows is braindead and will otherwise add carriage return characters
    // to the stream when it encounters newline characters, which breaks binary output.
    _setmode(_fileno(stdout), O_BINARY);

    bool validArguments {true};
    std::wstring mode;

    if (argc < 3)
        validArguments = false;
    else
        mode = argv[1];

    if ((mode == L"-fileinfo" && argc != 3) || (mode == L"-convert" && argc != 6))
        validArguments = false;

    if (!validArguments) {
        std::cout << "This binary is only intended to be executed by EmulationStation.exe (ES-DE)"
                  << std::endl;
        exit(-1);
    }

    const std::wstring path {argv[2]};

    int pageNum {0};
    int width {0};
    int height {0};

    if (mode == L"-convert") {
        pageNum = _wtoi(argv[3]);
        width = _wtoi(argv[4]);
        height = _wtoi(argv[5]);
#else
int main(int argc, char* argv[])
{
    bool validArguments {true};
    std::string mode;

    if (argc < 3)
        validArguments = false;
    else
        mode = argv[1];

    if ((mode == "-fileinfo" && argc != 3) || (mode == "-convert" && argc != 6))
        validArguments = false;

    if (!validArguments) {
#if defined(__APPLE__)
        std::cout << "This binary is only intended to be executed by EmulationStation (ES-DE)"
#else
        std::cout << "This binary is only intended to be executed by emulationstation (ES-DE)"
#endif
                  << std::endl;
        exit(-1);
    }

    const std::string path {argv[2]};
    int pageNum {0};
    int width {0};
    int height {0};

    if (mode == "-convert") {
        pageNum = atoi(argv[3]);
        width = atoi(argv[4]);
        height = atoi(argv[5]);
#endif
        if (width < 1 || width > 7680) {
            std::cerr << "Invalid horizontal resolution defined: " << argv[3] << std::endl;
            exit(-1);
        }

        if (height < 1 || height > 7680) {
            std::cerr << "Invalid vertical resolution defined: " << argv[4] << std::endl;
            exit(-1);
        }

        // std::cerr << "Converting file \"" << path << "\", page " << pageNum << " to resolution "
        //          << width << "x" << height << " pixels" << std::endl;
    }

    std::ifstream file;

    file.open(path.c_str(), std::ifstream::binary);
    if (file.fail()) {
        std::cerr << "Error: Couldn't open PDF file, permission problems?" << std::endl;
        exit(-1);
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
        std::cerr << "Error: Couldn't open document, invalid PDF file?" << std::endl;
        exit(-1);
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
                    std::cerr << "Error: Couldn't read page " << i + 1 << std::endl;
                    exit(-1);
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
        for (auto& row : pageInfo)
            std::cout << row << std::endl;
        exit(0);
    }

    if (pageNum < 1 || pageNum > pageCount) {
        std::cerr << "Error: Requested page " << pageNum << " does not exist in document"
                  << std::endl;
        exit(-1);
    }

    const poppler::page* page {document->create_page(pageNum - 1)};

    if (page == nullptr) {
        std::cerr << "Error: Couldn't read page " << pageNum << std::endl;
        exit(-1);
    }

    poppler::page_renderer pageRenderer;

    pageRenderer.set_render_hint(poppler::page_renderer::text_antialiasing);
    pageRenderer.set_render_hint(poppler::page_renderer::antialiasing);
    // pageRenderer.set_render_hint(poppler::page_renderer::text_hinting);

    const poppler::rectf pageRect {page->page_rect()};
    const bool rotate {page->orientation() == poppler::page::portrait ||
                       page->orientation() == poppler::page::upside_down};
    const double pageHeight {pageRect.height()};
    const double sizeFactor {static_cast<double>(rotate ? height : width) / pageHeight};

    poppler::image image {
        pageRenderer.render_page(page, 72.0 * sizeFactor, 72.0 * sizeFactor, 0, 0, width, height)};

    if (!image.is_valid()) {
        std::cerr << "Rendered image is invalid" << std::endl;
        exit(-1);
    }

    // Necessary as the image data stream may contain null characters.
    std::string imageARGB32;
    imageARGB32.insert(0, std::move(image.data()), width * height * 4);

    std::cout << imageARGB32;
    return 0;
}

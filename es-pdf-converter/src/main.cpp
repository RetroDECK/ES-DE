//  SPDX-License-Identifier: GPL-2.0-only
//
//  EmulationStation Desktop Edition (ES-DE) PDF converter
//  main.cpp
//
//  Converts PDF document pages to raw ARGB32 image data for maximum performance.
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
#include <iostream>

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
        std::cout << "This binary is only intended to be executed by emulationstation (ES-DE)"
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

    const poppler::document* document {poppler::document::load_from_file(path)};

    if (document == nullptr) {
        std::cerr << "Error: Couldn't open document, invalid PDF file?" << std::endl;
        exit(-1);
    }

    const int pageCount {document->pages()};

    if (mode == "-fileinfo") {
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
            const poppler::rectf pageRect {page->page_rect()};
            pageRow.append(std::to_string(i + 1))
                .append(";")
                .append(page->orientation() == poppler::page::portrait ? "portrait" : "landscape")
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
    const bool portraitOrientation {page->orientation() == poppler::page::portrait};
    const double pageHeight {pageRect.height()};
    const double sizeFactor {static_cast<double>(portraitOrientation ? height : width) /
                             pageHeight};

    poppler::image image {pageRenderer.render_page(
        page, static_cast<int>(std::round(72.0 * sizeFactor)),
        static_cast<int>(std::round(72.0 * sizeFactor)), 0, 0, width, height)};

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

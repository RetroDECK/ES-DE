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
    if (argc != 5) {
        std::cout << "Usage: es-pdf-convert <filename> <page number> <horizontal resolution> "
                     "<vertical resolution>"
                  << std::endl;
        exit(-1);
    }

    const std::string path {argv[1]};
    const int pageNum {atoi(argv[2])};
    const int width {atoi(argv[3])};
    const int height {atoi(argv[4])};

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

    const poppler::document* document {poppler::document::load_from_file(path)};

    if (document == nullptr)
        exit(-1);

    if (pageNum < 1 || pageNum > document->pages()) {
        std::cerr << "Error: Requested page " << pageNum << " does not exist in document"
                  << std::endl;
        exit(-1);
    }

    const poppler::page* page {document->create_page(pageNum - 1)};
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

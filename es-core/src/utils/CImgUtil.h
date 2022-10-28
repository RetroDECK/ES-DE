//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CImgUtil.h
//
//  Utility functions using the CImg image processing library.
//

#ifndef ES_CORE_UTILS_CIMG_UTIL_H
#define ES_CORE_UTILS_CIMG_UTIL_H

// Disable the CImg display capabilities.
#define cimg_display 0

#include "CImg.h"

#include <vector>

namespace Utils
{
    namespace CImg
    {
        void convertBGRAToCImg(const std::vector<unsigned char>& imageBGRA,
                               cimg_library::CImg<unsigned char>& image);
        void convertCImgToBGRA(const cimg_library::CImg<unsigned char>& image,
                               std::vector<unsigned char>& imageBGRA);
        void convertRGBAToCImg(const std::vector<unsigned char>& imageRGBA,
                               cimg_library::CImg<unsigned char>& image);
        void convertCImgToRGBA(const cimg_library::CImg<unsigned char>& image,
                               std::vector<unsigned char>& imageRGBA);
        void getTransparentPaddingCoords(cimg_library::CImg<unsigned char>& image,
                                         int (&imageCoords)[4]);
        void removeTransparentPadding(cimg_library::CImg<unsigned char>& image);
        void cropLetterboxes(cimg_library::CImg<unsigned char>& image);
        void cropPillarboxes(cimg_library::CImg<unsigned char>& image);
        void addDropShadow(cimg_library::CImg<unsigned char>& image,
                           unsigned int shadowDistance,
                           float transparency,
                           unsigned int iterations);

    } // namespace CImg

} // namespace Utils

#endif // ES_CORE_UTILS_CIMG_UTIL_H

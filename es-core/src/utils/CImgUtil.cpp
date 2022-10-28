//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CImgUtil.cpp
//
//  Utility functions using the CImg image processing library.
//

#include "utils/CImgUtil.h"

namespace Utils
{
    namespace CImg
    {
        void convertBGRAToCImg(const std::vector<unsigned char>& imageBGRA,
                               cimg_library::CImg<unsigned char>& image)
        {
            // CImg does not interleave pixels as in BGRABGRABGRA so a conversion is required.
            int counter = 0;
            for (int r = 0; r < image.height(); ++r) {
                for (int c = 0; c < image.width(); ++c) {
                    image(c, r, 0, 0) = imageBGRA[counter + 0];
                    image(c, r, 0, 1) = imageBGRA[counter + 1];
                    image(c, r, 0, 2) = imageBGRA[counter + 2];
                    image(c, r, 0, 3) = imageBGRA[counter + 3];
                    counter += 4;
                }
            }
        }

        void convertCImgToBGRA(const cimg_library::CImg<unsigned char>& image,
                               std::vector<unsigned char>& imageBGRA)
        {
            for (int r = image.height() - 1; r >= 0; --r) {
                for (int c = 0; c < image.width(); ++c) {
                    imageBGRA.emplace_back((unsigned char)image(c, r, 0, 0));
                    imageBGRA.emplace_back((unsigned char)image(c, r, 0, 1));
                    imageBGRA.emplace_back((unsigned char)image(c, r, 0, 2));
                    imageBGRA.emplace_back((unsigned char)image(c, r, 0, 3));
                }
            }
        }

        void convertRGBAToCImg(const std::vector<unsigned char>& imageRGBA,
                               cimg_library::CImg<unsigned char>& image)
        {
            // CImg does not interleave pixels as in RGBARGBARGBA so a conversion is required.
            int counter = 0;
            for (int r = 0; r < image.height(); ++r) {
                for (int c = 0; c < image.width(); ++c) {
                    image(c, r, 0, 0) = imageRGBA[counter + 2];
                    image(c, r, 0, 1) = imageRGBA[counter + 1];
                    image(c, r, 0, 2) = imageRGBA[counter + 0];
                    image(c, r, 0, 3) = imageRGBA[counter + 3];
                    counter += 4;
                }
            }
        }

        void convertCImgToRGBA(const cimg_library::CImg<unsigned char>& image,
                               std::vector<unsigned char>& imageRGBA)
        {
            for (int r = image.height() - 1; r >= 0; --r) {
                for (int c = 0; c < image.width(); ++c) {
                    imageRGBA.emplace_back((unsigned char)image(c, r, 0, 2));
                    imageRGBA.emplace_back((unsigned char)image(c, r, 0, 1));
                    imageRGBA.emplace_back((unsigned char)image(c, r, 0, 0));
                    imageRGBA.emplace_back((unsigned char)image(c, r, 0, 3));
                }
            }
        }

        void getTransparentPaddingCoords(cimg_library::CImg<unsigned char>& image,
                                         int (&imageCoords)[4])
        {
            // Check that the image actually has an alpha channel.
            if (image.spectrum() != 4)
                return;

            double pixelValueSum = 0.0l;
            int rowCounterTop = 0;
            int rowCounterBottom = 0;
            unsigned int columnCounterLeft = 0;
            unsigned int columnCounterRight = 0;

            // Count the number of rows and columns that are completely transparent.
            for (int i = image.height() - 1; i > 0; --i) {
                cimg_library::CImg<unsigned char> imageRow = image.get_rows(i, i);
                pixelValueSum = imageRow.get_shared_channel(3).sum();
                if (pixelValueSum == 0.0l)
                    ++rowCounterTop;
                else
                    break;
            }

            for (int i = 0; i < image.height(); ++i) {
                cimg_library::CImg<unsigned char> imageRow = image.get_rows(i, i);
                pixelValueSum = imageRow.get_shared_channel(3).sum();
                if (pixelValueSum == 0.0l)
                    ++rowCounterBottom;
                else
                    break;
            }

            for (int i = 0; i < image.width(); ++i) {
                cimg_library::CImg<unsigned char> imageColumn = image.get_columns(i, i);
                pixelValueSum = imageColumn.get_shared_channel(3).sum();
                if (pixelValueSum == 0.0l)
                    ++columnCounterLeft;
                else
                    break;
            }

            for (int i = image.width() - 1; i > 0; --i) {
                cimg_library::CImg<unsigned char> imageColumn = image.get_columns(i, i);
                pixelValueSum = imageColumn.get_shared_channel(3).sum();
                if (pixelValueSum == 0.0l)
                    ++columnCounterRight;
                else
                    break;
            }

            imageCoords[0] = columnCounterLeft;
            imageCoords[1] = rowCounterTop;
            imageCoords[2] = columnCounterRight;
            imageCoords[3] = rowCounterBottom;
        }

        void removeTransparentPadding(cimg_library::CImg<unsigned char>& image)
        {
            // Check that the image actually has an alpha channel.
            if (image.spectrum() != 4)
                return;

            double pixelValueSum = 0.0l;
            int rowCounterTop = 0;
            int rowCounterBottom = 0;
            unsigned int columnCounterLeft = 0;
            unsigned int columnCounterRight = 0;

            // Count the number of rows and columns that are completely transparent.
            for (int i = image.height() - 1; i > 0; --i) {
                cimg_library::CImg<unsigned char> imageRow = image.get_rows(i, i);
                pixelValueSum = imageRow.get_shared_channel(3).sum();
                if (pixelValueSum == 0.0l)
                    ++rowCounterTop;
                else
                    break;
            }

            for (int i = 0; i < image.height(); ++i) {
                cimg_library::CImg<unsigned char> imageRow = image.get_rows(i, i);
                pixelValueSum = imageRow.get_shared_channel(3).sum();
                if (pixelValueSum == 0.0l)
                    ++rowCounterBottom;
                else
                    break;
            }

            for (int i = 0; i < image.width(); ++i) {
                cimg_library::CImg<unsigned char> imageColumn = image.get_columns(i, i);
                pixelValueSum = imageColumn.get_shared_channel(3).sum();
                if (pixelValueSum == 0.0l)
                    ++columnCounterLeft;
                else
                    break;
            }

            for (int i = image.width() - 1; i > 0; --i) {
                cimg_library::CImg<unsigned char> imageColumn = image.get_columns(i, i);
                pixelValueSum = imageColumn.get_shared_channel(3).sum();
                if (pixelValueSum == 0.0l)
                    ++columnCounterRight;
                else
                    break;
            }

            if (rowCounterTop > 0)
                image.crop(0, 0, 0, 3, image.width() - 1, image.height() - 1 - rowCounterTop, 0, 0);

            if (rowCounterBottom > 0)
                image.crop(0, rowCounterBottom, 0, 3, image.width() - 1, image.height() - 1, 0, 0);

            if (columnCounterLeft > 0)
                image.crop(columnCounterLeft, 0, 0, 3, image.width() - 1, image.height() - 1, 0, 0);

            if (columnCounterRight > 0)
                image.crop(0, 0, 0, 3, image.width() - columnCounterRight - 1, image.height() - 1,
                           0, 0);
        }

        void cropLetterboxes(cimg_library::CImg<unsigned char>& image)
        {
            double pixelValueSum = 0.0l;
            int rowCounterUpper = 0;
            int rowCounterLower = 0;

            // Count the number of rows that are pure black.
            for (int i = image.height() - 1; i > 0; --i) {
                cimg_library::CImg<unsigned char> imageRow = image.get_rows(i, i);
                // Ignore the alpha channel.
                imageRow.channels(0, 2);
                pixelValueSum = imageRow.sum();
                if (pixelValueSum == 0.0l)
                    ++rowCounterUpper;
                else
                    break;
            }

            for (int i = 0; i < image.height(); ++i) {
                cimg_library::CImg<unsigned char> imageRow = image.get_rows(i, i);
                imageRow.channels(0, 2);
                pixelValueSum = imageRow.sum();
                if (pixelValueSum == 0.0l)
                    ++rowCounterLower;
                else
                    break;
            }

            if (rowCounterUpper > 0)
                image.crop(0, 0, 0, 3, image.width() - 1, image.height() - 1 - rowCounterUpper, 0,
                           0);

            if (rowCounterLower > 0)
                image.crop(0, rowCounterLower, 0, 3, image.width() - 1, image.height() - 1, 0, 0);
        }

        void cropPillarboxes(cimg_library::CImg<unsigned char>& image)
        {
            double pixelValueSum = 0.0l;
            unsigned int columnCounterLeft = 0;
            unsigned int columnCounterRight = 0;

            // Count the number of columns that are pure black.
            for (int i = 0; i < image.width(); ++i) {
                cimg_library::CImg<unsigned char> imageColumn = image.get_columns(i, i);
                // Ignore the alpha channel.
                imageColumn.channels(0, 2);
                pixelValueSum = imageColumn.sum();
                if (pixelValueSum == 0.0l)
                    ++columnCounterLeft;
                else
                    break;
            }

            for (int i = image.width() - 1; i > 0; --i) {
                cimg_library::CImg<unsigned char> imageColumn = image.get_columns(i, i);
                imageColumn.channels(0, 2);
                pixelValueSum = imageColumn.sum();
                if (pixelValueSum == 0.0l)
                    ++columnCounterRight;
                else
                    break;
            }

            if (columnCounterLeft > 0)
                image.crop(columnCounterLeft, 0, 0, 3, image.width() - 1, image.height() - 1, 0, 0);

            if (columnCounterRight > 0)
                image.crop(0, 0, 0, 3, image.width() - columnCounterRight - 1, image.height() - 1,
                           0, 0);
        }

        void addDropShadow(cimg_library::CImg<unsigned char>& image,
                           unsigned int shadowDistance,
                           float transparency,
                           unsigned int iterations)
        {
            // Check that the image actually has an alpha channel.
            if (image.spectrum() != 4)
                return;

            // Make the shadow image larger than the source image to leave space for the
            // drop shadow.
            cimg_library::CImg<unsigned char> shadowImage(
                image.width() + shadowDistance * 3, image.height() + shadowDistance * 3, 1, 4, 0);

            // Create a mask image.
            cimg_library::CImg<unsigned char> maskImage(image.width(), image.height(), 1, 4, 0);
            maskImage.draw_image(0, 0, image);
            // Fill the RGB channels with white so we end up with a simple mask.
            maskImage.get_shared_channels(0, 2).fill(255);

            // Make a black outline of the source image as a basis for the shadow.
            shadowImage.draw_image(shadowDistance, shadowDistance, image);
            shadowImage.get_shared_channels(0, 2).fill(0);
            // Lower the transparency and apply the blur.
            shadowImage.get_shared_channel(3) /= transparency;
            shadowImage.blur_box(static_cast<const float>(shadowDistance),
                                 static_cast<const float>(shadowDistance), 1, true, iterations);

            // Add the mask to the alpha channel of the shadow image.
            shadowImage.get_shared_channel(3).draw_image(0, 0, maskImage.get_shared_channels(0, 0),
                                                         maskImage.get_shared_channel(3), 1, 255);
            // Draw the source image on top of the shadow image.
            shadowImage.draw_image(0, 0, image.get_shared_channels(0, 2),
                                   image.get_shared_channel(3), 1, 255);
            // Remove the any unused space that we added to leave room for the shadow.
            removeTransparentPadding(shadowImage);

            image = shadowImage;
        }

    } // namespace CImg

} // namespace Utils

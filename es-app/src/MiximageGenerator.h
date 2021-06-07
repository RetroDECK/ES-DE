//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MiximageGenerator.h
//
//  Generates miximages from screenshots, marquees and 3D box/cover images.
//  Called from GuiScraperSearch.
//

#ifndef ES_APP_SCRAPERS_MIXIMAGE_GENERATOR_H
#define ES_APP_SCRAPERS_MIXIMAGE_GENERATOR_H

// Disable the CImg display capabilities.
#define cimg_display 0

#include "FileData.h"
#include "GuiComponent.h"

#include <CImg.h>
#include <FreeImage.h>
#include <future>

using namespace cimg_library;

class MiximageGenerator
{
public:
    MiximageGenerator(FileData* game, bool& result, std::string& resultMessage);
    ~MiximageGenerator();

    void startThread(std::promise<bool>* miximagePromise);

private:
    bool generateImage();
    void cropLetterboxes(CImg<unsigned char>& image);
    void cropPillarboxes(CImg<unsigned char>& image);
    void removeTransparentPadding(CImg<unsigned char>& image);
    void addDropShadow(CImg<unsigned char>& image, unsigned int shadowDistance);
    void sampleFrameColor(CImg<unsigned char>& screenshotImage, unsigned char (&frameColor)[4]);

    void convertToCImgFormat(CImg<unsigned char>& image, std::vector<unsigned char> imageVector);
    void convertFromCImgFormat(CImg<unsigned char> image, std::vector<unsigned char>& imageVector);

    std::string getSavePath();

    FileData* mGame;
    bool& mResult;
    std::string& mResultMessage;
    std::string mMessage;

    std::promise<bool>* mMiximagePromise;

    std::string mScreenshotPath;
    std::string mMarqueePath;
    std::string mBox3DPath;
    std::string mCoverPath;

    int mWidth;
    int mHeight;

    bool mMarquee;
    bool mBox3D;
    bool mCover;
};

#endif // ES_APP_SCRAPERS_MIXIMAGE_GENERATOR_H

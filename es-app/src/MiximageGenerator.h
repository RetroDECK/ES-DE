//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MiximageGenerator.h
//
//  Generates miximages from screenshots, marquees, 3D boxes/covers and physical media images.
//  Called from GuiScraperSearch and GuiOfflineGenerator.
//

#ifndef ES_APP_SCRAPERS_MIXIMAGE_GENERATOR_H
#define ES_APP_SCRAPERS_MIXIMAGE_GENERATOR_H

#include "FileData.h"
#include "GuiComponent.h"
#include "utils/CImgUtil.h"

#include <FreeImage.h>
#include <future>

using namespace cimg_library;

class MiximageGenerator
{
public:
    MiximageGenerator(FileData* game, std::string& resultMessage);

    void startThread(std::promise<bool>* miximagePromise);

private:
    bool generateImage();
    void calculateMarqueeSize(const unsigned int& targetWidth,
                              const unsigned int& targetHeight,
                              unsigned int& width,
                              unsigned int& height);
    void sampleFrameColor(CImg<unsigned char>& screenshotImage, unsigned char (&frameColor)[4]);

    std::string getSavePath() const;

    FileData* mGame;
    std::string& mResultMessage;
    std::string mMessage;
    std::promise<bool>* mMiximagePromise;

    std::string mScreenshotPath;
    std::string mMarqueePath;
    std::string mBox3DPath;
    std::string mCoverPath;
    std::string mPhysicalMediaPath;

    int mWidth;
    int mHeight;

    bool mMarquee;
    bool mBox3D;
    bool mCover;
    bool mPhysicalMedia;
};

#endif // ES_APP_SCRAPERS_MIXIMAGE_GENERATOR_H

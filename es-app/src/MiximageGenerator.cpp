//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MiximageGenerator.cpp
//
//  Generates miximages from screenshots, marquees, 3D boxes/covers and physical media images.
//  Called from GuiScraperSearch and GuiOfflineGenerator.
//

#include "MiximageGenerator.h"

#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/StringUtil.h"

#include <chrono>

MiximageGenerator::MiximageGenerator(FileData* game, std::string& resultMessage)
    : mGame {game}
    , mResultMessage {resultMessage}
    , mWidth {1280}
    , mHeight {960}
    , mMarquee {false}
    , mBox3D {false}
    , mCover {false}
    , mPhysicalMedia {false}
{
}

void MiximageGenerator::startThread(std::promise<bool>* miximagePromise)
{
    mMiximagePromise = miximagePromise;

    LOG(LogDebug) << "MiximageGenerator::MiximageGenerator(): Creating miximage for \""
                  << mGame->getFileName() << "\"";

    if (mGame->getMiximagePath() != "" && !Settings::getInstance()->getBool("MiximageOverwrite")) {
        LOG(LogDebug) << "MiximageGenerator::MiximageGenerator(): File already exists and miximage "
                         "overwriting has not been enabled, aborting";
        mMiximagePromise->set_value(true);
        return;
    }

    if ((mScreenshotPath = mGame->getScreenshotPath()) == "") {
        LOG(LogDebug) << "MiximageGenerator::MiximageGenerator(): "
                         "No screenshot image found, aborting";
        mResultMessage = "No screenshot image found, couldn't generate miximage";
        mMiximagePromise->set_value(true);
        return;
    }

    if (Settings::getInstance()->getBool("MiximageIncludeMarquee")) {
        if ((mMarqueePath = mGame->getMarqueePath()) != "") {
            mMarquee = true;
        }
        else {
            LOG(LogDebug) << "MiximageGenerator::MiximageGenerator(): No marquee image found";
        }
    }

    if (Settings::getInstance()->getBool("MiximageIncludeBox")) {
        if ((mBox3DPath = mGame->get3DBoxPath()) != "") {
            mBox3D = true;
        }
        else if (Settings::getInstance()->getBool("MiximageCoverFallback") &&
                 (mCoverPath = mGame->getCoverPath()) != "") {
            LOG(LogDebug) << "MiximageGenerator::MiximageGenerator(): "
                             "No 3D box image found, using cover image as fallback";
            mCover = true;
        }
        else if (Settings::getInstance()->getBool("MiximageCoverFallback")) {
            LOG(LogDebug) << "MiximageGenerator::MiximageGenerator(): "
                             "No 3D box or cover images found";
        }
        else {
            LOG(LogDebug) << "MiximageGenerator::MiximageGenerator(): No 3D box image found";
        }
    }

    if (Settings::getInstance()->getBool("MiximageIncludePhysicalMedia")) {
        if ((mPhysicalMediaPath = mGame->getPhysicalMediaPath()) != "") {
            mPhysicalMedia = true;
        }
        else {
            LOG(LogDebug)
                << "MiximageGenerator::MiximageGenerator(): No physical media image found";
        }
    }

    const auto startTime = std::chrono::system_clock::now();

    if (generateImage()) {
        LOG(LogError) << "Failed to generate miximage";
        mMiximagePromise->set_value(true);
        mResultMessage = mMessage;
        return;
    }
    else {
        LOG(LogDebug) << "MiximageGenerator::MiximageGenerator(): Processing completed in: "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now() - startTime)
                             .count()
                      << " ms";
    }

    mResultMessage = mMessage;
    mMiximagePromise->set_value(false);
}

bool MiximageGenerator::generateImage()
{
    FREE_IMAGE_FORMAT fileFormat;
    FIBITMAP* screenshotFile {nullptr};
    FIBITMAP* marqueeFile {nullptr};
    FIBITMAP* boxFile {nullptr};
    FIBITMAP* physicalMediaFile {nullptr};

    unsigned int fileWidth {0};
    unsigned int fileHeight {0};
    unsigned int filePitch {0};

#if defined(_WIN64)
    fileFormat = FreeImage_GetFileTypeU(Utils::String::stringToWideString(mScreenshotPath).c_str());
#else
    fileFormat = FreeImage_GetFileType(mScreenshotPath.c_str());
#endif

    if (fileFormat == FIF_UNKNOWN)
#if defined(_WIN64)
        fileFormat = FreeImage_GetFIFFromFilenameU(
            Utils::String::stringToWideString(mScreenshotPath).c_str());
#else
        fileFormat = FreeImage_GetFIFFromFilename(mScreenshotPath.c_str());
#endif

    if (fileFormat == FIF_UNKNOWN) {
        LOG(LogError) << "Screenshot image in unknown image format, aborting";
        mMessage = "Screenshot image in unknown format, couldn't generate miximage";
        return true;
    }

    // Make sure that we can actually read this format.
    if (FreeImage_FIFSupportsReading(fileFormat)) {
#if defined(_WIN64)
        screenshotFile =
            FreeImage_LoadU(fileFormat, Utils::String::stringToWideString(mScreenshotPath).c_str());
#else
        screenshotFile = FreeImage_Load(fileFormat, mScreenshotPath.c_str());
#endif
    }
    else {
        LOG(LogError) << "Screenshot file format not supported";
        mMessage = "Screenshot image in unsupported format, couldn't generate miximage";
        return true;
    }

    if (!screenshotFile) {
        LOG(LogError) << "Error loading screenshot image, corrupt file?";
        mMessage = "Error loading screenshot image, couldn't generate miximage";
        return true;
    }

    if (mMarquee) {
#if defined(_WIN64)
        fileFormat =
            FreeImage_GetFileTypeU(Utils::String::stringToWideString(mMarqueePath).c_str());
#else
        fileFormat = FreeImage_GetFileType(mMarqueePath.c_str());
#endif

        if (fileFormat == FIF_UNKNOWN)
#if defined(_WIN64)
            fileFormat = FreeImage_GetFIFFromFilenameU(
                Utils::String::stringToWideString(mMarqueePath).c_str());
#else
            fileFormat = FreeImage_GetFIFFromFilename(mMarqueePath.c_str());
#endif

        if (fileFormat == FIF_UNKNOWN) {
            LOG(LogDebug) << "Marquee in unknown format, skipping image";
            mMarquee = false;
        }

        if (!FreeImage_FIFSupportsReading(fileFormat)) {
            LOG(LogDebug) << "Marquee file format not supported, skipping image";
            mMarquee = false;
        }
        else {
#if defined(_WIN64)
            marqueeFile = FreeImage_LoadU(fileFormat,
                                          Utils::String::stringToWideString(mMarqueePath).c_str());
#else
            marqueeFile = FreeImage_Load(fileFormat, mMarqueePath.c_str());
#endif
            if (!marqueeFile) {
                LOG(LogError) << "Couldn't load marquee image, corrupt file?";
                mMessage = "Error loading marquee image, corrupt file?";
                mMarquee = false;
            }
        }
    }

    if (mBox3D) {
#if defined(_WIN64)
        fileFormat = FreeImage_GetFileTypeU(Utils::String::stringToWideString(mBox3DPath).c_str());
#else
        fileFormat = FreeImage_GetFileType(mBox3DPath.c_str());
#endif

        if (fileFormat == FIF_UNKNOWN)
#if defined(_WIN64)
            fileFormat = FreeImage_GetFIFFromFilenameU(
                Utils::String::stringToWideString(mBox3DPath).c_str());
#else
            fileFormat = FreeImage_GetFIFFromFilename(mBox3DPath.c_str());
#endif

        if (fileFormat == FIF_UNKNOWN) {
            LOG(LogDebug) << "3D box in unknown format, skipping image";
            mBox3D = false;
        }

        if (!FreeImage_FIFSupportsReading(fileFormat)) {
            LOG(LogDebug) << "3D box file format not supported, skipping image";
            mBox3D = false;
        }
        else {
#if defined(_WIN64)
            boxFile =
                FreeImage_LoadU(fileFormat, Utils::String::stringToWideString(mBox3DPath).c_str());
#else
            boxFile = FreeImage_Load(fileFormat, mBox3DPath.c_str());
#endif
            if (!boxFile) {
                LOG(LogError) << "Couldn't load 3D box image, corrupt file?";
                mMessage = "Error loading 3d box image, corrupt file?";
                mBox3D = false;
            }
        }
    }
    else if (mCover) {
#if defined(_WIN64)
        fileFormat = FreeImage_GetFileTypeU(Utils::String::stringToWideString(mCoverPath).c_str());
#else
        fileFormat = FreeImage_GetFileType(mCoverPath.c_str());
#endif

        if (fileFormat == FIF_UNKNOWN)
#if defined(_WIN64)
            fileFormat = FreeImage_GetFIFFromFilenameU(
                Utils::String::stringToWideString(mCoverPath).c_str());
#else
            fileFormat = FreeImage_GetFIFFromFilename(mCoverPath.c_str());
#endif

        if (fileFormat == FIF_UNKNOWN) {
            LOG(LogDebug) << "Box cover in unknown format, skipping image";
            mCover = false;
        }

        if (!FreeImage_FIFSupportsReading(fileFormat)) {
            LOG(LogDebug) << "Box cover file format not supported, skipping image";
            mCover = false;
        }
        else {
#if defined(_WIN64)
            boxFile =
                FreeImage_LoadU(fileFormat, Utils::String::stringToWideString(mCoverPath).c_str());
#else
            boxFile = FreeImage_Load(fileFormat, mCoverPath.c_str());
#endif
            if (!boxFile) {
                LOG(LogError) << "Couldn't load box cover image, corrupt file?";
                mMessage = "Error loading box cover image, corrupt file?";
                mCover = false;
            }
        }
    }

    if (mPhysicalMedia) {
#if defined(_WIN64)
        fileFormat =
            FreeImage_GetFileTypeU(Utils::String::stringToWideString(mPhysicalMediaPath).c_str());
#else
        fileFormat = FreeImage_GetFileType(mPhysicalMediaPath.c_str());
#endif

        if (fileFormat == FIF_UNKNOWN)
#if defined(_WIN64)
            fileFormat = FreeImage_GetFIFFromFilenameU(
                Utils::String::stringToWideString(mPhysicalMediaPath).c_str());
#else
            fileFormat = FreeImage_GetFIFFromFilename(mPhysicalMediaPath.c_str());
#endif

        if (fileFormat == FIF_UNKNOWN) {
            LOG(LogDebug) << "Physical media in unknown format, skipping image";
            mPhysicalMedia = false;
        }

        if (!FreeImage_FIFSupportsReading(fileFormat)) {
            LOG(LogDebug) << "Physical media file format not supported, skipping image";
            mPhysicalMedia = false;
        }
        else {
#if defined(_WIN64)
            physicalMediaFile = FreeImage_LoadU(
                fileFormat, Utils::String::stringToWideString(mPhysicalMediaPath).c_str());
#else
            physicalMediaFile = FreeImage_Load(fileFormat, mPhysicalMediaPath.c_str());
#endif
            if (!physicalMediaFile) {
                LOG(LogError) << "Couldn't load physical media image, corrupt file?";
                mMessage = "Error loading physical media image, corrupt file?";
                mPhysicalMedia = false;
            }
        }
    }

    unsigned int resolutionMultiplier = 0;

    if (Settings::getInstance()->getString("MiximageResolution") == "640x480") {
        mWidth = 640;
        mHeight = 480;
        resolutionMultiplier = 1;
    }
    else if (Settings::getInstance()->getString("MiximageResolution") == "1920x1440") {
        mWidth = 1920;
        mHeight = 1440;
        resolutionMultiplier = 3;
    }
    else {
        mWidth = 1280;
        mHeight = 960;
        resolutionMultiplier = 2;
    }

    const unsigned int screenshotWidth {530 * resolutionMultiplier};
    const unsigned int screenshotOffset {20 * resolutionMultiplier};
    const unsigned int screenshotFrameWidth {6 * resolutionMultiplier};
    const unsigned int screenshotHeight {400 * resolutionMultiplier};

    // These sizes are increased slightly when adding the drop shadow.
    const unsigned int marqueeTargetWidth {310 * resolutionMultiplier};
    const unsigned int marqueeTargetHeight {230 * resolutionMultiplier};
    unsigned int boxTargetWidth {0};
    unsigned int boxTargetHeight {0};
    unsigned int coverTargetWidth {0};
    unsigned int physicalMediaTargetWidth {0};
    unsigned int physicalMediaTargetHeight {0};

    if (Settings::getInstance()->getString("MiximageBoxSize") == "small") {
        boxTargetWidth = 264 * resolutionMultiplier;
        boxTargetHeight = 254 * resolutionMultiplier;
        coverTargetWidth = 212 * resolutionMultiplier;
    }
    else if (Settings::getInstance()->getString("MiximageBoxSize") == "large") {
        boxTargetWidth = 372 * resolutionMultiplier;
        boxTargetHeight = 360 * resolutionMultiplier;
        coverTargetWidth = 300 * resolutionMultiplier;
    }
    else { // Medium size.
        boxTargetWidth = 310 * resolutionMultiplier;
        boxTargetHeight = 300 * resolutionMultiplier;
        coverTargetWidth = 250 * resolutionMultiplier;
    }

    if (Settings::getInstance()->getString("MiximagePhysicalMediaSize") == "small") {
        physicalMediaTargetWidth = 120 * resolutionMultiplier;
        physicalMediaTargetHeight = 96 * resolutionMultiplier;
    }
    else if (Settings::getInstance()->getString("MiximagePhysicalMediaSize") == "large") {
        physicalMediaTargetWidth = 196 * resolutionMultiplier;
        physicalMediaTargetHeight = 156 * resolutionMultiplier;
    }
    else { // Medium size.
        physicalMediaTargetWidth = 150 * resolutionMultiplier;
        physicalMediaTargetHeight = 120 * resolutionMultiplier;
    }

    const unsigned int marqueeShadowSize {6 * resolutionMultiplier};
    const unsigned int boxShadowSize {6 * resolutionMultiplier};
    const unsigned int physicalMediaShadowSize {6 * resolutionMultiplier};

    if (FreeImage_GetBPP(screenshotFile) != 32) {
        FIBITMAP* screenshotTemp {FreeImage_ConvertTo32Bits(screenshotFile)};
        FreeImage_Unload(screenshotFile);
        screenshotFile = screenshotTemp;
    }

    fileWidth = FreeImage_GetWidth(screenshotFile);
    fileHeight = FreeImage_GetHeight(screenshotFile);
    filePitch = FreeImage_GetPitch(screenshotFile);

    std::vector<unsigned char> screenshotVector(fileWidth * fileHeight * 4);

    FreeImage_ConvertToRawBits(reinterpret_cast<BYTE*>(&screenshotVector.at(0)), screenshotFile,
                               filePitch, 32, FI_RGBA_BLUE, FI_RGBA_GREEN, FI_RGBA_RED, 1);

    CImg<unsigned char> screenshotImage(fileWidth, fileHeight, 1, 4, 0);

    // Convert the RGBA image to CImg internal format.
    Utils::CImg::convertBGRAToCImg(screenshotVector, screenshotImage);
    screenshotVector.clear();

    if (Settings::getInstance()->getBool("MiximageRemoveLetterboxes"))
        Utils::CImg::cropLetterboxes(screenshotImage);
    if (Settings::getInstance()->getBool("MiximageRemovePillarboxes"))
        Utils::CImg::cropPillarboxes(screenshotImage);

    if (Settings::getInstance()->getString("MiximageScreenshotScaling") == "smooth") {
        // Lanczos scaling is normally not recommended for low resolution graphics as
        // it makes the pixels appear smooth when scaling, but for more modern game
        // platforms it may be a good idea to use it.
        screenshotImage.resize(screenshotWidth, screenshotHeight, 1, 4, 6);
    }
    else {
        // Box interpolation gives completely sharp pixels, which is best suited for
        // low resolution retro games.
        screenshotImage.resize(screenshotWidth, screenshotHeight, 1, 4, 1);
    }

    // Remove any transparency information from the screenshot. There really should be no
    // alpha channel for these images, but if there is, it could interfere with the compositing
    // of the miximage.
    screenshotImage.get_shared_channel(3).fill(255);

    int xPosScreenshot {0};
    int yPosScreenshot {0};

    int xPosMarquee {0};
    int yPosMarquee {0};

    int xPosBox {0};
    int yPosBox {0};

    int xPosPhysicalMedia {0};
    int yPosPhysicalMedia {0};

    CImg<unsigned char> canvasImage(mWidth, mHeight, 1, 4, 0);

    CImg<unsigned char> marqueeImage;
    CImg<unsigned char> marqueeImageRGB;
    CImg<unsigned char> marqueeImageAlpha;

    CImg<unsigned char> boxImage;
    CImg<unsigned char> boxImageRGB;
    CImg<unsigned char> boxImageAlpha;

    CImg<unsigned char> physicalMediaImage;
    CImg<unsigned char> physicalMediaImageRGB;
    CImg<unsigned char> physicalMediaImageAlpha;

    CImg<unsigned char> frameImage(mWidth, mHeight, 1, 4, 0);

    xPosScreenshot = canvasImage.width() / 2 - screenshotImage.width() / 2 + screenshotOffset;
    yPosScreenshot = canvasImage.height() / 2 - screenshotImage.height() / 2;

    if (mMarquee) {
        if (FreeImage_GetBPP(marqueeFile) != 32) {
            FIBITMAP* marqueeTemp {FreeImage_ConvertTo32Bits(marqueeFile)};
            FreeImage_Unload(marqueeFile);
            marqueeFile = marqueeTemp;
        }

        fileWidth = FreeImage_GetWidth(marqueeFile);
        fileHeight = FreeImage_GetHeight(marqueeFile);
        filePitch = FreeImage_GetPitch(marqueeFile);

        std::vector<unsigned char> marqueeVector(fileWidth * fileHeight * 4);

        FreeImage_ConvertToRawBits(reinterpret_cast<BYTE*>(&marqueeVector.at(0)), marqueeFile,
                                   filePitch, 32, FI_RGBA_BLUE, FI_RGBA_GREEN, FI_RGBA_RED, 1);

        marqueeImage = CImg<unsigned char>(FreeImage_GetWidth(marqueeFile),
                                           FreeImage_GetHeight(marqueeFile), 1, 4, 0);

        Utils::CImg::convertBGRAToCImg(marqueeVector, marqueeImage);
        Utils::CImg::removeTransparentPadding(marqueeImage);

        unsigned int marqueeWidth {static_cast<unsigned int>(marqueeImage.width())};
        unsigned int marqueeHeight {static_cast<unsigned int>(marqueeImage.height())};

        calculateMarqueeSize(marqueeTargetWidth, marqueeTargetHeight, marqueeWidth, marqueeHeight);

        // We use Lanczos3 which is the highest quality resampling method available.
        marqueeImage.resize(marqueeWidth, marqueeHeight, 1, 4, 6);

        // Add a drop shadow using 4 iterations of box blur.
        Utils::CImg::addDropShadow(marqueeImage, marqueeShadowSize, 0.6f, 4);

        xPosMarquee = canvasImage.width() - marqueeImage.width();
        yPosMarquee = 0;

        // Only RGB channels for the image.
        marqueeImageRGB = CImg<unsigned char>(marqueeImage.get_shared_channels(0, 2));
        // Only alpha channel for the image.
        marqueeImageAlpha = CImg<unsigned char>(marqueeImage.get_shared_channel(3));
    }

    if (mBox3D || mCover) {
        if (FreeImage_GetBPP(boxFile) != 32) {
            FIBITMAP* boxTemp {FreeImage_ConvertTo32Bits(boxFile)};
            FreeImage_Unload(boxFile);
            boxFile = boxTemp;
        }

        fileWidth = FreeImage_GetWidth(boxFile);
        fileHeight = FreeImage_GetHeight(boxFile);
        filePitch = FreeImage_GetPitch(boxFile);

        std::vector<unsigned char> boxVector(fileWidth * fileHeight * 4);

        FreeImage_ConvertToRawBits(reinterpret_cast<BYTE*>(&boxVector.at(0)), boxFile, filePitch,
                                   32, FI_RGBA_BLUE, FI_RGBA_GREEN, FI_RGBA_RED, 1);

        boxImage =
            CImg<unsigned char>(FreeImage_GetWidth(boxFile), FreeImage_GetHeight(boxFile), 1, 4);

        Utils::CImg::convertBGRAToCImg(boxVector, boxImage);
        Utils::CImg::removeTransparentPadding(boxImage);

        float sizeRatio {static_cast<float>(boxImage.width()) /
                         static_cast<float>(boxImage.height())};

        if (sizeRatio > 1.14f && Settings::getInstance()->getBool("MiximageRotateHorizontalBoxes"))
            boxImage.rotate(90.0f);

        float scaleFactor {static_cast<float>(boxTargetHeight) /
                           static_cast<float>(boxImage.height())};
        unsigned int width {
            static_cast<unsigned int>(static_cast<float>(boxImage.width()) * scaleFactor)};
        unsigned int targetWidth {0};

        // We make this distinction as some cover images are in square format and would cover
        // too much surface otherwise.
        if (mBox3D)
            targetWidth = boxTargetWidth;
        else
            targetWidth = coverTargetWidth;

        if (width > targetWidth) {
            scaleFactor = static_cast<float>(targetWidth) / static_cast<float>(boxImage.width());
            int height {static_cast<int>(static_cast<float>(boxImage.height()) * scaleFactor)};
            // We use Lanczos3 which is the highest quality resampling method available.
            boxImage.resize(targetWidth, height, 1, 4, 6);
        }
        else {
            boxImage.resize(width, boxTargetHeight, 1, 4, 6);
        }

        Utils::CImg::addDropShadow(boxImage, boxShadowSize, 0.6f, 4);

        xPosBox = 0;
        yPosBox = canvasImage.height() - boxImage.height();

        // Only RGB channels for the image.
        boxImageRGB = CImg<unsigned char>(boxImage.get_shared_channels(0, 2));
        // Only alpha channel for the image.
        boxImageAlpha = CImg<unsigned char>(boxImage.get_shared_channel(3));
    }

    if (mPhysicalMedia) {
        if (FreeImage_GetBPP(physicalMediaFile) != 32) {
            FIBITMAP* physicalMediaTemp {FreeImage_ConvertTo32Bits(physicalMediaFile)};
            FreeImage_Unload(physicalMediaFile);
            physicalMediaFile = physicalMediaTemp;
        }

        fileWidth = FreeImage_GetWidth(physicalMediaFile);
        fileHeight = FreeImage_GetHeight(physicalMediaFile);
        filePitch = FreeImage_GetPitch(physicalMediaFile);

        std::vector<unsigned char> physicalMediaVector(fileWidth * fileHeight * 4);

        FreeImage_ConvertToRawBits(reinterpret_cast<BYTE*>(&physicalMediaVector.at(0)),
                                   physicalMediaFile, filePitch, 32, FI_RGBA_BLUE, FI_RGBA_GREEN,
                                   FI_RGBA_RED, 1);

        physicalMediaImage = CImg<unsigned char>(FreeImage_GetWidth(physicalMediaFile),
                                                 FreeImage_GetHeight(physicalMediaFile), 1, 4, 0);

        Utils::CImg::convertBGRAToCImg(physicalMediaVector, physicalMediaImage);
        Utils::CImg::removeTransparentPadding(physicalMediaImage);

        // Make sure the image size is not exceeding either the target width or height.
        float scaleFactorX {static_cast<float>(physicalMediaTargetWidth) /
                            static_cast<float>(physicalMediaImage.width())};
        float scaleFactorY {static_cast<float>(physicalMediaTargetHeight) /
                            static_cast<float>(physicalMediaImage.height())};
        float scaleFactor {std::min(scaleFactorX, scaleFactorY)};

        unsigned int width {static_cast<unsigned int>(
            static_cast<float>(physicalMediaImage.width()) * scaleFactor)};
        unsigned int height {static_cast<unsigned int>(
            static_cast<float>(physicalMediaImage.height()) * scaleFactor)};

        // We use Lanczos3 which is the highest quality resampling method available.
        physicalMediaImage.resize(width, height, 1, 4, 6);

        // Add a drop shadow using 4 iterations of box blur.
        Utils::CImg::addDropShadow(physicalMediaImage, physicalMediaShadowSize, 0.6f, 4);

        // Place it to the right of the 3D box or cover with a small margin in between.
        xPosPhysicalMedia = xPosBox + boxImage.width() + 16 * resolutionMultiplier;
        yPosPhysicalMedia = canvasImage.height() - physicalMediaImage.height();

        // Only RGB channels for the image.
        physicalMediaImageRGB = CImg<unsigned char>(physicalMediaImage.get_shared_channels(0, 2));
        // Only alpha channel for the image.
        physicalMediaImageAlpha = CImg<unsigned char>(physicalMediaImage.get_shared_channel(3));
    }

    CImg<unsigned char> frameImageAlpha(frameImage.get_shared_channel(3));
    frameImageAlpha.draw_image(xPosBox, yPosBox, boxImageAlpha);
    frameImageAlpha.draw_image(xPosPhysicalMedia, yPosPhysicalMedia, physicalMediaImageAlpha);
    frameImageAlpha.draw_image(xPosMarquee, yPosMarquee, marqueeImageAlpha);

    // Set a frame color based on an average of the screenshot contents.
    unsigned char frameColor[] = {0, 0, 0, 0};
    sampleFrameColor(screenshotImage, frameColor);

    // Upper / lower frame.
    frameImage.draw_rectangle(xPosScreenshot + 2, yPosScreenshot - screenshotFrameWidth,
                              xPosScreenshot + screenshotWidth - 2,
                              yPosScreenshot + screenshotHeight + screenshotFrameWidth - 1,
                              frameColor);

    // Left / right frame.
    frameImage.draw_rectangle(xPosScreenshot - screenshotFrameWidth, yPosScreenshot + 2,
                              xPosScreenshot + screenshotWidth + screenshotFrameWidth - 1,
                              yPosScreenshot + screenshotHeight - 2, frameColor);

    // We draw circles in order to get rounded corners for the frame.
    const unsigned int circleRadius {8 * resolutionMultiplier};
    const unsigned int circleOffset {2 * resolutionMultiplier};

    // Upper left corner.
    frameImage.draw_circle(xPosScreenshot + circleOffset, yPosScreenshot + circleOffset,
                           circleRadius, frameColor);
    // Upper right corner.
    frameImage.draw_circle(xPosScreenshot + screenshotWidth - circleOffset - 1,
                           yPosScreenshot + circleOffset, circleRadius, frameColor);
    // Lower right corner.
    frameImage.draw_circle(xPosScreenshot + screenshotWidth - circleOffset - 1,
                           yPosScreenshot + screenshotHeight - circleOffset - 1, circleRadius,
                           frameColor);
    // Lower left corner.
    frameImage.draw_circle(xPosScreenshot + circleOffset,
                           yPosScreenshot + screenshotHeight - circleOffset - 1, circleRadius,
                           frameColor);

    CImg<unsigned char> frameImageRGB(frameImage.get_shared_channels(0, 2));

    canvasImage.draw_image(0, 0, frameImage);
    canvasImage.draw_image(xPosScreenshot, yPosScreenshot, screenshotImage);

    if (mMarquee)
        canvasImage.draw_image(xPosMarquee, yPosMarquee, marqueeImageRGB, marqueeImageAlpha, 1,
                               255);
    if (mBox3D || mCover)
        canvasImage.draw_image(xPosBox, yPosBox, boxImageRGB, boxImageAlpha, 1, 255);

    if (mPhysicalMedia)
        canvasImage.draw_image(xPosPhysicalMedia, yPosPhysicalMedia, physicalMediaImageRGB,
                               physicalMediaImageAlpha, 1, 255);

    std::vector<unsigned char> canvasVector;

    // Convert the image from CImg internal format.
    Utils::CImg::convertCImgToBGRA(canvasImage, canvasVector);

    FIBITMAP* mixImage {nullptr};
    mixImage = FreeImage_ConvertFromRawBits(&canvasVector.at(0), canvasImage.width(),
                                            canvasImage.height(), canvasImage.width() * 4, 32,
                                            FI_RGBA_BLUE, FI_RGBA_GREEN, FI_RGBA_RED);

#if defined(_WIN64)
    bool savedImage {FreeImage_SaveU(FIF_PNG, mixImage,
                                     Utils::String::stringToWideString(getSavePath()).c_str()) !=
                     0};
#else
    bool savedImage {FreeImage_Save(FIF_PNG, mixImage, getSavePath().c_str()) != 0};
#endif

    if (!savedImage) {
        LOG(LogError) << "Couldn't save miximage, permission problems or disk full?";
    }

    FreeImage_Unload(screenshotFile);
    FreeImage_Unload(marqueeFile);
    FreeImage_Unload(boxFile);
    FreeImage_Unload(physicalMediaFile);
    FreeImage_Unload(mixImage);

    // Success.
    if (savedImage)
        return false;
    else
        return true;
}

void MiximageGenerator::calculateMarqueeSize(const unsigned int& targetWidth,
                                             const unsigned int& targetHeight,
                                             unsigned int& width,
                                             unsigned int& height)
{
    unsigned int adjustedTargetWidth {0};
    float widthModifier {0.5f};
    float scaleFactor {0.0f};

    // The idea is to adjust the size of the marquee based on its surface area, so that
    // wider but shorter images get a larger width than taller images in order to use
    // an approximately equivalent amount of space on the miximage.
    float widthRatio {static_cast<float>(width) / static_cast<float>(height)};

    widthModifier = glm::clamp(widthModifier + widthRatio / 6.5f, 0.0f, 1.0f);

    // Hack to increase the size slightly for wider and shorter images.
    if (widthRatio >= 4)
        widthModifier += glm::clamp(widthRatio / 40.0f, 0.0f, 0.3f);

    adjustedTargetWidth =
        static_cast<unsigned int>(static_cast<float>(targetWidth) * widthModifier);
    scaleFactor = static_cast<float>(adjustedTargetWidth) / static_cast<float>(width);

    // For really tall and narrow images, we may have exceeded the target height.
    if (static_cast<int>(scaleFactor * static_cast<float>(height)) >
        static_cast<float>(targetHeight))
        scaleFactor = static_cast<float>(targetHeight) / static_cast<float>(height);

    width = static_cast<int>(static_cast<float>(width) * scaleFactor);
    height = static_cast<int>(static_cast<float>(height) * scaleFactor);
}

void MiximageGenerator::sampleFrameColor(CImg<unsigned char>& screenshotImage,
                                         unsigned char (&frameColor)[4])
{
    // Calculate the number of samples relative to the configured resolution so we get
    // the same result regardless of miximage target size setting.
    unsigned int samples {static_cast<unsigned int>(static_cast<float>(mWidth) * 0.03125f)};

    unsigned int red {0};
    unsigned int green {0};
    unsigned int blue {0};

    unsigned int redLine {0};
    unsigned int greenLine {0};
    unsigned int blueLine {0};

    unsigned int counter {0};

    // This is a very simple method to get an average pixel value. It's limited in that it
    // does not consider dominant colors and such, so the result could possibly be a value
    // that does not match the perceived color palette of the image. In most cases it works
    // good enough though.
    for (int r = samples / 2; r < screenshotImage.height(); r += samples) {
        for (int c = samples / 2; c < screenshotImage.width(); c += samples) {
            red += screenshotImage(c, r, 0, 0);
            green += screenshotImage(c, r, 0, 1);
            blue += screenshotImage(c, r, 0, 2);
            ++counter;
        }

        if (counter > 0) {
            redLine += red / counter;
            greenLine += green / counter;
            blueLine += blue / counter;
            counter = 0;
        }
    }

    unsigned char redC {
        static_cast<unsigned char>(glm::clamp(static_cast<int>(redLine / 255), 0, 255))};
    unsigned char greenC {
        static_cast<unsigned char>(glm::clamp(static_cast<int>(greenLine / 255), 0, 255))};
    unsigned char blueC {
        static_cast<unsigned char>(glm::clamp(static_cast<int>(blueLine / 255), 0, 255))};

    // Convert to the HSL color space to be able to modify saturation and lightness.
    CImg<float> colorHSL = CImg<>(1, 1, 1, 3).fill(redC, greenC, blueC).RGBtoHSL();

    // float hue = colorHSL(0, 0, 0, 0);
    float saturation {colorHSL(0, 0, 0, 1)};
    float lightness {colorHSL(0, 0, 0, 2)};

    // Decrease saturation slightly and increase lightness a bit, these adjustments
    // makes the end result look better than the raw average pixel value. Also clamp
    // the lightness to a low value so we don't get a frame that is nearly pitch black
    // if the screenshot mostly contains blacks or dark colors.
    colorHSL(0, 0, 0, 1) = glm::clamp(saturation * 0.9f, 0.0f, 1.0f);
    colorHSL(0, 0, 0, 2) = glm::clamp(lightness * 1.25f, 0.10f, 1.0f);

    const CImg<unsigned char> colorRGB = colorHSL.HSLtoRGB();

    frameColor[0] = colorRGB(0, 0, 0, 0);
    frameColor[1] = colorRGB(0, 0, 0, 1);
    frameColor[2] = colorRGB(0, 0, 0, 2);
    frameColor[3] = 255;
}

std::string MiximageGenerator::getSavePath() const
{
    const std::string name {Utils::FileSystem::getStem(mGame->getPath())};
    std::string subFolders;

    // Extract possible subfolders from the path.
    if (mGame->getSystemEnvData()->mStartPath != "")
        subFolders = Utils::String::replace(Utils::FileSystem::getParent(mGame->getPath()),
                                            mGame->getSystemEnvData()->mStartPath, "");

    std::string path {FileData::getMediaDirectory()};

    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    path += mGame->getSystemName() + "/miximages" + subFolders + "/";

    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    path += name + ".png";

    // Success.
    return path;
}

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MiximageGenerator.cpp
//
//  Generates miximages from screenshots, marquees and 3D box/cover images.
//  Called from GuiScraperSearch.
//

#include "MiximageGenerator.h"

#include "math/Misc.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"

#include <chrono>

MiximageGenerator::MiximageGenerator(FileData* game, bool& result, std::string& resultMessage)
        : mGame(game),
        mResult(result),
        mResultMessage(resultMessage),
        mWidth(1280),
        mHeight(960),
        mMarquee(false),
        mBox3D(false),
        mCover(false)
{
}

MiximageGenerator::~MiximageGenerator()
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
        mResult = true;
        mMiximagePromise->set_value(true);
        return;
    }

    if ((mScreenshotPath = mGame->getScreenshotPath()) == "") {
        LOG(LogDebug) << "MiximageGenerator::MiximageGenerator(): "
                "No screenshot image found, aborting";
            mResultMessage = "No screenshot image found, couldn't generate miximage";
        mResult = true;
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
                (mCoverPath= mGame->getCoverPath()) != "") {
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

    const auto startTime = std::chrono::system_clock::now();

    if (generateImage()) {
        LOG(LogError) << "Failed to generate miximage";
        mResult = true;
    }
    else {
        const auto endTime = std::chrono::system_clock::now();

        LOG(LogDebug) << "MiximageGenerator::MiximageGenerator(): Processing completed in: " <<
            std::chrono::duration_cast<std::chrono::milliseconds>
            (endTime - startTime).count() << " ms";
    }

    mResult = false;
    mResultMessage = mMessage;
    mMiximagePromise->set_value(false);
}

bool MiximageGenerator::generateImage()
{
    FREE_IMAGE_FORMAT fileFormat;
    FIBITMAP* screenshotFile = nullptr;
    FIBITMAP* marqueeFile = nullptr;
    FIBITMAP* boxFile = nullptr;

    unsigned int fileWidth = 0;
    unsigned int fileHeight = 0;
    unsigned int filePitch = 0;

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
        screenshotFile = FreeImage_LoadU(fileFormat,
                Utils::String::stringToWideString(mScreenshotPath).c_str());
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
        fileFormat = FreeImage_GetFileTypeU(
                Utils::String::stringToWideString(mMarqueePath).c_str());
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
            boxFile = FreeImage_LoadU(fileFormat,
                    Utils::String::stringToWideString(mBox3DPath).c_str());
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
        fileFormat = FreeImage_GetFileTypeU(
                Utils::String::stringToWideString(mCoverPath).c_str());
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
            boxFile = FreeImage_LoadU(fileFormat,
                    Utils::String::stringToWideString(mCoverPath).c_str());
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

    const unsigned int screenshotWidth = 530 * resolutionMultiplier;
    const unsigned int screenshotOffset = 20 * resolutionMultiplier;
    const unsigned int screenshotFrameWidth = 5 * resolutionMultiplier;
    const unsigned int screenshotHeight = 400 * resolutionMultiplier;
    const unsigned int marqueeWidth = 260 * resolutionMultiplier;
    const unsigned int marqueeMaxHeight = 220 * resolutionMultiplier;
    const unsigned int boxHeight = 300 * resolutionMultiplier;
    const unsigned int boxMaxWidth = 340 * resolutionMultiplier;
    const unsigned int coverMaxWidth = 250 * resolutionMultiplier;
    const unsigned int marqueeShadow = 10;
    const unsigned int boxShadow = 14;

    if (FreeImage_GetBPP(screenshotFile) != 32) {
        FIBITMAP* screenshotTemp = FreeImage_ConvertTo32Bits(screenshotFile);
        FreeImage_Unload(screenshotFile);
        screenshotFile = screenshotTemp;
    }

    fileWidth = FreeImage_GetWidth(screenshotFile);
    fileHeight = FreeImage_GetHeight(screenshotFile);
    filePitch = FreeImage_GetPitch(screenshotFile);

    std::vector<unsigned char> screenshotVector(fileWidth * fileHeight * 4);

    FreeImage_ConvertToRawBits(reinterpret_cast<BYTE*>(&screenshotVector.at(0)), screenshotFile,
            filePitch, 32, FI_RGBA_RED, FI_RGBA_GREEN, FI_RGBA_BLUE, 1);

    CImg<unsigned char> screenshotImage(fileWidth, fileHeight, 1, 4, 0);

    // Convert image to CImg internal format.
    convertToCImgFormat(screenshotImage, screenshotVector);
    screenshotVector.clear();

    if (Settings::getInstance()->getBool("MiximageRemoveLetterboxes"))
        cropLetterboxes(screenshotImage);
    if (Settings::getInstance()->getBool("MiximageRemovePillarboxes"))
        cropPillarboxes(screenshotImage);

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

    int xPosScreenshot = 0;
    int yPosScreenshot = 0;

    int xPosMarquee = 0;
    int yPosMarquee = 0;

    int xPosBox = 0;
    int yPosBox = 0;

    CImg<unsigned char> canvasImage(mWidth, mHeight, 1, 4, 0);

    CImg<unsigned char> marqueeImage;
    CImg<unsigned char> marqueeImageRGB;
    CImg<unsigned char> marqueeImageAlpha;

    CImg<unsigned char> boxImage;
    CImg<unsigned char> boxImageRGB;
    CImg<unsigned char> boxImageAlpha;

    CImg<unsigned char> frameImage(mWidth, mHeight, 1, 4, 0);

    xPosScreenshot = canvasImage.width() / 2 - screenshotImage.width() / 2 + screenshotOffset;
    yPosScreenshot = canvasImage.height() / 2 - screenshotImage.height() / 2;

    if (mMarquee) {
        if (FreeImage_GetBPP(marqueeFile) != 32) {
            FIBITMAP* marqueeTemp = FreeImage_ConvertTo32Bits(marqueeFile);
            FreeImage_Unload(marqueeFile);
            marqueeFile = marqueeTemp;
        }

        fileWidth = FreeImage_GetWidth(marqueeFile);
        fileHeight = FreeImage_GetHeight(marqueeFile);
        filePitch = FreeImage_GetPitch(marqueeFile);

        std::vector<unsigned char> marqueeVector(fileWidth * fileHeight * 4);

        FreeImage_ConvertToRawBits(reinterpret_cast<BYTE*>(&marqueeVector.at(0)), marqueeFile,
                filePitch, 32, FI_RGBA_RED, FI_RGBA_GREEN, FI_RGBA_BLUE, 1);

        marqueeImage = CImg<unsigned char>(FreeImage_GetWidth(marqueeFile),
                FreeImage_GetHeight(marqueeFile), 1, 4, 0);

        convertToCImgFormat(marqueeImage, marqueeVector);
        removeTransparentPadding(marqueeImage);
        addDropShadow(marqueeImage, marqueeShadow);

        float scaleFactor = static_cast<float>(marqueeWidth) /
                static_cast<float>(marqueeImage.width());
        unsigned int height =
                static_cast<int>(static_cast<float>(marqueeImage.height()) * scaleFactor);

        if (height > marqueeMaxHeight) {
            scaleFactor = static_cast<float>(marqueeMaxHeight) /
                    static_cast<float>(marqueeImage.height());
            int width = static_cast<int>(static_cast<float>(marqueeImage.width()) * scaleFactor);
            // We use Lanczos3 which is the highest quality resampling method available.
            marqueeImage.resize(width, marqueeMaxHeight, 1, 4, 6);
        }
        else {
            marqueeImage.resize(marqueeWidth, height, 1, 4, 6);
        }

        xPosMarquee = canvasImage.width() - marqueeImage.width();
        yPosMarquee = 0;

        // Only RGB channels for the image.
        marqueeImageRGB = CImg<unsigned char>(marqueeImage.get_shared_channels(0,2));
        // Only alpha channel for the image.
        marqueeImageAlpha = CImg<unsigned char>(marqueeImage.get_shared_channel(3));
    }

    if (mBox3D || mCover) {
        if (FreeImage_GetBPP(boxFile) != 32) {
            FIBITMAP* boxTemp = FreeImage_ConvertTo32Bits(boxFile);
            FreeImage_Unload(boxFile);
            boxFile = boxTemp;
        }

        fileWidth = FreeImage_GetWidth(boxFile);
        fileHeight = FreeImage_GetHeight(boxFile);
        filePitch = FreeImage_GetPitch(boxFile);

        std::vector<unsigned char> boxVector(fileWidth * fileHeight * 4);

        FreeImage_ConvertToRawBits(reinterpret_cast<BYTE*>(&boxVector.at(0)), boxFile,
                filePitch, 32, FI_RGBA_RED, FI_RGBA_GREEN, FI_RGBA_BLUE, 1);

        boxImage = CImg<unsigned char>(FreeImage_GetWidth(boxFile),
                FreeImage_GetHeight(boxFile), 1, 4);

        convertToCImgFormat(boxImage, boxVector);
        removeTransparentPadding(boxImage);
        addDropShadow(boxImage, boxShadow);

        float scaleFactor = static_cast<float>(boxHeight) / static_cast<float>(boxImage.height());
        unsigned int width = static_cast<int>(static_cast<float>(boxImage.width()) * scaleFactor);
        unsigned int maxWidth = 0;

        // We make this distinction as some cover images are in square format and would cover
        // too much surface otherwise.
        if (mBox3D)
            maxWidth = boxMaxWidth;
        else
            maxWidth = coverMaxWidth;

        if (width > maxWidth) {
            scaleFactor = static_cast<float>(maxWidth) / static_cast<float>(boxImage.width());
            int height = static_cast<int>(static_cast<float>(boxImage.height()) * scaleFactor);
            // We use Lanczos3 which is the highest quality resampling method available.
            boxImage.resize(maxWidth, height, 1, 4, 6);
        }
        else {
            boxImage.resize(width, boxHeight, 1, 4, 6);
        }

        xPosBox = 0;
        yPosBox = canvasImage.height() - boxImage.height();

        // Only RGB channels for the image.
        boxImageRGB = CImg<unsigned char>(boxImage.get_shared_channels(0,2));
        // Only alpha channel for the image.
        boxImageAlpha = CImg<unsigned char>(boxImage.get_shared_channel(3));
    }

    CImg<unsigned char> frameImageAlpha(frameImage.get_shared_channel(3));
    frameImageAlpha.draw_image(xPosBox, yPosBox, boxImageAlpha);
    frameImageAlpha.draw_image(xPosMarquee, yPosMarquee, marqueeImageAlpha);

    // Set a frame color based on an average of the screenshot contents.
    unsigned char frameColor[] = { 0, 0, 0, 0 };
    sampleFrameColor(screenshotImage, frameColor);

    // Upper / lower frame.
    frameImage.draw_rectangle(
            xPosScreenshot + 2,
            yPosScreenshot - screenshotFrameWidth,
            xPosScreenshot + screenshotWidth - 2,
            yPosScreenshot + screenshotHeight + screenshotFrameWidth - 1,
            frameColor);

    // Left / right frame.
    frameImage.draw_rectangle(
            xPosScreenshot - screenshotFrameWidth,
            yPosScreenshot + 2,
            xPosScreenshot + screenshotWidth + screenshotFrameWidth - 1,
            yPosScreenshot + screenshotHeight - 2,
            frameColor);

    // We draw circles in order to get rounded corners for the frame.
    const unsigned int circleRadius = 7 * resolutionMultiplier;
    const unsigned int circleOffset = 2 * resolutionMultiplier;

    // Upper left corner.
    frameImage.draw_circle(xPosScreenshot + circleOffset, yPosScreenshot + circleOffset,
            circleRadius, frameColor);
    // Upper right corner.
    frameImage.draw_circle(xPosScreenshot + screenshotWidth - circleOffset - 1,
            yPosScreenshot + circleOffset, circleRadius, frameColor);
    // Lower right corner.
    frameImage.draw_circle(xPosScreenshot + screenshotWidth - circleOffset - 1,
            yPosScreenshot + screenshotHeight - circleOffset - 1, circleRadius, frameColor);
    // Lower left corner.
    frameImage.draw_circle(xPosScreenshot + circleOffset,
            yPosScreenshot + screenshotHeight - circleOffset - 1, circleRadius, frameColor);

    CImg<unsigned char> frameImageRGB(frameImage.get_shared_channels(0, 2));

    canvasImage.draw_image(0, 0, frameImage);
    canvasImage.draw_image(xPosScreenshot, yPosScreenshot, screenshotImage);

    if (mMarquee)
        canvasImage.draw_image(xPosMarquee, yPosMarquee, marqueeImageRGB,
                marqueeImageAlpha, 1, 255);
    if (mBox3D || mCover)
        canvasImage.draw_image(xPosBox, yPosBox, boxImageRGB, boxImageAlpha, 1, 255);

    std::vector<unsigned char> canvasVector;

    // Convert image from CImg internal format.
    convertFromCImgFormat(canvasImage, canvasVector);

    FIBITMAP* mixImage = nullptr;
    mixImage = FreeImage_ConvertFromRawBits(&canvasVector.at(0), canvasImage.width(),
            canvasImage.height(), canvasImage.width() * 4, 32,
            FI_RGBA_RED, FI_RGBA_GREEN, FI_RGBA_BLUE);

    #if defined(_WIN64)
    bool savedImage = (FreeImage_SaveU(FIF_PNG, mixImage,
            Utils::String::stringToWideString(getSavePath()).c_str()) != 0);
    #else
    bool savedImage = (FreeImage_Save(FIF_PNG, mixImage, getSavePath().c_str()) != 0);
    #endif

    if (!savedImage) {
        LOG(LogError) << "Couldn't save miximage, permission problems or disk full?";
    }

    FreeImage_Unload(screenshotFile);
    FreeImage_Unload(marqueeFile);
    FreeImage_Unload(boxFile);
    FreeImage_Unload(mixImage);

    // Success.
    if (savedImage)
        return false;
    else
        return true;
}

void MiximageGenerator::cropLetterboxes(CImg<unsigned char>& image)
{
    double pixelValueSum = 0.0l;
    int rowCounterUpper = 0;
    int rowCounterLower = 0;

    // Count the number of rows that are pure black.
    for (int i = image.height() - 1; i > 0; i--) {
        CImg<unsigned char> imageRow = image.get_rows(i, i);
        // Ignore the alpha channel.
        imageRow.channels(0, 2);
        pixelValueSum = imageRow.sum();
        if (pixelValueSum == 0.0l)
            rowCounterUpper++;
        else
            break;
    }

    for (int i = 0; i < image.height(); i++) {
        CImg<unsigned char> imageRow = image.get_rows(i, i);
        imageRow.channels(0, 2);
        pixelValueSum = imageRow.sum();
        if (pixelValueSum == 0.0l)
            rowCounterLower++;
        else
            break;
    }

    if (rowCounterUpper > 0)
        image.crop(0, 0, 0, 3, image.width() - 1, image.height() - 1 - rowCounterUpper, 0, 0);

    if (rowCounterLower > 0)
        image.crop(0, rowCounterLower, 0, 3, image.width() - 1, image.height() - 1, 0, 0);
}

void MiximageGenerator::cropPillarboxes(CImg<unsigned char>& image)
{
    double pixelValueSum = 0.0l;
    unsigned int columnCounterLeft = 0;
    unsigned int columnCounterRight = 0;

    // Count the number of columns that are pure black.
    for (int i = 0; i < image.width(); i++) {
        CImg<unsigned char> imageColumn = image.get_columns(i, i);
        // Ignore the alpha channel.
        imageColumn.channels(0, 2);
        pixelValueSum = imageColumn.sum();
        if (pixelValueSum == 0.0l)
            columnCounterLeft++;
        else
            break;
    }

    for (int i = image.width() - 1; i > 0; i--) {
        CImg<unsigned char> imageColumn = image.get_columns(i, i);
        imageColumn.channels(0, 2);
        pixelValueSum = imageColumn.sum();
        if (pixelValueSum == 0.0l)
            columnCounterRight++;
        else
            break;
    }

    if (columnCounterLeft > 0)
        image.crop(columnCounterLeft, 0, 0, 3, image.width() - 1, image.height() - 1, 0, 0);

    if (columnCounterRight > 0)
        image.crop(0, 0, 0, 3, image.width() - columnCounterRight - 1, image.height() - 1, 0, 0);
}

void MiximageGenerator::removeTransparentPadding(CImg<unsigned char>& image)
{
    if (image.spectrum() != 4)
        return;

    double pixelValueSum = 0.0l;
    int rowCounterUpper = 0;
    int rowCounterLower = 0;
    unsigned int columnCounterLeft = 0;
    unsigned int columnCounterRight = 0;

    // Count the number of rows and columns that are completely transparent.
    for (int i = image.height() - 1; i > 0; i--) {
        CImg<unsigned char> imageRow = image.get_rows(i, i);
        pixelValueSum = imageRow.get_shared_channel(3).sum();
        if (pixelValueSum == 0.0l)
            rowCounterUpper++;
        else
            break;
    }

    for (int i = 0; i < image.height(); i++) {
        CImg<unsigned char> imageRow = image.get_rows(i, i);
        pixelValueSum = imageRow.get_shared_channel(3).sum();
        if (pixelValueSum == 0.0l)
            rowCounterLower++;
        else
            break;
    }

    for (int i = 0; i < image.width(); i++) {
        CImg<unsigned char> imageColumn = image.get_columns(i, i);
        pixelValueSum = imageColumn.get_shared_channel(3).sum();
        if (pixelValueSum == 0.0l)
            columnCounterLeft++;
        else
            break;
    }

    for (int i = image.width() - 1; i > 0; i--) {
        CImg<unsigned char> imageColumn = image.get_columns(i, i);
        pixelValueSum = imageColumn.get_shared_channel(3).sum();
        if (pixelValueSum == 0.0l)
            columnCounterRight++;
        else
            break;
    }

    if (rowCounterUpper > 0)
        image.crop(0, 0, 0, 3, image.width() - 1, image.height() - 1 - rowCounterUpper, 0, 0);

    if (rowCounterLower > 0)
        image.crop(0, rowCounterLower, 0, 3, image.width() - 1, image.height() - 1, 0, 0);

    if (columnCounterLeft > 0)
        image.crop(columnCounterLeft, 0, 0, 3, image.width() - 1, image.height() - 1, 0, 0);

    if (columnCounterRight > 0)
        image.crop(0, 0, 0, 3, image.width() - columnCounterRight - 1, image.height() - 1, 0, 0);
}

void MiximageGenerator::addDropShadow(CImg<unsigned char>& image, unsigned int shadowDistance)
{
    // Make the shadow image larger than the source image to leave space for the drop shadow.
    CImg<unsigned char> shadowImage(image.width() + shadowDistance * 3,
            image.height() + shadowDistance * 3, 1, 4, 0);

    // Create a mask image.
    CImg<unsigned char> maskImage(image.width(), image.height(), 1, 4, 0);
    maskImage.draw_image(0, 0, image);
    // Fill the RGB channels with white so we end up with a simple mask.
    maskImage.get_shared_channels(0, 2).fill(255);

    // Make a black outline of the source image as a basis for the shadow.
    shadowImage.draw_image(shadowDistance, shadowDistance, image);
    shadowImage.get_shared_channels(0, 2).fill(0);
    // Lower the transparency and apply the blur.
    shadowImage.get_shared_channel(3) /= 0.6f;
    shadowImage.blur_box(static_cast<const float>(shadowDistance),
            static_cast<const float>(shadowDistance), 1, true, 2);
    shadowImage.blur(3, 0);

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

void MiximageGenerator::sampleFrameColor(CImg<unsigned char>& screenshotImage,
        unsigned char (&frameColor)[4])
{
    // Calculate the number of samples relative to the configured resolution so we get
    // the same result regardless of miximage target size seting.
    unsigned int samples = static_cast<int>(static_cast<float>(mWidth) * 0.03125f);

    unsigned int red = 0;
    unsigned int green = 0;
    unsigned int blue = 0;

    unsigned int redLine = 0;
    unsigned int greenLine = 0;
    unsigned int blueLine = 0;

    unsigned int counter = 0;

    // This is a very simple method to get an average pixel value. It's limited in that it
    // does not consider dominant colors and such, so the result could possibly be a value
    // that does not match the perceived color palette of the image. In most cases it works
    // good enough though.
    for (int r = samples / 2; r < screenshotImage.height(); r += samples) {
        for (int c = samples / 2; c < screenshotImage.width(); c += samples) {
            red += screenshotImage(c, r, 0, 0);
            green += screenshotImage(c, r, 0, 1);
            blue += screenshotImage(c, r, 0, 2);
            counter++;
        }

        if (counter > 0) {
            redLine += red / counter;
            greenLine += green / counter;
            blueLine += blue / counter;
            counter = 0;
        }
    }

    unsigned char redC = Math::clamp(static_cast<int>(redLine / 255), 0, 255);
    unsigned char greenC = Math::clamp(static_cast<int>(greenLine / 255), 0, 255);
    unsigned char blueC = Math::clamp(static_cast<int>(blueLine / 255), 0, 255);

    // Convert to the HSL color space to be able to modify saturation and lightness.
    CImg<float> colorHSL = CImg<>(1,1,1,3).fill(redC, greenC, blueC).RGBtoHSL();

    float hue = colorHSL(0, 0, 0, 0);
    float saturation = colorHSL(0, 0, 0, 1);
    float lightness = colorHSL(0, 0, 0, 2);

    // Decrease saturation slightly and increase lightness a bit, these adjustments
    // makes the end result look better than the raw average pixel value.
    colorHSL(0, 0, 0, 1)  = Math::clamp(saturation * 0.9f, 0.0f, 1.0f);
    colorHSL(0, 0, 0, 2) = Math::clamp(lightness * 1.2f, 0.0f, 1.0f);

    const CImg<unsigned char> colorRGB = colorHSL.HSLtoRGB();

    frameColor[0] = colorRGB(0, 0, 0, 0);
    frameColor[1] = colorRGB(0, 0, 0, 1);
    frameColor[2] = colorRGB(0, 0, 0, 2);
    frameColor[3] = 255;
}

void MiximageGenerator::convertToCImgFormat(CImg<unsigned char>& image,
        std::vector<unsigned char> imageVector)
{
    // CImg does not interleave the pixels as in RGBARGBARGBA so a conversion is required.
    int counter = 0;
    for (int r = 0; r < image.height(); r++) {
        for (int c = 0; c < image.width(); c++) {
            image(c, r, 0, 0) = imageVector[counter + 2];
            image(c, r, 0, 1) = imageVector[counter + 1];
            image(c, r, 0, 2) = imageVector[counter + 0];
            image(c, r, 0, 3) = imageVector[counter + 3];
            counter += 4;
        }
    }
}

void MiximageGenerator::convertFromCImgFormat(CImg<unsigned char> image,
        std::vector<unsigned char>& imageVector)
{
    for (int r = image.height() - 1; r >= 0; r--) {
        for (int c = 0; c < image.width(); c++) {
            imageVector.push_back((unsigned char)image(c,r,0,2));
            imageVector.push_back((unsigned char)image(c,r,0,1));
            imageVector.push_back((unsigned char)image(c,r,0,0));
            imageVector.push_back((unsigned char)image(c,r,0,3));
        }
    }
}

std::string MiximageGenerator::getSavePath()
{
    const std::string systemsubdirectory = mGame->getSystem()->getName();
    const std::string name = Utils::FileSystem::getStem(mGame->getPath());
    std::string subFolders;

    // Extract possible subfolders from the path.
    if (mGame->getSystemEnvData()->mStartPath != "")
        subFolders = Utils::String::replace(Utils::FileSystem::getParent(mGame->getPath()),
                mGame->getSystem()->getSystemEnvData()->mStartPath, "");

    std::string path = FileData::getMediaDirectory();

    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    path += systemsubdirectory + "/miximages" + subFolders + "/";

    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    path += name + ".png";

    // Success.
    return path;
}

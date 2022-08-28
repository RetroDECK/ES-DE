//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextureData.cpp
//
//  Low-level texture data functions.
//

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION

#include "resources/TextureData.h"

#include "ImageIO.h"
#include "Log.h"
#include "resources/ResourceManager.h"
#include "utils/StringUtil.h"

#include "nanosvg.h"
#include "nanosvgrast.h"

#include <string.h>

#define DPI 96

TextureData::TextureData(bool tile)
    : mRenderer {Renderer::getInstance()}
    , mTile {tile}
    , mTextureID {0}
    , mDataRGBA {}
    , mWidth {0}
    , mHeight {0}
    , mTileWidth {0.0f}
    , mTileHeight {0.0f}
    , mSourceWidth {0.0f}
    , mSourceHeight {0.0f}
    , mScalable {false}
    , mHasRGBAData {false}
    , mPendingRasterization {false}
    , mLinearMagnify {false}
    , mForceRasterization {false}
{
}

TextureData::~TextureData()
{
    releaseVRAM();
    releaseRAM();
}

void TextureData::initFromPath(const std::string& path)
{
    // Just set the path. It will be loaded later.
    mPath = path;
    // Only textures with paths are reloadable.
    mReloadable = true;
}

bool TextureData::initSVGFromMemory(const std::string& fileData)
{
    std::unique_lock<std::mutex> lock {mMutex};

    // If already initialized then don't process it again unless it needs to be rasterized.
    if (!mDataRGBA.empty() && !mPendingRasterization)
        return true;

    NSVGimage* svgImage {nsvgParse(const_cast<char*>(fileData.c_str()), "px", DPI)};

    if (!svgImage || svgImage->width == 0 || svgImage->height == 0) {
        LOG(LogError) << "Couldn't parse SVG image";
        return false;
    }

    bool rasterize {true};

    if (mTile) {
        if (mTileWidth == 0.0f && mTileHeight == 0.0f) {
            mSourceWidth = svgImage->width;
            mSourceHeight = svgImage->height;
        }
        else {
            mSourceWidth = static_cast<float>(mTileWidth);
            mSourceHeight = static_cast<float>(mTileHeight);
        }
    }

    // If there is no image size defined yet, then don't rasterize unless mForceRasterization has
    // been set.
    if (mSourceWidth == 0.0f && mSourceHeight == 0.0f) {
        if (!mForceRasterization)
            rasterize = false;
        // Set a small temporary size that maintains the image aspect ratio.
        mSourceWidth = 64.0f;
        mSourceHeight = 64.0f * (svgImage->height / svgImage->width);
    }

    mWidth = static_cast<int>(std::round(mSourceWidth));
    mHeight = static_cast<int>(std::round(mSourceHeight));

    if (mWidth == 0) {
        // Auto scale width to keep aspect ratio.
        mWidth = static_cast<size_t>(
            std::round((static_cast<float>(mHeight) / svgImage->height) * svgImage->width));
    }
    else if (mHeight == 0) {
        // Auto scale height to keep aspect ratio.
        mHeight = static_cast<size_t>(
            std::round((static_cast<float>(mWidth) / svgImage->width) * svgImage->height));
    }

    if (rasterize) {
        std::vector<unsigned char> tempVector;
        tempVector.reserve(mWidth * mHeight * 4);

        NSVGrasterizer* rast = nsvgCreateRasterizer();

        nsvgRasterize(rast, svgImage, 0, 0, mHeight / svgImage->height, tempVector.data(), mWidth,
                      mHeight, mWidth * 4);

        nsvgDeleteRasterizer(rast);

        mDataRGBA.insert(mDataRGBA.begin(), std::make_move_iterator(tempVector.data()),
                         std::make_move_iterator(tempVector.data() + (mWidth * mHeight * 4)));
        tempVector.erase(tempVector.begin(), tempVector.end());

        ImageIO::flipPixelsVert(mDataRGBA.data(), mWidth, mHeight);
        mPendingRasterization = false;
        mHasRGBAData = true;
    }
    else {
        // TODO: Fix this properly instead of using the single byte texture workaround.
        mDataRGBA.push_back(0);
        mPendingRasterization = true;
    }

    nsvgDelete(svgImage);

    return true;
}

bool TextureData::initImageFromMemory(const unsigned char* fileData, size_t length)
{
    // If already initialized then don't process it again.
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mDataRGBA.empty())
            return true;
    }

    size_t width;
    size_t height;

    std::vector<unsigned char> imageRGBA = ImageIO::loadFromMemoryRGBA32(
        static_cast<const unsigned char*>(fileData), length, width, height);

    if (imageRGBA.size() == 0) {
        LOG(LogError) << "Couldn't initialize texture from memory, invalid data ("
                      << (mPath != "" ? "file path: \"" + mPath + "\", " : "")
                      << "data ptr: " << reinterpret_cast<size_t>(fileData)
                      << ", reported size: " << length << ")";
        return false;
    }

    mSourceWidth = static_cast<float>(width);
    mSourceHeight = static_cast<float>(height);
    mScalable = false;

    return initFromRGBA(imageRGBA.data(), width, height);
}

bool TextureData::initFromRGBA(const unsigned char* dataRGBA, size_t width, size_t height)
{
    // If already initialized then don't process it again.
    std::unique_lock<std::mutex> lock(mMutex);
    if (!mDataRGBA.empty())
        return true;

    mDataRGBA.reserve(width * height * 4);
    mDataRGBA.insert(mDataRGBA.begin(), std::make_move_iterator(dataRGBA),
                     std::make_move_iterator(dataRGBA + (width * height * 4)));

    mWidth = static_cast<int>(width);
    mHeight = static_cast<int>(height);
    mHasRGBAData = true;

    return true;
}

bool TextureData::load()
{
    bool retval = false;

    // Need to load. See if there is a file.
    if (!mPath.empty()) {
        const ResourceData& data = ResourceManager::getInstance().getFileData(mPath);
        // Is it an SVG?
        if (Utils::String::toLower(mPath.substr(mPath.size() - 4, std::string::npos)) == ".svg") {
            mScalable = true;
            std::string dataString;
            dataString.assign(std::string(reinterpret_cast<char*>(data.ptr.get()), data.length));
            retval = initSVGFromMemory(dataString);
        }
        else {
            retval =
                initImageFromMemory(static_cast<const unsigned char*>(data.ptr.get()), data.length);
        }
    }
    return retval;
}

bool TextureData::isLoaded()
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (!mDataRGBA.empty() || mTextureID != 0)
        if (mHasRGBAData || mPendingRasterization || mTextureID != 0)
            return true;

    return false;
}

bool TextureData::uploadAndBind()
{
    // Check if it has already been uploaded.
    std::unique_lock<std::mutex> lock(mMutex);
    if (mTextureID != 0) {
        mRenderer->bindTexture(mTextureID);
    }
    else {
        // Make sure we're ready to upload.
        if (mWidth == 0 || mHeight == 0 || mDataRGBA.empty())
            return false;

        // Upload texture.
        mTextureID =
            mRenderer->createTexture(Renderer::TextureType::RGBA, true, mLinearMagnify, mTile,
                                     static_cast<const unsigned int>(mWidth),
                                     static_cast<const unsigned int>(mHeight), mDataRGBA.data());
    }
    return true;
}

void TextureData::releaseVRAM()
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mTextureID != 0) {
        mRenderer->destroyTexture(mTextureID);
        mTextureID = 0;
    }
}

void TextureData::releaseRAM()
{
    std::unique_lock<std::mutex> lock(mMutex);
    std::vector<unsigned char> swapVector;
    if (!mDataRGBA.empty()) {
        mDataRGBA.clear();
        mDataRGBA.swap(swapVector);
        mHasRGBAData = false;
    }
}

size_t TextureData::width()
{
    if (mWidth == 0) {
        load();
    }
    return static_cast<size_t>(mWidth);
}

size_t TextureData::height()
{
    if (mHeight == 0) {
        load();
    }
    return static_cast<size_t>(mHeight);
}

float TextureData::sourceWidth()
{
    if (mSourceWidth == 0) {
        load();
    }
    return mSourceWidth;
}

float TextureData::sourceHeight()
{
    if (mSourceHeight == 0) {
        load();
    }
    return mSourceHeight;
}

void TextureData::setSourceSize(float width, float height)
{
    if (mScalable) {
        if (mSourceWidth != width || mSourceHeight != height) {
            mSourceWidth = width;
            mSourceHeight = height;
            releaseVRAM();
            releaseRAM();
        }
    }
}

size_t TextureData::getVRAMUsage()
{
    if (mHasRGBAData || mTextureID != 0)
        return mWidth * mHeight * 4;
    else
        return 0;
}

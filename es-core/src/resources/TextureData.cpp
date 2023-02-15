//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextureData.cpp
//
//  Low-level texture data functions.
//

#include "resources/TextureData.h"

#include "ImageIO.h"
#include "Log.h"
#include "resources/ResourceManager.h"
#include "utils/StringUtil.h"

#include "lunasvg.h"

#include <string.h>

TextureData::TextureData(bool tile)
    : mRenderer {Renderer::getInstance()}
    , mTile {tile}
    , mTextureID {0}
    , mWidth {0}
    , mHeight {0}
    , mTileWidth {0.0f}
    , mTileHeight {0.0f}
    , mSourceWidth {0.0f}
    , mSourceHeight {0.0f}
    , mScalable {false}
    , mHasRGBAData {false}
    , mPendingRasterization {false}
    , mMipmapping {false}
    , mInvalidSVGFile {false}
    , mLinearMagnify {false}
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

    auto svgImage = lunasvg::Document::loadFromData(fileData);

    if (svgImage == nullptr) {
        LOG(LogError) << "TextureData::initSVGFromMemory(): Couldn't parse SVG image \"" << mPath
                      << "\"";
        mInvalidSVGFile = true;
        return false;
    }

    float svgWidth {static_cast<float>(svgImage->width())};
    float svgHeight {static_cast<float>(svgImage->height())};
    bool rasterize {true};

    if (mTile) {
        if (mTileWidth == 0.0f && mTileHeight == 0.0f) {
            rasterize = false;
            mSourceWidth = svgWidth;
            mSourceHeight = svgHeight;
        }
        else {
            mSourceWidth = static_cast<float>(mTileWidth);
            mSourceHeight = static_cast<float>(mTileHeight);
        }
    }

    // If there is no image size defined yet, then don't rasterize.
    if (mSourceWidth == 0.0f && mSourceHeight == 0.0f) {
        rasterize = false;
        // Set a small temporary size that maintains the image aspect ratio.
        mSourceWidth = 64.0f;
        mSourceHeight = 64.0f * (svgHeight / svgWidth);
    }

    mWidth = static_cast<int>(std::round(mSourceWidth));
    mHeight = static_cast<int>(std::round(mSourceHeight));

    if (mWidth == 0) {
        // Auto scale width to keep aspect ratio.
        mWidth =
            static_cast<size_t>(std::round((static_cast<float>(mHeight) / svgHeight) * svgWidth));
    }
    else if (mHeight == 0) {
        // Auto scale height to keep aspect ratio.
        mHeight =
            static_cast<size_t>(std::round((static_cast<float>(mWidth) / svgWidth) * svgHeight));
    }

    if (rasterize) {
        auto bitmap = svgImage->renderToBitmap(mWidth, mHeight);
        mDataRGBA.insert(mDataRGBA.begin(), std::move(bitmap.data()),
                         std::move(bitmap.data() + mWidth * mHeight * 4));

        ImageIO::flipPixelsVert(mDataRGBA.data(), mWidth, mHeight);
        mPendingRasterization = false;
        mHasRGBAData = true;
    }
    else {
        // TODO: Fix this properly instead of using the single byte texture workaround.
        mDataRGBA.push_back(0);
        mPendingRasterization = true;
    }

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

    std::vector<unsigned char> imageRGBA {ImageIO::loadFromMemoryRGBA32(
        static_cast<const unsigned char*>(fileData), length, width, height)};

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
    std::unique_lock<std::mutex> lock {mMutex};
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
    if (mInvalidSVGFile)
        return false;

    bool retval {false};

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
    std::unique_lock<std::mutex> lock {mMutex};
    if (!mDataRGBA.empty() || mTextureID != 0)
        if (mHasRGBAData || mPendingRasterization || mTextureID != 0)
            return true;

    return false;
}

bool TextureData::uploadAndBind()
{
    // Check if it has already been uploaded.
    std::unique_lock<std::mutex> lock {mMutex};
    if (mTextureID != 0) {
        mRenderer->bindTexture(mTextureID);
    }
    else {
        // Make sure we're ready to upload.
        if (mWidth == 0 || mHeight == 0 || mDataRGBA.empty())
            return false;

        // Upload texture.
        mTextureID =
            mRenderer->createTexture(Renderer::TextureType::BGRA, true, mLinearMagnify, mMipmapping,
                                     mTile, static_cast<const unsigned int>(mWidth),
                                     static_cast<const unsigned int>(mHeight), mDataRGBA.data());
    }
    return true;
}

void TextureData::releaseVRAM()
{
    std::unique_lock<std::mutex> lock {mMutex};
    if (mTextureID != 0) {
        mRenderer->destroyTexture(mTextureID);
        mTextureID = 0;
    }
}

void TextureData::releaseRAM()
{
    std::unique_lock<std::mutex> lock {mMutex};
    if (!mDataRGBA.empty()) {
        std::vector<unsigned char> swapVector;
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
        // Ugly hack to make sure SVG images matching the temporary size 64x64 get rasterized.
        const bool tempSizeMatch {mPendingRasterization && width == 64 && height == 64};
        if (tempSizeMatch || mSourceWidth != width || mSourceHeight != height) {
            mSourceWidth = width;
            mSourceHeight = height;
            releaseVRAM();
            releaseRAM();
        }
    }
}

size_t TextureData::getVRAMUsage()
{
    if (mHasRGBAData || mTextureID != 0) {
        // The estimated increase in VRAM usage with mipmapping enabled is 33%
        if (mMipmapping)
            return static_cast<size_t>(static_cast<float>(mWidth * mHeight * 4) * 1.33f);
        else
            return mWidth * mHeight * 4;
    }
    else {
        return 0;
    }
}

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
#include "renderers/Renderer.h"
#include "resources/ResourceManager.h"

#include "nanosvg.h"
#include "nanosvgrast.h"

#include <string.h>

#define DPI 96

TextureData::TextureData(bool tile)
    : mTile(tile)
    , mTextureID(0)
    , mDataRGBA({})
    , mWidth(0)
    , mHeight(0)
    , mSourceWidth(0.0f)
    , mSourceHeight(0.0f)
    , mScaleDuringLoad(1.0f)
    , mScalable(false)
    , mLinearMagnify(false)
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
    // If already initialized then don't process it again.
    std::unique_lock<std::mutex> lock(mMutex);

    if (!mDataRGBA.empty())
        return true;

    NSVGimage* svgImage = nsvgParse(const_cast<char*>(fileData.c_str()), "px", DPI);

    if (!svgImage) {
        LOG(LogError) << "Couldn't parse SVG image";
        return false;
    }

    // We want to rasterize this texture at a specific resolution. If the source size
    // variables are set then use them, otherwise get them from the parsed file.
    if ((mSourceWidth == 0.0f) && (mSourceHeight == 0.0f)) {
        mSourceWidth = svgImage->width;
        mSourceHeight = svgImage->height;
    }

    mWidth = static_cast<size_t>(floorf(floorf(mSourceWidth) * mScaleDuringLoad));
    mHeight = static_cast<size_t>(floorf(floorf(mSourceHeight) * mScaleDuringLoad));

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

    std::vector<unsigned char> tempVector;
    tempVector.reserve(mWidth * mHeight * 4);

    NSVGrasterizer* rast = nsvgCreateRasterizer();

    nsvgRasterize(rast, svgImage, 0, 0, mHeight / svgImage->height, tempVector.data(),
                  static_cast<int>(mWidth), static_cast<int>(mHeight),
                  static_cast<int>(mWidth) * 4);

    // This is important in order to avoid memory leaks.
    nsvgDeleteRasterizer(rast);
    nsvgDelete(svgImage);

    mDataRGBA.insert(mDataRGBA.begin(), tempVector.data(),
                     tempVector.data() + (mWidth * mHeight * 4));

    ImageIO::flipPixelsVert(mDataRGBA.data(), mWidth, mHeight);

    return true;
}

bool TextureData::initImageFromMemory(const unsigned char* fileData, size_t length)
{
    size_t width;
    size_t height;

    // If already initialized then don't process it again.
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mDataRGBA.empty())
            return true;
    }

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
    mDataRGBA.insert(mDataRGBA.begin(), dataRGBA, dataRGBA + (width * height * 4));

    mWidth = width;
    mHeight = height;
    return true;
}

bool TextureData::load()
{
    bool retval = false;

    // Need to load. See if there is a file.
    if (!mPath.empty()) {
        std::shared_ptr<ResourceManager>& rm = ResourceManager::getInstance();
        const ResourceData& data = rm->getFileData(mPath);
        // Is it an SVG?
        if (mPath.substr(mPath.size() - 4, std::string::npos) == ".svg") {
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
        return true;

    return false;
}

bool TextureData::uploadAndBind()
{
    // Check if it has already been uploaded.
    std::unique_lock<std::mutex> lock(mMutex);
    if (mTextureID != 0) {
        Renderer::bindTexture(mTextureID);
    }
    else {
        // Make sure we're ready to upload.
        if (mWidth == 0 || mHeight == 0 || mDataRGBA.empty())
            return false;

        // Upload texture.
        mTextureID =
            Renderer::createTexture(Renderer::Texture::RGBA, true, mLinearMagnify, mTile,
                                    static_cast<const unsigned int>(mWidth),
                                    static_cast<const unsigned int>(mHeight), mDataRGBA.data());
    }
    return true;
}

void TextureData::releaseVRAM()
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mTextureID != 0) {
        Renderer::destroyTexture(mTextureID);
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
    }
}

size_t TextureData::width()
{
    if (mWidth == 0)
        load();
    // If it's an SVG image, the size was correctly set to the scaled-up values during the
    // rasterization, so only multiply by the scale factor if it's a raster file.
    if (!mScalable)
        return static_cast<size_t>(mWidth * mScaleDuringLoad);
    else
        return mWidth;
}

size_t TextureData::height()
{
    if (mHeight == 0)
        load();
    if (!mScalable)
        return static_cast<size_t>(mHeight * mScaleDuringLoad);
    else
        return mHeight;
}

float TextureData::sourceWidth()
{
    if (mSourceWidth == 0)
        load();
    return mSourceWidth;
}

float TextureData::sourceHeight()
{
    if (mSourceHeight == 0)
        load();
    return mSourceHeight;
}

void TextureData::setSourceSize(float width, float height)
{
    if (mScalable) {
        if ((mSourceWidth != width) || (mSourceHeight != height)) {
            mSourceWidth = width;
            mSourceHeight = height;
            releaseVRAM();
            releaseRAM();
        }
    }
}

size_t TextureData::getVRAMUsage()
{
    if (mTextureID != 0 || !mDataRGBA.empty())
        return mWidth * mHeight * 4;
    else
        return 0;
}

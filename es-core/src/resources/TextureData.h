//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextureData.h
//
//  Low-level texture data functions.
//

#ifndef ES_CORE_RESOURCES_TEXTURE_DATA_H
#define ES_CORE_RESOURCES_TEXTURE_DATA_H

#include "renderers/Renderer.h"
#include "utils/MathUtil.h"

#include <atomic>
#include <cmath>
#include <mutex>
#include <string>
#include <vector>

class TextureResource;

class TextureData
{
public:
    TextureData(bool tile);
    ~TextureData();

    // These functions populate mDataRGBA but do not upload the texture to VRAM.

    // Needs to be canonical path. Caller should check for duplicates before calling this.
    void initFromPath(const std::string& path);
    bool initSVGFromMemory(const std::string& fileData);
    bool initImageFromMemory(const unsigned char* fileData, size_t length);
    bool initFromRGBA(const unsigned char* dataRGBA, size_t width, size_t height);

    // Read the data into memory if necessary.
    bool load();

    bool isLoaded();

    // Upload the texture to VRAM if necessary and bind.
    // Returns true if bound correctly.
    bool uploadAndBind();

    // Release the texture from VRAM.
    void releaseVRAM();

    // Release the texture from conventional RAM.
    void releaseRAM();

    // Get the amount of VRAM currenty used by this texture.
    size_t getVRAMUsage();

    size_t width();
    size_t height();
    float sourceWidth();
    float sourceHeight();
    void setSourceSize(float width, float height);
    void setTileSize(float tileWidth, float tileHeight)
    {
        mTileWidth = tileWidth;
        mTileHeight = tileHeight;
    }
    glm::vec2 getSize() { return glm::vec2 {static_cast<int>(mWidth), static_cast<int>(mHeight)}; }

    // Whether to use linear filtering when magnifying the texture.
    void setLinearMagnify(bool state) { mLinearMagnify = state; }
    // Whether to use mipmapping and trilinear filtering.
    void setMipmapping(bool state) { mMipmapping = state; }

    // Has the image been loaded but not yet been rasterized as the size was not known?
    const bool getPendingRasterization() { return mPendingRasterization; }

    const bool getScalable() { return mScalable; }
    const std::vector<unsigned char>& getRawRGBAData() { return mDataRGBA; }
    const std::string& getTextureFilePath() { return mPath; }
    const bool getTiled() { return mTile; }
    const bool getIsInvalidSVGFile() { return mInvalidSVGFile; }

private:
    Renderer* mRenderer;
    std::mutex mMutex;

    bool mTile;
    std::string mPath;
    std::atomic<unsigned int> mTextureID;
    std::vector<unsigned char> mDataRGBA;
    std::atomic<int> mWidth;
    std::atomic<int> mHeight;
    std::atomic<float> mTileWidth;
    std::atomic<float> mTileHeight;
    std::atomic<float> mSourceWidth;
    std::atomic<float> mSourceHeight;
    std::atomic<bool> mScalable;
    std::atomic<bool> mHasRGBAData;
    std::atomic<bool> mPendingRasterization;
    std::atomic<bool> mMipmapping;
    std::atomic<bool> mInvalidSVGFile;
    bool mLinearMagnify;
    bool mReloadable;
};

#endif // ES_CORE_RESOURCES_TEXTURE_DATA_H

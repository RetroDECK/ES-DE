//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextureResource.h
//
//  Handles textures including loading, unloading and cache management.
//

#ifndef ES_CORE_RESOURCES_TEXTURE_RESOURCE_H
#define ES_CORE_RESOURCES_TEXTURE_RESOURCE_H

#include "resources/ResourceManager.h"
#include "resources/TextureData.h"
#include "resources/TextureDataManager.h"
#include "utils/MathUtil.h"

#include <cmath>
#include <set>
#include <string>
#include <vector>

class TextureData;

// An OpenGL texture.
// Automatically recreates the texture with renderer deinit/reinit.
class TextureResource : public IReloadable
{
public:
    static std::shared_ptr<TextureResource> get(const std::string& path,
                                                bool tile = false,
                                                bool forceLoad = false,
                                                bool dynamic = true,
                                                bool linearMagnify = false,
                                                bool mipmapping = false,
                                                size_t width = 0,
                                                size_t height = 0,
                                                float tileWidth = 0.0f,
                                                float tileHeight = 0.0f);
    void initFromPixels(const unsigned char* dataRGBA, size_t width, size_t height);
    virtual void initFromMemory(const char* data, size_t length);
    static void manualUnload(const std::string& path, bool tile);
    static void manualUnloadAll() { sTextureMap.clear(); }

    // Returns the raw pixel values.
    std::vector<unsigned char> getRawRGBAData();

    // Has the image been loaded but not yet been rasterized as the size was not known?
    const bool getPendingRasterization() const
    {
        return (mTextureData != nullptr ? mTextureData->getPendingRasterization() : false);
    }

    const bool getScalable() const
    {
        return (mTextureData != nullptr ? mTextureData->getScalable() : false);
    }

    void setLinearMagnify(bool state) { mTextureData->setLinearMagnify(state); }

    std::string getTextureFilePath();

    void rasterizeAt(float width, float height);
    glm::vec2 getSourceImageSize() const { return mSourceSize; }
    const bool getIsInvalidSVGFile() const { return mInvalidSVGFile; }

    virtual ~TextureResource();

    bool isTiled() const;
    void setSize(float width, float height)
    {
        mSize.x = static_cast<int>(std::round(width));
        mSize.y = static_cast<int>(std::round(height));
    }

    const glm::ivec2 getSize() const { return mSize; }
    bool bind();

    // Returns an approximation of total VRAM used by textures (in bytes).
    static size_t getTotalMemUsage();
    // Returns the number of bytes that would be used if all textures were in memory.
    static size_t getTotalTextureSize();

protected:
    TextureResource(const std::string& path,
                    float tileWidth,
                    float tileHeight,
                    bool tile,
                    bool dynamic,
                    bool linearMagnify,
                    bool mipmapping,
                    bool scalable);
    virtual void unload(ResourceManager& rm);
    virtual void reload(ResourceManager& rm);

private:
    // mTextureData is used for textures that are not loaded from a file - these ones
    // are permanently allocated and cannot be loaded and unloaded based on resources.
    std::shared_ptr<TextureData> mTextureData;

    // The texture data manager manages loading and unloading of filesystem based textures.
    static inline TextureDataManager sTextureDataManager;

    glm::ivec2 mSize;
    glm::vec2 mSourceSize;
    bool mInvalidSVGFile;
    bool mForceLoad;

    // File path, tile, linear interpolation, mipmapping, scalable/SVG, width, height.
    using TextureKeyType = std::tuple<std::string, bool, bool, bool, bool, size_t, size_t>;
    // Map of textures, used to prevent duplicate textures.
    static inline std::map<TextureKeyType, std::weak_ptr<TextureResource>> sTextureMap;
    // Set of all textures, used for memory management.
    static inline std::set<TextureResource*> sAllTextures;
};

#endif // ES_CORE_RESOURCES_TEXTURE_RESOURCE_H

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextureResource.h
//
//  Handles OpenGL textures.
//

#ifndef ES_CORE_RESOURCES_TEXTURE_RESOURCE_H
#define ES_CORE_RESOURCES_TEXTURE_RESOURCE_H

#include "resources/ResourceManager.h"
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
                                                float scaleDuringLoad = 1.0f);
    void initFromPixels(const unsigned char* dataRGBA, size_t width, size_t height);
    virtual void initFromMemory(const char* data, size_t length);
    static void manualUnload(std::string path, bool tile);

    // Returns the raw pixel values.
    std::vector<unsigned char> getRawRGBAData();

    std::string getTextureFilePath();

    // For SVG graphics this function effectively rescales the image to the defined size.
    // It does unload and re-rasterize the texture though which may cause flickering in some
    // situations. An alternative is to set a scaling factor directly when loading the texture
    // using get(), by using the scaleDuringLoad parameter (which also works for raster graphics).
    void rasterizeAt(size_t width, size_t height);
    glm::vec2 getSourceImageSize() const { return mSourceSize; }

    virtual ~TextureResource() noexcept;

    bool isInitialized() const { return true; }
    bool isTiled() const;

    const glm::ivec2 getSize() const { return mSize; }
    bool bind();

    // Returns an approximation of total VRAM used by textures (in bytes).
    static size_t getTotalMemUsage();
    // Returns the number of bytes that would be used if all textures were in memory.
    static size_t getTotalTextureSize();

protected:
    TextureResource(const std::string& path,
                    bool tile,
                    bool dynamic,
                    bool linearMagnify,
                    float scaleDuringLoad);
    virtual void unload(std::shared_ptr<ResourceManager>& rm);
    virtual void reload(std::shared_ptr<ResourceManager>& rm);

private:
    // mTextureData is used for textures that are not loaded from a file - these ones
    // are permanently allocated and cannot be loaded and unloaded based on resources.
    std::shared_ptr<TextureData> mTextureData;

    // The texture data manager manages loading and unloading of filesystem based textures.
    static TextureDataManager sTextureDataManager;

    glm::ivec2 mSize;
    glm::vec2 mSourceSize;
    bool mForceLoad;

    typedef std::pair<std::string, bool> TextureKeyType;
    // Map of textures, used to prevent duplicate textures.
    static std::map<TextureKeyType, std::weak_ptr<TextureResource>> sTextureMap;
    // Set of all textures, used for memory management.
    static std::set<TextureResource*> sAllTextures;
};

#endif // ES_CORE_RESOURCES_TEXTURE_RESOURCE_H

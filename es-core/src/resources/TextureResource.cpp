//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextureResource.cpp
//
//  Handles textures including loading, unloading and cache management.
//

#include "resources/TextureResource.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#define DEBUG_RASTER_CACHING false
#define DEBUG_SVG_CACHING false

TextureResource::TextureResource(const std::string& path,
                                 float tileWidth,
                                 float tileHeight,
                                 bool tile,
                                 bool dynamic,
                                 bool linearMagnify,
                                 bool mipmapping,
                                 bool scalable)
    : mTextureData {nullptr}
    , mInvalidSVGFile {false}
    , mForceLoad {false}
{
    // Create a texture data object for this texture.
    if (!path.empty()) {
        // If there is a path then the 'dynamic' flag tells us whether to use the texture
        // data manager to manage loading/unloading of this texture.
        std::shared_ptr<TextureData> data;
        if (dynamic) {
            data = sTextureDataManager.add(this, tile);
            data->initFromPath(path);
            data->setTileSize(tileWidth, tileHeight);
            data->setLinearMagnify(linearMagnify);
            data->setMipmapping(mipmapping);
            // Force the texture manager to load it using a blocking load.
            sTextureDataManager.load(data, true);
            if (scalable)
                mInvalidSVGFile = data->getIsInvalidSVGFile();
        }
        else {
            mTextureData = std::shared_ptr<TextureData>(new TextureData(tile));
            data = mTextureData;
            data->initFromPath(path);
            data->setTileSize(tileWidth, tileHeight);
            data->setLinearMagnify(linearMagnify);
            data->setMipmapping(mipmapping);
            // Load it so we can read the width/height.
            data->load();
            if (scalable)
                mInvalidSVGFile = data->getIsInvalidSVGFile();
        }

        mSize = glm::ivec2 {static_cast<int>(data->width()), static_cast<int>(data->height())};
        mSourceSize = glm::vec2 {data->sourceWidth(), data->sourceHeight()};
    }
    else {
        // Create a texture managed by this class because it cannot be dynamically
        // loaded and unloaded. This would normally be a video texture, where the player
        // reserves a texture to later be used for the video rendering.
        mTextureData = std::shared_ptr<TextureData>(new TextureData(tile));
        mSize = glm::ivec2 {0.0f, 0.0f};
    }
    sAllTextures.insert(this);
}

TextureResource::~TextureResource()
{
    if (mTextureData == nullptr)
        sTextureDataManager.remove(this);

    sAllTextures.erase(sAllTextures.find(this));
}

void TextureResource::initFromPixels(const unsigned char* dataRGBA, size_t width, size_t height)
{
    // This is only valid if we have a local texture data object.
    assert(mTextureData != nullptr);
    mTextureData->releaseVRAM();
    mTextureData->releaseRAM();
    mTextureData->initFromRGBA(dataRGBA, width, height);
    // Cache the image dimensions.
    mSize = glm::ivec2 {static_cast<int>(width), static_cast<int>(height)};
    mSourceSize = glm::vec2 {mTextureData->sourceWidth(), mTextureData->sourceHeight()};
}

void TextureResource::initFromMemory(const char* data, size_t length)
{
    // This is only valid if we have a local texture data object.
    assert(mTextureData != nullptr);
    mTextureData->releaseVRAM();
    mTextureData->releaseRAM();
    mTextureData->initImageFromMemory(reinterpret_cast<const unsigned char*>(data), length);
    // Get the size from the texture data.
    mSize = glm::ivec2 {static_cast<int>(mTextureData->width()),
                        static_cast<int>(mTextureData->height())};
    mSourceSize = glm::vec2 {mTextureData->sourceWidth(), mTextureData->sourceHeight()};
}

void TextureResource::manualUnload(const std::string& path, bool tile)
{
    const std::string canonicalPath {Utils::FileSystem::getCanonicalPath(path)};

    // TODO: We always attempt to unload both the linear and nearest interpolation entries.
    // Rewrite this to only unload the requested image.
    for (auto it = sTextureMap.begin(); it != sTextureMap.end();) {
        if (std::get<0>((*it).first) == canonicalPath && std::get<1>((*it).first) == tile) {
            sTextureMap.erase(it++);
        }
        else {
            ++it;
        }
    }
}

std::vector<unsigned char> TextureResource::getRawRGBAData()
{
    std::shared_ptr<TextureData> data {sTextureDataManager.get(this)};

    if (data)
        return data.get()->getRawRGBAData();
    else
        return std::vector<unsigned char>(0);
}

std::string TextureResource::getTextureFilePath()
{
    std::shared_ptr<TextureData> data {sTextureDataManager.get(this)};

    if (data)
        return data->getTextureFilePath();
    else
        return "";
}

bool TextureResource::isTiled() const
{
    if (mTextureData != nullptr)
        return mTextureData->getTiled();
    std::shared_ptr<TextureData> data {sTextureDataManager.get(this)};
    return data->getTiled();
}

bool TextureResource::bind()
{
    if (mTextureData != nullptr) {
        mTextureData->uploadAndBind();
        return true;
    }
    else {
        return sTextureDataManager.bind(this);
    }
}

std::shared_ptr<TextureResource> TextureResource::get(const std::string& path,
                                                      bool tile,
                                                      bool forceLoad,
                                                      bool dynamic,
                                                      bool linearMagnify,
                                                      bool mipmapping,
                                                      size_t width,
                                                      size_t height,
                                                      float tileWidth,
                                                      float tileHeight)
{
    const std::string canonicalPath {Utils::FileSystem::getCanonicalPath(path)};
    if (canonicalPath.empty()) {
        std::shared_ptr<TextureResource> tex(new TextureResource(
            "", tileWidth, tileHeight, tile, false, linearMagnify, mipmapping, false));
        // Make sure we get properly deinitialized even though we do nothing on reinitialization.
        ResourceManager::getInstance().addReloadable(tex);
        return tex;
    }

    const bool isScalable {Utils::String::toLower(canonicalPath.substr(
                               canonicalPath.size() - 4, std::string::npos)) == ".svg"};

    TextureKeyType key {canonicalPath, tile, linearMagnify, mipmapping, isScalable, width, height};
    auto foundTexture = sTextureMap.find(key);

    std::string resolutionInfo;

    if (DEBUG_SVG_CACHING && isScalable) {
        resolutionInfo.append(" (resolution ")
            .append(std::to_string(width))
            .append("x")
            .append(std::to_string(height))
            .append(")");
    }

    if (foundTexture != sTextureMap.cend()) {
        if (!foundTexture->second.expired()) {
            if ((DEBUG_SVG_CACHING && isScalable) || (DEBUG_RASTER_CACHING && !isScalable)) {
                LOG(LogDebug) << "TextureResource::get(): Cache hit for "
                              << (isScalable ? "SVG" : "raster") << " image \"" << canonicalPath
                              << "\"" << resolutionInfo;
            }
            return foundTexture->second.lock();
        }
        else if ((DEBUG_SVG_CACHING && isScalable) || (DEBUG_RASTER_CACHING && !isScalable)) {
            LOG(LogDebug) << "TextureResource::get(): Cache expired for "
                          << (isScalable ? "SVG" : "raster") << " image \"" << canonicalPath << "\""
                          << resolutionInfo;
        }
    }
    else if ((DEBUG_SVG_CACHING && isScalable && width != 0.0f && height != 0.0f) ||
             (DEBUG_RASTER_CACHING && !isScalable)) {
        LOG(LogDebug) << "TextureResource::get(): Cache miss for "
                      << (isScalable ? "SVG" : "raster") << " image \"" << canonicalPath << "\""
                      << resolutionInfo;
    }

    // Need to create it.
    std::shared_ptr<TextureResource> tex {std::shared_ptr<TextureResource>(
        new TextureResource(std::get<0>(key), tileWidth, tileHeight, tile, dynamic, linearMagnify,
                            mipmapping, isScalable))};
    std::shared_ptr<TextureData> data {sTextureDataManager.get(tex.get())};

    if (!isScalable || (isScalable && width != 0.0f && height != 0.0f)) {
        if ((DEBUG_SVG_CACHING && isScalable) || (DEBUG_RASTER_CACHING && !isScalable)) {
            LOG(LogDebug) << "TextureResource::get(): Adding " << (isScalable ? "SVG" : "raster")
                          << " image to cache: \"" << canonicalPath << "\"" << resolutionInfo;
        }
        sTextureMap[key] = std::weak_ptr<TextureResource>(tex);
    }

    // Add it to the reloadable list.
    ResourceManager::getInstance().addReloadable(tex);

    // Force load it if necessary. Note that it may get dumped from VRAM if we run low.
    if (forceLoad) {
        tex->mForceLoad = forceLoad;
        data->load();
    }

    return tex;
}

void TextureResource::rasterizeAt(float width, float height)
{
    if (mTextureData != nullptr) {
        glm::vec2 textureSize {mTextureData.get()->getSize()};
        if (textureSize.x == width && textureSize.y == height &&
            !mTextureData.get()->getPendingRasterization())
            return;
    }

    std::shared_ptr<TextureData> data;
    if (mTextureData != nullptr)
        data = mTextureData;
    else
        data = sTextureDataManager.get(this);

    if (mTextureData && mTextureData.get()->getScalable())
        mSourceSize = glm::vec2 {static_cast<float>(width), static_cast<float>(height)};
    data->setSourceSize(static_cast<float>(width), static_cast<float>(height));
    if (mForceLoad || mTextureData != nullptr)
        data->load();
}

size_t TextureResource::getTotalMemUsage()
{
    size_t total {0};
    // Count up all textures that manage their own texture data.
    for (auto tex : sAllTextures) {
        if (tex->mTextureData != nullptr)
            total += tex->mTextureData->getVRAMUsage();
    }
    // Now get the committed memory from the manager.
    total += sTextureDataManager.getCommittedSize();
    // And the size of the loading queue.
    total += sTextureDataManager.getQueueSize();
    return total;
}

size_t TextureResource::getTotalTextureSize()
{
    size_t total {0};
    // Count up all textures that manage their own texture data.
    for (auto tex : sAllTextures) {
        if (tex->mTextureData != nullptr)
            total += tex->getSize().x * tex->getSize().y * 4;
    }
    // Now get the total memory from the manager.
    total += sTextureDataManager.getTotalSize();
    return total;
}

void TextureResource::unload(ResourceManager& /*rm*/)
{
    // Release the texture's resources.
    std::shared_ptr<TextureData> data;
    if (mTextureData == nullptr)
        data = sTextureDataManager.get(this);
    else
        data = mTextureData;

    data->releaseVRAM();
    data->releaseRAM();
}

void TextureResource::reload(ResourceManager& /*rm*/)
{
    // For dynamically loaded textures the texture manager will load them on demand.
    // For manually loaded textures we have to reload them here.
    if (mTextureData)
        mTextureData->load();
}

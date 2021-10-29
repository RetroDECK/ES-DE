//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextureResource.cpp
//
//  Handles OpenGL textures.
//

#include "resources/TextureResource.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

TextureDataManager TextureResource::sTextureDataManager;
std::map<TextureResource::TextureKeyType, std::weak_ptr<TextureResource>>
    TextureResource::sTextureMap;
std::set<TextureResource*> TextureResource::sAllTextures;

TextureResource::TextureResource(
    const std::string& path, bool tile, bool dynamic, bool linearMagnify, bool forceRasterization)
    : mTextureData(nullptr)
    , mForceLoad(false)
{
    // Create a texture data object for this texture.
    if (!path.empty()) {
        // If there is a path then the 'dynamic' flag tells us whether to use the texture
        // data manager to manage loading/unloading of this texture.
        std::shared_ptr<TextureData> data;
        if (dynamic) {
            data = sTextureDataManager.add(this, tile);
            data->initFromPath(path);
            data->setLinearMagnify(linearMagnify);
            data->setForceRasterization(forceRasterization);
            // Force the texture manager to load it using a blocking load.
            sTextureDataManager.load(data, true);
        }
        else {
            mTextureData = std::shared_ptr<TextureData>(new TextureData(tile));
            data = mTextureData;
            data->initFromPath(path);
            data->setLinearMagnify(linearMagnify);
            data->setForceRasterization(forceRasterization);
            // Load it so we can read the width/height.
            data->load();
        }

        mSize = glm::ivec2{static_cast<int>(data->width()), static_cast<int>(data->height())};
        mSourceSize = glm::vec2{data->sourceWidth(), data->sourceHeight()};
    }
    else {
        // Create a texture managed by this class because it cannot be dynamically
        // loaded and unloaded. This would normally be a video texture, where the player
        // reserves a texture to later be used for the video rendering.
        mTextureData = std::shared_ptr<TextureData>(new TextureData(tile));
        mSize = glm::ivec2{};
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
    mSize = glm::ivec2{static_cast<int>(width), static_cast<int>(height)};
    mSourceSize = glm::vec2{mTextureData->sourceWidth(), mTextureData->sourceHeight()};
}

void TextureResource::initFromMemory(const char* data, size_t length)
{
    // This is only valid if we have a local texture data object.
    assert(mTextureData != nullptr);
    mTextureData->releaseVRAM();
    mTextureData->releaseRAM();
    mTextureData->initImageFromMemory(reinterpret_cast<const unsigned char*>(data), length);
    // Get the size from the texture data.
    mSize = glm::ivec2{static_cast<int>(mTextureData->width()),
                       static_cast<int>(mTextureData->height())};
    mSourceSize = glm::vec2{mTextureData->sourceWidth(), mTextureData->sourceHeight()};
}

void TextureResource::manualUnload(std::string path, bool tile)
{
    const std::string canonicalPath = Utils::FileSystem::getCanonicalPath(path);

    TextureKeyType key(canonicalPath, tile);
    auto foundTexture = sTextureMap.find(key);

    if (foundTexture != sTextureMap.cend()) {
        sTextureMap.erase(foundTexture);
    }
}

std::vector<unsigned char> TextureResource::getRawRGBAData()
{
    std::shared_ptr<TextureData> data = sTextureDataManager.get(this);

    if (data)
        return data.get()->getRawRGBAData();
    else
        return std::vector<unsigned char>(0);
}

std::string TextureResource::getTextureFilePath()
{
    std::shared_ptr<TextureData> data = sTextureDataManager.get(this);

    if (data)
        return data->getTextureFilePath();
    else
        return "";
}

bool TextureResource::isTiled() const
{
    if (mTextureData != nullptr)
        return mTextureData->tiled();
    std::shared_ptr<TextureData> data = sTextureDataManager.get(this);
    return data->tiled();
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
                                                      bool forceRasterization)
{
    std::shared_ptr<ResourceManager>& rm = ResourceManager::getInstance();

    const std::string canonicalPath = Utils::FileSystem::getCanonicalPath(path);
    if (canonicalPath.empty()) {
        std::shared_ptr<TextureResource> tex(
            new TextureResource("", tile, false, linearMagnify, forceRasterization));
        // Make sure we get properly deinitialized even though we do nothing on reinitialization.
        rm->addReloadable(tex);
        return tex;
    }

    TextureKeyType key(canonicalPath, tile);
    auto foundTexture = sTextureMap.find(key);

    if (foundTexture != sTextureMap.cend()) {
        if (!foundTexture->second.expired())
            return foundTexture->second.lock();
    }

    // Need to create it.
    std::shared_ptr<TextureResource> tex;
    tex = std::shared_ptr<TextureResource>(
        new TextureResource(key.first, tile, dynamic, linearMagnify, forceRasterization));
    std::shared_ptr<TextureData> data = sTextureDataManager.get(tex.get());

    // Is it an SVG?
    if (Utils::String::toLower(key.first.substr(key.first.size() - 4, std::string::npos)) !=
        ".svg") {
        // Probably not. Add it to our map. We don't add SVGs because 2 SVGs might be
        // rasterized at different sizes.
        sTextureMap[key] = std::weak_ptr<TextureResource>(tex);
    }

    // Add it to the reloadable list.
    rm->addReloadable(tex);

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
        glm::vec2 textureSize = mTextureData.get()->getSize();
        if (textureSize.x == width && textureSize.y == height)
            return;
    }

    std::shared_ptr<TextureData> data;
    if (mTextureData != nullptr)
        data = mTextureData;
    else
        data = sTextureDataManager.get(this);
    mSourceSize = glm::vec2{static_cast<float>(width), static_cast<float>(height)};
    data->setSourceSize(static_cast<float>(width), static_cast<float>(height));
    if (mForceLoad || mTextureData != nullptr)
        data->load();

    mSize.x = static_cast<int>(width);
    mSize.y = static_cast<int>(height);
}

size_t TextureResource::getTotalMemUsage()
{
    size_t total = 0;
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
    size_t total{0};
    // Count up all textures that manage their own texture data.
    for (auto tex : sAllTextures) {
        if (tex->mTextureData != nullptr)
            total += tex->getSize().x * tex->getSize().y * 4;
    }
    // Now get the total memory from the manager.
    total += sTextureDataManager.getTotalSize();
    return total;
}

void TextureResource::unload(std::shared_ptr<ResourceManager>& /*rm*/)
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

void TextureResource::reload(std::shared_ptr<ResourceManager>& /*rm*/)
{
    // For dynamically loaded textures the texture manager will load them on demand.
    // For manually loaded textures we have to reload them here.
    if (mTextureData)
        mTextureData->load();
}

//
//  ResourceManager.cpp
//
//  Handles the application resources (fonts, graphics, sounds etc.).
//  Loading and unloading of these files are done here.
//

#include "ResourceManager.h"

#include "utils/FileSystemUtil.h"
#include <fstream>

#ifdef ES_INSTALL_PREFIX
std::string installPrefix = ES_INSTALL_PREFIX;
#else
std::string installPrefix = "/usr/local";
#endif

#ifdef ES_DATAROOTDIR
std::string dataRootDir = ES_DATAROOTDIR;
#else
std::string dataRootDir = "share";
#endif

auto array_deleter = [](unsigned char* p) { delete[] p; };
auto nop_deleter = [](unsigned char* /*p*/) { };

std::shared_ptr<ResourceManager> ResourceManager::sInstance = nullptr;

ResourceManager::ResourceManager()
{
}

std::shared_ptr<ResourceManager>& ResourceManager::getInstance()
{
    if (!sInstance)
        sInstance = std::shared_ptr<ResourceManager>(new ResourceManager());

    return sInstance;
}

std::string ResourceManager::getResourcePath(const std::string& path) const
{
    // Check if this is a resource file.
    if ((path[0] == ':') && (path[1] == '/')) {
        std::string test;

        // Check under the home directory.
        test = Utils::FileSystem::getHomePath() + "/.emulationstation/resources/" + &path[2];
        if (Utils::FileSystem::exists(test))
            return test;

        // Check under the data installation directory.
        // The base directory is the value set for CMAKE_INSTALL_DIRECTORY during build.
        // The datarootdir directory is the value set for CMAKE_INSTALL_DATAROOTDIR during build.
        // If not defined, the default prefix path '/usr/local' and the default datarootdir
        // directory 'share' will be used, i.e. '/usr/local/share'.
        test = installPrefix + "/" + dataRootDir + "/emulationstation/resources/" + &path[2];
        if (Utils::FileSystem::exists(test))
            return test;
    }

    // Not a resource, return unmodified path.
    return path;
}

const ResourceData ResourceManager::getFileData(const std::string& path) const
{
    // Check if its a resource.
    const std::string respath = getResourcePath(path);

    if (Utils::FileSystem::exists(respath)) {
        ResourceData data = loadFile(respath);
        return data;
    }

    // If the file doesn't exist, return an "empty" ResourceData.
    ResourceData data = {NULL, 0};
    return data;
}

ResourceData ResourceManager::loadFile(const std::string& path) const
{
    std::ifstream stream(path, std::ios::binary);

    stream.seekg(0, stream.end);
    size_t size = (size_t)stream.tellg();
    stream.seekg(0, stream.beg);

    // Supply custom deleter to properly free array.
    std::shared_ptr<unsigned char> data(new unsigned char[size], array_deleter);
    stream.read((char*)data.get(), size);
    stream.close();

    ResourceData ret = {data, size};
    return ret;
}

bool ResourceManager::fileExists(const std::string& path) const
{
    // If it exists as a resource file, return true.
    if (getResourcePath(path) != path)
        return true;

    return Utils::FileSystem::exists(path);
}

void ResourceManager::unloadAll()
{
    auto iter = mReloadables.cbegin();
    while (iter != mReloadables.cend()) {
        if (!iter->expired()) {
            iter->lock()->unload(sInstance);
            iter++;
        }
        else {
            iter = mReloadables.erase(iter);
        }
    }
}

void ResourceManager::reloadAll()
{
    auto iter = mReloadables.cbegin();
    while (iter != mReloadables.cend()) {
        if (!iter->expired()) {
            iter->lock()->reload(sInstance);
            iter++;
        }
        else {
            iter = mReloadables.erase(iter);
        }
    }
}

void ResourceManager::addReloadable(std::weak_ptr<IReloadable> reloadable)
{
    mReloadables.push_back(reloadable);
}

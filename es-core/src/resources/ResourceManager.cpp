//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ResourceManager.cpp
//
//  Handles the application resources (fonts, graphics, sounds etc.).
//  Loading and unloading of these files are done here.
//

#include "ResourceManager.h"

#include "Log.h"
#include "utils/FileSystemUtil.h"
#include "utils/PlatformUtil.h"
#include "utils/StringUtil.h"

#include <fstream>

ResourceManager& ResourceManager::getInstance()
{
    static ResourceManager instance;
    return instance;
}

std::string ResourceManager::getResourcePath(const std::string& path, bool terminateOnFailure) const
{
    // Check if this is a resource file.
    if ((path[0] == ':') && (path[1] == '/')) {

        // Check under the home directory.
        std::string testHome {Utils::FileSystem::getHomePath() + "/.emulationstation/resources/" +
                              &path[2]};
        if (Utils::FileSystem::exists(testHome))
            return testHome;

#if defined(__APPLE__)
        // For macOS, check in the ../Resources directory relative to the executable directory.
        std::string applePackagePath {Utils::FileSystem::getExePath() + "/../Resources/resources/" +
                                      &path[2]};

        if (Utils::FileSystem::exists(applePackagePath)) {
            return applePackagePath;
        }
#elif defined(__unix__) && !defined(APPIMAGE_BUILD)
        // Check under the data installation directory (Unix only).
        std::string testDataPath {Utils::FileSystem::getProgramDataPath() + "/resources/" +
                                  &path[2]};

        if (Utils::FileSystem::exists(testDataPath)) {
            return testDataPath;
        }
#endif
        // Check under the ES executable directory.
        std::string testExePath {Utils::FileSystem::getExePath() + "/resources/" + &path[2]};

        if (Utils::FileSystem::exists(testExePath)) {
            return testExePath;
        }
        // For missing resources, log an error and terminate the application. This should
        // indicate that we have a broken EmulationStation installation. If the argument
        // terminateOnFailure is set to false though, then skip this step.
        else {
            if (terminateOnFailure) {
                LOG(LogError) << "Program resource missing: " << path;
                LOG(LogError) << "Tried to find the resource in the following locations:";
                LOG(LogError) << testHome;
#if defined(__APPLE__)
                LOG(LogError) << applePackagePath;
#elif defined(__unix__) && !defined(APPIMAGE_BUILD)
                LOG(LogError) << testDataPath;
#endif
                LOG(LogError) << testExePath;
                LOG(LogError) << "Has EmulationStation been properly installed?";
                Utils::Platform::emergencyShutdown();
            }
            else {
                return "";
            }
        }
    }

    // Not a resource, return unmodified path.
    return path;
}

const ResourceData ResourceManager::getFileData(const std::string& path) const
{
    // Check if its a resource.
    const std::string respath {getResourcePath(path)};

    if (Utils::FileSystem::exists(respath)) {
        ResourceData data {loadFile(respath)};
        return data;
    }

    // If the file doesn't exist, return an "empty" ResourceData.
    ResourceData data {nullptr, 0};
    return data;
}

ResourceData ResourceManager::loadFile(const std::string& path) const
{
#if defined(_WIN64)
    std::ifstream stream {Utils::String::stringToWideString(path).c_str(), std::ios::binary};
#else
    std::ifstream stream {path, std::ios::binary};
#endif

    stream.seekg(0, stream.end);
    size_t size {static_cast<size_t>(stream.tellg())};
    stream.seekg(0, stream.beg);

    // Supply custom deleter to properly free array.
    std::shared_ptr<unsigned char> data {new unsigned char[size],
                                         [](unsigned char* p) { delete[] p; }};
    stream.read(reinterpret_cast<char*>(data.get()), size);
    stream.close();

    ResourceData ret {data, size};
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
            iter->lock()->unload(getInstance());
            ++iter;
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
            iter->lock()->reload(getInstance());
            ++iter;
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

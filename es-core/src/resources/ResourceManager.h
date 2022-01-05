//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ResourceManager.h
//
//  Handles the application resources (fonts, graphics, sounds etc.).
//  Loading and unloading of these files are done here.
//

#ifndef ES_CORE_RESOURCES_RESOURCE_MANAGER_H
#define ES_CORE_RESOURCES_RESOURCE_MANAGER_H

#include <list>
#include <memory>
#include <string>

// The ResourceManager exists to:
// Allow loading resources embedded into the executable like an actual file.
// Allow embedded resources to be optionally remapped to actual files for further customization.

struct ResourceData {
    const std::shared_ptr<unsigned char> ptr;
    const size_t length;
};

class ResourceManager;

class IReloadable
{
public:
    virtual void unload(ResourceManager& rm) = 0;
    virtual void reload(ResourceManager& rm) = 0;
};

class ResourceManager
{
public:
    static ResourceManager& getInstance();

    void addReloadable(std::weak_ptr<IReloadable> reloadable);

    void unloadAll();
    void reloadAll();

    std::string getResourcePath(const std::string& path, bool terminateOnFailure = true) const;
    const ResourceData getFileData(const std::string& path) const;
    bool fileExists(const std::string& path) const;

private:
    ResourceManager() noexcept {}

    std::list<std::weak_ptr<IReloadable>> mReloadables;

    ResourceData loadFile(const std::string& path) const;
};

#endif // ES_CORE_RESOURCES_RESOURCE_MANAGER_H

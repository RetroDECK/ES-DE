//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMPlatformMapping.h
//
//  Persists, per ES-DE system, whether RomM library sync is enabled and which RomM platform
//  id it's mapped to. This is a list of tuples rather than a scalar value, so it's kept in
//  its own small XML file (romm_mapping.xml, alongside es_settings.xml) instead of being
//  shoehorned into the flat key/value Settings class.
//

#ifndef ES_APP_ROMM_ROMM_PLATFORM_MAPPING_H
#define ES_APP_ROMM_ROMM_PLATFORM_MAPPING_H

#include <string>
#include <vector>

struct RomMSystemMapping {
    // Matches SystemData::getName().
    std::string systemName;
    bool syncEnabled {false};
    // The RomM platform id this system is mapped to, or -1 if not yet mapped.
    int rommPlatformId {-1};
};

class RomMPlatformMapping
{
public:
    static RomMPlatformMapping& getInstance();

    // Creates or updates the mapping entry for the given system and persists it immediately.
    void setMapping(const RomMSystemMapping& mapping);

    // Returns nullptr if no mapping has been configured yet for this system.
    const RomMSystemMapping* getMapping(const std::string& systemName) const;

    const std::vector<RomMSystemMapping>& getAllMappings() const { return mMappings; }

private:
    RomMPlatformMapping();

    void loadFile();
    void saveFile();

    std::vector<RomMSystemMapping> mMappings;
};

#endif // ES_APP_ROMM_ROMM_PLATFORM_MAPPING_H

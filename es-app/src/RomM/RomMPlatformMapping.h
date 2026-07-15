//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMPlatformMapping.h
//
//  Persists, per RomM platform, whether library sync is enabled and which ES-DE system it's
//  mapped to. Keyed by RomM platform id rather than by ES-DE system name so that a platform
//  can be marked "sync enabled" before any local system exists for it yet - systemName stays
//  empty until GuiRomMSync::activatePendingSystems() creates the ROM directory and fills it
//  in. This is a list of tuples rather than a scalar value, so it's kept in its own small XML
//  file (romm_mapping.xml, alongside es_settings.xml) instead of being shoehorned into the
//  flat key/value Settings class.
//

#ifndef ES_APP_ROMM_ROMM_PLATFORM_MAPPING_H
#define ES_APP_ROMM_ROMM_PLATFORM_MAPPING_H

#include <string>
#include <vector>

struct RomMSystemMapping {
    // The RomM platform id this mapping is for. Always valid (>= 0) once persisted.
    int rommPlatformId {-1};
    // RomM's slug and fs_slug, used to match a SystemData::InactiveSystemTemplate when this
    // platform hasn't been activated into a real local system yet.
    std::string platformSlug;
    std::string platformFsSlug;
    // Matches SystemData::getName(). Empty until a local system has been assigned/activated.
    std::string systemName;
    bool syncEnabled {false};
};

class RomMPlatformMapping
{
public:
    static RomMPlatformMapping& getInstance();

    // Creates or updates the mapping entry for the given RomM platform id and persists it
    // immediately.
    void setMapping(const RomMSystemMapping& mapping);

    // Fills in systemName for an already-persisted platform mapping (used once a pending
    // platform's ROM directory has actually been created) and persists it immediately. No-op
    // if no mapping exists yet for this platform id.
    void setSystemNameForPlatform(int rommPlatformId, const std::string& systemName);

    // Returns nullptr if no mapping references this system name. Linear scan - only used for
    // the occasional reverse lookup (e.g. SystemData::loadConfig()'s empty-directory exemption),
    // not on any hot path.
    const RomMSystemMapping* getMapping(const std::string& systemName) const;

    // Returns nullptr if no mapping exists yet for this RomM platform id. The platform-to-system
    // relationship is enforced to be one-to-one: setMapping() clears systemName from any other
    // mapping that previously claimed the same system.
    const RomMSystemMapping* findByPlatformId(int rommPlatformId) const;

    const std::vector<RomMSystemMapping>& getAllMappings() const { return mMappings; }

private:
    RomMPlatformMapping();

    void loadFile();
    void saveFile();

    std::vector<RomMSystemMapping> mMappings;
};

#endif // ES_APP_ROMM_ROMM_PLATFORM_MAPPING_H

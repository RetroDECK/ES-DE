//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMUtils.cpp
//

#include "RomM/RomMUtils.h"

#include "FileData.h"
#include "HttpReq.h"
#include "Log.h"
#include "RomM/RomMLocalFavorites.h"
#include "Settings.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace
{
    // Known mismatches between ES-DE's platform names (PlatformIds::platformNames) and RomM's
    // filesystem-friendly platform slugs. Key is the ES-DE name, value the RomM fs_slug/slug(s)
    // it should be considered equivalent to - some names have two, where RomM has both a legacy
    // IGDB slug and a newer "universal" slug (see UniversalPlatformSlug in
    // backend/handler/metadata/base_handler.py of rommapp/romm, the authoritative source these
    // were checked against) for the same platform.
    const std::unordered_map<std::string, std::vector<std::string>> sPlatformAliases {
        {"adam", {"colecoadam"}},
        {"amigacd32", {"amiga-cd32"}},
        {"amstradcpc", {"acpc"}},
        {"apple2", {"appleii"}},
        {"apple2gs", {"apple-iigs"}},
        {"arcadia", {"arcadia-2001"}},
        {"archimedes", {"acorn-archimedes"}},
        {"astrocde", {"astrocade"}},
        {"atarijaguar", {"jaguar"}},
        {"atarijaguarcd", {"atari-jaguar-cd"}},
        {"atarilynx", {"lynx"}},
        {"atarist", {"atari-st"}},
        {"atarixe", {"atari-xegs"}},
        {"cdimono1", {"philips-cd-i"}},
        {"cdtv", {"commodore-cdtv"}},
        {"channelf", {"fairchild-channel-f"}},
        {"coco", {"trs-80-color-computer"}},
        {"crvision", {"creativision"}},
        {"dragon32", {"dragon-32-slash-64"}},
        {"dreamcast", {"dc"}},
        {"electron", {"acorn-electron"}},
        {"fm7", {"fm-7"}},
        {"fmtowns", {"fm-towns"}},
        {"gameandwatch", {"g-and-w"}},
        {"gamecom", {"game-dot-com"}},
        {"gamegear", {"gg"}},
        {"gc", {"ngc"}},
        {"genesis", {"genesis-slash-megadrive"}},
        {"gmaster", {"hartung"}},
        {"gx4000", {"amstrad-gx4000"}},
        {"lcdgames", {"handheld-electronic-lcd"}},
        {"macintosh", {"mac"}},
        {"mastersystem", {"sms"}},
        {"megadrive", {"genesis-slash-megadrive", "genesis"}},
        {"megaduck", {"mega-duck-slash-cougar-boy"}},
        {"msxturbor", {"msx-turbo"}},
        {"n3ds", {"3ds"}},
        {"neogeo", {"neogeoaes"}},
        {"neogeocd", {"neo-geo-cd"}},
        {"ngp", {"neo-geo-pocket"}},
        {"ngpc", {"neo-geo-pocket-color"}},
        {"odyssey2", {"odyssey-2"}},
        {"palm", {"palm-os"}},
        {"pc88", {"pc-8800-series"}},
        {"pc98", {"pc-9800-series"}},
        {"pcengine", {"turbografx-16-slash-pc-engine", "tg16"}},
        {"pcenginecd", {"turbografx-cd"}},
        {"pcfx", {"pc-fx"}},
        {"pcwindows", {"win"}},
        {"plus4", {"c-plus-4"}},
        {"pokemini", {"pokemon-mini"}},
        {"psx", {"ps"}},
        {"pv1000", {"casio-pv-1000"}},
        {"samcoupe", {"sam-coupe"}},
        {"scv", {"epoch-super-cassette-vision"}},
        {"sega32x", {"sega32"}},
        {"sg-1000", {"sg1000"}},
        {"sufami", {"sufami-turbo"}},
        {"supracan", {"super-acan"}},
        {"ti99", {"ti-99"}},
        {"tic80", {"tic-80"}},
        {"vic20", {"vic-20"}},
        {"wasm4", {"wasm-4"}},
        {"windows3x", {"win3x"}},
        {"wonderswancolor", {"wonderswan-color"}},
        {"x68000", {"sharp-x68000"}},
        {"zmachine", {"z-machine"}},
        {"zxnext", {"zx-spectrum-next"}},
        {"zxspectrum", {"zxs"}},
    };

    // Parses the fixed "YYYY-MM-DDTHH:MM:SS" prefix of an ISO-8601 timestamp as returned by
    // RomM's "expires_at" field into Unix seconds, ignoring any fractional-seconds/offset
    // suffix. Returns 0 on any parse failure.
    int64_t parseIso8601ToUnixSeconds(const std::string& value)
    {
        tm parsedTime {};
        if (sscanf(value.c_str(), "%d-%d-%dT%d:%d:%d", &parsedTime.tm_year, &parsedTime.tm_mon,
                   &parsedTime.tm_mday, &parsedTime.tm_hour, &parsedTime.tm_min,
                   &parsedTime.tm_sec) != 6)
            return 0;

        parsedTime.tm_year -= 1900;
        parsedTime.tm_mon -= 1;

#if defined(_WIN64)
        return static_cast<int64_t>(_mkgmtime(&parsedTime));
#else
        return static_cast<int64_t>(timegm(&parsedTime));
#endif
    }
} // namespace

namespace RomMUtils
{
    bool isLoggedIn()
    {
        if (Settings::getInstance()->getString("RomMToken").empty())
            return false;

        const std::string expiresAt {Settings::getInstance()->getString("RomMTokenExpiresAt")};
        if (expiresAt.empty())
            return true; // No expiry set - RomM's own "expires_at": null.

        const int64_t expiresAtUnix {parseIso8601ToUnixSeconds(expiresAt)};
        if (expiresAtUnix == 0)
            return true; // Unparseable - fail open rather than lock the user out over this.

        return std::time(nullptr) < expiresAtUnix;
    }

    std::string joinUrl(const std::string& serverURL, const std::string& path)
    {
        return stripTrailingSlashes(serverURL) + path;
    }

    std::string stripTrailingSlashes(const std::string& url)
    {
        std::string trimmed {url};
        while (!trimmed.empty() && trimmed.back() == '/')
            trimmed.pop_back();
        return trimmed;
    }

    std::string getServerUrl()
    {
        return stripTrailingSlashes(Settings::getInstance()->getString("RomMServerURL"));
    }

    bool checkHeartbeat(const std::string& serverURL)
    {
        HttpReq req {joinUrl(serverURL, "/api/heartbeat"), false};
        for (int i {0}; i < 30000 / 50; ++i) {
            const HttpReq::Status status {req.status()};
            if (status == HttpReq::REQ_SUCCESS)
                return true;
            if (status != HttpReq::REQ_IN_PROGRESS)
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return false;
    }

    bool resolveServerUrl(const std::string& rawInput, std::string& outResolvedUrl)
    {
        const std::string trimmed {stripTrailingSlashes(Utils::String::trim(rawInput))};

        if (trimmed.empty())
            return false;

        const std::string lowerTrimmed {Utils::String::toLower(trimmed)};
        if (lowerTrimmed.rfind("http://", 0) == 0 || lowerTrimmed.rfind("https://", 0) == 0) {
            if (!checkHeartbeat(trimmed))
                return false;
            outResolvedUrl = trimmed;
            return true;
        }

        const std::string httpsUrl {"https://" + trimmed};
        if (checkHeartbeat(httpsUrl)) {
            outResolvedUrl = httpsUrl;
            return true;
        }

        const std::string httpUrl {"http://" + trimmed};
        if (checkHeartbeat(httpUrl)) {
            outResolvedUrl = httpUrl;
            return true;
        }

        return false;
    }

    bool platformNameMatches(const std::string& esdePlatformName,
                             const std::string& rommSlug,
                             const std::string& rommFsSlug)
    {
        const std::string esdeLower {Utils::String::toLower(esdePlatformName)};
        const std::string slugLower {Utils::String::toLower(rommSlug)};
        const std::string fsSlugLower {Utils::String::toLower(rommFsSlug)};

        if (esdeLower == slugLower || esdeLower == fsSlugLower)
            return true;

        const auto it {sPlatformAliases.find(esdeLower)};
        if (it != sPlatformAliases.cend()) {
            for (const auto& alias : it->second) {
                if (alias == slugLower || alias == fsSlugLower)
                    return true;
            }
        }

        return false;
    }

    std::string formatTimestampUtc(time_t time)
    {
        tm utcTime {};
#if defined(_WIN64)
        gmtime_s(&utcTime, &time);
#else
        gmtime_r(&time, &utcTime);
#endif
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utcTime);
        return std::string {buffer};
    }

    std::string formatReleaseDate(int64_t firstReleaseDateUnixSeconds, const std::string& gameName)
    {
        if (firstReleaseDateUnixSeconds <= 0)
            return "";

        // Converted to a UTC calendar date via gmtime_r()/gmtime_s() rather than through
        // Utils::Time::DateTime/stringToTime, since those work in local time and would shift
        // the date by a day in most timezones for what is meant to be a plain calendar date,
        // not a precise instant.
        const time_t releaseTimestamp {static_cast<time_t>(firstReleaseDateUnixSeconds)};
        tm utcTime {};
#if defined(_WIN64)
        gmtime_s(&utcTime, &releaseTimestamp);
#else
        gmtime_r(&releaseTimestamp, &utcTime);
#endif
        const int year {utcTime.tm_year + 1900};
        const int currentYear {Utils::Time::DateTime(Utils::Time::now()).getTimeStruct().tm_year +
                               1900};

        if (year < 1950 || year > currentYear + 2) {
            LOG(LogWarning) << "RomM: Ignoring implausible release date for \"" << gameName
                            << "\" (raw timestamp " << firstReleaseDateUnixSeconds << " s -> year "
                            << year << ")";
            return "";
        }

        // Formatted directly to MD_DATE's raw storage format, see the "releasedate" default
        // value ("19700101T000000") in MetaData.cpp.
        std::stringstream dateStream;
        dateStream << std::setfill('0') << std::setw(4) << year << std::setw(2)
                   << (utcTime.tm_mon + 1) << std::setw(2) << utcTime.tm_mday << "T000000";
        return dateStream.str();
    }

    std::string formatCommunityRating(float averageRating0to100)
    {
        float ratingVal {averageRating0to100 / 100.0f};
        ratingVal = std::min(1.0f, std::max(0.0f, ratingVal));
        ratingVal = ceilf(ratingVal / 0.1f) / 10.0f;
        if (ratingVal <= 0.0f)
            return "";
        std::stringstream ss;
        ss << ratingVal;
        return ss.str();
    }

    // A game can be played through ES-DE (once downloaded) or through RomM's own web player, so
    // neither source can be trusted as authoritative - only ever move the value forward. Exposed
    // on its own (not folded into applyRomMData() below) since an already-downloaded entry still
    // wants its last-played time merged in on every sync, without the rest of applyRomMData()
    // touching fields the downloaded copy now owns (see the "Already downloaded" branch in
    // RomMLibrarySync::applyResults()).
    void mergeLastPlayed(FileData* file, int64_t rommLastPlayedUnix)
    {
        if (rommLastPlayedUnix <= 0)
            return;
        const int64_t currentUnix {Utils::Time::stringToTime(file->metadata.get("lastplayed"))};
        if (rommLastPlayedUnix > currentUnix)
            file->metadata.set("lastplayed",
                               Utils::Time::DateTime(static_cast<time_t>(rommLastPlayedUnix)));
    }

    namespace
    {
        // Descriptive fields (from RomM's rom metadata, IGDB-sourced) and per-user/local fields
        // (from RomM's rom_user resource, plus local favorite intent) are kept as separate
        // helpers below since they come from distinct data provenance/update streams (see the
        // comment on Rom::lastPlayed in RomMApiClient.h) - but every current caller wants both
        // together, so applyRomMData() below is the only one exposed outside this file.
        void applyRomMGameData(FileData* file, const RomMApiClient::Rom& rom)
        {
            const std::string releaseDate {formatReleaseDate(rom.firstReleaseDate, rom.name)};
            if (!releaseDate.empty())
                file->metadata.set("releasedate", releaseDate);

            // Community rating first, so the value doesn't change meaning once downloaded and
            // scraped - personal rom_user.rating is only a fallback for an unmatched rom.
            std::string rating {formatCommunityRating(rom.averageRating)};
            if (rating.empty() && rom.userRating > 0) {
                std::stringstream ss;
                ss << (static_cast<float>(rom.userRating) / 10.0f);
                rating = ss.str();
            }
            if (!rating.empty())
                file->metadata.set("rating", rating);

            if (!rom.genres.empty())
                file->metadata.set("genre",
                                   Utils::String::vectorToDelimitedString(rom.genres, ", "));

            // RomM doesn't distinguish developer from publisher.
            if (!rom.companies.empty()) {
                const std::string companies {
                    Utils::String::vectorToDelimitedString(rom.companies, ", ")};
                file->metadata.set("developer", companies);
                file->metadata.set("publisher", companies);
            }

            if (!rom.playerCount.empty())
                file->metadata.set("players", rom.playerCount);
        }

        void applyRomMPlayerData(FileData* file, const RomMApiClient::Rom& rom)
        {
            file->metadata.set("hidden", rom.userHidden ? "true" : "false");
            file->metadata.set("completed",
                               (rom.userStatus == "finished" || rom.userStatus == "completed_100") ?
                                   "true" :
                                   "false");
            // Favorite is local-only intent (RomMLocalFavorites), not mirrored from RomM -
            // reapply it since a still-remote FileData is rebuilt from scratch every sync.
            file->metadata.set("favorite", RomMLocalFavorites::getInstance().isFavorite(rom.id) ?
                                               "true" :
                                               "false");
        }
    } // namespace

    void applyRomMData(FileData* file, const RomMApiClient::Rom& rom)
    {
        applyRomMGameData(file, rom);
        applyRomMPlayerData(file, rom);
        mergeLastPlayed(file, rom.lastPlayed);
    }

} // namespace RomMUtils

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FileSorts.cpp
//
//  Gamelist sorting functions.
//  Actual sorting takes place in FileData.
//

#include "FileSorts.h"

#include "SystemData.h"
#include "utils/StringUtil.h"

#include <algorithm>
#include <string>

namespace FileSorts
{
    const FileData::SortType typesArr[] {
        FileData::SortType(&compareName, "filename, ascending"),
        FileData::SortType(&compareNameDescending, "filename, descending"),

        FileData::SortType(&compareRating, "rating, ascending"),
        FileData::SortType(&compareRatingDescending, "rating, descending"),

        FileData::SortType(&compareReleaseDate, "release date, ascending"),
        FileData::SortType(&compareReleaseDateDescending, "release date, descending"),

        FileData::SortType(&compareDeveloper, "developer, ascending"),
        FileData::SortType(&compareDeveloperDescending, "developer, descending"),

        FileData::SortType(&comparePublisher, "publisher, ascending"),
        FileData::SortType(&comparePublisherDescending, "publisher, descending"),

        FileData::SortType(&compareGenre, "genre, ascending"),
        FileData::SortType(&compareGenreDescending, "genre, descending"),

        FileData::SortType(&compareNumPlayers, "players, ascending"),
        FileData::SortType(&compareNumPlayersDescending, "players, descending"),

        FileData::SortType(&compareLastPlayed, "last played, ascending"),
        FileData::SortType(&compareLastPlayedDescending, "last played, descending"),

        FileData::SortType(&compareTimesPlayed, "times played, ascending"),
        FileData::SortType(&compareTimesPlayedDescending, "times played, descending"),

        FileData::SortType(&compareSystem, "system, ascending"),
        FileData::SortType(&compareSystemDescending, "system, descending")};

    const std::vector<FileData::SortType> SortTypes {typesArr, typesArr + sizeof(typesArr) /
                                                                              sizeof(typesArr[0])};

    bool compareName(const FileData* file1, const FileData* file2)
    {
        // We compare the actual metadata name, as collection files have the system
        // appended which messes up the order.
        std::string name1;
        std::string name2;

        if (file1->getSystem()->isCustomCollection()) {
            if (Utils::String::toUpper(file1->metadata.get("collectionsortname")) != "")
                name1 = Utils::String::toUpper(file1->metadata.get("collectionsortname"));
            else if (Utils::String::toUpper(file1->metadata.get("sortname")) != "")
                name1 = Utils::String::toUpper(file1->metadata.get("sortname"));
            else
                name1 = Utils::String::toUpper(file1->metadata.get("name"));

            if (Utils::String::toUpper(file2->metadata.get("collectionsortname")) != "")
                name2 = Utils::String::toUpper(file2->metadata.get("collectionsortname"));
            else if (Utils::String::toUpper(file2->metadata.get("sortname")) != "")
                name2 = Utils::String::toUpper(file2->metadata.get("sortname"));
            else
                name2 = Utils::String::toUpper(file2->metadata.get("name"));
            return name1.compare(name2) < 0;
        }

        name1 = Utils::String::toUpper(file1->metadata.get("sortname"));
        name2 = Utils::String::toUpper(file2->metadata.get("sortname"));
        if (name1.empty())
            name1 = Utils::String::toUpper(file1->metadata.get("name"));
        if (name2.empty())
            name2 = Utils::String::toUpper(file2->metadata.get("name"));
        return name1.compare(name2) < 0;
    }

    bool compareNameDescending(const FileData* file1, const FileData* file2)
    {
        std::string name1;
        std::string name2;

        if (file1->getSystem()->isCustomCollection()) {
            if (Utils::String::toUpper(file1->metadata.get("collectionsortname")) != "")
                name1 = Utils::String::toUpper(file1->metadata.get("collectionsortname"));
            else if (Utils::String::toUpper(file1->metadata.get("sortname")) != "")
                name1 = Utils::String::toUpper(file1->metadata.get("sortname"));
            else
                name1 = Utils::String::toUpper(file1->metadata.get("name"));

            if (Utils::String::toUpper(file2->metadata.get("collectionsortname")) != "")
                name2 = Utils::String::toUpper(file2->metadata.get("collectionsortname"));
            else if (Utils::String::toUpper(file2->metadata.get("sortname")) != "")
                name2 = Utils::String::toUpper(file2->metadata.get("sortname"));
            else
                name2 = Utils::String::toUpper(file2->metadata.get("name"));
            return name1.compare(name2) > 0;
        }

        name1 = Utils::String::toUpper(file1->metadata.get("sortname"));
        name2 = Utils::String::toUpper(file2->metadata.get("sortname"));
        if (name1.empty())
            name1 = Utils::String::toUpper(file1->metadata.get("name"));
        if (name2.empty())
            name2 = Utils::String::toUpper(file2->metadata.get("name"));
        return name1.compare(name2) > 0;
    }

    bool compareRating(const FileData* file1, const FileData* file2)
    {
        return file1->metadata.getFloat("rating") < file2->metadata.getFloat("rating");
    }

    bool compareRatingDescending(const FileData* file1, const FileData* file2)
    {
        return file1->metadata.getFloat("rating") > file2->metadata.getFloat("rating");
    }

    bool compareReleaseDate(const FileData* file1, const FileData* file2)
    {
        // Since it's stored as an ISO string (YYYYMMDDTHHMMSS), we can compare as a string
        // which is a lot faster than the time casts and the time comparisons.
        return (file1)->metadata.get("releasedate") < (file2)->metadata.get("releasedate");
    }

    bool compareReleaseDateDescending(const FileData* file1, const FileData* file2)
    {
        return (file1)->metadata.get("releasedate") > (file2)->metadata.get("releasedate");
    }

    bool compareDeveloper(const FileData* file1, const FileData* file2)
    {
        std::string developer1 = Utils::String::toUpper(file1->metadata.get("developer"));
        std::string developer2 = Utils::String::toUpper(file2->metadata.get("developer"));
        return developer1.compare(developer2) < 0;
    }

    bool compareDeveloperDescending(const FileData* file1, const FileData* file2)
    {
        std::string developer1 = Utils::String::toUpper(file1->metadata.get("developer"));
        std::string developer2 = Utils::String::toUpper(file2->metadata.get("developer"));
        return developer1.compare(developer2) > 0;
    }

    bool comparePublisher(const FileData* file1, const FileData* file2)
    {
        std::string publisher1 = Utils::String::toUpper(file1->metadata.get("publisher"));
        std::string publisher2 = Utils::String::toUpper(file2->metadata.get("publisher"));
        return publisher1.compare(publisher2) < 0;
    }

    bool comparePublisherDescending(const FileData* file1, const FileData* file2)
    {
        std::string publisher1 = Utils::String::toUpper(file1->metadata.get("publisher"));
        std::string publisher2 = Utils::String::toUpper(file2->metadata.get("publisher"));
        return publisher1.compare(publisher2) > 0;
    }

    bool compareGenre(const FileData* file1, const FileData* file2)
    {
        std::string genre1 = Utils::String::toUpper(file1->metadata.get("genre"));
        std::string genre2 = Utils::String::toUpper(file2->metadata.get("genre"));
        return genre1.compare(genre2) < 0;
    }

    bool compareGenreDescending(const FileData* file1, const FileData* file2)
    {
        std::string genre1 = Utils::String::toUpper(file1->metadata.get("genre"));
        std::string genre2 = Utils::String::toUpper(file2->metadata.get("genre"));
        return genre1.compare(genre2) > 0;
    }

    bool compareNumPlayers(const FileData* file1, const FileData* file2)
    {
        std::string file1Players = (file1)->metadata.get("players");
        std::string file2Players = (file2)->metadata.get("players");
        unsigned int file1Int = 0;
        unsigned int file2Int = 0;
        size_t dashPos;
        // If there is a range of players such as '1-4' then capture the number after the dash.
        dashPos = file1Players.find("-");
        if (dashPos != std::string::npos)
            file1Players = file1Players.substr(dashPos + 1, file1Players.size() - dashPos - 1);
        dashPos = file2Players.find("-");
        if (dashPos != std::string::npos)
            file2Players = file2Players.substr(dashPos + 1, file2Players.size() - dashPos - 1);
        // Any non-numeric value will end up as zero.
        if (!file1Players.empty() &&
            std::all_of(file1Players.begin(), file1Players.end(), ::isdigit)) {
            file1Int = stoi(file1Players);
        }
        if (!file2Players.empty() &&
            std::all_of(file2Players.begin(), file2Players.end(), ::isdigit)) {
            file2Int = stoi(file2Players);
        }
        return file1Int < file2Int;
    }

    bool compareNumPlayersDescending(const FileData* file1, const FileData* file2)
    {
        std::string file1Players = (file1)->metadata.get("players");
        std::string file2Players = (file2)->metadata.get("players");
        unsigned int file1Int = 0;
        unsigned int file2Int = 0;
        size_t dashPos;
        dashPos = file1Players.find("-");
        if (dashPos != std::string::npos)
            file1Players = file1Players.substr(dashPos + 1, file1Players.size() - dashPos - 1);
        dashPos = file2Players.find("-");
        if (dashPos != std::string::npos)
            file2Players = file2Players.substr(dashPos + 1, file2Players.size() - dashPos - 1);
        if (!file1Players.empty() &&
            std::all_of(file1Players.begin(), file1Players.end(), ::isdigit)) {
            file1Int = stoi(file1Players);
        }
        if (!file2Players.empty() &&
            std::all_of(file2Players.begin(), file2Players.end(), ::isdigit)) {
            file2Int = stoi(file2Players);
        }
        return file1Int > file2Int;
    }

    bool compareLastPlayed(const FileData* file1, const FileData* file2)
    {
        // Since it's stored as an ISO string (YYYYMMDDTHHMMSS), we can compare as a string
        // which is a lot faster than the time casts and the time comparisons.
        return (file1)->metadata.get("lastplayed") > (file2)->metadata.get("lastplayed");
    }

    bool compareLastPlayedDescending(const FileData* file1, const FileData* file2)
    {
        return (file1)->metadata.get("lastplayed") < (file2)->metadata.get("lastplayed");
    }

    bool compareTimesPlayed(const FileData* file1, const FileData* file2)
    {
        // Only games have playcount metadata.
        if (file1->metadata.getType() == GAME_METADATA &&
            file2->metadata.getType() == GAME_METADATA) {
            return (file1)->metadata.getInt("playcount") < (file2)->metadata.getInt("playcount");
        }
        return false;
    }

    bool compareTimesPlayedDescending(const FileData* file1, const FileData* file2)
    {
        if (file1->metadata.getType() == GAME_METADATA &&
            file2->metadata.getType() == GAME_METADATA) {
            return (file1)->metadata.getInt("playcount") > (file2)->metadata.getInt("playcount");
        }
        return false;
    }

    bool compareSystem(const FileData* file1, const FileData* file2)
    {
        std::string system1 = Utils::String::toUpper(file1->getSystemName());
        std::string system2 = Utils::String::toUpper(file2->getSystemName());
        return system1.compare(system2) < 0;
    }

    bool compareSystemDescending(const FileData* file1, const FileData* file2)
    {
        std::string system1 = Utils::String::toUpper(file1->getSystemName());
        std::string system2 = Utils::String::toUpper(file2->getSystemName());
        return system1.compare(system2) > 0;
    }

} // namespace FileSorts

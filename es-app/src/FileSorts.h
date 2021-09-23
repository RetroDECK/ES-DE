//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FileSorts.h
//
//  Gamelist sorting functions.
//  Actual sorting takes place in FileData.
//

#ifndef ES_APP_FILE_SORTS_H
#define ES_APP_FILE_SORTS_H

#include "FileData.h"

#include <vector>

namespace FileSorts
{
    bool compareName(const FileData* file1, const FileData* file2);
    bool compareNameDescending(const FileData* file1, const FileData* file2);
    bool compareRating(const FileData* file1, const FileData* file2);
    bool compareRatingDescending(const FileData* file1, const FileData* file2);
    bool compareReleaseDate(const FileData* file1, const FileData* file2);
    bool compareReleaseDateDescending(const FileData* file1, const FileData* file2);
    bool compareDeveloper(const FileData* file1, const FileData* file2);
    bool compareDeveloperDescending(const FileData* file1, const FileData* file2);
    bool comparePublisher(const FileData* file1, const FileData* file2);
    bool comparePublisherDescending(const FileData* file1, const FileData* file2);
    bool compareGenre(const FileData* file1, const FileData* file2);
    bool compareGenreDescending(const FileData* file1, const FileData* file2);
    bool compareNumPlayers(const FileData* file1, const FileData* file2);
    bool compareNumPlayersDescending(const FileData* file1, const FileData* file2);
    bool compareLastPlayed(const FileData* file1, const FileData* file2);
    bool compareLastPlayedDescending(const FileData* file1, const FileData* file2);
    bool compareTimesPlayed(const FileData* file1, const FileData* fil2);
    bool compareTimesPlayedDescending(const FileData* file1, const FileData* fil2);
    bool compareSystem(const FileData* file1, const FileData* file2);
    bool compareSystemDescending(const FileData* file1, const FileData* file2);

    extern const std::vector<FileData::SortType> SortTypes;
} // namespace FileSorts

#endif // ES_APP_FILE_SORTS_H

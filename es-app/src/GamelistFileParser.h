//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GamelistFileParser.h
//
//  Parses and updates the gamelist.xml files.
//

#ifndef ES_APP_GAMELIST_FILE_PARSER_H
#define ES_APP_GAMELIST_FILE_PARSER_H

class SystemData;

namespace GamelistFileParser
{
    // Loads gamelist.xml data into a SystemData.
    void parseGamelist(SystemData* system);

    // Writes currently loaded metadata for a SystemData to gamelist.xml.
    void updateGamelist(SystemData* system, bool updateAlternativeEmulator = false);

} // namespace GamelistFileParser

#endif // ES_APP_GAMELIST_FILE_PARSER_H

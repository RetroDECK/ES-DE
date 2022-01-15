//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GamelistFileParse.h
//
//  Parses and updates the gamelist.xml files.
//

#ifndef ES_APP_GAMELIST_FILE_PARSE_H
#define ES_APP_GAMELIST_FILE_PARSE_H

class SystemData;

// Loads gamelist.xml data into a SystemData.
void parseGamelist(SystemData* system);

// Writes currently loaded metadata for a SystemData to gamelist.xml.
void updateGamelist(SystemData* system, bool updateAlternativeEmulator = false);

#endif // ES_APP_GAMELIST_FILE_PARSE_H

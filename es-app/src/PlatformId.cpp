//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  PlatformId.cpp
//
//  Index of all supported systems/platforms.
//

#include "PlatformId.h"

#include <string.h>
#include <vector>

namespace PlatformIds
{
    std::vector<std::string> platformNames = {
        "unknown", // Nothing set.

        "3do",                  // 3DO
        "ags",                  // Adventure Game Studio
        "amiga",                // Commodore Amiga
        "amigacd32",            // Commodore Amiga CD32
        "amstradcpc",           // Amstrad CPC
        "apple2",               // Apple II
        "apple2gs",             // Apple IIGS
        "arcade",               // Arcade
        "astrocade",            // Bally Astrocade
        "atari2600",            // Atari 2600
        "atari5200",            // Atari 5200
        "atari7800",            // Atari 7800
        "atari800",             // Atari 800
        "atarijaguar",          // Atari Jaguar
        "atarijaguarcd",        // Atari Jaguar CD
        "atarilynx",            // Atari Lynx
        "atarist",              // Atari ST/STE/Falcon
        "atarixe",              // Atari XE
        "atomiswave",           // Atomiswave
        "bbcmicro",             // BBC Micro
        "c64",                  // Commodore 64
        "cavestory",            // Cave Story (NXEngine)
        "cdtv",                 // Commodore CDTV
        "channelf",             // Fairchild Channel F
        "coco",                 // Tandy Color Computer
        "colecovision",         // ColecoVision
        "daphne",               // Daphne Arcade Laserdisc Emulator
        "dos",                  // DOS (PC)
        "dragon32",             // Dragon 32
        "dreamcast",            // Sega Dreamcast
        "famicom",              // Nintendo Family Computer
        "fds",                  // Nintendo Famicom Disk System
        "gameandwatch",         // Nintendo Game and Watch
        "gamegear",             // Sega Game Gear
        "gb",                   // Nintendo Game Boy
        "gba",                  // Nintendo Game Boy Advance
        "gbc",                  // Nintendo Game Boy Color
        "gc",                   // Nintendo GameCube
        "genesis",              // Sega Genesis
        "gx4000",               // Amstrad GX4000
        "intellivision",        // Mattel Electronics Intellivision
        "love",                 // ChaiLove game engine
        "lutro",                // Lutro game engine
        "macintosh",            // Apple Macintosh
        "mastersystem",         // Sega Master System
        "megadrive",            // Sega Mega Drive
        "mess",                 // Multi Emulator Super System
        "moonlight",            // Moonlight game streaming
        "moto",                 // Thomson MO/TO series
        "msx",                  // MSX
        "msx2",                 // MSX2
        "msxturbor",            // MSX Turbo R
        "n3ds",                 // Nintendo 3DS
        "n64",                  // Nintendo 64
        "naomi",                // Sega NAOMI
        "nds",                  // Nintendo DS
        "neogeo",               // SNK Neo Geo
        "neogeocd",             // SNK Neo Geo CD
        "nes",                  // Nintendo Entertainment System
        "ngp",                  // SNK Neo Geo Pocket
        "ngpc",                 // SNK Neo Geo Pocket Color
        "odyssey2",             // Magnavox Odyssey2
        "openbor",              // OpenBOR game engine
        "oric",                 // Tangerine Computer Systems Oric
        "palm",                 // Palm OS
        "pc",                   // IBM PC
        "pc88",                 // NEC PC-8800 series
        "pc98",                 // NEC PC-9800 series
        "pcengine",             // NEC PC Engine
        "pcenginecd",           // NEC PC Engine CD
        "pcfx",                 // NEC PC-FX
        "pokemini",             // Nintendo Pokémon Mini
        "ps2",                  // Sony PlayStation 2
        "ps3",                  // Sony PlayStation 3
        "ps4",                  // Sony PlayStation 4
        "psp",                  // Sony PlayStation Portable
        "psvita",               // Sony PlayStation Vita
        "psx",                  // Sony PlayStation 1
        "residualvm",           // ResidualVM game engine
        "samcoupe",             // SAM Coupé
        "satellaview",          // Nintendo Satellaview
        "saturn",               // Sega Saturn
        "scummvm",              // ScummVM game engine
        "sega32x",              // Sega Mega Drive 32X
        "segacd",               // Sega CD
        "sg-1000",              // Sega SG-1000
        "snes",                 // Nintendo SNES (Super Nintendo)
        "solarus",              // Solarus game engine
        "spectravideo",         // Spectravideo
        "stratagus",            // Stratagus game engine
        "sufami",               // Bandai SuFami Turbo
        "supergrafx",           // NEC SuperGrafx
        "ti99",                 // Texas Instruments TI-99
        "trs-80",               // Tandy TRS-80
        "uzebox",               // Uzebox
        "vectrex",              // Vectrex
        "videopac",             // Philips Videopac G7000 (Magnavox Odyssey2)
        "virtualboy",           // Nintendo Virtual Boy
        "wii",                  // Nintendo Wii
        "wiiu",                 // Nintendo Wii U
        "wonderswan",           // Bandai WonderSwan
        "wonderswancolor",      // Bandai WonderSwan Color
        "x1",                   // Sharp X1
        "x68000",               // Sharp X68000
        "xbox",                 // Microsoft Xbox
        "xbox360",              // Microsoft Xbox 360
        "zmachine",             // Infocom Z-machine
        "zx81",                 // Sinclair ZX81
        "zxspectrum",           // Sinclair ZX Spectrum

        "ignore",  // Do not allow scraping for this system.
        "invalid"
    };

    PlatformId getPlatformId(const std::string& str)
    {
        if (str == "")
            return PLATFORM_UNKNOWN;

        for (unsigned int i = 1; i < PLATFORM_COUNT; i++) {
            if (platformNames[i] == str)
                return (PlatformId)i;
        }

        return PLATFORM_UNKNOWN;
    }

    const std::string getPlatformName(PlatformId id)
    {
        return platformNames[id];
    }
}

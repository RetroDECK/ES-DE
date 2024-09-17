//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  PlatformId.cpp
//
//  Index of all supported systems/platforms.
//

#include "PlatformId.h"

#include <string.h>
#include <vector>

namespace PlatformIds
{
    // clang-format off
    std::vector<std::string> platformNames {
        "unknown", // Nothing set.

        "3do",                  // 3DO Interactive Multiplayer
        "adam",                 // Coleco Adam
        "amiga",                // Commodore Amiga
        "amigacd32",            // Commodore Amiga CD32
        "amstradcpc",           // Amstrad CPC
        "android",              // Google Android
        "apple2",               // Apple II
        "apple2gs",             // Apple IIGS
        "arcade",               // Arcade
        "arcadia",              // Emerson Arcadia 2001
        "archimedes",           // Acorn Archimedes
        "arduboy",              // Arduboy Miniature Game System
        "astrocde",             // Bally Astrocade
        "atari2600",            // Atari 2600
        "atari5200",            // Atari 5200
        "atari7800",            // Atari 7800
        "atari800",             // Atari 800
        "atarijaguar",          // Atari Jaguar
        "atarijaguarcd",        // Atari Jaguar CD
        "atarilynx",            // Atari Lynx
        "atarist",              // Atari ST/STE/Falcon
        "atarixe",              // Atari XE
        "atomiswave",           // Sammy Corporation Atomiswave
        "bbcmicro",             // Acorn Computers BBC Micro
        "c64",                  // Commodore 64
        "cdimono1",             // Philips CD-i
        "cdtv",                 // Commodore CDTV
        "channelf",             // Fairchild Channel F
        "coco",                 // Tandy Color Computer
        "colecovision",         // Coleco ColecoVision
        "crvision",             // VTech CreatiVision
        "daphne",               // Daphne Arcade Laserdisc Emulator
        "dos",                  // DOS (PC)
        "dragon32",             // Dragon Data Dragon 32
        "dreamcast",            // Sega Dreamcast
        "easyrpg",              // EasyRPG eame engine
        "electron",             // Acorn Electron
        "famicom",              // Nintendo Family Computer
        "fds",                  // Nintendo Famicom Disk System
        "flash",                // Adobe Flash
        "fm7",                  // Fujitsu FM-7
        "fmtowns",              // Fujitsu FM Towns
        "fpinball",             // Future Pinball
        "gamate",               // Bit Corporation Gamate
        "gameandwatch",         // Nintendo Game and Watch
        "gamecom",              // Tiger Electronics Game.com
        "gamegear",             // Sega Game Gear
        "gb",                   // Nintendo Game Boy
        "gba",                  // Nintendo Game Boy Advance
        "gbc",                  // Nintendo Game Boy Color
        "gc",                   // Nintendo GameCube
        "genesis",              // Sega Genesis
        "gmaster",              // Hartung Game Master
        "gx4000",               // Amstrad GX4000
        "intellivision",        // Mattel Electronics Intellivision
        "lcdgames",             // LCD Handheld Games
        "love",                 // ChaiLove Game Engine
        "lowresnx",             // LowRes NX Fantasy Console
        "lutro",                // Lutro Game Engine
        "macintosh",            // Apple Macintosh
        "mastersystem",         // Sega Master System
        "megadrive",            // Sega Mega Drive
        "megaduck",             // Creatronic Mega Duck
        "mess",                 // Multi Emulator Super System
        "moto",                 // Thomson MO/TO series
        "msx",                  // MSX
        "msx2",                 // MSX2
        "msxturbor",            // MSX Turbo R
        "mugen",                // M.U.G.E.N Game Engine
        "n3ds",                 // Nintendo 3DS
        "n64",                  // Nintendo 64
        "naomi",                // Sega NAOMI
        "nds",                  // Nintendo DS
        "neogeo",               // SNK Neo Geo
        "neogeocd",             // SNK Neo Geo CD
        "nes",                  // Nintendo Entertainment System
        "ngage",                // Nokia N-Gage
        "ngp",                  // SNK Neo Geo Pocket
        "ngpc",                 // SNK Neo Geo Pocket Color
        "odyssey2",             // Magnavox Odyssey2 / Philips Videopac G7000
        "openbor",              // OpenBOR Game Engine
        "oric",                 // Tangerine Computer Systems Oric
        "palm",                 // Palm OS
        "pc",                   // IBM PC
        "pc88",                 // NEC PC-8800 Series
        "pc98",                 // NEC PC-9800 Series
        "pcengine",             // NEC PC Engine / TurboGrafx-16
        "pcenginecd",           // NEC PC Engine CD / TurboGrafx-CD
        "pcfx",                 // NEC PC-FX
        "pcwindows",            // Microsoft Windows
        "pico8",                // PICO-8 Fantasy Console
        "plus4",                // Commodore Plus/4
        "pokemini",             // Nintendo Pokémon Mini
        "ps2",                  // Sony PlayStation 2
        "ps3",                  // Sony PlayStation 3
        "ps4",                  // Sony PlayStation 4
        "psp",                  // Sony PlayStation Portable
        "psvita",               // Sony PlayStation Vita
        "psx",                  // Sony PlayStation 1
        "pv1000",               // Casio PV-1000
        "residualvm",           // ResidualVM Game Engine
        "samcoupe",             // MGT SAM Coupé
        "satellaview",          // Nintendo Satellaview
        "saturn",               // Sega Saturn
        "scummvm",              // ScummVM Game Engine
        "scv",                  // Epoch Super Cassette Vision
        "sega32x",              // Sega Mega Drive 32X
        "segacd",               // Sega CD
        "sg-1000",              // Sega SG-1000
        "sgb",                  // Nintendo Super Game Boy
        "snes",                 // Nintendo SNES (Super Nintendo)
        "snes-msu1",            // Nintendo SNES MSU-1
        "solarus",              // Solarus Game Engine
        "spectravideo",         // Spectravideo
        "steam",                // Valve Steam
        "sufami",               // Bandai SuFami Turbo
        "supergrafx",           // NEC SuperGrafx
        "supervision",          // Watara Supervision
        "supracan",             // Funtech Super A'Can
        "switch",               // Nintendo Switch
        "ti99",                 // Texas Instruments TI-99
        "tic80",                // TIC-80 Fantasy Computer
        "trs-80",               // Tandy TRS-80
        "uzebox",               // Uzebox Open Source Console
        "vectrex",              // GCE Vectrex
        "vic20",                // Commodore VIC-20
        "virtualboy",           // Nintendo Virtual Boy
        "vpinball",             // Visual Pinball
        "vsmile",               // VTech V.Smile
        "wasm4",                // WASM-4 Fantasy Console
        "wii",                  // Nintendo Wii
        "wiiu",                 // Nintendo Wii U
        "windows3x",            // Microsoft Windows 3.x
        "wonderswan",           // Bandai WonderSwan
        "wonderswancolor",      // Bandai WonderSwan Color
        "x1",                   // Sharp X1
        "x68000",               // Sharp X68000
        "xbox",                 // Microsoft Xbox
        "xbox360",              // Microsoft Xbox 360
        "zmachine",             // Infocom Z-machine
        "zx81",                 // Sinclair ZX81
        "zxnext",               // Sinclair ZX Spectrum Next
        "zxspectrum",           // Sinclair ZX Spectrum

        "ignore",  // Do not allow scraping for this system.
        "invalid"
    };
    // clang-format on

    PlatformId getPlatformId(const std::string& str)
    {
        if (str == "")
            return PLATFORM_UNKNOWN;

        for (unsigned int i = 1; i < PLATFORM_COUNT; ++i) {
            if (platformNames[i] == str)
                return (PlatformId)i;
        }

        return PLATFORM_UNKNOWN;
    }

    const std::string getPlatformName(PlatformId id)
    {
        if (id > platformNames.size() - 1)
            return "unknown";

        // Return the platform name.
        return platformNames[id];
    }

} // namespace PlatformIds

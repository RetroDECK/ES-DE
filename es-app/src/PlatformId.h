//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  PlatformId.h
//
//  Index of all supported systems/platforms.
//

#ifndef ES_APP_PLATFORM_ID_H
#define ES_APP_PLATFORM_ID_H

#include <string>

namespace PlatformIds
{
    enum PlatformId : unsigned int {
        PLATFORM_UNKNOWN = 0,

        THREEDO, // Names can't start with a number.
        ADVENTUREGAMESTUDIO,
        COMMODORE_AMIGA,
        COMMODORE_AMIGA_CD32,
        AMSTRAD_CPC,
        APPLE_II,
        APPLE_IIGS,
        ARCADE,
        BALLY_ASTROCADE,
        ATARI_2600,
        ATARI_5200,
        ATARI_7800,
        ATARI_800,
        ATARI_JAGUAR,
        ATARI_JAGUAR_CD,
        ATARI_LYNX,
        ATARI_ST,
        ATARI_XE,
        ATOMISWAVE,
        BBC_MICRO,
        COMMODORE_64,
        CAVESTORY,
        COMMODORE_CDTV,
        FAIRCHILD_CHANNELF,
        TANDY_COLOR_COMPUTER,
        COLECOVISION,
        DAPHNE,
        DOS,
        DRAGON32,
        SEGA_DREAMCAST,
        NINTENDO_FAMICOM,
        NINTENDO_FAMICOM_DISK_SYSTEM,
        NINTENDO_GAME_AND_WATCH,
        SEGA_GAME_GEAR,
        NINTENDO_GAME_BOY,
        NINTENDO_GAME_BOY_ADVANCE,
        NINTENDO_GAME_BOY_COLOR,
        NINTENDO_GAMECUBE,
        SEGA_GENESIS,
        AMSTRAD_GX4000,
        INTELLIVISION,
        LOVE,
        LUTRO,
        APPLE_MACINTOSH,
        SEGA_MASTER_SYSTEM,
        SEGA_MEGA_DRIVE,
        MESS,
        MOONLIGHT,
        THOMSON_MOTO,
        MSX,
        NINTENDO_3DS,
        NINTENDO_64,
        SEGA_NAOMI,
        NINTENDO_DS,
        SNK_NEO_GEO,
        SNK_NEO_GEO_CD,
        NINTENDO_ENTERTAINMENT_SYSTEM,
        SNK_NEO_GEO_POCKET,
        SNK_NEO_GEO_POCKET_COLOR,
        VIDEOPAC_ODYSSEY2,
        OPENBOR,
        TANGERINE_ORIC,
        PALM_OS,
        PC,
        NEC_PC_ENGINE,
        NEC_PC_ENGINE_CD,
        NEC_PCFX,
        NINTENDO_POKEMON_MINI,
        SONY_PLAYSTATION_2,
        SONY_PLAYSTATION_3,
        SONY_PLAYSTATION_4,
        SONY_PLAYSTATION_PORTABLE,
        SONY_PLAYSTATION_VITA,
        SONY_PLAYSTATION,
        RESIDUALVM,
        SAMCOUPE,
        NINTENDO_SATELLAVIEW,
        SEGA_SATURN,
        SCUMMVM,
        SEGA_32X,
        SEGA_CD,
        SEGA_SG1000,
        SUPER_NINTENDO,
        SOLARUS,
        SPECTRAVIDEO,
        STRATAGUS,
        BANDAI_SUFAMI_TURBO,
        NEC_SUPERGRAFX,
        TEXAS_INSTRUMENTS_TI99,
        TANDY_TRS80,
        UZEBOX,
        VECTREX,
        PHILIPS_VIDEOPAC,
        NINTENDO_VIRTUAL_BOY,
        NINTENDO_WII,
        NINTENDO_WII_U,
        BANDAI_WONDERSWAN,
        BANDAI_WONDERSWAN_COLOR,
        SHARP_X1,
        SHARP_X68000,
        MICROSOFT_XBOX,
        MICROSOFT_XBOX_360,
        SINCLAIR_Z_MACHINE,
        SINCLAIR_ZX81_SINCLAR,
        SINCLAIR_ZX_SPECTRUM,

        PLATFORM_IGNORE, // Do not allow scraping for this system.
        PLATFORM_COUNT
    };

    PlatformId getPlatformId(const std::string& str);
    const std::string getPlatformName(PlatformId id);
}

#endif // ES_APP_PLATFORM_ID_H

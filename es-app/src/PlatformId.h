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
        AMIGA,
        AMIGACD32,
        AMSTRAD_CPC,
        APPLE_II,
        APPLE_IIGS,
        ARCADE,
        ASTROCADE,
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
        TANDY,
        COLECOVISION,
        DAPHNE,
        DOS,
        DRAGON32,
        SEGA_DREAMCAST,
        FAMICOM,
        FAMICOM_DISK_SYSTEM,
        NINTENDO_GAME_AND_WATCH,
        NINTENDO_GAMECUBE,
        SEGA_GAME_GEAR,
        GAME_BOY,
        GAME_BOY_ADVANCE,
        GAME_BOY_COLOR,
        SEGA_GENESIS,
        AMSTRAD_GX4000,
        INTELLIVISION,
        LOVE,
        LUTRO,
        MAC_OS,
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
        NEOGEO,
        NEOGEO_CD,
        NINTENDO_ENTERTAINMENT_SYSTEM,
        NEOGEO_POCKET,
        NEOGEO_POCKET_COLOR,
        VIDEOPAC_ODYSSEY2,
        OPENBOR,
        ORIC,
        PALM_OS,
        PC,
        TURBOGRAFX_16,
        TURBOGRAFX_CD,
        PCFX,
        NINTENDO_POKEMON_MINI,
        PLAYSTATION_2,
        PLAYSTATION_3,
        PLAYSTATION_4,
        PLAYSTATION_PORTABLE,
        PLAYSTATION_VITA,
        PLAYSTATION,
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
        SUFAMI_TURBO,
        SUPERGRAFX,
        TI99,
        TRS80_COLOR_COMPUTER,
        UZEBOX,
        VECTREX,
        VIDEOPAC,
        NINTENDO_VIRTUAL_BOY,
        NINTENDO_WII,
        NINTENDO_WII_U,
        WONDERSWAN,
        WONDERSWAN_COLOR,
        SHARP_X1,
        SHARP_X68000,
        XBOX,
        XBOX_360,
        ZMACHINE,
        ZX81_SINCLAR,
        ZX_SPECTRUM,

        PLATFORM_IGNORE, // Do not allow scraping for this system.
        PLATFORM_COUNT
    };

    PlatformId getPlatformId(const std::string& str);
    const std::string getPlatformName(PlatformId id);
}

#endif // ES_APP_PLATFORM_ID_H

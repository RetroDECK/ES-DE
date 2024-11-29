# ES-DE Frontend - Haiku documentation

ES-DE is available via HaikuDepot but if you still want to build it yourself then you can find detailed instructions in the _Building on Haiku_ section of the [INSTALL.md](INSTALL.md#building-on-haiku) document.

Note that support for Haiku is currently experimental as the operating system itself is experimental.

Table of contents:

[[_TOC_]]

## Known ES-DE problems

* Key repeat doesn't work in text editing fields (but it works elsewhere in the application)

* There is no 3D acceleration as the operating system does not support that

* ES-DE may crash or behave strange when used on Haiku Nightly (i.e. it's due to operating system issues, make sure to run R1/beta5 which should work fine)

## Emulator problems

In contrast with all other platforms which ES-DE runs on, on Haiku emulators which are not working correctly are still included in the configuration. This is done with the belief that things will improve in the future as the operating system matures.

### Atari800

Can't run compressed game files such as those with the .zip extension, and does not seem to be able to correctly emulate any games even if they are uncompressed? (The emulator starts but the games don't.)

### Beetle Lynx

Games don't start, just displays a black screen.

### Beetle PSX HW

Crashes on game start.

### blueMSX

Can't run compressed game files such as those with the .zip extension.

### bsnes

Can't run compressed game files such as those with the .zip extension.

### Caprice32

Can't run compressed game files such as those with the .zip extension.

### DeSmuME

Can't run compressed game files such as those with the .zip extension.

### DOSBox-X (Standalone)

Games can only be launched if ES-DE has been started from the command line, i.e. from a _Terminal_ window. And when existing a game the OS screen resolution is sometimes not reset back to its previous state meaning it has to be manually set to the correct resolution using the operating system's _Screen_ utility.

### EasyRPG

Crashes on game start.

### Flycast

Too slow to be usable in practice, probably due to lack of 3D acceleration.

### fMSX

Can't run compressed game files such as those with the .zip extension.

### FreeIntv

Can't run compressed game files such as those with the .zip extension.

### FS-UAE (Standalone)

This emulator does not seem to accept command-line arguments, meaning games can't be launched from ES-DE.

### Genesis Plus GX Wide

Can't run compressed game files such as those with the .zip extension (it works fine in Genesis Plus GX).

### gpSP

Can't run compressed game files such as those with the .zip extension.

### Hatari

Can't run compressed game files such as those with the .zip extension, and IPF files are not supported.

### MAME (Standalone)

When existing a game the OS screen resolution is sometimes not reset back to its previous state meaning it has to be manually set to the correct resolution using the operating system's _Screen_ utility.

### melonDS

Crashes on game start.

### melonDS (Standalone)

Crashes on game start if attempting to launch a zipped game file.

### Mupen64Plus-Next

Crashes on game start.

### PCSX ReARMed

Games don't run, emulator instantly exits.

### PUAE

Crashes on game start.

### ScummVM (Standalone)

Games can only be launched if ES-DE has been started from the command line, i.e. from a _Terminal_ window.

### Stella

Crashes on game start (Stella 2014 works fine).

### ZEsarUX

Crashes on game start.

## Supported game systems

The **@** symbol indicates that the emulator is _deprecated_ and will be removed in a future ES-DE release.

| System name           | Full name                                      | Default emulator                  | Alternative emulators             | Needs BIOS   | Recommended game setup               |
| :-------------------- | :--------------------------------------------- | :-------------------------------- | :-------------------------------- | :----------- | :----------------------------------- |
| 3do                   | 3DO Interactive Multiplayer                    | Opera                             |                                   | Yes          |                                      |
| adam                  | Coleco Adam                                    | MAME [Diskette] **(Standalone)**  | MAME [Tape] **(Standalone)**,<br>MAME [Cartridge] **(Standalone)**,<br>MAME [Software list] **(Standalone)** | Yes          |                                      |
| ags                   | Adventure Game Studio Game Engine              | _Placeholder_                     |                                   |              |                                      |
| amiga                 | Commodore Amiga                                | PUAE                              | FS-UAE **(Standalone)**           | Yes          | See the specific _Commodore Amiga and CDTV_ section in the user guide |
| amiga1200             | Commodore Amiga 1200                           | PUAE                              | FS-UAE **(Standalone)**           | Yes          | See the specific _Commodore Amiga and CDTV_ section in the user guide |
| amiga600              | Commodore Amiga 600                            | PUAE                              | FS-UAE **(Standalone)**           | Yes          | See the specific _Commodore Amiga and CDTV_ section in the user guide |
| amigacd32             | Commodore Amiga CD32                           | PUAE                              | FS-UAE **(Standalone)**           | Yes          | See the specific _Commodore Amiga and CDTV_ section in the user guide |
| amstradcpc            | Amstrad CPC                                    | Caprice32                         | MAME **(Standalone)**             | Yes for MAME | Single archive or disk file          |
| android               | Google Android                                 | _Placeholder_                     |                                   |              |                                      |
| androidapps           | Android Apps                                   | _Placeholder_                     |                                   |              |                                      |
| androidgames          | Android Games                                  | _Placeholder_                     |                                   |              |                                      |
| apple2                | Apple II                                       | Mednafen **(Standalone)**         | MAME **(Standalone)**             | Yes          | See the specific _Apple II_ section in the user guide |
| apple2gs              | Apple IIGS                                     | MAME **(Standalone)**             |                                   | Yes          | See the specific _Apple IIGS_ section in the user guide |
| arcade                | Arcade                                         | MAME 2003-Plus                    | MAME **(Standalone)**,<br>FinalBurn Neo,<br>FinalBurn Neo **(Standalone)**,<br>Geolith,<br>Flycast,<br> _Script_ | Depends      | See the specific _Arcade and Neo Geo_ section in the user guide |
| arcadia               | Emerson Arcadia 2001                           | MAME **(Standalone)**             |                                   | No           | Single archive or ROM file           |
| archimedes            | Acorn Archimedes                               | MAME [Model A440/1] **(Standalone)** | MAME [Model A3000] **(Standalone)**,<br>MAME [Model A310] **(Standalone)**,<br>MAME [Model A540] **(Standalone)** | Yes          |                                      |
| arduboy               | Arduboy Miniature Game System                  | _Placeholder_                     |                                   |              |                                      |
| astrocde              | Bally Astrocade                                | MAME **(Standalone)**             |                                   | Yes          | Single archive or ROM file           |
| atari2600             | Atari 2600                                     | Stella                            | Stella 2014                       | No           | Single archive or ROM file           |
| atari5200             | Atari 5200                                     | Atari800                          |                                   | Yes          | Single archive or ROM file           |
| atari7800             | Atari 7800 ProSystem                           | MAME **(Standalone)**             |                                   | Yes          | Single archive or ROM file           |
| atari800              | Atari 800                                      | Atari800                          |                                   | Yes          |                                      |
| atarijaguar           | Atari Jaguar                                   | Virtual Jaguar                    | MAME **(Standalone)**             | Yes for MAME | See the specific _Atari Jaguar and Atari Jaguar CD_ section in the user guide |
| atarijaguarcd         | Atari Jaguar CD                                | _Placeholder_                     |                                   |              |                                      |
| atarilynx             | Atari Lynx                                     | Handy                             | Beetle Lynx,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file           |
| atarist               | Atari ST [also STE and Falcon]                 | Hatari                            |                                   | Yes          | Single archive or image file for single-diskette games, .m3u playlist for multi-diskette games |
| atarixe               | Atari XE                                       | Atari800                          |                                   | Yes          |                                      |
| atomiswave            | Sammy Corporation Atomiswave                   | Flycast                           |                                   | Yes          | Single archive  file                 |
| bbcmicro              | Acorn Computers BBC Micro                      | MAME **(Standalone)**             |                                   | Yes          | Single archive or diskette image file |
| c64                   | Commodore 64                                   | VICE x64sc Accurate               | VICE x64 Fast,<br>VICE x64 SuperCPU,<br>VICE x128 | No           | Single archive or image file for tape, cartridge or single-diskette games, .m3u playlist for multi-diskette games |
| cdimono1              | Philips CD-i                                   | MAME **(Standalone)**             |                                   | Yes          |                                      |
| cdtv                  | Commodore CDTV                                 | PUAE                              | FS-UAE **(Standalone)**           | Yes          | See the specific _Commodore Amiga and CDTV_ section in the user guide |
| chailove              | ChaiLove Game Engine                           | _Placeholder_                     |                                   |              |                                      |
| channelf              | Fairchild Channel F                            | MAME **(Standalone)**             |                                   | Yes          | Single archive or ROM file           |
| coco                  | Tandy Color Computer                           | MAME [Cartridge] **(Standalone)** | MAME [Tape] **(Standalone)**      | Yes          | See the specific _Tandy Color Computer_ section in the user guide |
| colecovision          | Coleco ColecoVision                            | blueMSX                           | Gearcoleco                        | Yes          | Single archive or ROM file           |
| consolearcade         | Console Arcade Systems                         | MAME **(Standalone)**             | Flycast,<br>Mednafen [Sega Saturn] **(Standalone)**,<br> _Script_ | Depends      | See the specific _Console Arcade Systems_ section in the user guide |
| cps                   | Capcom Play System                             | MAME 2003-Plus                    | MAME **(Standalone)**,<br>FinalBurn Neo,<br>FinalBurn Neo **(Standalone)** | Depends      | See the specific _Arcade and Neo Geo_ section in the user guide |
| cps1                  | Capcom Play System I                           | MAME 2003-Plus                    | MAME **(Standalone)**,<br>FinalBurn Neo,<br>FinalBurn Neo **(Standalone)** | Depends      | See the specific _Arcade and Neo Geo_ section in the user guide |
| cps2                  | Capcom Play System II                          | MAME 2003-Plus                    | MAME **(Standalone)**,<br>FinalBurn Neo,<br>FinalBurn Neo **(Standalone)** | Depends      | See the specific _Arcade and Neo Geo_ section in the user guide |
| cps3                  | Capcom Play System III                         | MAME 2003-Plus                    | MAME **(Standalone)**,<br>FinalBurn Neo,<br>FinalBurn Neo **(Standalone)** | Depends      | See the specific _Arcade and Neo Geo_ section in the user guide |
| crvision              | VTech CreatiVision                             | MAME **(Standalone)**             |                                   | Yes          | Single archive or ROM file           |
| daphne                | Daphne Arcade LaserDisc Emulator               | MAME **(Standalone)**             |                                   | Depends      | See the specific _LaserDisc Games_ section in the user guide |
| desktop               | Desktop Applications                           | _Suspend ES-DE_                   | _Keep ES-DE running_              | No           | See the specific _Ports and desktop applications_ section in the user guide |
| doom                  | Doom                                           | _Script_                          |                                   | No           |                                      |
| dos                   | DOS (PC)                                       | DOSBox-Pure                       | DOSBox,<br>DOSBox-X **(Standalone)** | No           | See the specific _DOS / PC_ section in the user guide |
| dragon32              | Dragon Data Dragon 32                          | MAME Dragon 32 [Tape] **(Standalone)** | MAME Dragon 32 [Cartridge] **(Standalone)**,<br>MAME Dragon 64 [Tape] **(Standalone)**,<br>MAME Dragon 64 [Cartridge] **(Standalone)** | Yes          | See the specific _Dragon 32 and Tano Dragon_ section in the user guide |
| dreamcast             | Sega Dreamcast                                 | Flycast                           |                                   | No           | In separate folder interpreted as a file, with .m3u playlist if multi-disc game |
| easyrpg               | EasyRPG Game Engine                            | EasyRPG                           |                                   | No           | See the specific _EasyRPG Game Engine_ section in the user guide |
| electron              | Acorn Electron                                 | MAME [Tape] **(Standalone)**      | MAME [Diskette DFS] **(Standalone)**,<br>MAME [Diskette ADFS] **(Standalone)** | Yes          | Single archive, or single tape or diskette image file |
| emulators             | Emulators                                      | _Suspend ES-DE_                   | _Keep ES-DE running_              | No           | See the specific _Ports and desktop applications_ section in the user guide |
| epic                  | Epic Games Store                               | _Placeholder_                     |                                   |              |                                      |
| famicom               | Nintendo Family Computer                       | Mesen                             | Nestopia UE,<br>FCEUmm,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file           |
| fba                   | FinalBurn Alpha                                | _Placeholder_                     |                                   |              |                                      |
| fbneo                 | FinalBurn Neo                                  | FinalBurn Neo                     | FinalBurn Neo **(Standalone)**    | Yes          | See the specific _Arcade and Neo Geo_ section in the user guide |
| fds                   | Nintendo Famicom Disk System                   | Mesen                             | Nestopia UE,<br>FCEUmm,<br>Mednafen **(Standalone)** | Yes          | Single archive or ROM file           |
| flash                 | Adobe Flash                                    | _Placeholder_                     |                                   |              |                                      |
| fm7                   | Fujitsu FM-7                                   | MAME [FM-7 Diskette] **(Standalone)** | MAME [FM-7 Tape] **(Standalone)**,<br>MAME [FM-7 Software list] **(Standalone)**,<br>MAME [FM77AV Diskette] **(Standalone)**,<br>MAME [FM77AV Tape] **(Standalone)**,<br>MAME [FM77AV Software list] **(Standalone)** | Yes          | For tape files you need to manually start the cassette player from the MAME menu after the "load" command, as well as entering the "run" command after loading is complete |
| fmtowns               | Fujitsu FM Towns                               | MAME **(Standalone)**             |                                   | Yes          | See the specific _Fujitsu FM Towns_ section in the user guide  |
| fpinball              | Future Pinball                                 | _Placeholder_                     |                                   |              |                                      |
| gamate                | Bit Corporation Gamate                         | MAME **(Standalone)**             |                                   | Yes          | Single archive or ROM file           |
| gameandwatch          | Nintendo Game and Watch                        | MAME Local Artwork **(Standalone)** | MAME **(Standalone)**,<br>Handheld Electronic (GW) | No           | See the specific _LCD handheld games_ section in the user guide |
| gamecom               | Tiger Electronics Game.com                     | MAME **(Standalone)**             |                                   | Yes          | Single archive or ROM file           |
| gamegear              | Sega Game Gear                                 | Genesis Plus GX                   | Genesis Plus GX Wide,<br>Gearsystem,<br>PicoDrive,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file     |
| gb                    | Nintendo Game Boy                              | Gambatte                          | SameBoy,<br>Gearboy,<br>mGBA,<br>mGBA **(Standalone)**,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file |
| gba                   | Nintendo Game Boy Advance                      | mGBA                              | mGBA **(Standalone)**,<br>VBA Next,<br>gpSP,<br>Mednafen **(Standalone)** | No                 | Single archive or ROM file |
| gbc                   | Nintendo Game Boy Color                        | Gambatte                          | SameBoy,<br>Gearboy,<br>mGBA,<br>mGBA **(Standalone)**,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file |
| gc                    | Nintendo GameCube                              | _Placeholder_                     |                                   |              |                                      |
| genesis               | Sega Genesis                                   | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive,<br>BlastEm,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file |
| gmaster               | Hartung Game Master                            | MAME **(Standalone)**             |                                   | Yes          | Single archive or ROM file           |
| gx4000                | Amstrad GX4000                                 | Caprice32                         | MAME **(Standalone)**             | No           | Single archive or ROM file           |
| intellivision         | Mattel Electronics Intellivision               | FreeIntv                          | MAME **(Standalone)**             | Yes          | Single archive or ROM file           |
| j2me                  | Java 2 Micro Edition (J2ME)                    | _Placeholder_                     |                                   |              |                                      |
| kodi                  | Kodi Home Theatre Software                     | _Placeholder_                     |                                   |              |                                      |
| laserdisc             | LaserDisc Games                                | MAME **(Standalone)**             |                                   | Depends      | See the specific _LaserDisc Games_ section in the user guide |
| lcdgames              | LCD Handheld Games                             | MAME Local Artwork **(Standalone)** | MAME **(Standalone)**,<br>Handheld Electronic (GW) | No           | See the specific _LCD handheld games_ section in the user guide |
| lowresnx              | LowRes NX Fantasy Console                      | _Placeholder_                     |                                   |              |                                      |
| lutris                | Lutris Open Gaming Platform                    | _Placeholder_                     |                                   |              |                                      |
| lutro                 | Lutro Game Engine                              | _Placeholder_                     |                                   |              |                                      |
| macintosh             | Apple Macintosh                                | MAME Mac SE Bootable **(Standalone)** | MAME Mac SE Boot Disk **(Standalone)**,<br>MAME Mac Plus Bootable **(Standalone)**,<br>MAME Mac Plus Boot Disk **(Standalone)** | Yes          | See the specific _Apple Macintosh_ section in the user guide |
| mame                  | Multiple Arcade Machine Emulator               | MAME 2003-Plus                    | MAME **(Standalone)**,<br>FinalBurn Neo,<br>FinalBurn Neo **(Standalone)**,<br>Geolith,<br>Flycast,<br> _Script_ | Depends      | See the specific _Arcade and Neo Geo_ section in the user guide |
| mame-advmame          | AdvanceMAME                                    | AdvanceMAME **(Standalone)**      |                                   | Depends      | See the specific _Arcade and Neo Geo_ section in the user guide |
| mastersystem          | Sega Master System                             | Genesis Plus GX                   | Genesis Plus GX Wide,<br>Gearsystem,<br>PicoDrive,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file |
| megacd                | Sega Mega-CD                                   | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive | Yes          |                                      |
| megacdjp              | Sega Mega-CD [Japan]                           | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive | Yes          |                                      |
| megadrive             | Sega Mega Drive                                | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive,<br>BlastEm,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file |
| megadrivejp           | Sega Mega Drive [Japan]                        | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive,<br>BlastEm,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file |
| megaduck              | Creatronic Mega Duck                           | _Placeholder_                     |                                   |              |                                      |
| mess                  | Multi Emulator Super System                    | _Placeholder_                     |                                   |              |                                      |
| model2                | Sega Model 2                                   | _Placeholder_                     |                                   |              |                                      |
| model2                | Sega Model 2                                   | MAME **(Standalone)**             |                                   | Yes          | See the specific _Arcade and Neo Geo_ section in the user guide  |
| model3                | Sega Model 3                                   | _Placeholder_                     |                                   |              |                                      |
| moto                  | Thomson MO/TO Series                           | _Placeholder_                     |                                   |              |                                      |
| msx                   | MSX                                            | blueMSX                           | fMSX                              | Yes          |                                      |
| msx1                  | MSX1                                           | blueMSX                           | fMSX                              | Yes          |                                      |
| msx2                  | MSX2                                           | blueMSX                           | fMSX                              | Yes          |                                      |
| msxturbor             | MSX Turbo R                                    | blueMSX                           |                                   | Yes          |                                      |
| mugen                 | M.U.G.E.N Game Engine                          | _Placeholder_                     |                                   |              |                                      |
| multivision           | Othello Multivision                            | Gearsystem                        |                                   | No           | Single archive or ROM file           |
| n3ds                  | Nintendo 3DS                                   | _Placeholder_                     |                                   |              |                                      |
| n64                   | Nintendo 64                                    | Mupen64Plus-Next                  | Mupen64Plus **(Standalone)**,<br>ParaLLEl N64 | No           | Single archive or ROM file |
| n64dd                 | Nintendo 64DD                                  | ParaLLEl N64                      | Mupen64Plus-Next                  | Yes          | See the specific _Nintendo 64DD_ section in the user guide |
| naomi                 | Sega NAOMI                                     | Flycast                           |                                   | Yes          | Single archive file + .chd file in subdirectory if GD-ROM game |
| naomi2                | Sega NAOMI 2                                   | Flycast                           |                                   | Yes          | Single archive file + .chd file in subdirectory if GD-ROM game |
| naomigd               | Sega NAOMI GD-ROM                              | Flycast                           |                                   | Yes          | Single archive file + .chd file in subdirectory if GD-ROM game |
| nds                   | Nintendo DS                                    | melonDS                           | melonDS **(Standalone)**,<br>DeSmuME | No           | Single archive or ROM file |
| neogeo                | SNK Neo Geo                                    | FinalBurn Neo                     | FinalBurn Neo **(Standalone)**,<br>Geolith,<br>MAME **(Standalone)** | Yes          | See the specific _Arcade and Neo Geo_ section in the user guide |
| neogeocd              | SNK Neo Geo CD                                 | NeoCD                             | FinalBurn Neo,<br>FinalBurn Neo **(Standalone)**,<br>MAME **(Standalone)** | Yes          | .chd (NeoCD and MAME only) or .cue file |
| neogeocdjp            | SNK Neo Geo CD [Japan]                         | NeoCD                             | FinalBurn Neo,<br>FinalBurn Neo **(Standalone)**,<br>MAME **(Standalone)** | Yes          | .chd (NeoCD and MAME only) or .cue file |
| nes                   | Nintendo Entertainment System                  | Mesen                             | Mesen                             | Nestopia UE,<br>FCEUmm,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file           |
| ngage                 | Nokia N-Gage                                   | _Placeholder_                     |                                   |              |                                      |
| ngp                   | SNK Neo Geo Pocket                             | Beetle NeoPop                     | Mednafen **(Standalone)**         | No           | Single archive or ROM file           |
| ngpc                  | SNK Neo Geo Pocket Color                       | Beetle NeoPop                     | Mednafen **(Standalone)**         | No           | Single archive or ROM file           |
| odyssey2              | Magnavox Odyssey 2                             | O2EM                              | MAME **(Standalone)**             | Yes          | Single archive or ROM file           |
| openbor               | OpenBOR Game Engine                            | _Placeholder_                     |                                   |              |                                      |
| oric                  | Tangerine Computer Systems Oric                | MAME **(Standalone)**             |                                   | Yes          | See the specific _Tangerine Computer Systems Oric_ section in the user guide |
| palm                  | Palm OS                                        | _Placeholder_                     |                                   |              |                                      |
| pc                    | IBM PC                                         | DOSBox-Pure                       | DOSBox,<br>DOSBox-X **(Standalone)** | No           | See the specific _DOS / PC_ section in the user guide |
| pc88                  | NEC PC-8800 Series                             | _Placeholder_                     |                                   |              |                                      |
| pc98                  | NEC PC-9800 Series                             | Neko Project II                   |                                   |              |                                      |
| pcarcade              | PC Arcade Systems                              | _Script_                          |                                   | No           |                                      |
| pcengine              | NEC PC Engine                                  | Beetle PCE                        | Beetle PCE FAST,<br>Beetle SuperGrafx,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file |
| pcenginecd            | NEC PC Engine CD                               | Beetle PCE                        | Beetle PCE FAST,<br>Beetle SuperGrafx,<br>Mednafen **(Standalone)** | Yes          |                                      |
| pcfx                  | NEC PC-FX                                      | Beetle PC-FX                      | Mednafen **(Standalone)**         | Yes          |                                      |
| pico8                 | PICO-8 Fantasy Console                         | _Placeholder_                     |                                   |              |                                      |
| plus4                 | Commodore Plus/4                               | VICE xplus4                       |                                   | No           | Single archive or image file for tape, cartridge or single-diskette games, .m3u playlist for multi-diskette games |
| pokemini              | Nintendo Pokémon Mini                          | _Placeholder_                     |                                   |              |                                      |
| ports                 | Ports                                          | _Script_                          | OpenLara (Tomb Raider)            | No           | See the specific _Ports and desktop applications_ section in the user guide |
| ps2                   | Sony PlayStation 2                             | _Placeholder_                     |                                   |              |                                      |
| ps3                   | Sony PlayStation 3                             | _Placeholder_                     |                                   |              |                                      |
| ps4                   | Sony PlayStation 4                             | _Placeholder_                     |                                   |              |                                      |
| psp                   | Sony PlayStation Portable                      | PPSSPP **(Standalone)**           |                                   | No           | Single disc image file               |
| psvita                | Sony PlayStation Vita                          | _Placeholder_                     |                                   |              |                                      |
| psx                   | Sony PlayStation                               | Beetle PSX                        | Beetle PSX HW,<br>PCSX ReARMed,<br>Mednafen **(Standalone)** | Yes          | .chd file for single-disc games, .m3u playlist for multi-disc games |
| pv1000                | Casio PV-1000                                  | MAME **(Standalone)**             |                                   | No           | Single archive or ROM file           |
| quake                 | Quake                                          | TyrQuake                          | _Script_                          | No           |                                      |
| samcoupe              | MGT SAM Coupé                                  | _Placeholder_                     |                                   |              |                                      |
| satellaview           | Nintendo Satellaview                           | Snes9x - Current                  | bsnes                             |              |                                      |
| saturn                | Sega Saturn                                    | Beetle Saturn                     | Yabause,<br>Mednafen **(Standalone)** | Yes          | .chd file for single-disc games, .m3u playlist for multi-disc games |
| saturnjp              | Sega Saturn [Japan]                            | Beetle Saturn                     | Yabause,<br>Mednafen **(Standalone)** | Yes          | .chd file for single-disc games, .m3u playlist for multi-disc games |
| scummvm               | ScummVM Game Engine                            | ScummVM                           | ScummVM **(Standalone)**          | No           | See the specific _ScummVM_ section in the user guide |
| scv                   | Epoch Super Cassette Vision                    | MAME **(Standalone)**             |                                   | Yes          | Single archive or ROM file           |
| sega32x               | Sega Mega Drive 32X                            | PicoDrive                         |                                   | No           | Single archive or ROM file           |
| sega32xjp             | Sega Super 32X [Japan]                         | PicoDrive                         |                                   | No           | Single archive or ROM file           |
| sega32xna             | Sega Genesis 32X [North America]               | PicoDrive                         |                                   | No           | Single archive or ROM file           |
| segacd                | Sega CD                                        | Genesis Plus GX                   | Genesis Plus GX Wide,<br>PicoDrive | Yes          |                                      |
| sfc                   | Nintendo SFC (Super Famicom)                   | Snes9x - Current                  | bsnes,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file            |
| sg-1000               | Sega SG-1000                                   | Genesis Plus GX                   | Genesis Plus GX Wide,<br>Gearsystem,<br>blueMSX | No           | Single archive or ROM file |
| sgb                   | Nintendo Super Game Boy                        | SameBoy                           | mGBA,<br>mGBA **(Standalone)**    |              |  Single archive or ROM file |
| snes                  | Nintendo SNES (Super Nintendo)                 | Snes9x - Current                  | Snes9x - Current                  | bsnes,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file            |
| snesna                | Nintendo SNES (Super Nintendo) [North America] | Snes9x - Current                  | Snes9x - Current                  | bsnes,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file            |
| solarus               | Solarus Game Engine                            | Solarus **(Standalone)**          |                                   | No           | Single .solarus game file |
| spectravideo          | Spectravideo                                   | blueMSX                           |                                   |              |                                      |
| steam                 | Valve Steam                                    | _Placeholder_                     |                                   |              |                                      |
| stv                   | Sega Titan Video Game System                   | MAME **(Standalone)**             | Mednafen **(Standalone)**         | Yes          | Single archive file                  |
| sufami                | Bandai SuFami Turbo                            | Snes9x - Current                  | bsnes                             |              |                                      |
| supergrafx            | NEC SuperGrafx                                 | Beetle SuperGrafx                 | Beetle PCE,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file    |
| supervision           | Watara Supervision                             | _Placeholder_                     |                                   |              |                                      |
| supracan              | Funtech Super A'Can                            | MAME **(Standalone)**             |                                   | Yes          | Single archive or ROM file. You need a supracan.zip archive that contains a valid internal_68k.bin file and an empty file named umc6650.bin |
| switch                | Nintendo Switch                                | _Placeholder_                     |                                   |              |                                      |
| symbian               | Symbian                                        | _Placeholder_                     |                                   |              |                                      |
| tanodragon            | Tano Dragon                                    | MAME [Tape] **(Standalone)**      | MAME [Cartridge] **(Standalone)** | Yes          | See the specific _Dragon 32 and Tano Dragon_ section in the user guide |
| tg16                  | NEC TurboGrafx-16                              | Beetle PCE                        | Beetle PCE FAST,<br>Beetle SuperGrafx,<br>Mednafen **(Standalone)** | No           | Single archive or ROM file |
| tg-cd                 | NEC TurboGrafx-CD                              | Beetle PCE                        | Beetle PCE FAST,<br>Beetle SuperGrafx,<br>Mednafen **(Standalone)** | Yes          |                                      |
| ti99                  | Texas Instruments TI-99                        | MAME **(Standalone)**             |                                   | Yes          | See the specific _Texas Instruments TI-99_ section in the user guide |
| tic80                 | TIC-80 Fantasy Computer                        | _Placeholder_                     |                                   |              |                                      |
| to8                   | Thomson TO8                                    | _Placeholder_                     |                                   |              |                                      |
| triforce              | Namco-Sega-Nintendo Triforce                   | _Placeholder_                     |                                   |              |                                      |
| trs-80                | Tandy TRS-80                                   | _Placeholder_                     |                                   |              |                                      |
| type-x                | Taito Type X                                   | _Placeholder_                     |                                   |              |                                      |
| uzebox                | Uzebox Open Source Console                     | _Placeholder_                     |                                   |              |                                      |
| vectrex               | GCE Vectrex                                    | vecx                              | MAME **(Standalone)**             | Yes for MAME | Single archive or ROM file           |
| vic20                 | Commodore VIC-20                               | VICE xvic                         |                                   | No           | Single archive or tape, cartridge or diskette image file |
| videopac              | Philips Videopac G7000                         | O2EM                              | MAME **(Standalone)**             | Yes          | Single archive or ROM file           |
| virtualboy            | Nintendo Virtual Boy                           | Beetle VB                         | Mednafen **(Standalone)**         | No           |                                      |
| vpinball              | Visual Pinball                                 | _Placeholder_                     |                                   |              |                                      |
| vsmile                | VTech V.Smile                                  | MAME **(Standalone)**             |                                   | Yes          | Single archive or ROM file           |
| wasm4                 | WASM-4 Fantasy Console                         | _Placeholder_                     |                                   |              |                                      |
| wii                   | Nintendo Wii                                   | _Placeholder_                     |                                   |              |                                      |
| wiiu                  | Nintendo Wii U                                 | _Placeholder_                     |                                   |              |                                      |
| windows               | Microsoft Windows                              | _Placeholder_                     |                                   |              |                                      |
| windows3x             | Microsoft Windows 3.x                          | DOSBox-X **(Standalone)**         | DOSBox-Pure,<br> _Script (Suspend ES-DE)_,<br> _Script (Keep ES-DE running)_ | No           | See the specific _Microsoft Windows 3.x and 9x_ section in the user guide |
| windows9x             | Microsoft Windows 9x                           | DOSBox-X **(Standalone)**         | DOSBox-Pure,<br> _Script (Suspend ES-DE)_,<br> _Script (Keep ES-DE running)_ | No           | See the specific _Microsoft Windows 3.x and 9x_ section in the user guide |
| wonderswan            | Bandai WonderSwan                              | Beetle Cygne                      | Mednafen **(Standalone)**         | No           | Single archive or ROM file           |
| wonderswancolor       | Bandai WonderSwan Color                        | Beetle Cygne                      | Mednafen **(Standalone)**         | No           | Single archive or ROM file           |
| x1                    | Sharp X1                                       | MAME [Diskette] **(Standalone)**  | MAME [Tape] **(Standalone)**      | Yes          | Single archive or diskette/tape file |
| x68000                | Sharp X68000                                   | MAME **(Standalone)**             |                                   | Yes          |                                      |
| xbox                  | Microsoft Xbox                                 | _Placeholder_                     |                                   |              |                                      |
| xbox360               | Microsoft Xbox 360                             | _Placeholder_                     |                                   |              |                                      |
| zmachine              | Infocom Z-machine                              | _Placeholder_                     |                                   |              |                                      |
| zx81                  | Sinclair ZX81                                  | EightyOne                         |                                   | No           |                                      |
| zxnext                | Sinclair ZX Spectrum Next                      | ZEsarUX **(Standalone)**          |                                   | No           | In separate folder interpreted as a file |
| zxspectrum            | Sinclair ZX Spectrum                           | Fuse                              | Fuse **(Standalone)**             | No           | Single archive or ROM file           |

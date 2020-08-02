# EmulationStation Desktop Edition - User Guide

**Note:** This document is intended as a quick start guide, for more in-depth information and details on how to compile EmulationStation and perform more advanced configuration, please refer to the [INSTALL.md](INSTALL.md) document.


**_Note: Currently under construction!_**

[[_TOC_]]

## Getting started

Getting started with EmulationStation is very easy, just make sure to install the software properly, either manually as built from source code or using one of the supplied packages. On Windows you'll use the installer instead of a package.

The installation procedure will not be covered here as it differs between operating system, so please refer to your operating system documentation for information regarding this topic. EmulationStation Desktop Edition is currently supplied as .deb and .rpm packages for Linux and as a standard NSIS installer for Windows.

The following operating systems have been tested:

* Kubuntu 20.04
* Windows 10 (x86)
* Windows 8.1 (x86)

Upon first startup, ES will create its home directory, by default the location is ~/.emulationstation.

On Unix this defaults to /home/<username>/.emulationstation/ and on Windows it defaults to C:\Users\<username>\.emulationstation\

A settings file, `es_settings.cfg` will be generated with all the default settings, and a `es_systems.cfg` file will also be copied from the program resource folder. This file contains the game ROM and emulator settings and can be modified if needed. For information on how to do this, refer to the [INSTALL.md](INSTALL.md) document.

There's a log file in the home directory as well named `es_log.txt`, please refer to this in case of any errors as it should provide information on what went wrong.

Upon startup with the default settings, ES is set to the gamelist view style `AUTOMATIC`. In this mode the application will look for any game media files (videos and images) and set the view style accordingly. If at least one image is found for any game for a certain system, the view style `DETAILED` will be shown, and if at least one video file is found, the view style `VIDEO` will be selected. The gamelist view styles are described in more detail later in this document.


## Main screen

When starting EmulationStation with the default settings, you will see the main screen first. From here you can navigate your game systems and enter their respective gamelists. If there are no game systems installed, you will not see this screen but rather an error message will be displayed, informing you that no games could be found.

Depending on the theme, the system navigation carousel can be horizontal or vertical. The default theme rbsimple-DE provides horizontal navigation, i.e. you browse your systems be scrolling left or right.


## Help system and general navigation

There is a help system available throughout the application that provides an overview of the possible actions and buttons that can be used. It's possible to disable the help system, but it's enabled by default.


## Gamelist view

The gamelist is where you browse and start the games, and it's where you will spend most of your time when using ES.


## Main menu

This menu can be accessed from both the main screen and from the gamelist views. It contains the scraper, the quit menu as well as the application settings.

For a breakdown of all the settings in the main menu, see the reference section lower in this document.


## Game options menu

This menu is opened from the gamelists, and can't be accessed directly from the main screen. The menu changes slightly depending on the context, namely whether a game file or a folder is selected, and whether the current system is a collection or a normal game platform.

You open this menu by pressing the `Select` key.

Here's a summary of the menu entries:

### JUMP TO..

This provides the ability to quick jump to a certain letter. If the setting to sort favorite games above non-favorites has been selected (it is enabled by default), then it's also possible to jump to the favorites games by choosing the star symbol.

### SORT GAMES BY

This is the sort order for the gamelist. There's is a global sort order setting that can be changed in the main menu, and it's this sort order that is also shown here unless it's been modified during the program sessions. The sort order is persistent per game system throughout the program session.

### FILTER GAMELIST

Choosing this entry opens a separate screen where it's possible to apply a filter for the gamelists, which is persistent throughout the program session, or until the filter is reset. The option to reset the filter is also shown on the separate screen.

### EDIT THIS GAME'S METADATA / EDIT THIS FOLDER'S METADATA

This opens the metadata editor, which will be described in detail below.

### ADD/REMOVE GAMES TO THIS GAME COLLECTION

This is only shown if the system is a collection. This will also be described in more detail below.


## Metadata editor

In the metadata editor, you can modify the metadata for a game, scrape for game info and media files and delete media files or the entire game.

The following entries can be modified:

**NAME**

This is the game that will be shown when browsing the gamelist. If no sortname has been defined, the games are sorted using this field.

**SORTNAME**

This entry makes it possible to change the sorting of a game without having to change its name. For instance it can be used to sort **Mille Miglia** as **1000 Miglia** or **The Punisher** as **Punisher, The**.

**DESCRIPTION**

Usually provided by the scraper although it's possible to update this manually or write your own game description.

**RATING**

Ratings in half-star increments. Can be set as such manually or be scraped, if the scraper service provides ratings (currently only ScreenScraper does).

**RELEASE DATE**

Release date in ISO 8601 format (YYYY-MM-DD).

**DEVELOPER**

Developer of the game.

**PUBLISHER**

Publisher of the game.

**GENRE**

One or multiple genres for the game.

**PLAYERS**

The amount of players the game supports.

**FAVORITE**

A flag to indicate whether this is a favorite. Can also be set directly from the gamelist by using the **Y** key.

**COMPLETED**

A flag to indicate whether you have completed this game.

**BROKEN/NOT WORKING**

A flag to indicate whether the game is broken. Useful for MAME games for instance where future releases may make the game functional.

**HIDDEN**

A flag to indicate the game is hidden. If the corresponding option has been set on the main menu, the game will not be shown. Useful for examle for DOS games to hide batch scripts, configuration tools etc.

**KIDGAME**

A flag to mark whether the game is suitable for children. This will be applied as a filter when starting ES in 'Kid mode'.

**COUNT AS GAME**

A flag to indicate whether the game should be counted. It's only used for the game system counter on the main screen, but is quite useful for multi-file games such multi-disk Amiga or Commodore 64 games, or for DOS games configuration executables that you want to keep in ES and therefore can't hide.

**LAUNCH COMMAND**

Here you can override the launch command for the game, for example to use a different emulator than the default for the game system. Very useful for MAME/arcade games.

**PLAY COUNT**

A statistics counter that counts how many times you're played the game. You normally don't need to touch this, but if you want to, the possibility is there.


## Scraper

The scraper supports downloading of game metadata and media files from the Internet. Currently two scraper services are supported, ScreenScraper.fr and TheGamesDB.net.


## Getting your games into EmulationStation

For most systems, this is very straightforward, just put your game files into the folder corresponding to the system name. These names can be found in the end of this document.



# Reference information

## Main menu entries

### SCRAPER

**SCRAPE FROM**

**FILTER**

**SYSTEMS**

#### CONTENT SETTINGS

**SCRAPE GAME NAMES**

**SCRAPE RATINGS**

**SCRAPE OTHER METADATA**

**SCRAPE SCREENSHOT IMAGES**

**SCRAPE BOX COVER IMAGES**

**SCRAPE MARQUEE (WHEEL) IMAGES**

**SCRAPE 3D BOX IMAGES**

#### OTHER SETTINGS

**REGION**

**LANGUAGE**

**OVERWRITE FILES AND DATA**

**INTERACTIVE MODE**

**AUTO-ACCEPT SINGLE GAME MATCHES**


### UI SETTINGS

**GAMELIST TO SHOW ON STARTUP**

**GAMELIST VIEW STYLE**

**TRANSITION STYLE**

**THEME SET**

**UI MODE**

**DEFAULT SORT ORDER**

**SORT FOLDERS ON TOP OF GAMELISTS**

**SORT FAVORITE GAMES ABOVE NON-FAVORITES**

**GAMELIST FILTERS**

**QUICK SYSTEM SELECT**

**CAROUSEL TRANSITIONS**

**ON-SCREEN HELP**

**SHOW START MENU IN KID MODE**

#### SCREENSAVER SETTINGS

**SCREENSAVER AFTER**

**SCREENSAVER CONTROLS**

**SCREENSAVER BEHAVIOR**

#### VIDEO SCRENSAVER SETTINGS

**SWAP VIDEOS AFTER (SECS)**

**STRETCH VIDEOS TO SCREEN RESOLUTION**

**PLAY AUDIO FOR SCREENSAVER VIDEO FILES**

#### SLIDESHOW SCREENSAVER SETTINGS

**SWAP IMAGES AFTER (SECS)**

**STRETCH IMAGES TO SCREEN RESOLUTION**

**BACKGROUND AUDIO**

**USE CUSTOM IMAGES**

**CUSTOM IMAGE DIR**

**CUSTOM IMAGE DIR RECURSIVE**

**CUSTOM IMAGE FILTER**


### SOUND SETTINGS

**SYSTEM VOLUME**

**PLAY AUDIO FOR VIDEO FILES IN GAMELIST VIEWS**

**NAVIGATION SOUNDS**


### GAME COLLECTION SETTINGS

**AUTOMATIC GAME COLLECTIONS**

**CUSTOM GAME COLLECTIONS**

**CREATE NEW CUSTOM COLLECTION FROM THEME**

**CREATE NEW CUSTOM COLLECTION**

**SORT FAVORITES ON TOP FOR CUSTOM COLLECTIONS**

**GROUP UNTHEMED CUSTOM COLLECTIONS**

**SHOW SYSTEM NAMES IN COLLECTIONS**


### OTHER SETTINGS

**VRAM LIMIT**

**FULLSCREEN MODE (REQUIRES RESTART)**

**POWER SAVER MODES**

**WHEN TO SAVE METADATA**

**GAME MEDIA DIRECTORY**

**PER GAME LAUNCH COMMAND OVERRIDE**

**SHOW HIDDEN FILES AND FOLDERS (REQUIRES RESTART)**

**SHOW HIDDEN GAMES (REQUIRES RESTART)**

**CUSTOM EVENT SCRIPTS**

**ONLY SHOW ROMS FROM GAMELIST.XML FILES**

**DISPLAY GAME ART FROM ROM DIRECTORIES**

**SHOW FRAMERATE**

**SHOW "REBOOT SYSTEM" MENU ENTRY**

**SHOW "POWER OFF SYSTEM" MENU ENTRY**


### CONFIGURE INPUT


### QUIT

**QUIT EMULATIONSTATION**

**REBOOT SYSTEM**

**POWER OFF SYSTEM**


## Supported game systems

Here is the list of supported game systems:
| Platform Name         | Full Name                 | Recommmended game setup        |
| --------------------- | ------------------------- | ------------------------------ |
| 3do                   | 3DO                       |                                |
| ags                   | Adventure Game Studio     |                                |
| amiga                 | Amiga                     | .hdf WHDLoad harddisk images or .adf disk images |
| amiga600              | Amiga 600                 | .hdf WHDLoad harddisk images or .adf disk images |
| amiga1200             | Amiga 1200                |
| amstradcpc            | Amstrad CPC               |
| apple2                | Apple II                  |
| arcade                | Arcade                    |
| astrocade             | Bally Astrocade           |
| atari2600             | Atari 2600                |
| atari5200             | Atari 5200                |
| atari7800             | Atari 7800 ProSystem      |
| atari800              | Atari 800                 |
| atarijaguar           | Atari Jaguar              |
| atarijaguarcd         | Atari Jaguar CD           |
| atarilynx             | Atari Lynx                |
| atarist               | Atari ST                  |
| atarixe               | Atari XE                  |

bbcmicro	BBC Micro
c64	Commodore 64
cavestory	Cave Story (NXEngine)
channelf	Fairchild Channel F
coco	Tandy Color Computer
coleco	ColecoVision
daphne	Daphne Arcade Laserdisc Emulator
doom	Doom
dos	DOS (PC)
dragon32	Dragon 32
dreamcast	Sega Dreamcast
famicom	Nintendo Family Computer
fba	Final Burn Alpha
fbneo	FinalBurn Neo
fds	Famicom Disk System
gameandwatch	Nintendo Game and Watch
gamegear	Sega Gamegear
gamecube	Nintendo GameCube
gb	Game Boy
gba	Game Boy Advance
gbc	Game Boy Color
genesis	Sega Genesis
intellivision	Mattel Electronics Intellivision
chailove	ChaiLove game engine
lutro	Lutro game engine
macintosh	Apple Macintosh
mame	Multiple Arcade Machine Emulator
mame-advmame	AdvanceMAME
mame-libretro	Multiple Arcade Machine Emulator
mame-mame4all	MAME4ALL
mastersystem	Sega Master System
megadrive	Sega Mega Drive
mess	Multi Emulator Super System
moonlight	Moonlight game streaming
msx	MSX
msx1	MSX1
msx2	MSX2
n64	Nintendo 64
nds	Nintendo DS
neogeo	Neo Geo
nes	Nintendo Entertainment System
ngp	Neo Geo Pocket
ngpc	Neo Geo Pocket Color
odyssey2	Magnavox Odyssey2
openbor	OpenBOR game engine
oric	Tangerine Computer Systems Oric
pc    IBM PC
pcengine	NEC PC Engine
pcenginecd	NEC PC Engine CD
pcfx	NEC PC-FX
ports	Ports
ps2	Sony PlayStation 2
psp	PlayStation Portable
psvita	PlayStation Vita
psx	Sony PlayStation 1
residualvm	ResidualVM game engine
samcoupe	SAM Coupé
saturn	Sega Saturn
scummvm	ScummVM game engine
sega32x	Sega 32X
segacd	Sega Mega-CD
sg-1000	Sega SG-1000
snes	Super Nintendo
solarus	Solarus game engine
stratagus	Stratagus game engine
supergrafx	NEC SuperGrafx
tg16	NEC TurboGrafx-16
tg-cd	NEC TurboGrafx-CD
ti99	Texas Instruments TI-99
trs-80	Tandy TRS-80
vectrex	Vectrex
videopac	Philips Videopac G7000 (Magnavox Odyssey2)
virtualboy	Nintendo Virtual Boy
wii	Nintendo Wii
wiiu	Nintendo Wii U
wonderswan	Bandai WonderSwan
wonderswancolor	Bandai WonderSwan Color
x68000	Sharp X68000
xbox	Microsoft Xbox
xbox360	Microsoft Xbox 360
zmachine	Infocom Z-machine
zx81	Sinclair ZX81
zxspectrum	Sinclair ZX Spectrum


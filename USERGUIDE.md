EmulationStation Desktop Edition - User Guide
=============================================

**Note:** This document is intended as a quick start guide, for more in-depth information and details on how to compile EmulationStation and perform more advanced configuration, please refer to the [INSTALL.md](INSTALL.md) document.


## Note: This guide is currently under construction!


### Getting started

Getting started with EmulationStation is very easy, just make sure to install the software properly, either manually as built from source code or using one of the supplied packages. On Windows you'll use the installer instead of a package.

Upon first startup, ES will create its home directory, by default the location is ~/.emulationstation.

On Unix this defaults to /home/<username>/.emulationstation/ and on Windows it defaults to C:\Users\<username>\.emulationstation\

A settings file, `es_settings.cfg` will be generated with all the default settings, and a `es_systems.cfg` file will also be copied from the program resource folder, that contains the game ROM and emulator settings.

There's a log file in the home directory as well named `es_log.txt`, please refer to this in case on any errors as it should provide information on what went wrong.

Upon startup with the default settings, ES is set to the gamelist view style `AUTOMATIC`. In this mode the application will look for any game media files (videos and images) and set the view style accordingly. If at least one image is found for any game for a certain system, the view style `DETAILED` will be shown and if at least one video file is found, the view style `VIDEO` will be selected. The gamelist view styles are described in more detail later in this document.


Main screen
===========

When starting EmulationStation with the default settings, you will see the main screen first. From here you can navigate your systems and enter their respective gamelists. If there are no game systems installed, you will not see this screen but rather an error message will be displayed, informing you that no games could be found.

Depending on the theme, the system navigation carousel can be horizontal or vertical. The default theme rbsimple-DE provides horizontal navigation, i.e. you browse your systems be scrolleft left or right.


Help system and general navigation
==================================

There is a help system available throughout the application that provides an overview of the possible actions and buttons that can be used. It's possible to disable the help system, but it's enabled by default.


Main menu
=========

This menu can accessed from both the main screen, and from the gamelist views. It contains the scraper, the quit/restart/power-off menu as well as the application settings.

Here is a breakdown of the complete menu, with a brief explanation for each entry.

## SCRAPER

**SCRAPE FROM**

**FILTER**

**SYSTEMS**

### CONTENT SETTINGS

**SCRAPE GAME NAMES**

**SCRAPE RATINGS**

**SCRAPE OTHER METADATA**

**SCRAPE SCREENSHOT IMAGES**

**SCRAPE BOX COVER IMAGES**

**SCRAPE MARQUEE (WHEEL) IMAGES**

**SCRAPE 3D BOX IMAGES**

### OTHER SETTINGS

**REGION**

**LANGUAGE**

**OVERWRITE FILES AND DATA**

**INTERACTIVE MODE**

**AUTO-ACCEPT SINGLE GAME MATCHES**


## UI SETTINGS

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

### SCREENSAVER SETTINGS

**SCREENSAVER AFTER**

**SCREENSAVER CONTROLS**

**SCREENSAVER BEHAVIOR**

### VIDEO SCRENSAVER SETTINGS

**SWAP VIDEOS AFTER (SECS)**

**STRETCH VIDEOS TO SCREEN RESOLUTION**

**PLAY AUDIO FOR SCREENSAVER VIDEO FILES**

### SLIDESHOW SCREENSAVER SETTINGS

**SWAP IMAGES AFTER (SECS)**

**STRETCH IMAGES TO SCREEN RESOLUTION**

**BACKGROUND AUDIO**

**USE CUSTOM IMAGES**

**CUSTOM IMAGE DIR**

**CUSTOM IMAGE DIR RECURSIVE**

**CUSTOM IMAGE FILTER**


## SOUND SETTINGS

**SYSTEM VOLUME**

**PLAY AUDIO FOR VIDEO FILES IN GAMELIST VIEWS**

**NAVIGATION SOUNDS**


## GAME COLLECTION SETTINGS

**AUTOMATIC GAME COLLECTIONS**

**CUSTOM GAME COLLECTIONS**

**CREATE NEW CUSTOM COLLECTION FROM THEME**

**CREATE NEW CUSTOM COLLECTION**

**SORT FAVORITES ON TOP FOR CUSTOM COLLECTIONS**

**GROUP UNTHEMED CUSTOM COLLECTIONS**

**SHOW SYSTEM NAMES IN COLLECTIONS**


## OTHER SETTINGS

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


## CONFIGURE INPUT


## QUIT

**QUIT EMULATIONSTATION**

**REBOOT SYSTEM**

**POWER OFF SYSTEM**


Game options menu
=================

This menu is opened from the gamelists, and can't be accessed directly from the main screen. The menu changes slightly depending on the context, namely whether a game file or a folder is selected, and whether the current system is a collection or a normal game platform.

## JUMP TO..

This provides the ability to quick jump to a certain letter.

## SORT GAMES BY

## FILTER GAMELIST

## EDIT THIS GAME'S METADATA / EDIT THIS FOLDER'S METADATA

## ADD/REMOVE GAMES TO THIS GAME COLLECTION


Scraper
=======

The scraper supports downloading of game metadata and media files from the Internet. Currently two scraper services are supported, ScreenScraper.fr and TheGamesDB.net.


Getting your games into EmulationStation
========================================

For most systems, this is very straightforward, just put your game files into the folder corresponding to the system name. These names can be found in the end of this document.


Supported game systems
======================

Here is the list of supported game systems:

    3do
    ags
    amiga
    amiga600
    amiga1200
    amstradcpc
    apple2
    arcade
    astrocade
    atari2600
    atari5200
    atari7800
    atari800
    atarijaguar
    atarijaguarcd
    atarilynx
    atarist
    atarixe
    bbcmicro
    c64
    cavestory
    channelf
    coco
    coleco
    daphne
    doom
    dos
    dragon32
    dreamcast
    famicom
    fba
    fbneo
    fds
    gameandwatch
    gamegear
    gamecube
    gb
    gba
    gbc
    genesis
    intellivision
    chailove
    lutro
    macintosh
    mame
    mame-advmame
    mame-libretro
    mame-mame4all
    mastersystem
    megadrive
    mess
    moonlight
    msx
    msx1
    msx2
    n64
    nds
    neogeo
    nes
    ngp
    ngpc
    odyssey2
    openbor
    oric
    pcengine
    pcenginecd
    pcfx
    ports
    ps2
    psp
    psvita
    psx
    residualvm
    samcoupe
    saturn
    scummvm
    sega32x
    segacd
    sg-1000
    snes
    solarus
    stratagus
    supergrafx
    tg16
    tg-cd
    ti99
    trs-80
    vectrex
    videopac
    virtualboy
    wii
    wiiu
    wonderswan
    wonderswancolor
    x68000
    xbox
    xbox360
    zmachine
    zx81
    zxspectrum

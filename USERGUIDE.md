# EmulationStation Desktop Edition (ES-DE) - User guide

**Note:** This document is intended as a quick start guide as well as a reference for the user interface settings and functionality. For more in-depth information and details on how to compile ES-DE and perform more advanced configuration, please refer to [INSTALL.md](INSTALL.md).

Table of contents:

[[_TOC_]]

## Basic steps to get ES-DE up and running

If you just want to get started as quickly as possible, simply follow these steps:

1) Install ES-DE
2) Start the application and press the _Create directories_ button to generate the ROMs directory structure
3) Put your game ROMs in the directories created by the previous step, or see [here](USERGUIDE.md#supported-game-systems) for additional details
4) Install and configure [RetroArch](https://www.retroarch.com)
5) _Windows only: add the RetroArch directory to your system path environmental variable_
6) Start RetroArch and install the required emulator cores - to see which ones you need look in the systeminfo.txt files in the directories created by step 2, or again see [here](USERGUIDE.md#supported-game-systems)
7) Start ES-DE and follow the on-screen instructions to configure your controller
8) Scrape game media for your collection and play some games!

As long as ES-DE hasn't frozen, you can always press F4 to close the application.

For additional details, read on below.

There are also installation videos available at the ES-DE YouTube channel:

[https://www.youtube.com/channel/UCosLuC9yIMQPKFBJXgDpvVQ](https://www.youtube.com/channel/UCosLuC9yIMQPKFBJXgDpvVQ)

## Getting started

Getting started with ES-DE is easy, just make sure to install the software properly, either manually as built from source code or using one of the supplied packages. On Windows and macOS you'll use the installer instead of a package.

The following operating systems have been tested (all for the x86 architecture):

* Ubuntu 20.04
* Ubuntu 20.10 *
* Linux Mint 20
* Manjaro
* Fedora 33 Workstation
* FreeBSD 12.2
* NetBSD 9.1
* OpenBSD 6.8 (limited testing only)
* macOS 11 "Big Sur" (limited testing only)
* macOS 10.15 "Catalina" (limited testing only)
* macOS 10.11 "El Capitan"
* Windows 10
* Windows 8.1

***)** On Ubuntu 20.10, attempting to play videos crashes ES-DE due to a libVLC bug, refer to the Known issues section in [CHANGELOG.md](CHANGELOG.md#known-issues) for a workaround.

The installation procedure is just covered briefly here and may differ a bit for your specific operating system, so in case of problems refer to your system documentation.

**Installing a Linux .deb package**

The .deb package is used for Linux distributions based on Debian, such as Ubuntu, Linux Mint etc.
Running the following should install ES-DE and resolve any dependencies:

```
sudo apt install ./emulationstation-de-1.0.0.deb
```

**Installing a Linux .rpm package**

On Fedora you run this command to install ES-DE, which should automatically resolve all dependencies:

```
sudo dnf install ./emulationstation-de-1.0.0.rpm
```

Note that this requires the RPM Fusion repository as there's a dependency on VLC, which is not part of the standard operating system repo. See [INSTALL.md](INSTALL.md#building-on-unix) for details on how to add this.

**Installing on macOS and Windows**

There's not really much to say about these operating systems, just install ES-DE as you would any other application. On maCOS it's via the .dmg drag-and-drop installer, and on Windows it's just a normal application installer.

**On first application startup**

Upon first startup, ES-DE will create its home directory, by default the location is ~/.emulationstation.

On Unix this means /home/\<username\>/.emulationstation/, on macOS /Users/\<username\>/.emulationstation/ and on Windows C:\Users\\<username\>\\.emulationstation\

**Note:** As of ES-DE v1.0 there is no internationalization support, so you would always need to supply the English directory name for your home directory, which is by the way always the real physical name on the file system. For instance in macOS, /Users/leon will be required instead of /Användare/leon which is what's shown inside the operating system for a Swedish localized installation. But using the tilde home symbol '~' is a workaround for this, and it's recommended to always use it for any ES-DE configuration settings that require a path to your home directory as it removes any confusion regarding localized home directory paths.

On first startup the configuration file `es_settings.cfg` will be generated in the ES-DE home directory, containing all the default settings. A file named `es_systems.cfg` will also be copied from the program resource folder. This file contains the game ROM paths and emulator settings and can be modified if needed. For information on how to do this, refer to the [INSTALL.md](INSTALL.md#es_systemscfg) document.

There's a log file in the ES-DE home directory named `es_log.txt`, please refer to this in case of any issues as it should hopefully provide information on what went wrong. Starting ES-DE with the --debug flag provides even more detailed information.

After ES-DE finds at least one game file, it will populate that game system and the application will start. If there are no game files, a dialog will be shown explaining that you need to install your game files into your ROM directory, and you will also be given a choice to change that ROM directory if you don't want to use the default one. As well you have the option to generate the complete game systems directory structure based on information in es_systems.cfg.

When generating the directory structure, a file named systeminfo.txt will be created in each game system folder which will provide you with some information about the system such as the supported file extensions. Here's an example for the _gc_ system as seen on macOS:
```
System name:
gc

Full system name:
Nintendo GameCube

Supported file extensions:
.gcm .GCM .iso .ISO .wbfs .WBFS .ciso .CISO .gcz .GCZ .elf .ELF .dol .DOL .dff .DFF .tgc .TGC .wad .WAD .7z .7Z .zip .ZIP

Launch command:
/Applications/RetroArch.app/Contents/MacOS/RetroArch -L %EMUPATH%/../Resources/cores/dolphin_libretro.dylib %ROM%

Platform (for scraping):
gc

Theme folder:
gc
```

In addition to this, a file named systems.txt will be created in the root ROM directory which shows the mapping between the directory names and the full system names.

For example:

```
gc: Nintendo GameCube
genesis: Sega Genesis
gx4000: Amstrad GX4000
```

Note that neither the systeminfo.txt files or the systems.txt file are needed to run ES-DE, they're only generated as a convenience.

Be aware that there will be a lot of directories created if using the template es_systems.cfg file bundled with the installation, so it may be a good idea to remove the ones you don't need. It's recommended to move them to another location to be able to use them later if more systems should be added. For example a directory named _DISABLED could be created inside the ROMs folder (i.e. ~/ROMs/_DISABLED) and all game system directories you don't need could be moved there. Doing this reduces the application startup time significantly as ES-DE would otherwise need to scan for game files for all these systems.

![alt text](images/current/es-de_ui_easy_setup.png "ES-DE Easy Setup")
_This is the dialog shown if no game files were found. It lets you configure the ROM directory if you don't want to use the default one, and you can also generate the game systems directory structure. Note that the directory is the real physical path, and that your operating system may present this as a localized path if you are using a language other than English._


## Migrating from other EmulationStation forks

**IMPORTANT!!! IMPORTANT!!! IMPORTANT!!!**

ES-DE is designed to be backwards compatible to a certain degree, that is, it should be able to read data from other/previous EmulationStation versions such as the RetroPie fork. But the opposite is not true and it's a one-way ticket for your gamelist.xml files and your custom collection files when migrating to ES-DE as they will be modified in ways that previous ES versions will see as data loss. For instance ES-DE does not use image tags inside the gamelist.xml files to find game media but instead matches the media to the names of the game/ROM files. So it will not save any such tags back to the gamelist files during updates, effectively removing the display of the game media if the files are opened in another ES fork.

Due to this, always make backups of at least the following data:

```
~/.emulationstation/gameslists/
~/.emulationstation/collections/
~/.emulationstation/es_settings.cfg
~/.emulationstation/es_systems.cfg
```

Actually as for `es_systems.cfg` you should probably rename or move it somewhere else as it's strongly recommended to use the es_systems.cfg template shipped with ES-DE (which will be automatically copied from the resources directory on startup as long as a file with this name does not already exist). It may still work to use an existing customized es_systems.cfg file, as again ES-DE should be backwards compatible, but if you have issues launching games this is the first thing to look at.

It's probably also a good idea to rename or move `es_settings.cfg` elsewhere as a huge amount of things have changed in the configuration files in ES-DE.


## Running on 4K displays

ES-DE fully supports 4K displays (as well as 1440p and other higher resolutions) but some emulators such as RetroArch will always run using the active screen resolution, meaning the emulation will also run in 4K. On slower computers and when resource intensive shaders are in use, the performance may be quite bad. Although it's possible to start ES-DE with the `--resolution` option (which also applies to any launched emulators), this is not really recommended. Full screen mode only works on Unix with this option and even then it's highly dependent on well-written graphics drivers for proper behavior. ES-DE uses the SDL library which insists on setting xrandr to panning mode when doing resolution changes, which is incredibly annoying especially when using Nvidia drivers.

A better approach is to use the custom event scripts functionality in ES-DE to set a temporary resolution upon launching a game that will be reverted when returning from the emulator. This is detailed as an example for Unix in [INSTALL.md](INSTALL.md#custom-event-scripts) but should be possible to implement similarly on other operating systems such as macOS and Windows.


## Input device configuration

When first starting ES-DE, the application will look for any attached controllers (joysticks and gamepads). If no devices are found, it will be assumed that only keyboard navigation is to be used and the default keyboard mappings will be applied. It's recommended to change these default values, and a message will be displayed describing just this. It's however possible to hide this notification permanently and continue to use the default keyboard mappings if you're happy with them.

If a controller is attached when starting ES-DE and no `es_input.cfg` input configuration file exists, you will be presented with the input configuration dialog. Just follow the steps as described to map the inputs.

If an es_input.cfg configuration file exists, you will not be presented with the input device configuration screen as that would normally just be annoying. If you need to configure a device to control the application (i.e. you've replaced your controller), you can do so by starting ES-DE with the command line argument `--force-input-config` or you can manually delete the es_input.cfg file prior to starting the application. Alternatively you can navigate to the menu using your keyboard and select **Configure input** to configure your new device.

The actual procedure to map the inputs should be self-explanatory, just follow the on-screen instructions.

Both new and old devices can be (re)configured at any time via the `Configure input` menu entry. New devices will be appended to the existing input configuration file, so your old devices will retain their configuration.


## System view (main screen)

When starting EmulationStation with the default settings, you will see the main screen first. From here you can navigate your game systems and enter their respective gamelists.

Depending on the theme, the system navigation carousel can be either horizontal or vertical. The default theme rbsimple-DE provides horizontal navigation, i.e. you browse your systems be scrolling left or right.

The game systems are sorted by their full names, as defined in es_systems.cfg.

![alt text](images/current/es-de_system_view.png "ES-DE System View")
_The **System view** is the default starting point for the application, it's here that you browse through your game systems._

## Gamelist view

The gamelist view is where you browse and start your games, and it's where you will spend most of your time using ES-DE.

Upon startup with the default settings, ES-DE is set to the gamelist view style **Automatic**. In this mode the application will look for any game media files (videos and images) and set the view style accordingly. If at least one image is found for any game, the view style **Detailed** will be shown, and if at least one video file is found, the view style **Video** will be selected (superceding the Detailed style). If no game media files are found for a system, the simple **Basic** view will be selected. Note that this automatic selection is applied per game system.

Also note that the Video view style requires that the theme supports it. If not, the Detailed style will be selected instead. (The default theme rbsimple-DE supports both of these view styles).

It's possible to manually set a specific gamelist view style in the UI settings entry of the main menu, but this is applied globally regardless of what media files are available per game system. The manual setting also overrides the theme-supported view styles which has the potential of making ES-DE very ugly indeed.

In addition to the styles just described, there is a **Grid** view style as well, but as of ES-DE version 1.0 this is highly experimental and its use is not recommended. Future versions will update this style to a more useful state.

If the theme supports it, there's a gamelist information field displayed in the gamelist view, showing the number of games for the system (total and favorites) as well as a folder icon if a folder has been entered. When applying any filters to the gamelist, the game counter is replaced with the amount of games filtered, as in 'filtered / total games', e.g. '19 / 77'. If there are game entries in the filter result that are marked not to be counted as games, the number of such files will be indicated as 'filtered + filtered non-games / total games', for example '23 + 4 / 77' indicating 23 normal games, 4 non-games out of a total of 77. Due to this approach it's theoretically possible that the combined filtered game amount exceeds the number of counted games in the collection, for instance '69 + 11 / 77'. This is not considered a bug and is so by design. This gamelist information field functionality is specific to EmulationStation Desktop Edition so older themes will not support this.

![alt text](images/current/es-de_gamelist_view.png "ES-DE Gamelist View")
_The **Gamelist view** is where you browse the games for a specific system._

![alt text](images/current/es-de_basic_view_style.png "ES-DE Basic View Style")
_Here's an example of what the Basic view style looks like. Needless to say, ES-DE is not intended to be used like this. After scraping some game media for the system, the view style will automatically change to Detailed or Video (assuming the Automatic view style has been selected)._

## UI modes

ES-DE supports three separate modes, **Full**, **Kiosk** and **Kid**.

These modes mandate the functionalty provided by the application in the following way:

* Full - This is the default mode which enables all functionality.
* Kiosk - The main menu will be severely restricted, only displaying the entry to change the audio volume. The game options menu will be restricted as well, removing the metadata editor and the ability to modify custom game collections. And finally the ability to flag or unflag games as favorites will be removed. Apart from this all games will be playable.
* Kid - Only games marked as being suitable for children will be displayed (this flag is set manually per game using the metadata editor). Additionally, the game options menu is disabled, as well as the screensaver controls and the ability to flag and unflag games as favorites. There is also a separate option available to enable or disable the main menu when in Kid mode, see **Enable menu in kid mode** for additional information.

There is an unlock code available to revert to the Full mode from the Kiosk or Kid mode, as is described when changing this setting from the main menu. By default the button sequence is **Up, Up, Down, Down, Left, Right, Left, Right, B, A**. It works to use either a keyboard or a configured controller to input the passkey sequence, but it can't be entered when a menu is open.

The application can also be forced into any of the three modes via the command line options `-force-full`, `--force-kiosk` and `-force-kid`. Note that this is only temporary until the restart of the application, unless the settings menu is entered and the setting is saved to the configuration file (this assumes that the main menu is available in the selected UI mode of course).

## Help system

There is a help system available throughout the application that provides an overview of the possible actions and buttons that can be used. Note though that some general button actions are never shown, such as the ability to quick jump in gamelists, menus and text input fields using the shoulder and trigger buttons. It's also possible to disable the help system using a menu option for a somewhat cleaner look.

![alt text](images/current/es-de_folder_support.png "ES-DE Help System")
_The help system is displayed at the bottom of the screen, showing the various actions currently available._


## General navigation

The built-in help system will provide a contextual summary of the available navigation options, but here's still a general overview. These are the inputs you mapped in the previous input device configuration step. Note that this is not an exhaustive list, but it covers most situations.

The default keyboard mappings are shown in brackets.

**Up and down**\
_(Arrow up / Arrow down)_

Navigate up and down in gamelists, between systems in the system view (if the theme has a vertical carousel) and in menus.

**Left and right**\
_(Arrow left / Arrow right)_

Navigate between gamelists (if _Quick system select_ has been activated in the options), or between systems in the system view (if the theme has a horizontal carousel).

**Start button**\
_(Escape)_

Opens and closes the main menu.

**Select button**\
_(F1)_

Opens and closes the game options menu in the gamelist view, or toggles the screensaver in the system view (if the _Enable screensaver controls_ setting is activated).

**Left and right shoulder buttons**\
_(Page up / Page down)_

Provides quick jumping in gamelists and menus, jumps 10 games in the gamelists and 6 entries in the menus. Also jumps forward in text edit dialogs.

**Left and right trigger buttons**\
_(Home / End)_

Jumps to the first and last entry of the gamelists, menus and text edit dialogs.

**Left and right thumbstick click**\
_(F2 / F3)_

Reserved for future use.

**A button**\
_(Enter)_

Select button to open gamelists from the system view, launch games, choose menu entries etc.

**B button**\
_(Back key)_

Back button, self explanatory.

**X button**\
_(Delete)_

Selects random games and systems. Used by some other minor functions as explained by the help system and/or this guide.

**Y button**\
_(Insert on Unix and Windows, F13 on macOS)_

Marks games as favorites in the gamelist views. Used by some other minor functions as explained by the help system and/or this guide.

**F4 (keyboard only)**

Quits the application.


## Getting your games into ES-DE

For most systems, this is very straightforward, just put your game files into the folder corresponding to the platform name (these names can be found at the [end](USERGUIDE.md#supported-game-systems) of this guide.)

For some systems though, a more elaborate setup is required, and we will attempt to cover such situations in this guide as well.

### Single game file installation

Let's start with the simple scenario of a single ROM game file per platform, which is the case for the majority of systems. In this example we're setting up ES-DE to play Nintendo Entertainment System games.

The supported file extensions are listed in [es_systems.cfg_unix](resources/templates/es_systems.cfg_unix), [es_systems.cfg_macos](resources/templates/es_systems.cfg_macos) and [es_systems.cfg_windows](resources/templates/es_systems.cfg_windows).

Here is the snippet from the es_systems.cfg_unix file:

```
<system>
  <name>nes</name>
  <fullname>Nintendo Entertainment System</fullname>
  <path>%ROMPATH%/nes</path>
  <extension>.nes .NES .unf .UNF .unif .UNIF .7z .7Z .zip .ZIP</extension>
  <command>retroarch -L %COREPATH%/nestopia_libretro.so %ROM%</command>
  <platform>nes</platform>
  <theme>nes</theme>
</system>
```

It's required that the ROM files are in one of the supported file extensions, or ES-DE won't find them.

It's highly recommended to use filenames that are corresponding to the full name of the game, otherwise you will need to manually feed the scraper the game name when scraping which is very tedious.

The default game directory folder is ~/ROMs. On Unix this defaults to /home/\<username\>/ROMs, on macOS /Users/\<username\>/ROMs and on Windows C:\Users\\<username\>\ROMs\.

If ES-DE can't find any game files during startup, an error message will be displayed with the option to change the ROM directory path.

Assuming the default ROM directory is used, we need to create a directory corresponding to the \<path\> tag in es_systems.cfg, in this example it's `nes`.

This would look something like the following:

```
/home/myusername/ROMs/nes     # On Unix
/Users/myusername/ROMs/nes    # On macOS
C:\Users\myusername\ROMs\nes  # On Windows
```

Then simply copy your game ROMs into this folder, and you should end up with something like this (example for Unix):

```
~/ROMs/nes/Legend of Zelda, the.zip
~/ROMs/nes/Metal Gear.zip
~/ROMs/nes/Super Mario Bros. 3.zip
```

**Note:** These directories are case sensitive on Unix, so creating a directory named `Nes` instead of `nes` won't work.

That's it, start ES-DE and the NES game system should be populated. You can now scrape information and media for the games, and assuming you've setup RetroArch correctly with the Nestopia UE core, you can launch the games.

### Multiple game files installation

For some systems, there are sometimes (or always) multiple gamefiles per game. Such an example would be the Commodore 64 when multidisk games are being played. For such instances, simply group the files inside folders.

The platform name for the Commodore 64 is `c64`, so the following structure would be a possible approach:

```
~/ROMs/c64/Cartridge
~/ROMs/c64/Tape
~/ROMs/c64/Disk
~/ROMs/c64/Multidisk
~/ROMs/c64/Multidisk/Last Ninja 2/LNINJA2A.D64
~/ROMs/c64/Multidisk/Last Ninja 2/LNINJA2B.D64
~/ROMs/c64/Multidisk/Last Ninja 2/Last Ninja 2.m3u
~/ROMs/c64/Multidisk/Pirates/PIRAT-E0.d64
~/ROMs/c64/Multidisk/Pirates/PIRAT-E1.d64
~/ROMs/c64/Multidisk/Pirates/PIRAT-E2.d64
~/ROMs/c64/Multidisk/Pirates/Pirates!.m3u
```

It's highly recommended to create `.m3u` playlist files for multi-disk images as this normally automates disk swapping in the emulator. It's then this .m3u file that should be selected for launching the game.

The .m3u file simply contains a list of the game files, for example in the case of Last Ninja 2.m3u:

```
LNINJA2A.D64
LNINJA2B.D64
```

It's of course also possible to skip this type of directory structure and put all the games in the root folder, but then there will be multiple entries for the same game which is not so tidy. Another approach would be to put all the files in the root folder and then hide the game files, showing only the .m3u playlist files. ES-DE is flexible so do whatever makes most sense for the situation.

When setting up games in this fashion, it's recommended to scrape the directory in addition to the .m3u file as it looks nicer to see the metadata for the games also when browsing the folders. ES fully supports scraping folders, although some metadata is not included for folders for logical reasons. If you only scrape the folders and not the actual game files, it may look somehow ok when browsing, but when a game is part of a collection, the metadata will be missing there. This includes the **Last played** and **All games** collections for instance. Also note that while it's possible to mark a folder as a favorite, it will never be part of a collection, such as **Favorites**.

As well it's recommended to set the flags **Exclude from game counter** and **Exclude from automatic scraper** for the actual game files so that they are not counted for the game statistics display and not scraped when running the multi-scraper. If you don't want to hide the individual game files but still want a cleaner look, it's also possible to set the flag **Hide metadata fields** for the game files.

### Special game installation considerations

Not all systems are as simple as described above, or sometimes there are multiple ways to configure the systems. So specifics to such systems will be covered here. Consider this a work in progress as there are many systems supported by ES-DE.

#### Arcade and Neo Geo

For all the supported MAME variants as well as Final Burn Alpha/FinalBurn Neo and Neo Geo, single file archives should be used. But these should retain the MAME names as filenames since ES-DE ships with MAME lookup tables, meaning the MAME names are expanded to the full game names.

For instance `topgunnr.7z` will be expanded to `Top Gunner`.

This is used by the TheGamesDB scraper where the expanded file names are used for game searches. (Screenscraper natively supports searches using the MAME names). It's also quite nice to have the gamelist populated with the expanded game names even before any scraping has taken place.

#### Commodore Amiga

There are multiple ways to run Amiga games, but the recommended approach is to use WHDLoad. The best way is to use hard disk images in `.hdf` or `.hdz` format, meaning there will be a single file per game. This makes it just as easy to play Amiga games as any console with game ROMs.

An alternative would be to use `.adf` images as not all games may be available with WHDLoad support. For this, you can either put single-disk images in the root folder or in a dedicated adf directory, or multiple-disk games in separate folders. It's highly recommended to create `.m3u` playlist files for multi-disk images as described above.

Here's an example of what the file structure could look like:

```
~/ROMs/amiga/Multidisk/ZakMcKracken/ZakMcKracken (Disk 1 of 2).adf
~/ROMs/amiga/Multidisk/ZakMcKracken/ZakMcKracken (Disk 2 of 2).adf
~/ROMs/amiga/Multidisk/ZakMcKracken/ZakMcKracken.m3u
~/ROMs/amiga/Robbeary.adf
~/ROMs/amiga/Dungeon Master.hdf
```

Advanced topics such as the need for the Amiga Kickstart ROMs to run Amiga games is beyond the scope of this guide, but the following page is recommended for reading more about how this setup can be achieved:

[https://github.com/libretro/libretro-uae/blob/master/README.md](https://github.com/libretro/libretro-uae/blob/master/README.md)

#### DOS / PC

The DOS (and PC) platform uses the DOSBox emulator and the recommended approach here is to keep the directory structure intact, just as if running the game on a real DOS computer. So this means one folder per game in ES-DE. It's also recommended to set the metadata field **Count as game** to off for all files but the actual file used to launch the game, i.e. the binary or a .bat batch file. This is done so that the game counter correctly reflects the number of games you have installed. It's also possible to mark files and subdirectories as hidden to avoid seeing them in ES-DE. Both of these fields can be set using the metadata editor.

Apart from this, DOS games should work the same as any other system. The game folders can be scraped so that it looks nice when browsing the list of games, but make sure to also scrape the files used to launch the games or otherwise the entries in the collections **Last played**, **Favorites** and **All games** as well as any custom collections will miss the game metadata and game media. If you don't have these collections activated, then this can of course be skipped.

#### Ports

**Note: On Unix/Linux, if you get a white screen in ES-DE after returning from a game that switches screen resolution then refer to the Known issues section in [CHANGELOG.md](CHANGELOG.md#known-issues) for a workaround.**

Ports are not really executed using emulators, but are rather applications running natively in the operating system. The easiest way to handle these is to add a simple shell script or batch file where you can customize the exact launch parameters for the game.

It's of course possible to add these as single files to the root folder, but normally it's recommended to setup a separate folder per game as there may be more than a single file available per game. You very often want to have easy access to the game setup utility for instance.

Here's an example for setting up Chocolate-Doom, GZDoom and DarkPlaces on Unix:

```
~/ROMs/ports/Chocolate-Doom/chocolate-doom.sh
~/ROMs/ports/Chocolate-Doom/chocolate-doom-setup.sh
~/ROMs/ports/GZDoom/gzdoom.sh
~/ROMs/ports/DarkPlaces/darkplaces.sh
```

chocolate-doom.sh:

```
#!/bin/bash
chocolate-doom
```

chocolate-doom-setup.sh:

```
#!/bin/bash
chocolate-doom-setup
```

gzdoom.sh:

```
#!/bin/bash
GZ_dir=/home/myusername/Games/Ports/GZDoom

gzdoom -iwad /home/myusername/Games/Ports/GameData/Doom/doom.wad -config $GZ_dir/gzdoom.ini -savedir $GZ_dir/Savegames \
-file $GZ_dir/Mods/DoomMetalVol4_44100.wad \
-file $GZ_dir/Mods/brutalv21.pk3 \
-file $GZ_dir/Mods/DHTP-2019_11_17.pk3
```

darkplaces.sh:

```
#!/bin/bash
darkplaces -basedir ~/Games/Ports/GameData/Quake
```

You don't need to set execution permissions for these scripts, ES-DE will run them anyway.

#### Lutris

**Note: If you get a white screen in ES-DE after returning from a game that switches screen resolution then refer to the Known issues section in [CHANGELOG.md](CHANGELOG.md#known-issues) for a workaround.**

Lutris runs only on Unix so it's only present as a placeholder in the es_systems.cfg templates for macOS and Windows.

These games are executed via the Lutris binary (well it's actually a Python script), and you simply create a shell script per game using the syntax `lutris lutris:rungame/<game name>`

You can see the list of installed games by running this command:
```
lutris --list-games
```

Here's an example for adding Diablo and Fallout:

```
~/ROMs/lutris/Diablo.sh
~/ROMs/lutris/Fallout.sh
```

Diablo.sh:

```
lutris lutris:rungame/diablo
```

Fallout.sh:

```
lutris lutris:rungame/fallout
```

You don't need to set execution permissions for these scripts, ES-DE will run them anyway.

As an alternative, you can add the Lutris games to the Ports game system, if you prefer to not separate them. The instructions above are identical in this case except that the shell scripts should be located inside the `ports` directory rather than inside the `lutris` directory.

#### Steam

**Note:** Launching Steam games currently has some limitations such as missing error messages when a game fails to start as well as missing game output logging. ES-DE also needs to keep running in the background when launching Steam games, which has some minor side effects.

As for the setup, it's recommended to place shell scripts/batch files directly in the root folder, with the filenames of these scripts corresponding to the game names.

Add the game information to each file using the syntax `steam://rungameid/<game ID>`

Here's an example for the game Broforce, first on Unix with the filename `Broforce.sh`:

```
steam steam://rungameid/274190
```

And on macOS with the filename `Broforce.sh`:
```
/Applications/Steam.app/Contents/MacOS/steam_osx steam://rungameid/274190
```

And finally on Windows with the filename `Broforce.bat`:
```
@echo off
"c:\Program Files (x86)\Steam\steam.exe" steam://rungameid/26800
```

The game ID can be found by going to [https://store.steampowered.com](https://store.steampowered.com) and searching for a game. The Broforce example would have an URL such as this:

https://store.steampowered.com/app/274190/Broforce

On Linux it's very easy to find all your game ID's by looking in the desktop entries.

```
grep steam ~/.local/share/applications/*desktop | grep rungameid
/home/myusername/.local/share/applications/FEZ.desktop:Exec=steam steam://rungameid/224760
/home/myusername/.local/share/applications/INSIDE.desktop:Exec=steam steam://rungameid/304430
/home/myusername/.local/share/applications/Subnautica.desktop:Exec=steam steam://rungameid/264710
```

This of course assumes that you have menu entries setup for your Steam games.


## Emulator setup

ES-DE is a game browsing frontend and does not provide any emulation by itself. It does however come preconfigured for use with emulators as setup in the `es_systems.cfg` file. By default it's primarily setup for use with [RetroArch](https://www.retroarch.com) but this can be modified if needed. If you're interested in customizing your es_systems.cfg file, please refer to the [INSTALL.md](INSTALL.md) document which goes into details on the structure of this file and more advanced configuration topics in general.

Installation and configuration of RetroArch and other emulators is beyond the scope of this guide, but many good resources can be found online on how to do this.

A general recommendation regarding installation on Linux though is to try to avoid the versions included in the OS repositories as they're usually quite limited with regards to the number of available cores, and they're usually older versions. Instead go for either the Snap or Flatpak distributions or build from source.

In order to use the default es_systems.cfg file, you need to make sure that the emulator binary is in the operating system's path. On Unix systems this is normally not an issue (unless Flatpak is used) as a package manager has typically been used to install the emulator, and even if compiled from source there is a standardized directory structure which should have you covered. Likewise on macOS, it's assumed that the emulator is installed under the Applications folder so everything should just work. But for Windows you may need to add the emulator directory to your %PATH% variable manually (tip: open Settings from the Start menu and search for _path_).

If installing RetroArch as Flatpak on Linux you have to work around an incredibly annoying deficiency of this type of software distribution which is that there will be no RetroArch executable directly available to run. To run RetroArch you would instead need to execute `flatpak run org.libretro.RetroArch` and to get ES-DE to work properly with this you either need to use a customized es_systems.cfg file, or create a shell script somewhere in your path that executes the Flatpak command.
Here's an example of such a script:
```
#!/bin/sh
flatpak run org.libretro.RetroArch "$@"
```

For instance on Fedora you could place the shell script in `~/bin` and name it `retroarch` and then everything will work fine.

There is also a Flatpak-specific es_systems.cfg template shipped with ES-DE, but you need to manually install it if you would like to use it:
```
cp /usr/share/emulationstation/resources/templates/es_systems.cfg_unix_flatpak ~/.emulationstation/es_systems.cfg
```

The source path may differ from this example depending on which prefix was used when building ES-DE.

As an alternative, if the emulator is not in your search path, you can either hardcode an absolute path in es_systems.cfg or use the %ESPATH% variable to set the emulator relative to the ES-DE binary location. Again, please refer to [INSTALL.md](INSTALL.md#es_systemscfg) for details regarding this.

In any instance, ES-DE will display an error notification if attempting to launch a game where the emulator binary is not found. Likewise it will notify if the defined emulator core is not installed. The es_log.txt file will also provide additional details.


## Scraping

Scraping means downloading metadata and game media files (images and videos) for the games in your collections.

ES-DE supports the two scrapers [ScreenScraper.fr](https://www.screenscraper.fr) and [TheGamesDB.net](https://thegamesdb.net). In general TheGamesDB supports less formats and less systems, but in some areas such PC gaming, the quality is better and sometimes ScreenScraper is missing some specific information such as release dates where TheGamesDB may be able to fill in the gaps.

Here is an overview of what's supported by ES-DE and these scrapers:

| Media type or option     | ScreenScraper | TheGamesDB |
| :----------------------- | :-----------: | :--------: |
| Region (EU/JP/US/WOR)    | Yes           | No         |
| Multi-language           | Yes           | No         |
| Game names               | Yes           | Yes        |
| Ratings                  | Yes           | No         |
| Other game metadata      | Yes           | Yes        |
| Videos                   | Yes           | No         |
| Screenshots              | Yes           | Yes        |
| Box covers (2D)          | Yes           | Yes        |
| Marquees/wheels          | Yes           | Yes        |
| 3D boxes                 | Yes           | No         |

The category **Other game metadata** includes Description, Release date, Developer, Publisher, Genre and Players.

The **Multi-language** support includes translated game genres and game descriptions for a number of languages.

There are two approaches to scraping, either for a single game from the metadata editor, or for many games and systems using the multi-scraper.

![alt text](images/current/es-de_scraper_running.png "ES-DE Scraper Running")
_Here's an example of the multi-scraper running in interactive mode, asking the user to make a selection from the multiple matching games returned by the scraper service._

### Single-game scraper

The single-game scraper is launched from the metadata editor. You navigate to a game, open the game options menu, choose **Edit this game's metadata** and then select the **Scrape** button.

### Multi-scraper

The multi-scraper is accessed from the main menu by selecting **Scrape**.

### Scraping process

The process of scraping games is basically identical between the single-game scraper and the multi-scraper. You're presented with the returned scraper results, and you're able to refine the search if the scraper could not find your game. Sometimes small changes like adding or removing a colon or a minus sign can yield better results. Note that the searching is handled entirely by the scraper service, ES-DE just presents the results returned from the service.

By default, ES-DE will search using the metadata name of the game. If no name has been defined via scraping or manually using the metadata editor, this name will correspond to the physical file name minus all text inside either normal brackets `()` or square brackets `[]`. So for example the physical filename `Mygame (U) [v2].zip` will be stripped to simply `Mygame` when performing the scraping.

The behavior of using the metadata name rather than the file name can be changed using the setting **Search using metadata name**.

Note that there is  an exception to this behavior for arcade games (MAME and Neo Geo). For ScreenScraper the short MAME names are used by default as this scraper service fully supports that. For TheGamesDB the short names are instead expanded to the full games names using a lookup in the MAME name database supplied with the ES-DE installation. It's possible to override this automatic behavior by using the _Refine Search_ button in the scraper GUI if the search did not yield any results, or if the wrong game was returned. In general though, searching for arcade games is very reliable assuming the physical game files follow the MAME name standard.

Apart from this, hopefully the scraping process should be self-explanatory once you try it in ES-DE.

### Manually copying game media files

If you already have a library of game media (images and videos) you can manually copy it into ES-DE.

The default directory is `~/.emulationstation/downloaded_media/<game system>/<media type>`

For example on Unix:
```
/home/myusername/.emulationstation/downloaded_media/c64/screenshots/
```

For example on macOS:

```
/Users/myusername/.emulationstation/downloaded_media/c64/screenshots/
```

For example on Windows:

```
C:\Users\Myusername\.emulationstation\downloaded_media\c64\screenshots\
```

The media type directories are:

* 3dboxes
* covers
* marquees
* miximages
* screenshots
* videos

**Note:** The miximages files are not generated by ES-DE as of v1.0, but if you have used something like [Skyscraper](https://github.com/muldjord/skyscraper) to generate this type of images for your game collection, then they can be displayed inside ES-DE by locating them in this directory.

The media files must correspond exactly to the game files. For example the following game:

```
~/ROMs/c64/Multidisk/Last Ninja 2/Last Ninja 2.m3u
```

Must have corresponding filenames for its media files in this fashion:

```
~/.emulationstation/downloaded_media/c64/screenshots/Multidisk/Last Ninja 2/Last Ninja 2.jpg
~/.emulationstation/downloaded_media/c64/videos/Multidisk/Last Ninja 2/Last Ninja 2.mp4
```

JPG and PNG file formats and file extensions are supported for images, and AVI, MKV, MOV, MP4 and WMV are supported for videos.

Remember that on Unix files are case sensitive, and as well the file extensions must be in lower case, such as .png instead of .PNG or .Png or the file won't be found.

As an alternative, you can also locate your game media in the ROM directory. This is explained below when covering the option **Display game media from ROM directories**. This is however not recommended and the built-in scraper will never save any game media to this folder structure.

It's possible to change the game media directory from within ES-DE, for this see the option **Game media directory**.


## Main menu

This menu can be accessed from both the system view and gamelist views. It contains the scraper, the input configuration tool and the application settings. Settings are saved when navigating back from any menu screen, assuming at least one setting was changed. Pressing F4 to quit the application will also save any pending changes.

![alt text](images/current/es-de_main_menu.png "ES-DE Main Menu")
_The ES-DE main menu._

Following is a breakdown of the main menu entries.

### Scraper

Contains the various options for the scraper, which is used to download metadata, images and videos for your games.

![alt text](images/current/es-de_scraper_settings.png "ES-DE Scraper Settings")
_Some of the scraper settings._

**Scrape from**

Scraper service selection, currently ScreenScraper.fr and TheGamesDB.net are supported.

**Scrape these games**

Criteria for what games to include in the scraping. It can be set to _All games, Favorite games, No metadata, No game image, No game video_ or _Folders only_.

**Scrape these systems**

A selection of which systems to scrape for. It's possible to automatically scrape several or all systems in one go.

#### Account settings

Setup of ScreenScraper account.

**Use this account for ScreenScraper**

Whether to use the account that has been setup here. If this is disabled, the username and password configured on this screen will be ignored during scraping. This can be useful if you have scraping issues and want to check whether it's related to your account or if it's a general problem. Note that screenscraper.fr does not seem to return a proper error message regarding incorrect username and password, but starting ES-DE with the --debug flag will indicate in the log file whether the username was included in the server response.

**ScreenScraper username**

Username as registered on screenscraper.fr.

**ScreenScraper password**

The password as registered on screenscraper.fr. Note that the password is masked using asterisks on this screen, and the password input field will be blank when attempting to update an existing password. Entering a new password will of course work, and it will be saved accordingly. Be aware though that the es_settings.cfg file contains the password in clear text.

#### Content settings

Describes the content types to include in the scraping. Most users will probably not need to adjust so many of these.

**Scrape game names**

Whether to scrape the names of the games. This does not affect the actual files on the filesystem and is only used for viewing and sorting purposes. The downloaded media files are also matched against the physical game files on the filesystem, and not against this metadata. See the comments under _Overwrite files and data_ below to understand some additional implications regarding the game names.

**Scrape ratings** _(ScreenScraper only)_

Downloads game ratings.

**Scrape other metadata**

This includes the game description, release date, developer, publisher, genre and the number of players.

**Scrape videos** _(ScreenScraper only)_

Videos of actual gameplay.

**Scrape screenshot images**

Screenshot images of actual gameplay.

**Scrape box cover images**

Cover art.

**Scrape marquee (wheel) images**

Logotype for the game.

**Scrape 3D box images** _(ScreenScraper only)_

These images are currently unused, but will be used for future versions of ES-DE so it's recommended to keep this option enabled.

#### Other settings

Various scraping settings. Most users will probably not need to adjust so many of these.

**Region** _(ScreenScraper only)_

The region to scrape for. This affects game names, game media and release dates. Possible options are Europe, Japan, USA and World.

**Preferred language** _(ScreenScraper only)_

Multiple languages are supported by ScreenScraper, and this affects translations of game genres and game descriptions. As the option name implies this is the preferred language only as not all games have had their metadata translated. Unfortunately some less used languages have quite few games translated to them but hopefully this will improve over time as there's an ongoing community effort to make more translations. If the preferred language is not available for a game, ES-DE will fall back to scraping the English metadata.

**Overwrite files and data**

Affects both overwriting of metadata as well as actual game media files on the filesystem. Even with this option disabled, metadata entries which are set to their default values will of course be populated by the scraper. In other words, this option only affects overwriting of previously scraped data. Game names are considered as set to their default vaules if either corresponding to the physical game file on disk minus the extension (e.g. the entry _Commando_ if the file is named _Commando.zip_), or for arcade games if corresponding to the MAME names as defined in the bundled mamenames.xml.

**Halt on invalid media files**

With this setting enabled, if any media files returned by the scraper seem to be invalid, the scraping is halted and an error message is presented where it's possible to retry or cancel the scraping of the specific game. In the case of multi-scraping it's also possible to skip the game and proceed to the next one in the queue. With this setting disabled, all media files will always be accepted and saved to disk. As of ES-DE v1.0 the file verification is crude as it's just checking if the file is less than 350 bytes in size which should indicate a server error response rather than a real media file. In some exceedingly rare situations, proper media files may be smaller than 350 bytes, and for those rare instances, simply disabling this setting temporarily allows these files to be scraped. Future versions of ES-DE will implement proper CRC/checksum verifications for ScreenScraper and possibly media file integrity checks for TheGamesDB (as this scraper service does not provide file checksums).

**Search using metadata names**

By default ES-DE will perform scraper searches based on the game name that has been manually set in the metadata editor, or that has been previously scraped. If you prefer to search using the physical name regardless of such data being available, then turn off this option.

Note that when using TheGamesDB as scraper service for arcade games (MAME/Neo Geo), the short MAME name will always be expanded to the full game name as this scraper does not properly support searches using MAME names. Also note that you need to save the game name in the metadata editor before you can use it for scraping.

**Interactive mode** _(Multi-scraper only)_

If turned off, the scraping will be fully automatic and will not stop on multiple results or on missing games.

**Auto-accept single game matches** _(Multi-scraper only)_

Used in conjunction with interactive mode, to not having to confirm searches where a single result is returned from the scraper service.

**Respect per-file scraper exclusions** _(Multi-scraper only)_

It's possible to set a flag per game file or directory to indicate that it should be excluded from the multi-scraper. This setting makes it possible to override that restriction and scrape all entries anyway.

**Exclude folders recursively** _(Multi-scraper only)_

If this setting is enabled and a folder has its flag set to be excluded from the scraper, then the entire folder contents are skipped when running the multi-scraper.

**Scrape actual folders** _(Multi-scraper only)_

Enabling this option causes folders themselves to be included by the scraper. This is useful for DOS games or any multi-disk games where there is a folder for each individual game.

### UI settings

Various settings that affects the user interface.

**Gamelist on startup**

If set to _None_, the system view will be showed. Any other value will jump to that game system automatically on startup.

**Gamelist view style**

Sets the view style to _Automatic, Basic, Detailed, Video_ or _Grid_. See the description [above](USERGUIDE.md#gamelist-view) in this document for more information regarding view styles.

**Transition style**

Graphical transition effect, either _Slide, Fade_ or _Instant_.

**Theme set**

The theme to use. Defaults to rbsimple-DE, the theme shipped with ES-DE.

**UI mode**

Sets the user interface mode for the application to _Full, Kiosk_ or _Kid_. See the description [above](USERGUIDE.md#ui-modes) in this document for additional information.

**Default sort order**

The order in which to sort your gamelists. This can be overriden per game system using the game options menu, but that override will only be persistent during the application session. The _System_ sorting can not be selected here as it's only applicable to collection systems.

**Menu opening effect** _(OpenGL renderer only)_

Animation to play when opening the main menu or the game options menu. Can be set to _Scale-up, _Fade-in_ or _None_.

**Blur background when menu is open** _(OpenGL renderer only)_

This option will blur the background behind the menu slightly. Normally this can be left enabled, but if you have a really slow GPU, disabling this option may make the application feel a bit more responsive.

**Display pillarboxes for gamelist videos**

With this option enabled, there are black pillarboxes (and to a lesser extent letterboxes) displayed around videos with non-standard aspect ratios. This will probably be most commonly used for vertical arcade shooters, or for game systems that has a screen in portrait orientation. For wider than normal videos, letterboxes are added, but this is quite rare compared to videos in portrait orientation. This option looks good with some theme sets such as rbsimple-DE, but on others it may be more visually pleasing to disable it. On less wide displays such as those in 4:3 aspect ratio this option should probably be disabled as it may otherwise add quite excessive letterboxing.

**Render scanlines for gamelist videos** _(OpenGL renderer only)_

Whether to use a shader to render scanlines for videos in the gamelist view. The effect is usually pretty subtle as the video is normally renderered in a limited size in the GUI, and the scanlines are sized relative to the video window size.

**Sort folders on top of gamelists**

Whether to place all folders on top of the gamelists. If enabled the folders will not be part of the quick selector index, meaning they can no longer be quick-jumped to. That is, unless there are only folders in the gamelist in which case the folders will be handled like files.

**Sort favorite games above non-favorites**

Whether to sort your favorite games above your other games in the gamelists.

**Add star markings to favorite games**

With this setting enabled, there is a star symbol added at the beginning of the game name in the gamelist views. It's strongly recommended to keep this setting enabled if the option to sort favorite games above non-favorites has been enabled. If not, favorite games would be sorted on top of the gamelist with no visual indication that they are favorites, which would be very confusing.

**Enable quick list scrolling overlay**

With this option enabled, there will be an overlay displayed when scrolling the gamelists quickly, i.e. when holding down the _Up_, _Down_, _Left shoulder_ or _Right shoulder_ buttons for some time. The overlay will darken the background slightly and display the first two characters of the game name. If the game is a favorite and the setting to sort favorites above non-favorites has been enabled, a star will be shown instead.

**Enable shortcut to toggle favorites**

This setting enables the _Y_ button for quickly toggling a game as favorite. Although this may be convenient at times, it's also quite easy to accidentally remove a favorite tagging of a game when using the application more casually. As such it could sometimes make sense to disable this functionality. It's of course still possible to mark a game as favorite using the metadata editor when this setting is disabled. For additional restrictions, the application can be set to Kid or Kiosk mode as is explained [elsewhere](USERGUIDE.md#ui-modes) in this guide. Note that this setting does not affect the functionality to use the _Y_ button to add games to custom collections.

**Enable gamelist filters**

Activating or deactivating the ability to filter your gamelists. This can normally be left enabled.

**Enable quick system select**

If enabled, it will be possible to jump between gamelists using the _Left_ and _Right_ buttons without having to first go back to the system view.

**Display on-screen help**

Activating or deactivating the built-in help system that provides contextual information regarding button usage.

**Play videos immediately (override theme)**

Some themes (including rbsimple-DE) display the game images briefly before playing the game videos. This setting forces the videos to be played immediately, regardless of the configuration in the theme. Note though that if there is a video available for a game, but no images, the video will always start to play immediately no matter the theme configuration or whether this settings has been enabled or not.

#### Screensaver settings

Settings for the built-in screensaver.

**Start screensaver after (minutes)**

After how many minutes to start the screensaver. If set to 0 minutes, the timer will be deactivated and the screensaver will never start automatically. It's however still possible to start the screensaver manually in this case, assuming the _Enable screensaver controls_ setting is enabled. Note that while any menu is open, the screensaver will not start regardless of how this timer setting is configured.

**Screensaver type**

The screensaven type to use; _Dim_, _Black_, _Slideshow_ or _Video_.

**Enable screensaver controls**

This includes the ability to start the screensaver manually using the _Select_ button from the system view, but also while the screensaver is running to jump to a new random game using the _Left_ and _Right_ buttons, to launch the game currently shown using the _A_ button and to jump to the game in its gamelist using the _Y_ button. If this setting is disabled, any key or button press will stop the screensaver.

#### Slideshow screensaver settings

Options specific to the slideshow screensaver.

**Swap images after (seconds)**

For how long to display images before changing to the next game. Allowed range is between 2 and 120 seconds in 2-second increments. The default value is 10 seconds.

**Stretch images to screen resolution**

This will fill the entire screen surface but will possibly break the aspect ratio of the image.

**Display game info overlay**

This will display an overlay in the upper left corner, showing the game name and the game system name. A star following the game name indicates that it's a favorite.

**Render scanlines** _(OpenGL renderer only)_

Whether to use a shader to render scanlines for the images.

**Use custom images**

Using this option, it's possible to use custom images instead of random images from the game library. As is the case with the rest of ES-DE, the supported file formats are JPG and PNG.

**Custom image directory recursive search**

Whether to search the custom image directory recursively.

**Custom image directory**

The directory for the custom images.

#### Video screensaver settings

Options specific to the video screensaver.

**Swap videos after (seconds)**

For how long to play videos before changing to the next game. Allowed range is between 0 and 120 seconds in 2-second increments. If set to 0 (which is the default setting), the next game will be selected after the entire video has finished playing.

**Play audio for screensaver videos**

Muting or playing the audio.

**Stretch videos to screen resolution**

This will fill the entire screen surface but will possibly break the aspect ratio of the video.

**Display game info overlay**

This will display an overlay in the upper left corner, showing the game name and the game system name. A star following the game name indicates that it's a favorite.

**Render scanlines** _(OpenGL renderer only)_

Whether to use a shader to render scanlines for the videos. Be aware that this is quite demanding for the GPU.

**Render blur** _(OpenGL renderer only)_

Whether to use a shader to render a slight horizontal blur which somewhat simulates a well-used CRT monitor. Be aware that this is quite demanding for the GPU.


### Sound settings

General sound settings.

**System volume** _(Linux and Windows only)_

As the name implies, this sets the overall system volume and not the volume specifically for ES-DE. Note that the volume change is applied only after leaving the sound settings menu.

**Navigation sounds volume**

Sets the volume for the navigation sounds.

**Video player volume**

Sets the volume for the video player. This applies to both the gamelist views and the video screensaver.

**Play audio for videos in the gamelist view**

With this turned off, audio won't play for game videos in the gamelists.

**Enable navigation sounds**

Enable or disable navigation sounds throughout the application. Sounds are played when browsing systems and lists, starting games, adding and removing games as favorites etc. The sounds can be customized per theme, but if the theme does not support navigation sounds, ES-DE will fall back to its built-in sounds.


### Game collection settings

Handles collections, which are built using the games already present in your game systems. See the [collections](USERGUIDE.md#game-collections) section below in this document for more information.

**Finish editing _'COLLECTION NAME'_ collection** _(Entry only displayed when editing a custom collection)_

Self explanatory.

**Automatic game collections**

This opens a screen that lets you enable or disable the automatic game collections _All games, Favorites_ and _Last played_.

**Custom game collections**

This lets you enable or disable your own custom game collections.

**Create new custom collection from theme** _(Entry only displayed if the ability is provided by the theme set)_

If the theme set in use provides themes for custom collections, then this entry can be selected here. For example, there could be themes for _"Fighting games"_ or _"Driving games"_ etc. The default rbsimple-DE theme set does not provide such themes for custom collections and in general it's not recommended to use this approach, as is explained [later](USERGUIDE.md#custom-collections) in this guide.

**Create new custom collection**

This lets you create a completely custom collection with a name of your choice.

**Delete custom collection**

This permanently deletes a custom collection, including its configuration file on the file system. A list of available collections is shown, and a confirmation dialog is displayed before committing the actual deletion. Only one collection at a time can be deleted.

**Sort favorites on top for custom collections**

Whether to sort your favorite games above your other games. This is disabled by default, as for collections you probably want to be able to mix all games regardless of whether they are favorites or not.

**Display star markings for custom collections**

With this option enabled, there is a star marking added to each favorite game name. It works identically to the setting _Add star markings to favorite games_ under the _UI settings_ but is applied specifically to custom collections. It's disabled by default.

**Group unthemed custom collections**

With this enabled, if you have created custom collections and there is no theme support for the names you've selected, the collections will be grouped in a general collection which is correctly themed. It's strongly recommended to keep this option enabled as otherwise your collections would be completely unthemed which doesn't make much sense. This option is provided mostly for testing and theme development purposes.

**Show system names in collections**

Enables the system name to be shown in square brackets after the game name, for example _CONTRA [NES]_ or _DOOM [DOS]_. This is applied to both automatic and custom collections. It's recommended to keep this option enabled.

### Other settings

These are mostly technical settings.

**VRAM limit**

The amount of video RAM to use for the application. Defaults to 256 MiB which works fine most of the time when running at 1080p resolution and with a moderate amount of game systems. If running at 4K resolution and with lots of game systems enabled, it's recommended to increase this number to 512 MiB or so to avoid stuttering transition animations caused by unloading and loading of textures from the cache. Enabling the GPU statistics overlay gives some indications regarding the amount of texture memory currently used by ES-DE, which is helpful to determine a reasonable value for this option. The allowed range for the settings is 80 to 1024 MiB. If you try to set it lower or higher than this by passing such values as command line parameters or by editing the es_settings.cfg file manually, ES-DE will log a warning and automatically adjust the value within the allowable range.

**Display/monitor index (requires restart)**

This option sets the display to use for ES-DE for multi-monitor setups. The possible values are the monitor index numbers 1, 2, 3 or 4. If a value is set here for a display that does not actually exist, then ES-DE will set it to 1 upon startup. Index 1 is the primary display of the computer. It's also possible to override the setting by passing the --display command line argument. Doing so will also overwrite the display index setting in es_settings.cfg. Be aware that the Display/monitor index option only changes the display used by ES-DE; the emulators need to be configured separately (which can easily be done globally if using RetroArch).

**Fullscreen mode (requires restart)** _(Unix only)_

This gives you a choice between _Normal_ and _Borderless_ modes. With the borderless being more seamless as the ES-DE window will always stay on top of other windows so the taskbar will not be visible when launching and returning from games. It will however break the alt-tab application switching of your window manager. For normal fullscreen mode, if a lower resolution than the screen resolution has been set via the --resolution command line argument, ES-DE will render in full screen at the lower resolution. If a higher resolution than the screen resolution has been set, ES-DE will run in a window. For the borderless mode, any changes to the resolution will make ES-DE run in a window.

**When to save game metadata**

The metadata for a game is updated by scraping and by manual editing (using the metadata editor), but also when launching it as this updates the _Times played_ counter and the _Last played_ date. This setting enables you to define when to write such metadata changes to the gamelist.xml files. Setting the option to _Never_ will disable writing to these files altogether, except for some special conditions such as when a game is manually deleted using the metadata editor, or when scraping using the multi-scraper (the multi-scraper will always save any updates immediately to the gamelist.xml files). In theory _On exit_ will give some performance gains, but it's normally recommended to leave the setting at its default value which is _Always_. Note that with the settings set to _Never_, any updates such as the _Last played_ date will still be shown on screen, but during the next application startup, any values previously saved to the gamelist.xml files will be read in again. As well, when changing this setting to _Always_ from either of the two other options, any pending changes will be immediately written to the gamelist.xml files.

**Game media directory**

This setting defines the directory for the game media, i.e. game images and videos. The default location is _~/.emulationstation/downloaded_media_

**Emulator core path**

This setting defines the path for which to search for emulator cores. This is used by the variable %COREPATH% which can be included in the systems configuration file es_systems.cfg. By default this variable and corresponding setting is only used on Unix. For macOS and Windows the %COREPATH% variable is not included in the es_systems.cfg template and the default core path value is therefore set to blank. If required for special setups it can be used for these operating systems, but the primary use is on Unix where the core path may vary depending on the operating system, how the emulator was packaged etc. For example the default RetroArch core directory is ~/.config/retroarch/cores if compiled from source code but if installed as a Snap package or as part of the OS repository the cores could be stored elsewhere. The setting is primarily intended for RetroArch but it can be used for any emulator that utilizes discrete emulator cores. When attempting to launch a game, the core for the game system will be searched in each of the defined directories until the first match occurs. Multiple directories can be defined by separating them using colons on Unix and macOS and by semicolons on Windows. Please see [INSTALL.md](INSTALL.md#es_systemscfg) for more information about this.

**Hide taskbar (requires restart)** _(Windows only)_

With this setting enabled, the taskbar will be hidden when launching ES-DE, and it will be restored when the application exits. This can make for a more seamless experience as the taskbar could otherwise flash by briefly when launching games and when returning from games. It could potentially cause some issues on some Windows installations though, so the option is disabled by default.

**Run in background (while game is launched)** _(Windows only)_

This is really a last-resort setting if ES-DE freezes when launching games. This issue seems to only occur on Windows 8.1 and older but that's not fully confirmed. ES-DE will behave a bit strange with this option enabled so keep it disabled unless you absolutely need it.

**Per game launch command override**

If enabled, you can override the launch command defined in es_systems.cfg on a per-game basis. It's only recommended to disable this option for testing purposes, such as when a game won't start and you're unsure if it's your custom launch command that causes the problem.

**Show hidden files and folders (requires restart)**

If this option is disabled, hidden files and folders within the ROM directory tree are excluded from ES-DE. On Unix this means those starting with a dot, and on Windows it's those set as hidden by using an NTFS attribute. This setting is probably mostly useful for special situations and is not to be confused with the next option which hides files based on metadata configuration within ES-DE.

**Show hidden games (requires restart)**

You can mark games as hidden in the metadata editor, which is useful for instance for DOS games where you may not want to see some batch files and executables inside ES-DE, or for multi-disk games where you may only want to show the .m3u playlists and not the individual game files, as is discussed [here](USERGUIDE.md#multiple-gamefiles-installation). By disabling this option these files will not be processed at all when ES-dE starts up. If you enable the setting you will see the files, but their name entries will be almost transparent in the gamelist view to visually indicate that they are hidden.

**Enable custom event scripts**

It's possible to trigger custom scripts for a number of actions in ES-DE, as is discussed [below](USERGUIDE.md#custom-event-scripts), and this setting decides whether this functionality is enabled. It's recommended to leave it at its default off value unless you need it as it generates unnecessary debug logging.

**Only show ROMs from gamelist.xml files**

If enabled, only ROMs that have metadata saved to the gamelist.xml files will be shown in ES-DE. This option is intended primarily for testing and debugging purposes so it should normally not be enabled.

**Display game media from ROM directories**

Using this option, you can place game images and videos in the ROM directory tree. The media files are searched inside the directory _\<ROM directory\>/\<system name\>/images/_ and _\<ROM directory\>/\<system name\>/videos/_ and the filenames must match the ROM names, followed by a dash and the media type. For example _~/ROMs/nes/images/Contra-screenshot.jpg, ~/ROMs/nes/images/Contra-marquee.jpg_ and _~/ROMs/nes/videos/Contra-video.jpg_. This option is mostly intended for legacy purposes, if you have an existing game collection with this media setup that you would like to open in ES-DE. The scraper will never save files to this directory structure and will instead use the standard media directory logic. It's recommended to keep this option disabled unless you really need it since it slows down the application somewhat.

**Disable desktop composition (requires restart)** _(Unix only)_

The window manager desktop composition can adversely affect the framerate of ES-DE, especially on weaker graphic cards and when running in 4K resolution. As such the desktop compositor is disabled by default, although the window manager has to be configured to allow applications to do this for the option to have any effect. Note that this setting can cause problems with some graphic drivers (notably the Nvidia proprietary drivers) so if you see strange flickering and similar after quitting ES-DE, then disable the setting. In case of such issues, make sure that the emulator is also not blocking the composition (e.g. RetroArch has a corresponding option).

**Display GPU statistics overlay**

Displays the framerate and VRAM statistics as an overlay. You normally never need to use this unless you're debugging a performance problem or similar. **Note:** As of ES-DE version 1.0 the VRAM usage statistics is not accurate. This will be addressed in a future version.

**Enable menu in kid mode**

Enabling or disabling the menu when the UI mode is set to Kid. Mostly intended for testing purposes as it's normally not recommended to enable the menu in this restricted mode.

**Show quit menu (reboot and power off entries)** _(Unix and Windows only)_

With this setting enabled, there is a Quit menu shown as the last entry on the main menu which provides options to quit ES-DE, to reboot the computer or to power off the computer. With this setting disabled, there will simply be an entry shown to quit the application instead of the complete quit menu.


### Configure input

This tool allows the configuration of button mappings for known or new input devices, as is explained [here](USERGUIDE.md#input-device-configuration).

### Quit
The menu where you quit ES-DE, or reboot or power off your computer. This menu can be disabled using an option, and in this case it's replaced with a _Quit EmulationStation_ entry.

**Quit EmulationStation**

If the option _When to save game metadata_ has been set to _On exit_, the gamelist.xml files will be updated at this point.

**Reboot system** _(Unix and Windows only)_

Self explanatory.

**Power off system** _(Unix and Windows only)_

Self explanatory.


## Game options menu

This menu is opened from the gamelist views, and can't be accessed from the system view. The menu changes slightly depending on the context, for example if a game file or a folder is selected, or whether the current system is a collection or a normal game platform.

You open the menu using the **Select** button, and by pressing **B** or selecting the **Apply** button any settings such as letter jumping using the quick selector or sorting changes are applied. If you instead press the Select button again, the menu is closed without applying any changes.

![alt text](images/current/es-de_game_options_menu.png "ES-DE Game Options Menu")
_The game options menu as laid out when opening it from within a custom collection, which adds the menu entry to add or remove games from the collection._

Here's a summary of the menu entries:

### Jump to..

This provides the ability to jump to a certain letter using a quick selector. If the setting to sort favorite games above non-favorites has been selected (it is enabled by default), then it's also possible to jump to the favorites games by choosing the star symbol. If there are only folders or only favorite games in a certain game list, these games and folders will be indexed as well, making it possible to jump betwen them using the quick selector.

### Sort games by

This is the sort order for the gamelist. The default sorting shown here is taken from the setting **Default sort order** under **UI settings** in the main menu. Any sorting that is applied via the game options menu will be persistent throughout the program session, and it can be set individually per game system and per collection.

Sorting can be applied using the following metadata, in either ascending or descending order:

* Filename
* Rating
* Release date
* Developer
* Publisher
* Genre
* Players
* Last played
* Times played
* System _(Only for collections)_

The secondary sorting is always in ascending filename order.

### Filter gamelist

Choosing this entry opens a separate screen where it's possible to apply a filter to the gamelist. The filter is persistent throughout the program session, or until it's manually reset. The option to reset all filters is also shown on the same screen.

![alt text](images/current/es-de_gamelist_filters.png "ES-DE Gamelist Filters")
_The gamelist filter screen, accessed from the game options menu._

The following filters can be applied:

**Text Filter (Game name)**

**Favorites**

**Genre**

**Players**

**Publisher / Developer**

**Rating**

**Kidgame**

**Completed**

**Broken**

**Hidden**

With the exception of the text filter, all available filter values are assembled from metadata from the actual gamelist, so if there for instance are no games marked as completed, the Completed filter will only have the selectable option False, i.e. True will be missing.

Be aware that although folders can have most of the metadata values set, the filters are only applied to files (this is also true for the text/game name filter). So if you for example set a filter to only display your favorite games, any folder that contains a favorite game will be displayed, and other folders which are themselves marked as favorites but that do not contain any favorite games will be hidden.

The filters are always applied for the complete game system, including all folder content.

### Add/remove games to this collection

This entry is only shown if the system is a custom collection. The way this works is described in more detail [below](USERGUIDE.md#custom-collections).

### Finish editing _'COLLECTION NAME'_ collection

This entry is only visible if the system is a custom collection and it's currently being edited.

### Edit this game's metadata / Edit this folder's metadata

This opens the metadata editor for the currently selected game file or folder.


## Metadata editor

In the metadata editor, you can modify the metadata for a game, scrape for game info and media files, delete media files and gamelist entries, or delete the entire game. When manually modifying a value, it will change color from gray to blue, and if the scraper has changed a value, it will change to red. When leaving the metadata editor you will be asked whether you want to save any settings done manually or by the scraper.

![alt text](images/current/es-de_metadata_editor.png "ES-DE Metadata Editor")
_The metadata editor._

### Metadata entries

The following entries can be modified (note that some of these are not available for folders, only for game files):

**Name**

This is the name that will be shown when browsing the gamelist. If no sortname has been defined, the games are sorted using this field.

**Sortname** _(files only)_

This entry makes it possible to change the sorting of a game without having to change its name. For instance it can be used to sort _Mille Miglia_ as _1000 Miglia_ or _The Punisher_ as _Punisher, The_. Be aware though that the _Jump to..._ quick selector on the game options menu will base its index on the first character of the sortname if it exists for a game, which could be slightly confusing in some instances when quick jumping in the gamelist.

**Description**

Usually provided by the scraper although it's possible to update this manually or write your own game description.

**Rating**

Rating in half-star increments. This can be set as such manually or it can be scraped, assuming the scraper service provides ratings (currently only ScreenScraper does). If an external scraper application such as [Skyscraper](https://github.com/muldjord/skyscraper) has been used that may set the ratings in fractional values such as three-quarter stars, then ES-DE will round them to the nearest half-star. When this happens, the rating stars will be colored green to notify that a rounding has taken place and a question will be asked whether to save the changes even if no other manual changes were performed.

**Release date**

Release date in ISO 8601 format (YYYY-MM-DD).

**Developer**

Developer of the game.

**Publisher**

Publisher of the game.

**Genre**

One or multiple genres for the game.

**Players**

The amount of players the game supports.

**Favorite**

A flag to indicate whether this is a favorite. Can also be set directly from the gamelist view by using the _Y_ button (unless this has been disabled in the main menu settings).

**Completed**

A flag to indicate whether you have completed the game.

**Kidgame** _(files only)_

A flag to mark whether the game is suitable for children. This will be applied as a filter when starting ES-DE in _Kid_ mode.

**Hidden**

A flag to indicate that the game is hidden. If the corresponding option has been set in the main menu, the game will not be shown. Useful for example for DOS games to hide batch scripts and unnecessary binaries or to hide the actual game files for multi-disk games. If a file or folder is flagged as hidden but the correponding option to hide hidden games has not been enabled, then the opacity of the text will be lowered significantly to make it clear that it's a hidden game.

**Broken/not working**

A flag to indicate whether the game is broken. Useful for MAME games for instance where future releases may make the game functional.

**Exclude from game counter** _(files only)_

A flag to indicate whether the game should be excluded from being counted. If this is set for a game, it will not be included in the game counter shown per system on the system view, and it will not be included in the system information field in the gamelist view. As well, it will be excluded from all automatic and custom collections. This option is quite useful for multi-file games such as multi-disk Amiga or Commodore 64 games, or for DOS games where you want to exclude setup programs and similar but still need them available in ES-DE and therefore can't hide them. Files that have this flag set will have a lower opacity in the gamelists, making them easy to spot.

**Exclude from multi-scraper**

Whether to exclude the file from the multi-scraper. This is quite useful in order to avoid scraping all the disks for multi-disk games for example. There is an option in the scraper settings to ignore this flag, but by default the scraper will respect it. Note that the manual single-file scraper will work regardless of whether this flag is set or not.

**Hide metadata fields**

This option will hide most metadata fields in the gamelist view. The intention is to be able to hide the fields for situations such as general folders (Multi-disk, Cartridges etc.) and for setup programs and similar (e.g. SETUP.EXE or INSTALL.BAT for DOS games). It could also be used on the game files for multi-disk games where perhaps only the .m3u playlist should have any metadata values. The only fields shown with this option enabled are the game name and description. Using the description it's possible to write some comments regarding the file or folder, should you want to. It's also possible to display game images and videos with this setting enabled.

**Launch command** _(files only)_

Here you can override the launch command for the game, for example to use a different emulator than the default one for the game system. Very useful for MAME/arcade games.

**Times played** _(files only)_

A statistics counter that tracks how many times you have played the game. You normally don't need to touch this, but if you want to, the possibility is there.

### Buttons

For game files, there will be five buttons displayed on the bottom of the metadata editor window, and for folders there will be four. These are their functions:

**Scrape**

Opens the single-game scraper, which is explained [above](USERGUIDE.md#single-game-scraper) in this guide. The _Y_ button can also be used as a shortcut to start the scraper without having to navigate to this button.

**Save**

Saves any changes and closes the window.

**Cancel**

Cancels any changes and closes the window.

**Clear**

This will remove any media files for the file or folder and also remove its entry from the gamelist.xml file. The actual game file or folder will however _not_ be deleted. A prompt will be shown asking for confirmation.

**Delete** _(Files only)_

This will remove the actual game file, its gamelist.xml entry, its entry in any custom collections and its media files. A prompt will be shown asking for confirmation. The deletion of folders is not supported as that would potentially be a bit dangerous, instead use the appropriate operating system tools to handle deletion of folders.


## Screensaver

There are four types of screensavers built into ES-DE: **Dim**, **Black**, **Slideshow** and **Video**.

Numerous options can be set for these screensavers, as detailed [here](USERGUIDE.md#screensaver-settings).

The Dim screensaver simply dims and desaturates the current view and Black will show a black screen. The Slideshow and Video screensavers are more interesting as they can display images and videos from your game collection. In addition to this, the Slideshow screensaver can be configured to only show images from a specified directory.

If the option **Enable screensaver controls** has been activated, you can manually toggle the screensaver from the system view by pressing the **Select** key. In addition to this, for the Slideshow and Video screensavers, the controls will allow you to jump to a new random image or video using the **Left** and **Right** buttons on your keyboard or controller. It's also possible to launch the game currently displayed using the **A** button, and the **Y** button will jump to the game in its gamelist without starting it.

![alt text](images/current/es-de_screensaver.png "ES-DE Screensaver")
_An example of what the video screensaver looks like._

## Game collections

ES-DE provides two types of collections, **Automatic collections** and **Custom collections**, the latter being defined by the user. Collections are as the name implies only collections of games already present in your actual game systems, so they're basically grouping your games together into convenient views. As such the use of collections is entirely optional, but it is a very nice feature and it's worth some effort to setup.

The numerous collection settings available are covered [here](USERGUIDE.md#game-collection-settings).

### Automatic collections

The automatic collections are named **All games**, **Favorites** and **Last played**. The 'All games' collection simply groups all your game systems into one big list, 'Favorites' combines all your games marked as favorites from all your game systems, and 'Last played' is a list of the 50 last games you have launched.

These automatic collections can be individually enabled or disabled by going to the main menu, selecting **Game collection settings** and then **Automatic game collections**.

Note that you should only enable these collections if you really need them as they slow down the application quite a bit. By default these collections are therefore disabled.

### Custom collections

These are collections that you create yourself. Examples could be grouping in genres like `Shoot em up`, `Fighting games` etc. or perhaps a time period like `1980s`, `1990s` and so on.

If the theme set supports it, you can create a custom collection directly from a theme. However, rbsimple-DE does not provide such themes as it's believed that grouping them together in a dedicated Collections system is a more elegant solution. Especially since the theme set would need to ship with an almost endless amount of collection themes for whatever categories the users would like to use for their game collections.

So if you have enabled the option **Group unthemed custom collections** (it's enabled by default), any collections you add will show up in the special Collections system. Here you can access them just as you would access folders inside a regular gamelist. The amount of games per collection is shown in the description, and a random game is displayed each time you browse through the list (you can quick jump to this game by pressing the **Y** button).

To create a custom collection, go to **Game collection settings** from the main menu and choose **Create new custom collection**.

Choose a name and press enter, let's use the name `Platform` for this example.

The collection will now be created and the collection edit mode will be entered. You can now add games to the collection by navigating to any gamelist and pressing the **Y** button. Any number of games from any of your game systems can be added. A game can also be part of multiple collections, there is no real limit for this in ES-DE.

Removing games works the same way, just press **Y** to remove it if it's already present in your collection. You can do this either from the gamelist where the game was added, or from the collection itself.

Only files can be part of collections, not folders. As well, games marked as hidden or to not be counted as games can't be added either.

During the time that the collection is being edited, any game that is part of the collection is marked with a leading tick symbol in the game name.

When you are done adding games, you can either open the main menu and go to **Game collection settings** and select the **Finish editing 'Platform' collection** or you can open the game options menu and select the same option there. The latter works from within any game system, so you don't need to first navigate back to the collection that you're editing.

You can later add additional games to the collection by navigating to it, bringing up the game options menu and choosing **Add/remove games to this game collection**.

![alt text](images/current/es-de_custom_collections.png "ES-DE Custom Collections")
_Example of custom collections, here configured as genres._

![alt text](images/current/es-de_custom_collections_editing.png "ES-DE Custom Collections")
_When editing a custom collection, a tick symbol will be displayed for any game that is already part of the collection._


The way that custom collections are implemented is very simple. There is a single configuration file per collection inside the folder `~/.emulationstation/collections`

For this example a file will have been created named `~/.emulationstation/collections/custom-platform.cfg`

The file contents is simply a list of ROM files, such as the following:

```
%ROMPATH%/amiga/Flashback_v3.2_1163.hdf
%ROMPATH%/amiga/JamesPond2_v1.1_AGA_1354.hdf
%ROMPATH%/amiga/Nebulus_v1.3_0361.hdf
%ROMPATH%/c64/Bionic Commando.d64
%ROMPATH%/c64/Great Giana Sisters, The.d64
%ROMPATH%/c64/Trantor.d64
%ROMPATH%/c64/Zorro.d64
```

Any changes to custom collections (for example adding or removing a game) will be immediately written to the corresponding collection configuration file.

Note that if you copy or migrate a collection from a previous version of EmulationStation or if you're setting up ES-DE on a new computer, even though you copy the files into the collections directory, they will not show up inside the application as you always need to enable the collections from the menu. ES-DE looks inside the es_settings.cfg file during startup to see which collections should be enabled.

If you're migrating from a previous version of EmulationStation that has absolute paths in the collection files, these will be rewritten with the %ROMPATH% variable the first time you make a change to the collection.


## Themes

ES-DE is fully themeable, and although the application ships with the comprehensive rbsimple-DE theme set, you can replace it with a number of themes available from various locations on the Internet.

Somewhat confusingly the terms _theme_ and _theme set_ are used to refer to the same thing. The technically correct term for what you apply to the application to achieve a different look is actually _theme set_ as it's a collection of a number of themes for a number of game systems. The bundled rbsimple-DE is an example of such a theme set. But in this guide and in other EmulationStation resources on the Internet, the term theme is often used to refer to the same thing as a theme set.

Note that this Desktop Edition fork adds additional features to the themes and more still will be added in future versions. This means that you may not get the full benefits of the application if you're using a different theme set. But effort is spent trying to make ES-DE backwards compatible with the available themes used by other EmulationStation versions. The exception to this are some themes made for the Recalbox and Batocera forks of EmulationStation as they have added a lot of additional theme functionality that ES-DE has no intention to replicate.

Themes are most easily installed in your ES-DE home directory, i.e. `~/.emulationstation/themes`. By just adding the theme sets there, one folder each, they will be found by ES-DE during startup and you can then choose between them via UI Settings on the main menu.

Note that although you can put additional themes in your ES-DE home directory, the default rbsimple-DE theme is located in your installation folder. For example this could be `/usr/share/emulationstation/themes` or `/usr/local/share/emulationstation/themes` on Unix, `/Applications/EmulationStation Desktop Edition.app/Contents/Resources/themes` on macOS or `C:\Program Files\EmulationStation-DE\themes` on Windows.

If you would like to customize the rbsimple-DE theme, simply make a copy of the complete rbsimple-DE directory to ~/.emulationstation/themes and then that copy of the theme will take precedence over the one in the application installation directory.

In this example, we've downloaded the [Carbon](https://github.com/RetroPie/es-theme-carbon) and [Fundamental](https://github.com/G-rila/es-theme-fundamental) themes and uncompressed them to the ES-DE home folder:

```
~/.emulationstation/themes/es-theme-carbon
~/.emulationstation/themes/es-theme-fundamental
```

You would now have three entries for the Theme set selector in the UI settings menu, i.e. rbsimple-DE, es-theme-carbon and es-theme-fundamental.

Here are some resources where additional theme sets can be downloaded.

https://aloshi.com/emulationstation#themes

https://github.com/RetroPie

https://gitlab.com/recalbox/recalbox-themes

https://wiki.batocera.org/themes

![alt text](images/current/es-de_ui_theme_support.png "ES-DE Theme Support")
_An example of a modified version of the [Fundamental](https://github.com/G-rila/es-theme-fundamental) theme applied to ES-DE._



## Custom event scripts

There are numerous locations throughout ES-DE where custom scripts will be executed if the option to do so has been enabled in the settings. By default it's deactivated so be sure to enable it to use this feature.

The setup for event scripts is a bit technical, so please refer to the [INSTALL.md](INSTALL.md#custom-event-scripts) document to see how it's configured.


## Portable installation (Windows only)

On Windows, ES-DE can be installed to and run from a removable media device such as a USB memory stick. Together with games and emulators this makes for a fully portable retro gaming solution. The setup is somewhat technical, please refer to the [INSTALL.md](INSTALL.md#building-on-windows) document to see how it's done.


## Command line arguments

Please refer to the [INSTALL.md](INSTALL.md#command-line-arguments) document for a list of the command line arguments per operating system.


## Supported game systems

**Note:** The following list is what the default es_systems.cfg files and the rbsimple-DE theme supports. This theme set is very comprehensive, so if you're using another theme, it may be that some or many of these systems are not supported. ES-DE will still work but the game system will not be themed which looks very ugly.

Note as well that the list and corresponding es_systems.cfg templates may not reflect what is readily available for all supported operating system. This is especially true on Unix/Linux if installing RetroArch via the OS repository instead of using the Snap or Flatpak distributions (or compiling from source code) as the repository versions are normally quite crippled.

The column **Game system name** corresponds to the directory where you should put your game files, e.g. `~/ROMs/c64` or `~/ROMs/megadrive`.

Regional differences are handled by simply using the game system name corresponding to your region. For example for Sega Mega Drive, _megadrive_ would be used by most people in the world, although persons from North America would use _genesis_ instead. The same is true for _pcengine_ vs _tg16_ etc. This only affects the theme selection and the corresponding theme graphics, the same emulator and scraper settings are still used for the regional variants although that can of course be modified in the es_systems.cfg file if you wish.

Sometimes the name of the console is (more or less) the same for multiple regions, and in those circumstances the region has been added as a suffix to the game system name. For instance `na` for North America has been added to `snes` (Super Nintendo) giving the system name `snesna`. The same goes for Japan, as in `megacd` and `megacdjp`. Again, this only affects the theme and theme graphics.

For the **Full name** column, text inside square brackets [] are comments and not part of the actual game system name.

The **Default emulator** column shows the emulator configured in es_systems.cfg, and for emulators that support multiple cores, the configured core is shown inside brackets. Any system marked with an asterisk (*) in this column requires additional system/BIOS ROMs to run, as should be explained in the emulator documentation.

For additional details regarding which game file extensions are supported per system, refer to the es_systems.cfg templates [es_systems.cfg_unix](resources/templates/es_systems.cfg_unix), [es_systems.cfg_macos](resources/templates/es_systems.cfg_macos) and [es_systems.cfg_windows](resources/templates/es_systems.cfg_windows). Normally the extensions setup in these files should cover everything that the emulators support.

MAME emulation is a bit special as the choice of emulator or core depends on which ROM set you're using. It's recommended to go for the latest available set, as MAME is constantly improved with more complete and accurate emulation. Therefore the default `arcade` system is preconfigured to use the RetroArch core _MAME - Current_ which as the name implies will be the latest available MAME version. For really slow computers though, the 0.78 ROM set is a popular choice. To use this you either need to modify the es_systems.cfg file, or you can use the `mame` system which comes preconfigured for the RetroArch core _MAME 2003-Plus_. There are other alternatives as well such as _MAME 2010_ that uses the 0.139 ROM set but this would require a manual change of the es_systems.cfg file and is generally not recommended.

There are other MAME versions and derivates available as well such as MAME4ALL, AdvanceMAME, FinalBurn Alpha and FinalBurn Neo but it's beyond the scope of this document to describe those in detail. For more information, refer to the [RetroPie arcade documentation](https://retropie.org.uk/docs/Arcade) which has a good overview of the various MAME alternatives.

Running RetroArch on macOS is a bit problematic as some cores (e.g. the Nintendo 64 emulators) don't exist at all, and some cores are unusable on older macOS versions as the compilation was done without the necessary backwards compatibility support. On macOS you may therefore need to compile some cores yourself.

Consider the table below a work in progress as it's obvioulsy not fully populated yet!

| Game system name      | Full name                                      | Default emulator                  | Recommended game setup               |
| :-------------------- | :--------------------------------------------- | :-------------------------------- | :----------------------------------- |
| 3do                   | 3DO                                            |                                   |                                      |
| 64dd                  | Nintendo 64DD                                  | RetroArch (Mupen64Plus-Next) [no n64 emulator available on macOS?] |                                      |
| ags                   | Adventure Game Studio game engine              |                                   |                                      |
| amiga                 | Commodore Amiga                                | RetroArch (P-UAE)*                | WHDLoad hard disk image in .hdf or .hdz format in root folder, or diskette image in .adf format in root folder if single-disk, or in separate folder with .m3u playlist if multi-disk |
| amiga600              | Commodore Amiga 600                            | RetroArch (P-UAE)*                | WHDLoad hard disk image in .hdf or .hdz format in root folder, or diskette image in .adf format in root folder if single-disk, or in separate folder with .m3u playlist if multi-disk |
| amiga1200             | Commodore Amiga 1200                           | RetroArch (P-UAE)*                | WHDLoad hard disk image in .hdf or .hdz format in root folder, or diskette image in .adf format in root folder if single-disk, or in separate folder with .m3u playlist if multi-disk |
| amigacd32             | Commodore Amiga CD32                           |                                   |                                      |
| amstradcpc            | Amstrad CPC                                    |                                   |                                      |
| apple2                | Apple II                                       |                                   |                                      |
| apple2gs              | Apple IIGS                                     |                                   |                                      |
| arcade                | Arcade                                         | RetroArch (MAME - Current)*       | Single archive file following MAME name standard in root folder |
| astrocade             | Bally Astrocade                                |                                   |                                      |
| atari2600             | Atari 2600                                     | RetroArch (Stella)                | Single archive or ROM file in root folder |
| atari5200             | Atari 5200                                     |                                   |                                      |
| atari7800             | Atari 7800 ProSystem                           |                                   |                                      |
| atari800              | Atari 800                                      |                                   |                                      |
| atarijaguar           | Atari Jaguar                                   |                                   |                                      |
| atarijaguarcd         | Atari Jaguar CD                                |                                   |                                      |
| atarilynx             | Atari Lynx                                     |                                   |                                      |
| atarist               | Atari ST [also STE and Falcon]                 |                                   |                                      |
| atarixe               | Atari XE                                       |                                   |                                      |
| atomiswave            | Atomiswave                                     |                                   |                                      |
| bbcmicro              | BBC Micro                                      |                                   |                                      |
| c64                   | Commodore 64                                   | RetroArch (VICE x64sc, accurate)  | Single disk, tape or cartridge image in root folder and/or multi-disk images in separate folder |
| cavestory             | Cave Story (NXEngine)                          |                                   |                                      |
| cdtv                  | Commodore CDTV                                 |                                   |                                      |
| chailove              | ChaiLove game engine                           |                                   |                                      |
| channelf              | Fairchild Channel F                            |                                   |                                      |
| coco                  | Tandy Color Computer                           |                                   |                                      |
| colecovision          | ColecoVision                                   |                                   |                                      |
| daphne                | Daphne Arcade Laserdisc Emulator               |                                   |                                      |
| desktop               | Desktop applications                           | N/A                               |                                      |
| doom                  | Doom                                           |                                   |                                      |
| dos                   | DOS (PC)                                       | RetroArch (DOSBox-core)           | In separate folder (one folder per game, with complete file structure retained) |
| dragon32              | Dragon 32                                      |                                   |                                      |
| dreamcast             | Sega Dreamcast                                 |                                   |                                      |
| famicom               | Nintendo Family Computer                       | RetroArch (Nestopia UE)           | Single archive or ROM file in root folder |
| fba                   | FinalBurn Alpha                                | RetroArch (FB Alpha 2012)*        | Single archive file following MAME name standard in root folder |
| fbneo                 | FinalBurn Neo                                  | RetroArch (FinalBurn Neo)*        | Single archive file following MAME name standard in root folder |
| fds                   | Nintendo Famicom Disk System                   | RetroArch (Nestopia UE)*          | Single archive or ROM file in root folder |
| gameandwatch          | Nintendo Game and Watch                        |                                   |                                      |
| gamegear              | Sega Game Gear                                 |                                   |                                      |
| gb                    | Nintendo Game Boy                              |                                   |                                      |
| gba                   | Nintendo Game Boy Advance                      |                                   |                                      |
| gbc                   | Nintendo Game Boy Color                        |                                   |                                      |
| gc                    | Nintendo GameCube                              |                                   |                                      |
| genesis               | Sega Genesis                                   | RetroArch (Genesis Plus GX)       | Single archive or ROM file in root folder |
| gx4000                | Amstrad GX4000                                 |                                   |                                      |
| intellivision         | Mattel Electronics Intellivision               |                                   |                                      |
| kodi                  | Kodi home theatre software                     | N/A                               |                                      |
| lutris                | Lutris open gaming platform                    | Lutris application (Unix only)    | Shell script in root folder          |
| lutro                 | Lutro game engine                              |                                   |                                      |
| macintosh             | Apple Macintosh                                |                                   |                                      |
| mame                  | Multiple Arcade Machine Emulator               | RetroArch (MAME 2003-Plus)*       | Single archive file following MAME name standard in root folder |
| mame-advmame          | AdvanceMAME                                    |                                   | Single archive file following MAME name standard in root folder |
| mame-mame4all         | MAME4ALL                                       |                                   | Single archive file following MAME name standard in root folder |
| mastersystem          | Sega Master System                             | RetroArch (Genesis Plus GX)       | Single archive or ROM file in root folder |
| megacd                | Sega Mega-CD                                   |                                   |                                      |
| megacdjp              | Sega Mega-CD [Japan]                           |                                   |                                      |
| megadrive             | Sega Mega Drive                                | RetroArch (Genesis Plus GX)       | Single archive or ROM file in root folder |
| mess                  | Multi Emulator Super System                    |                                   |                                      |
| moonlight             | Moonlight game streaming                       |                                   |                                      |
| moto                  | Thomson MO/TO series                           | RetroArch (Theodore)              |                                      |
| msx                   | MSX                                            | RetroArch (blueMSX)               |                                      |
| msx1                  | MSX1                                           | RetroArch (blueMSX)               |                                      |
| msx2                  | MSX2                                           | RetroArch (blueMSX)               |                                      |
| msxturbor             | MSX Turbo R                                    | RetroArch (blueMSX)               |                                      |
| multivision           | Othello Multivision                            | RetroArch (Gearsystem)            |                                      |
| naomi                 | Sega NAOMI                                     | RetroArch (Flycast)               |                                      |
| naomigd               | Sega NAOMI GD-ROM                              | RetroArch (Flycast)               |                                      |
| n3ds                  | Nintendo 3DS                                   | RetroArch (Citra)                 |                                      |
| n64                   | Nintendo 64                                    | RetroArch (Mupen64Plus-Next) [no n64 emulator available on macOS?] | Single archive or ROM file in root folder |
| nds                   | Nintendo DS                                    |                                   |                                      |
| neogeo                | SNK Neo Geo                                    | RetroArch (FinalBurn Neo)*        | Single archive file following MAME name standard in root folder |
| neogeocd              | SNK Neo Geo CD                                 | RetroArch (NeoCD)*                | Single archive in root folder (which includes the CD image and ripped audio) |
| neogeocdjp            | SNK Neo Geo CD [Japan]                         | RetroArch (NeoCD)*                | Single archive in root folder (which includes the CD image and ripped audio) |
| nes                   | Nintendo Entertainment System                  | RetroArch (Nestopia UE)           | Single archive or ROM file in root folder |
| ngp                   | SNK Neo Geo Pocket                             |                                   |                                      |
| ngpc                  | SNK Neo Geo Pocket Color                       |                                   |                                      |
| odyssey2              | Magnavox Odyssey2                              |                                   |                                      |
| openbor               | OpenBOR game engine                            |                                   |                                      |
| oric                  | Tangerine Computer Systems Oric                |                                   |                                      |
| palm                  | Palm OS                                        |                                   |                                      |
| pc                    | IBM PC                                         | RetroArch (DOSBox-core)           | In separate folder (one folder per game, with complete file structure retained) |
| pc88                  | NEC PC-8800 series                             | RetroArch (QUASI88)               |                                      |
| pc98                  | NEC PC-9800 series                             | RetroArch (Neko Project II Kai)   |                                      |
| pcengine              | NEC PC Engine                                  | RetroArch (Beetle PCE)            | Single archive or ROM file in root folder |
| pcenginecd            | NEC PC Engine CD                               | RetroArch (Beetle PCE)            |                                      |
| pcfx                  | NEC PC-FX                                      |                                   |                                      |
| pokemini              | Nintendo Pokémon Mini                          |                                   |                                      |
| ports                 | Ports                                          | N/A                               | Shell/batch script in separate folder (possibly combined with game data) |
| ps2                   | Sony PlayStation 2                             |                                   |                                      |
| ps3                   | Sony PlayStation 3                             |                                   |                                      |
| ps4                   | Sony PlayStation 4                             |                                   |                                      |
| psp                   | Sony PlayStation Portable                      |                                   |                                      |
| psvita                | Sony PlayStation Vita                          |                                   |                                      |
| psx                   | Sony PlayStation                               |                                   |                                      |
| residualvm            | ResidualVM game engine                         |                                   |                                      |
| samcoupe              | SAM Coupé                                      |                                   |                                      |
| satellaview           | Nintendo Satellaview                           |                                   |                                      |
| saturn                | Sega Saturn                                    | RetroArch (Beetle Saturn)         |                                      |
| saturnjp              | Sega Saturn [Japan]                            | RetroArch (Beetle Saturn)         |                                      |
| scummvm               | ScummVM game engine                            | RetroArch (ScummVM)               | In separate folder (one folder per game, with complete file structure retained) |
| sega32x               | Sega Mega Drive 32X                            | RetroArch (PicoDrive)             | Single archive or ROM file in root folder |
| sega32xjp             | Sega Super 32X [Japan]                         | RetroArch (PicoDrive)             | Single archive or ROM file in root folder |
| sega32xna             | Sega Genesis 32X [North America]               | RetroArch (PicoDrive)             | Single archive or ROM file in root folder |
| segacd                | Sega CD                                        |                                   |                                      |
| sg-1000               | Sega SG-1000                                   |                                   |                                      |
| snes                  | Nintendo SNES (Super Nintendo)                 | RetroArch (Snes9x - Current)      | Single archive or ROM file in root folder |
| snesna                | Nintendo SNES (Super Nintendo) [North America] | RetroArch (Snes9x - Current)      | Single archive or ROM file in root folder |
| solarus               | Solarus game engine                            |                                   |                                      |
| spectravideo          | Spectravideo                                   |                                   |                                      |
| steam                 | Valve Steam                                    | Steam application                 | Shell script/batch file in root folder |
| stratagus             | Stratagus game engine                          |                                   |                                      |
| sufami                | Bandai SuFami Turbo                            |                                   |                                      |
| supergrafx            | NEC SuperGrafx                                 |                                   |                                      |
| tanodragon            | Tano Dragon                                    |                                   |                                      |
| tg16                  | NEC TurboGrafx-16                              | RetroArch (Beetle PCE)            | Single archive or ROM file in root folder |
| tg-cd                 | NEC TurboGrafx-CD                              | RetroArch (Beetle PCE)            |                                      |
| ti99                  | Texas Instruments TI-99                        |                                   |                                      |
| tic80                 | TIC-80 game engine                             |                                   |                                      |
| to8                   | Thomson TO8                                    | RetroArch (Theodore)              |                                      |
| trs-80                | Tandy TRS-80                                   |                                   |                                      |
| uzebox                | Uzebox                                         |                                   |                                      |
| vectrex               | Vectrex                                        |                                   |                                      |
| videopac              | Philips Videopac G7000 (Magnavox Odyssey2)     |                                   |                                      |
| virtualboy            | Nintendo Virtual Boy                           |                                   |                                      |
| wii                   | Nintendo Wii                                   |                                   |                                      |
| wiiu                  | Nintendo Wii U                                 |                                   |                                      |
| wonderswan            | Bandai WonderSwan                              |                                   |                                      |
| wonderswancolor       | Bandai WonderSwan Color                        |                                   |                                      |
| x1                    | Sharp X1                                       | RetroArch (x1)                    | Single archive or ROM file in root folder |
| x68000                | Sharp X68000                                   |                                   |                                      |
| xbox                  | Microsoft Xbox                                 |                                   |                                      |
| xbox360               | Microsoft Xbox 360                             |                                   |                                      |
| zmachine              | Infocom Z-machine                              |                                   |                                      |
| zx81                  | Sinclair ZX81                                  |                                   |                                      |
| zxspectrum            | Sinclair ZX Spectrum                           |                                   |                                      |

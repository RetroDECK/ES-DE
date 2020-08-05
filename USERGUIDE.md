# EmulationStation Desktop Edition - User Guide

**Note:** This document is intended as a quick start guide, for more in-depth information and details on how to compile EmulationStation and perform more advanced configuration, please refer to the [INSTALL.md](INSTALL.md) document.

**_This guide is currently under construction!_**

Table of contents:

[[_TOC_]]

## Getting started

Getting started with EmulationStation is very easy, just make sure to install the software properly, either manually as built from source code or using one of the supplied packages. On Windows you'll use the installer instead of a package.

The installation procedure will not be covered here as it differs between operating system, so please refer to your operating system documentation for information regarding this topic. EmulationStation Desktop Edition is currently supplied as .deb and .rpm packages for Linux and as a standard NSIS installer for Windows.

The following operating systems have been tested:

* Kubuntu 20.04
* Windows 10 (x86)
* Windows 8.1 (x86)

Upon first startup, ES will create its home directory, by default the location is ~/.emulationstation.

On Unix this defaults to /home/\<username\>/.emulationstation/ and on Windows it defaults to C:\Users\\<username>\\.emulationstation\

A settings file, **es_settings.cfg** will be generated with all the default settings, and a **es_systems.cfg** file will also be copied from the program resource folder. This file contains the game ROM and emulator settings and can be modified if needed. For information on how to do this, refer to the [INSTALL.md](INSTALL.md) document.

There's a log file in the home directory as well named **es_log.txt**, please refer to this in case of any errors as it should provide information on what went wrong.

After ES finds at least one game file, it will populate that game system and the application will start. If there are no game files, an error messsage will be shown, explaining that you need to install your game files into your ROM directory. Please refer to the game installation procedure below in this document.


## Input device configuration

When first starting ES, the application will look for any attached controllers (joysticks and gamepads). If no devices are found, it will be assumed that only keyboard navigation is to be used and the default keyboard mappings will be applied. It's recommended to change these default values, and a message will be displayed describing just this. It's however possible to hide this notification permanently and continue to use the default keyboard mappings indefinitely if you're happy with them.

If a controller is attached when starting ES and no **es_input.cfg** input configuration file exists, you will be presented with the input configuration dialog. Just follow the steps as described to map the inputs.

If an es_input.cfg configuration file exists, you will not be presented with the input device configuration screen as that would normally just be annoying. If you however need to configure a device to control the application (i.e. you've replaced your controller), you can do so by starting ES with the command line argument **--force-input-config** or you can manually delete the es_input.cfg file prior to starting the application. Alternatively you can navigate to the menu using your keyboard and select **Configure input** manually to configure your new device.

The actual procedure to map the inputs should be self-explanatory, just follow the on-screen instructions.


## System view (main screen)

When starting EmulationStation with the default settings, you will see the main screen first. From here you can navigate your game systems and enter their respective gamelists. If there are no game systems installed, you will not see this screen but rather an error message will be displayed, informing you that no games could be found.

Depending on the theme, the system navigation carousel can be either horizontal or vertical. The default theme rbsimple-DE provides horizontal navigation, i.e. you browse your systems be scrolling left or right.


## Gamelist view

The gamelist view is where you browse and start your games, and it's where you will spend most of your time using ES.

Upon startup with the default settings, ES is set to the gamelist view style to **Automatic**. In this mode the application will look for any game media files (videos and images) and set the view style accordingly. If at least one image is found for any game, the view style **Detailed** will be shown, and if at least one video file is found, the view style **Video** will be selected (superceding the Detailed style). If no game media files are found for a system, the simple **Basic** view will be selected. Note that this automatic selection is applied per game system.

It's possible to manually set a specific gamelist view style in the UI settings entry of the main menu, but this is applied globally regardless of what media files are available per game system.

In additions to the styles just described, there is a **Grid** view style as well, but as of version 1.0.0 this is very limited and not recommended. Future versions of EmulationStation may update this style to a more useful state.


## Help system

There is a help system available throughout the application that provides an overview of the possible actions and buttons that can be used. It's possible to disable the help system for a somewhat cleaner look using a menu option. Note that some general button actions are never shown, such as the ability to quick jump in gamelists, menus and text input fields using the shoulder and trigger buttons.


## General navigation

The help system will provide an overview per screen on the navigation options for the application, however here is a general overview. These are the inputs you mapped in the previous input device configuration step. Note that this is not an exhaustive list, but it covers most situations.

**Up and down**

Navigate up and down in gamelists, system view (if the theme has a vertical carousel) and in menus.

**Left and right**

Navigate between gamelists (if _"Quick system select"_ has been activated in the options), or between systems (if the theme has a horizontal carousel).

**Start button**

Opens and closes the main menu.

**Select button**

Opens and closes the game options menu if in the gamelist view, or toggles the screensaver if in the system view (main screen).

**Shoulder buttons left and right**

Provides quick jumping in gamelists and menus, jumps 10 games in the gamelists and 6 entries in the menus. Also jumps forward in text edit dialogs.

**Trigger buttons left and right**

Jumps to the first and last entry of the gamelists, menus and text edit dialogs.

**A button**

Select button to open gamelists from the systems view, start games, choose menu entries etc.

**B button**

Back button, self explanatory.

**X button**

Selects random games and systems.

**Y button**

Marks games as favorites in the gamelist views. Used by some other minor functions as explained by the help system.


## Getting your games into EmulationStation

For most systems, this is very straightforward, just put your game files into the folder corresponding to the platform name. (These names can be found in the end of this document.)

For some systems though, a more elaborate setup is required, and we will attempt to cover such situations in this guide as well.

### Single gamefile installation

Let's start with the simple scenario of a single game file per game platform, which is the case for the majority of systems. In this example we're setting up ES to play Nintendo Entertainment System games.

The ROM files supported are listed in [es_systems.cfg_unix](resources/templates/es_systems.cfg_unix) and [es_systems.cfg_windows](resources/templates/es_systems.cfg_windows). Here is the snippet from the es_systems.cfg_unix for the NES platform:

```
<system>
  <name>nes</name>
  <fullname>Nintendo Entertainment System</fullname>
  <path>%ROMPATH%/nes</path>
  <extension>.nes .NES .unf .UNF .unif .UNIF .7z .7Z .zip .ZIP</extension>
  <command>retroarch -L ~/.config/retroarch/cores/fceumm_libretro.so %ROM%</command>
  <platform>nes</platform>
  <theme>nes</theme>
</system>
```

It's important that the ROM files are in one of the supported file extensions, or ES won't find them.

The default ROM directory folder is ~/ROMs. On Unix this defaults to /home/\<username\>/.emulationstation/ and on Windows it defaults to C:\Users\\<username\>\.emulationstation\

Assuming the default ROM directory is used, we need to create a directory corresponding to the **\<platform\>** tag in es_systems.cfg, in this example it's **nes**.

This would look something like this on Unix and Windows:

```
/home/myusername/ROMs/nes
C:\Users\myusername\ROMs\nes
```

Then simply copy your game ROMs into this folder, and you should end up with something like this:

```
~/ROMs/nes/Legend of Zelda, the.zip
~/ROMs/nes/Metal Gear.zip
~/ROMs/nes/Super Mario Bros. 3.zip
```

**Note: These directories are case sensitive on Unix, so creating a directory called NES instead of nes won't work!**

That's it, start ES and the NES game system should be populated. You can now scrape game information and media for the games, and assuming you've setup RetroArch correctly with the FCEUmm core, you can launch the games.

### Multiple gamefiles installation

For some systems, there are sometimes multiple gamefiles per game. Such an example would be the Commodore 64, when multidisk games are being played. For such instances, simply group the files inside folders.

The platform name for the Commodore 64 is **c64**, so the following structure would be a possible approach:

```
~/ROMs/c64/Cartridge
~/ROMs/c64/Tape
~/ROMs/c64/Disk
~/ROMs/c64/Multidisk
~/ROMs/c64/Multidisk/Last Ninja 2/LNINJA2A.D64
~/ROMs/c64/Multidisk/Last Ninja 2/LNINJA2B.D64
~/ROMs/c64/Multidisk/Last Ninja 2/LNINJA2.m3u
~/ROMs/c64/Multidisk/Pirates/PIRAT-E0.d64
~/ROMs/c64/Multidisk/Pirates/PIRAT-E1.d64
~/ROMs/c64/Multidisk/Pirates/PIRAT-E2.d64
~/ROMs/c64/Multidisk/Pirates/PIRATES.m3u
```

It's highly recommended to create **.m3u** playlist files for multi-disk images as this simplifies the disk swapping in the emulator. It's then this .m3u file that should be selected for launching the game.

It's of course also possible to skip this type of directory structure and put all the games in the root folder, but then there will be multiple entries for the same game which is not so tidy.

When setting up games in this fashion, it's recommended to scrape the directory in addition to the .m3u file as it looks nicer to see images and metadata for the games also when browsing the folders. ES fully supports scraping folders, although some metadata is not included for folders for logical reasons. If you only scrape the folders and not the actual game files, it looks ok when browsing, but when a game is part of a collection, the metadata and images will be missing there. This includes the **Last played** and **All games** collections for instance. Also note that while it's possible to mark a folder as a favorite, it will never be part of a collection, such as **Favorites**.

### Special game installation considerations

Not all systems are as simple as described above, or sometimes there are multiple ways to configure the systems. So specifics to such systems will be covered here. Consider this a work in progress though as there are many systems supported by ES.

#### Arcade and Neo Geo

For all the MAME variants supported as well as Final Burn Alpha/FinalBurn Neo and Neo Geo, single file archives should be used. However these should retain the MAME names as filenames since ES ships with MAME lookup files, meaning the MAME names are expanded to the full game names.

For instance **avsp.7z** will be expanded to **Alien vs. Predator**.

This is used by the TheGamesDB scraper where the expanded file name is used for game searches. (Screenscraper supports native search using the MAME names.) It's also quite nice to have the gamelist populated with the expanded game names even before any scraping has taken place.

#### Commodore Amiga

There are multiple ways to run Amiga games, but the recommended approach is to use WHDLoad. The best way is to use hard disk images in **.hdf** or **.hdz** format, meaning there will be a single file per game. This makes it just as easy to play Amiga games as any console with game ROMs.

An alternative would be to use **.adf** images as not all games may be available with WHDLoad support. For this, you can either put single-disk images in the gamelist root or in a dedicated ADF directory, or multiple-disk games in separate folders. It's highly recommended to create **.m3u** playlist files for multi-disk images as this simplifies the disk swapping in the emulator. It's then this .m3u file that should be selected for launching the game.

This and other topics such as the need for the Amiga Kickstart ROMs is beyond the scope of this guide, but the following page is recommended for reading more about how this setup can be achieved:

[https://github.com/libretro/libretro-uae/blob/master/README.md](https://github.com/libretro/libretro-uae/blob/master/README.md)

#### DOS / PC

The DOS (and PC) platform uses the DOSBox emulator and the recommended approach here is to keep the directory structure intact, just as if running the game on a real DOS computer. So this means one folder per game in ES. It's also recommended to set the metadata field **Count as game** to off for all files but the actual game binary. This is done so that the game counter on the system view correctly reflects the number of games you have installed. It's also possible to mark files and subdirectories as hidden to avoid seeing them in ES. Both of these fields can be set using the metadata editor.

Apart from this the game should work as normal. The game folders can be scraped so that it looks nice when browsing the list of games, but make sure to also scrape the game binary or the entries in the collections **Last played**, **Favorites** and **All games** as well as any custom collections will look strange. If you don't have these collections activated, then this can of course be skipped.

Although the words _game binary_ were just used, it's of course possible to also start a DOS game using a **.bat** batch file. It's DOS after all.

#### Ports

Ports are not really executed using emulators, but is instead software running natively on your operating system. The easiest way to handle these is to add a simple shell script or batch file where you can customize the exact settings for the game.

It's of course possible to add these as single files to the root of the gamelist, but normally it's recommended to setup a separate folder per game as there may be more than a single file available per game. You very often want to have easy access to the game setup utility for instance.

Here's an example for running Chocolate-doom and Quakespasm:

```
~/ROMs/ports/Chocolate-doom/chocolate-doom.sh
~/ROMs/ports/Chocolate-doom/chocolate-doom-setup.sh
~/ROMs/ports/Quakespasm/quakespasm.sh
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

quakespasm.sh:

```
#!/bin/bash
cd ~/games/quake # Required to find the .pak files.
quakespasm
```

Don't forget to make the scripts executable (e.g. 'chmod 755 ./chocolate-doom.sh').

#### Steam

For steam, it's recommended to put shell scripts/batch files directly in the root folder, where the file names of these scripts correspond to the game names. Add the following information to each file:

steam steam://rungameid/\<game ID\>

An example for the game Broforce:

Contents of **~/ROMs/steam/Broforce.sh**:

```
steam steam://rungameid/274190
```

The game ID can be found by going to [https://store.steampowered.com](https://store.steampowered.com) and searching for a game. The Broforce example would have an URL such as this:

https://store.steampowered.com/app/274190/Broforce

On Linux it's very easy to find all your game ID's be looking in the desktop entries.

```
grep steam ~/.local/share/applications/*desktop | grep rungameid
/home/myusername/.local/share/applications/FEZ.desktop:Exec=steam steam://rungameid/224760
/home/myusername/.local/share/applications/INSIDE.desktop:Exec=steam steam://rungameid/304430
/home/myusername/.local/share/applications/Subnautica.desktop:Exec=steam steam://rungameid/264710
```

This of course assumes that you have desktop entries setup for the games in question.


## Emulator setup

EmulationStation is a game browsing frontend and does not provide any emulation by itself. It does however come preconfigured for use with emulators as setup in the **es_systems.cfg** file. By default it's primarily setup for use with [RetroArch](https://www.retroarch.com) but this can be modified if needed. If you're interested in customizing your es_systems.cfg file, please refer to the [INSTALL.md](INSTALL.md) document which goes into details on the structure of this file and more advanced configuration topics in general.

Installation and configuration of RetroArch and other emulators is beyond the scope of this guide, but many good resources can be found online on how to do this.

In order to use the default es_systems.cfg file, you need to make sure that the emulator installation directory is in the operating system's path variable. On Unix systems this is normally not an issue as a package manager has typically been used to install the emulator, and there is a standardized directory structure. But for Windows you may need to add the emulator directory to your %PATH% variable manually. If on Windows, a simple test is to open a command window and type the name of the emulator and press enter, if the emulator is not found, then EmulationStation won't find it either and you need to update your %PATH% variable.

As an alternative, if the emulator is not in your search path, you can either hardcode an absolute path in es_systems.cfg or use the %ESPATH% variable to set the emulator path relative to the EmulationStation location. Again, please refer to the INSTALL.md document on details regarding this.


## Main menu

This menu can be accessed from both the main screen and from the gamelist views. It contains the scraper, the quit menu as well as the application settings. When it comes to saving settings, this is done automatically when navigating back from a screen, or when closing the menu altogether.

Here is a breakdown of the main menu entires:

### Scraper

Contains the various options for the scraper, which is used to download metadata and images for your games.

**Scrape from**

Scraper service selection, currently ScreenScraper.fr and TheGamesDB.net are supported.

**Filter**

Criteria for what games to include in the scraping. It can be set to _All games_, _No metadata_ or _No game image_.

**Systems**

A selection of which systems to scrape for. It's recommended to scrape one system at a time, but it's possible to automatically scrap several or all systems in one go.

#### Content settings

Describes the content types to include in the scraping.

**Scrape game names**

Whether to scrape the names of the games. This does not affect the actual files on the filesystem and is only used for viewing and sorting purposes. The downloaded media files are also matched against the actually game file on the filesystem, not against this name.

**Scrape ratings**

Currently only supported by ScreenScraper.

**Scrape other metadata**

This includes the game description, release date, developer, publisher, genre and the number of players.

**Scrape screenshot images**

Screenshot images of actual gameplay.

**Scrape box cover images**

Cover art.

**Scrape marquee (wheel) images**

Logotype for the game, is used primarily for the Video view style.

**Scrape 3D box images**

Currently unused, but will be used for future versions of ES so it's recommended to keep this option ticked.

#### Other settings

Various scraping settings.

**Region**

The region to scrape for, affects game names.

**Language**

Currently only English or World are supported, not really significant at the moment.

**Overwrite files and data**

Affects both overwriting of metadata as well as actual game media files on the filesystem.

**Interactive mode**

If turned off, the scraping will be fully automatic and will not stop on multiple results or on missing games.

**Auto-accept single game matches**

Used in conjunction with interactive mode, to not having to confirm every single game if a single matched is returned from the scraper service.

### UI settings

Various settings that affects the user interface.

**Gamelist to show on startup**

If set to None, the system view will be showed. Any other value will jump to that game system automatically on startup.

**Gamelist view style**

See the description earlier in this document regarding gamelist view styles.

**Transition style**

Graphical transition effect, either Instant (the default value), Fade or Slide.

**Theme set**

The theme to use. Defaults to rbsimple-DE, the theme shipped with EmulationStation Desktop Edition.

**UI mode**

Defaults to Full which enables all functionality within the application. If set to Kid, only games marked as being suitable for children will be displayed, and there will be an option to disable the menu. In Kiosk mode, most settings are disabled.

**Default sort order**

The order in which to sort your gamelists. This can be overriden per game system using the game options menu, but that override will only be persistent during the application session.

**Sort folders on top of gamelists**

Whether to place all folders on top of the gamelists. If done so, the folders will not be part of the quick selector index, meaning they can no longer be quick-jumped to. Also, if this option is enabled, folders marked as favorites will not be sorted above non-favorite folders.

**Sort favorite games above non-favorites**

Whether to sort your favorite games above your other games in the gamelists.

**Gamelist filters**

Activating or deactivating the ability to filter your gamelists. Can normally be left on.

**Quick system select**

If activated, it will be possible to jump between gamelists using the Left and Right buttons without having to first go back to the system view.

**Carousel transitions**

Whether to perform an animation when transitioning between systems in the system view.

**On-screen help**

Activating or deactivating the built-in help systems that provides contextual information regarding button usage.

**Show start menu in kid mode**

Hiding or showing the menu when the UI mode is set to Kid.

#### Screensaver settings

Settings for the built-in screensaver.

**Screensaver after**

After how many minutes to start the screensaver. If set to 0 minutes, the automatic screensaver will be deactivated. It can however still be started manually, if the Screensaver controls settings is activated.

**Screensaver controls**

This includes the ability to start the screensaver manually, but also to browse Left and Right between images or videos, and to launch the game shown by the screensaver using the A button.

**Screensaver behavior**

The screensaven style to use, which includes _Dim_, _Black_, _Slideshow_ and _Video_.

#### Video screensaver settings

Options specific to the video screensaver.

**Swap videos after (secs)**

How long to play videos before change to the next game.

**Stretch videos to screen resolution**

This will fill the entire screen surface but will possibly break the aspect ratio of the video.

**Play audio for screensaver video files**

Muting or playing the audio.

#### Slideshow screensaver settings

Options specific to the slideshow screensaver.

**Swap images after (secs)**

How long to show images before change to the next game.

**Stretch images to screen resolution**

This will fill the entire screen surface but will possibly break the aspect ratio of the image.

**Background audio**

Background audio to play when the screensaver is active.

**Use custom images**

Using this option, it's possible to use custom images instead of random images from the game library.

**Custom image dir**

The directory for the custom images.

**Custom image dir recursive**

Whether to search the custom image directory recursively.

**Custom image filter**

The file extensions to consider for the custom images.


### Sound settings

General sound settings.

**System volume**

As the name implies, this sets the overall system volume and not the volume specifically for ES.

**Play audio for video files in gamelist views**

With this turned off, audio won't play for you game videos in the gamelists.

**Navigation sounds**

Enable or disable navigation sounds throughout the application. Sounds are played when browsing systems and lists, starting games, adding and removing games as favorites etc. The sounds can be customized per theme, but if the theme does not support navigation sounds, ES will fall back to built-in sounds.


### Game collection settings

Handles collections, which are built using the games already present for your game systems. _(Details on how this works are discussed later in this guide.)_

**Automatic game collections**

This opens a screen that lets you enable or disable the automatic game collections _All games_, _Favorites_ and _Last played_.

**Custom game collections**

This lets you create your own custom game collections.

**Create new custom collection from theme**

If the theme set in use provides themes for custom collections, then this can be selected here. For example, there could be themes for _"Fighting games"_ or _"Driving games"_ etc. As of version 1.0.0, the default rbsimple-DE theme set does not provides such themes for custom collections.

**Create new custom collection**

This lets you create a completely custom collection with a name that you choose.

**Sort favorites on top for custom collections**

Whether to sort your favorite games above your other games.

**Group unthemed custom collections**

With this enabled, if you have created custom collections and there is no theme support for the names you've selected, the collections will be grouped in a general collection which is correctly themed. It's strongly recommended to keep this option enabled as otherwise your collections would be completely unthemed which doesn't make much sense.

**Show system names in collections**

Enables the system name to be shown in square brackets after the game name, for example "CONTRA [NES]" or "DOOM [DOS]". It's recommended to keep this option enabled.

### Other settings

These are mostly technical settings.

**VRAM limit**

The amount of video RAM to use for the application. Defaults to 100 MB which seems to work fine most of the time. Don't put it too low or you may see strange application behavior.

**Fullscreen mode (requires restart) - Unix only**

This gives you a choice between Normal and Borderless modes. With the borderless being more seamless as the ES window will always stay on top of other windows so the taskbar will not be visible when launching and exiting from games. It will however break the alt-tab application switching of your window manager.

**Power saver modes**

Can be set to Disabled, Default, Enhanced or Instant. Set to Disabled by default.

**When to save metadata**

The metadata for a game is updated both by scraping and modifying data in the metadata editor, but also when launching a game, as the play count and last played date is then updated. This settings enabled you to define when to write such metadata changes to the gamelist.xml files. Setting the option to "Never" will disable writing to these files altogether, except for some special conditions such as when a game is manually deleted using the metadata editor. In theory "On exit" will give some performance gains, but it's normally recommended to leave the setting at its default value which is "Always".

**Game media directory**

Here you can override the directory to your game media, i.e. the game images and videos. The default location is "~/.emulationstation/downloaded_media".

**Per game launch command override**

If enabled, you can override the launch command defined in es_systems.cfg on a per-game basis. It's only recommended to disable this option for testing purposes, such as when a game won't start and you're unsure if it's your custom launch command that causes the problem.

**Show hidden files and folders (requires restart)**

Allows hiding of hidden files, which on Unix means files and directories starting with a dot, and on Windows it's directories set as hidden as an NTFS option.

**Show hidden games (requires restart)**

You can mark games as hidden in the metadata editor, which is useful for instance for DOS games where you may not want to see some batch files and executables inside ES.

**Custom event scripts**

It's possible to trigger custom scripts for a number of actions in ES. _(Details on how this works are discussed later in this guide.)_

**Only show roms from gamelist.xml files**

If enabled, only ROMs that have metadata saved to the gamelist.xml files will be shown in ES. This option is intended primarily for testing and debugging purposes so it should normally not be enabled.

**Display game art from rom directories**

Using this option, you can locate game images in the ROM directory tree. The images are searched inside the directory "\<ROM directory\>/\<system name\>/images/" and the filenames must be the same as the ROM names, followed by a dash and the image type. For example "~/ROMs/nes/images/Contra-screenshot.jpg" and "~/ROMs/nes/images/Contra-marquee.jpg". This option is mostly intended for legacy purposes, if you have an existing game collection with this media setup that you would like to open in ES. The scraper will never save files to this directory structure and will instead use the standard media directory logic.

**Show framerate**

Shows the framerate and some other statistics as an overlay. You normally never need to use this.

**Show "reboot system" menu entry**

Gives the ability to hide the "Reboot system" entry on the quit menu. Anyone who has accidentally rebooted a system from such a menu will appreciate this.

**Show "power off system" menu entry**

Gives the ability to hide the "Power off system" entry on the quit menu. Anyone who has accidentally powered off a system from such a menu will appreciate this.


### Configure input

Gives you the possibility to reconfigure you devices or configure additional devices. This procedure is explained earlier in this guide.


### Quit

The menu where you quit ES, or reboot or power off your system.

**Quit emulationstation**

If the option _"When to save metadata"_ has been set to _"On exit"_, the gamelist.xml files will be updated at this point.

**Reboot system**

Can be disabled, meaning the entry will not show up at all.

**Power off system**

Can be disabled, meaning the entry will not show up at all.


## Game options menu

This menu is opened from the gamelists, and can't be accessed directly from the main screen. The menu changes slightly depending on the context, namely whether a game file or a folder is selected, and whether the current system is a collection or a normal game platform.

You open this menu by pressing the **Select** key.

Here's a summary of the menu entries:

### Jump to..

This provides the ability to jump to a certain letter using a quick selector. If the setting to sort favorite games above non-favorites has been selected (it is enabled by default), then it's also possible to jump to the favorites games by choosing the star symbol. If there are only folders or only favorite games in a certain game list, these games and folders will be indexed as well, making it possible to jump betwen them using the quick selector.

### Sort games by

This is the sort order for the gamelist. There's is a global sort order setting that can be changed in the main menu, and it's this sort order that is also shown here unless it's been modified during the program sessions. The sort order is persistent per game system throughout the program session.

### Filter gamelist

Choosing this entry opens a separate screen where it's possible to apply a filter for the gamelists, which is persistent throughout the program session, or until the filter is reset. The option to reset the filter is also shown on the separate screen.

### Edit this game's metadata / Edit this folder's metadata

This opens the metadata editor, which will be described in detail below.

### Add/remove games to this game collection

This is only shown if the system is a collection. This will also be described in more detail below.


## Metadata editor

In the metadata editor, you can modify the metadata for a game, scrape for game info and media files and delete media files or the entire game.

The following entries can be modified:

**Name**

This is the game that will be shown when browsing the gamelist. If no sortname has been defined, the games are sorted using this field.

**Sortname**

This entry makes it possible to change the sorting of a game without having to change its name. For instance it can be used to sort _"Mille Miglia"_ as _"1000 Miglia"_ or _"The Punisher"_ as _"Punisher, The"_.

**Description**

Usually provided by the scraper although it's possible to update this manually or write your own game description.

**Rating**

Ratings in half-star increments. Can be set as such manually or be scraped, if the scraper service provides ratings (currently only ScreenScraper does).

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

A flag to indicate whether this is a favorite. Can also be set directly from the gamelist by using the **Y** key.

**Completed**

A flag to indicate whether you have completed this game.

**Broken/not working**

A flag to indicate whether the game is broken. Useful for MAME games for instance where future releases may make the game functional.

**Hidden**

A flag to indicate the game is hidden. If the corresponding option has been set on the main menu, the game will not be shown. Useful for examle for DOS games to hide batch scripts, configuration tools etc.

**Kidgame**

A flag to mark whether the game is suitable for children. This will be applied as a filter when starting ES in 'Kid mode'.

**Count as game**

A flag to indicate whether the game should be counted. It's only used for the game system counter on the main screen, but is quite useful for multi-file games such multi-disk Amiga or Commodore 64 games, or for DOS games configuration executables that you want to keep in ES and therefore can't hide.

**Launch command**

Here you can override the launch command for the game, for example to use a different emulator than the default for the game system. Very useful for MAME/arcade games.

**Play count**

A statistics counter that counts how many times you're played the game. You normally don't need to touch this, but if you want to, the possibility is there.


## Scraper

The scraper supports downloading of game metadata and media files from the Internet. Currently two scraper services are supported, ScreenScraper.fr and TheGamesDB.net.


## Game collections

There are two types of game collections supported by ES; the automatic collections **All games**, **Favorites** and **Last played** as well as custom collections that you create yourself. Example of such could be grouping of genres like _"Shoot 'em up"_, _"Fighting"_ etc. or perhaps a time period like _"1980s"_, _"1990s"_ and so on.

For either types of collections, they are always based on the games you currently have available in your normal game systems.


## Custom event scripts

There are numerous locations throughout ES where custom scripts can be executes if the option to do so has been enabled in the settings.


## Command line arguments

You can use **--help** or **-h** to view a list of command line options, as shown here.

### Unix

```
--resolution [width] [height]   Try to force a particular resolution
--gamelist-only                 Skip automatic game ROM search, only read from gamelist.xml
--ignore-gamelist               Ignore the gamelist files (useful for troubleshooting)
--show-hidden-files             Show hidden files and folders
--show-hidden-games             Show hidden games
--draw-framerate                Display the framerate
--no-exit                       Don't show the exit option in the menu
--no-splash                     Don't show the splash screen
--debug                         Print debug information
--windowed                      Windowed mode, should be combined with --resolution
--fullscreen-normal             Normal fullscreen mode
--fullscreen-borderless         Borderless fullscreen mode (always on top)
--vsync [1/on or 0/off]         Turn vsync on or off (default is on)
--max-vram [size]               Max VRAM to use in Mb before swapping
                                Set to at least 20 to avoid unpredictable behavior
--force-full                    Force the UI mode to Full
--force-kid                     Force the UI mode to Kid
--force-kiosk                   Force the UI mode to Kiosk
--force-disable-filters         Force the UI to ignore applied filters in gamelist
--force-input-config            Force configuration of input device
--home [path]                   Directory to use as home path
--version, -v                   Displays version information
--help, -h                      Summon a sentient, angry tuba
```

### Windows

```
--resolution [width] [height]   Try to force a particular resolution
--gamelist-only                 Skip automatic game ROM search, only read from gamelist.xml
--ignore-gamelist               Ignore the gamelist files (useful for troubleshooting)
--show-hidden-files             Show hidden files and folders
--show-hidden-games             Show hidden games
--draw-framerate                Display the framerate
--no-exit                       Don't show the exit option in the menu
--no-splash                     Don't show the splash screen
--debug                         Print debug information
--vsync [1/on or 0/off]         Turn vsync on or off (default is on)
--max-vram [size]               Max VRAM to use in Mb before swapping
                                Set to at least 20 to avoid unpredictable behavior
--force-full                    Force the UI mode to Full
--force-kid                     Force the UI mode to Kid
--force-kiosk                   Force the UI mode to Kiosk
--force-disable-filters         Force the UI to ignore applied filters in gamelist
--force-input-config            Force configuration of input device
--home [path]                   Directory to use as home path
--version, -v                   Displays version information
--help, -h                      Summon a sentient, angry tuba
```

## Supported game systems

For details regarding the systems such as which emulator or core is setup as default or which file extensions are supported, refer to the **es_systems.cfg** templates [es_systems.cfg_unix](resources/templates/es_systems.cfg_unix) and [es_systems.cfg_windows](resources/templates/es_systems.cfg_windows).

| Platform name         | Full name                                     | Recommended game setup               |
| :-------------------- | :-------------------------------------------- | :----------------------------------- |
| 3do                   | 3DO                                           |                                      |
| ags                   | Adventure Game Studio                         |                                      |
| amiga                 | Amiga                                         | **.hdf or .hdz** WHDLoad harddisk images or **.adf** and **.m3u** diskette images |
| amiga600              | Amiga 600                                     | **.hdf or .hdz** WHDLoad harddisk images or **.adf** and **.m3u** diskette images |
| amiga1200             | Amiga 1200                                    | **.hdf or .hdz** WHDLoad harddisk images or **.adf** and **.m3u** diskette images |
| amigacd32             | Amiga CD32                                    |                                      |
| amstradcpc            | Amstrad CPC                                   |                                      |
| apple2                | Apple II                                      |                                      |
| apple2gs              | Apple IIGS                                    |                                      |
| arcade                | Arcade                                        | Single archive files following MAME name standard in root folder |
| astrocade             | Bally Astrocade                               |                                      |
| atari2600             | Atari 2600                                    |                                      |
| atari5200             | Atari 5200                                    |                                      |
| atari7800             | Atari 7800 ProSystem                          |                                      |
| atari800              | Atari 800                                     |                                      |
| atarijaguar           | Atari Jaguar                                  |                                      |
| atarijaguarcd         | Atari Jaguar CD                               |                                      |
| atarilynx             | Atari Lynx                                    |                                      |
| atarist               | Atari ST                                      |                                      |
| atarixe               | Atari XE                                      |                                      |
| atomiswave            |                                               |                                      |
| bbcmicro              | BBC Micro                                     |                                      |
| c64                   | Commodore 64                                  | Single disk, tape or cartridge images in root folder and/or multi-disk images in separate folders |
| cavestory             | Cave Story (NXEngine)                         |                                      |
| cdtv                  | Commodore CDTV                                |                                      |
| channelf              | Fairchild Channel F                           |                                      |
| coco                  | Tandy Color Computer                          |                                      |
| coleco                | ColecoVision                                  |                                      |
| daphne                | Daphne Arcade Laserdisc Emulator              |                                      |
| doom                  | Doom                                          |                                      |
| dos                   | DOS (PC)                                      | Games in separate folders (one folder per game, with complete file structure) |
| dragon32              | Dragon 32                                     |                                      |
| dreamcast             | Sega Dreamcast                                |                                      |
| famicom               | Nintendo Family Computer                      | Single archive files in root folder  |
| fba                   | Final Burn Alpha                              | Single archive files following MAME name standard |
| fbneo                 | FinalBurn Neo                                 | Single archive files following MAME name standard |
| fds                   | Famicom Disk System                           |                                      |
| gameandwatch          | Nintendo Game and Watch                       |                                      |
| gamegear              | Sega Gamegear                                 |                                      |
| gamecube              | Nintendo GameCube                             |                                      |
| gb                    | Game Boy                                      |                                      |
| gba                   | Game Boy Advance                              |                                      |
| gbc                   | Game Boy Color                                |                                      |
| genesis               | Sega Genesis                                  | Single archive files in root folder  |
| gx4000                | Amstrad GX4000                                |                                      |
| intellivision         | Mattel Electronics Intellivision              |                                      |
| chailove              | ChaiLove game engine                          |                                      |
| lutro                 | Lutro game engine                             |                                      |
| macintosh             | Apple Macintosh                               |                                      |
| mame                  | Multiple Arcade Machine Emulator              | Single archive files following MAME name standard in root folder |
| mame-advmame          | AdvanceMAME                                   | Single archive files following MAME name standard in root folder |
| mame-libretro         | Multiple Arcade Machine Emulator              | Single archive files following MAME name standard in root folder |
| mame-mame4all         | MAME4ALL                                      | Single archive files following MAME name standard in root folder |
| mastersystem          | Sega Master System                            |                                      |
| megadrive             | Sega Mega Drive                               | Single archive files in root folder  |
| mess                  | Multi Emulator Super System                   |                                      |
| moonlight             | Moonlight game streaming                      |                                      |
| msx                   | MSX                                           |                                      |
| msx1                  | MSX1                                          |                                      |
| msx2                  | MSX2                                          |                                      |
| naomi                 | Sega NAOMI                                    |                                      |
| n64                   | Nintendo 64                                   | Single archive files in root folder  |
| nds                   | Nintendo DS                                   |                                      |
| neogeo                | Neo Geo                                       | Single archive files following MAME name standard |
| neogeocd              | Neo Geo CD                                    |                                      |
| nes                   | Nintendo Entertainment System                 | Single archive files in root folder  |
| ngp                   | Neo Geo Pocket                                |                                      |
| ngpc                  | Neo Geo Pocket Color                          |                                      |
| odyssey2              | Magnavox Odyssey2                             |                                      |
| openbor               | OpenBOR game engine                           |                                      |
| oric                  | Tangerine Computer Systems Oric               |                                      |
| palm                  | Palm OS                                       |                                      |
| pc                    | IBM PC                                        | Games in separate folders (one folder per game, with complete file structure) |
| pcengine              | NEC PC Engine                                 | Single archive files in root folder  |
| pcenginecd            | NEC PC Engine CD                              |                                      |
| pcfx                  | NEC PC-FX                                     |                                      |
| pokemini              | Nintendo Pokémon Mini                         |                                      |
| ports                 | Ports                                         | Shell/batch scripts in separate folders (possibly combined with game data) |
| ps2                   | Sony PlayStation 2                            |                                      |
| psp                   | PlayStation Portable                          |                                      |
| psvita                | PlayStation Vita                              |                                      |
| psx                   | Sony PlayStation 1                            |                                      |
| residualvm            | ResidualVM game engine                        |                                      |
| samcoupe              | SAM Coupé                                     |                                      |
| satellaview           | Nintendo Satellaview                          |                                      |
| saturn                | Sega Saturn                                   |                                      |
| scummvm               | ScummVM game engine                           |                                      |
| sega32x               | Sega 32X                                      | Single archive files in root folder  |
| segacd                | Sega Mega-CD                                  |                                      |
| sg-1000               | Sega SG-1000                                  |                                      |
| snes                  | Super Nintendo                                | Single archive files in root folder  |
| solarus               | Solarus game engine                           |                                      |
| spectravideo          | Spectravideo                                  |                                      |
| steam                 | Valve Steam                                   | Shell/batch scripts in root folder   |
| stratagus             | Stratagus game engine                         |                                      |
| sufami                | Bandai SuFami Turbo                           |                                      |
| supergrafx            | NEC SuperGrafx                                |                                      |
| thomson               | Thomson TO/MO series                          |                                      |
| tg16                  | NEC TurboGrafx-16                             |                                      |
| tg-cd                 | NEC TurboGrafx-CD                             |                                      |
| ti99                  | Texas Instruments TI-99                       |                                      |
| trs-80                | Tandy TRS-80                                  |                                      |
| uzebox                | Uzebox                                        |                                      |
| vectrex               | Vectrex                                       |                                      |
| videopac              | Philips Videopac G7000 (Magnavox Odyssey2)    |                                      |
| virtualboy            | Nintendo Virtual Boy                          |                                      |
| wii                   | Nintendo Wii                                  |                                      |
| wiiu                  | Nintendo Wii U                                |                                      |
| wonderswan            | Bandai WonderSwan                             |                                      |
| wonderswancolor       | Bandai WonderSwan Color                       |                                      |
| x68000                | Sharp X68000                                  |                                      |
| xbox                  | Microsoft Xbox                                |                                      |
| xbox360               | Microsoft Xbox 360                            |                                      |
| zmachine              | Infocom Z-machine                             |                                      |
| zx81                  | Sinclair ZX81                                 |                                      |
| zxspectrum            | Sinclair ZX Spectrum                          |                                      |

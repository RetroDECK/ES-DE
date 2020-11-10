# EmulationStation Desktop Edition v1.0.0 - User Guide

**Note:** This document is intended as a quick start guide, for more in-depth information and details on how to compile EmulationStation and perform more advanced configuration, please refer to the [INSTALL.md](INSTALL.md) document.

Table of contents:

[[_TOC_]]

## Getting started

Getting started with EmulationStation is very easy, just make sure to install the software properly, either manually as built from source code or using one of the supplied packages. On Windows you'll use the installer instead of a package.

The installation procedure will not be covered here as it differs between operating system, so please refer to your operating system documentation for information regarding this topic. EmulationStation Desktop Edition is currently supplied as .deb and .rpm packages for Linux and as a standard NSIS installer for Windows.

The following operating systems have been tested:

* Kubuntu 20.04
* macOS 10.11.6 (El Capitan)
* Windows 10 (x86)
* Windows 8.1 (x86)

Upon first startup, ES will create its home directory, by default the location is ~/.emulationstation.

On Unix this defaults to /home/\<username\>/.emulationstation/, on macOS /Users/\<username\>/.emulationstation/ and on Windows C:\Users\\<username>\\.emulationstation\

A settings file, **es_settings.cfg** will be generated with all the default settings, and a **es_systems.cfg** file will also be copied from the program resource folder. This file contains the game ROM and emulator settings and can be modified if needed. For information on how to do this, refer to the [INSTALL.md](INSTALL.md) document.

**Note:** On Unix it's assumed that RetroArch is using the default configuration directory location, i.e. the cores should be located in ~/.config/retroarch/cores. If you've installed RetroArch via a Snap package, make a symlink from the Snap .config directory:

```
ln -s ~/snap/retroarch/current/.config/retroarch ~/.config/
```

There's a log file in the home directory as well named **es_log.txt**, please refer to this in case of any errors as it should provide information on what went wrong.

After ES finds at least one game file, it will populate that game system and the application will start. If there are no game files, an error messsage will be shown, explaining that you need to install your game files into your ROM directory. You will also be given a choice to change the ROM directory if you don't want to use the default path. Please refer to the game installation procedure below in this document for more information regarding this.


## Input device configuration

When first starting ES, the application will look for any attached controllers (joysticks and gamepads). If no devices are found, it will be assumed that only keyboard navigation is to be used and the default keyboard mappings will be applied. It's recommended to change these default values, and a message will be displayed describing just this. It's however possible to hide this notification permanently and continue to use the default keyboard mappings indefinitely if you're happy with them.

If a controller is attached when starting ES and no **es_input.cfg** input configuration file exists, you will be presented with the input configuration dialog. Just follow the steps as described to map the inputs.

If an es_input.cfg configuration file exists, you will not be presented with the input device configuration screen as that would normally just be annoying. If you however need to configure a device to control the application (i.e. you've replaced your controller), you can do so by starting ES with the command line argument **--force-input-config** or you can manually delete the es_input.cfg file prior to starting the application. Alternatively you can navigate to the menu using your keyboard and select **Configure input** manually to configure your new device.

The actual procedure to map the inputs should be self-explanatory, just follow the on-screen instructions.

Both new and old devices can be (re)configured at any time by pressing the Start button and choosing "CONFIGURE INPUT". New devices will be appended to the existing input configuration file, so your old devices will retain their configuration.


## System view (main screen)

When starting EmulationStation with the default settings, you will see the main screen first. From here you can navigate your game systems and enter their respective gamelists. If there are no game systems installed, you will not see this screen but rather an error message will be displayed, informing you that no games could be found.

Depending on the theme, the system navigation carousel can be either horizontal or vertical. The default theme rbsimple-DE provides horizontal navigation, i.e. you browse your systems be scrolling left or right.

The game systems are sorted by their full names, as defined in es_systems.cfg.


## Gamelist view

The gamelist view is where you browse and start your games, and it's where you will spend most of your time using ES.

Upon startup with the default settings, ES is set to the gamelist view style to **Automatic**. In this mode the application will look for any game media files (videos and images) and set the view style accordingly. If at least one image is found for any game, the view style **Detailed** will be shown, and if at least one video file is found, the view style **Video** will be selected (superceding the Detailed style). If no game media files are found for a system, the simple **Basic** view will be selected. Note that this automatic selection is applied per game system.

Also note that the Video view style requires that the theme supports it. If not, the Detailed style will be selected instead. (The default theme rbsimple-DE supports both of these view styles).

It's possible to manually set a specific gamelist view style in the UI settings entry of the main menu, but this is applied globally regardless of what media files are available per game system. The manual setting also overrides the theme-supported view styles which has the potential of making ES very ugly indeed.

In additions to the styles just described, there is a **Grid** view style as well, but as of version 1.0.0 this is very limited and not recommended. Future versions of EmulationStation may update this style to a more useful state.

If the theme supports it, there's a gamelist information field displayed in the gamelist view, showing the number of games for the system (total and favorites) as well as a folder icon if a folder has been entered. When applying any filters to the gamelist, the game counter is replaced with the amount of games filtered, as in 'filtered / total games', e.g. '19 / 77'. If there are game entries in the filter result that are marked not to be counted as games, the number of such files will be indicated like 'filtered + filtered non-games / total games', for example '23 + 4 / 77' indicating 23 normal games, 4 non-games out of a total of 77. Due to this approach it's theoretically possible that the combined filtered game amount exceeds the number of counted games in the collection, for instance '69 + 11 / 77'. This is not considered a bug and is so by design. This gamelist information field functionality is specific to EmulationStation Desktop Edition so older themes will not support this.

## Help system

There is a help system available throughout the application that provides an overview of the possible actions and buttons that can be used. It's possible to disable the help system for a somewhat cleaner look using a menu option. Note that some general button actions are never shown, such as the ability to quick jump in gamelists, menus and text input fields using the shoulder and trigger buttons.


## General navigation

The help system will provide an overview per screen on the navigation options for the application, however here is a general overview. These are the inputs you mapped in the previous input device configuration step. Note that this is not an exhaustive list, but it covers most situations.

The default keyboard mappings are shown in brackets. These can be changed by running the input device configuration as described earlier in this document.

**Up and down**\
_(Arrow up/Arrow down)_

Navigate up and down in gamelists, system view (if the theme has a vertical carousel) and in menus.

**Left and right**\
_(Arrow left/Arrow right)_

Navigate between gamelists (if 'Quick system select' has been activated in the options), or between systems (if the theme has a horizontal carousel).

**Start button**\
_(Escape)_

Opens and closes the main menu.

**Select button**\
_(F1)_

Opens and closes the game options menu if in the gamelist view, or toggles the screensaver if in the system view (main screen).

**Shoulder buttons left and right**\
_(Page up/Page down)_

Provides quick jumping in gamelists and menus, jumps 10 games in the gamelists and 6 entries in the menus. Also jumps forward in text edit dialogs.

**Trigger buttons left and right**\
_(Home/End)_

Jumps to the first and last entry of the gamelists, menus and text edit dialogs.

**A button**\
_(Enter)_

Select button to open gamelists from the systems view, start games, choose menu entries etc.

**B button**\
_(Back key)_

Back button, self explanatory.

**X button**\
_(Delete)_

Selects random games and systems.

**Y button**\
_(Insert on Unix and Windows, F13 on macOS)_

Marks games as favorites in the gamelist views. Used by some other minor functions as explained by the help system.


## Getting your games into EmulationStation

For most systems, this is very straightforward, just put your game files into the folder corresponding to the platform name. (These names can be found at the end of this document.)

For some systems though, a more elaborate setup is required, and we will attempt to cover such situations in this guide as well.

### Single gamefile installation

Let's start with the simple scenario of a single ROM game file per platform, which is the case for the majority of systems. In this example we're setting up ES to play Nintendo Entertainment System games.

The supported file extensions are listed in [es_systems.cfg_unix](resources/templates/es_systems.cfg_unix) and [es_systems.cfg_windows](resources/templates/es_systems.cfg_windows).

Here is the snippet from the es_systems.cfg_unix file:

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

It's required that the ROM files are in one of the supported file extensions, or ES won't find them.

It's highly recommended to use filenames that are corresponding to the full name of the game, or otherwise you will need to manually feed the scraper the game name when scraping which is very tedious.

The default game directory folder is ~/ROMs. On Unix this defaults to /home/\<username\>/ROMs/, on macOS /Users/\<username\>/ROMs/ and on Windows C:\Users\\<username\>\ROMs\.

If ES can't find any game files during startup, an error message will be displayed with the option to change the ROM directory path.

Assuming the default ROM directory is used, we need to create a directory corresponding to the \<path\> tag in es_systems.cfg, in this example it's **nes**.

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

**Note: These directories are case sensitive on Unix, so creating a directory named _NES_ instead of _nes_ won't work!**

That's it, start ES and the NES game system should be populated. You can now scrape game information and media for the games, and assuming you've setup RetroArch correctly with the FCEUmm core, you can launch the games.

### Multiple gamefiles installation

For some systems, there are sometimes (or always) multiple gamefiles per game. Such an example would be the Commodore 64, when multidisk games are being played. For such instances, simply group the files inside folders.

The platform name for the Commodore 64 is **c64**, so the following structure would be a possible approach:

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

It's highly recommended to create **.m3u** playlist files for multi-disk images as this simplifies the disk swapping in the emulator. It's then this .m3u file that should be selected for launching the game.

The .m3u file simply contains a list of the game files, for example in the case of Last Ninja 2.m3u:

```
LNINJA2A.D64
LNINJA2B.D64
```

It's recommended to have the exact same filename for the .m3u file as for the directory as the game media files will then be shared between the two. This saves some unnecessary scraping as well as some disk space.

It's of course also possible to skip this type of directory structure and put all the games in the root folder, but then there will be multiple entries for the same game which is not so tidy. Another approach would be to put all the files in the root folder and then hide the game files, showing only the .m3u playlist. But it's probably quite confusing to start a game that looks like a single-disk game and then be prompted for disk swaps by the emulator (even if the .m3u playlists automates disk swapping, it's still somehow confusing and I wouldn't recommend it).

When setting up games in this fashion, it's recommended to scrape the directory in addition to the .m3u file as it looks nicer to see the metadata for the games also when browsing the folders. ES fully supports scraping folders, although some metadata is not included for folders for logical reasons. If you only scrape the folders and not the actual game files, it looks ok when browsing, but when a game is part of a collection, the metadata will be missing there. This includes the **Last played** and **All games** collections for instance. Also note that while it's possible to mark a folder as a favorite, it will never be part of a collection, such as **Favorites**.

As well it's recommended to set the flags **Exclude from game counter** and **Exclude from automatic scraper** for the actual game files so that they are not counted (the game counter is shown on the system view) and not scraped if running the multi-scraper. It's enough to scrape the .m3u playlist file and the game folder. But if you only intend to manually scrape file-per-file then you don't need to bother with this. For a cleaner look, it's also possible to set the flag **Hide metadata fields** for the game files.

### Special game installation considerations

Not all systems are as simple as described above, or sometimes there are multiple ways to configure the systems. So specifics to such systems will be covered here. Consider this a work in progress though as there are many systems supported by ES.

#### Arcade and Neo Geo

For all the supported MAME variants as well as Final Burn Alpha/FinalBurn Neo and Neo Geo, single file archives should be used. However these should retain the MAME names as filenames since ES ships with MAME lookup tables, meaning the MAME names are expanded to the full game names.

For instance **avsp.7z** will be expanded to **Alien vs. Predator**.

This is used by the TheGamesDB scraper where the expanded file name is used for game searches. (Screenscraper natively supports searches using the MAME names). It's also quite nice to have the gamelist populated with the expanded game names even before any scraping has taken place.

#### Commodore Amiga

There are multiple ways to run Amiga games, but the recommended approach is to use WHDLoad. The best way is to use hard disk images in **.hdf** or **.hdz** format, meaning there will be a single file per game. This makes it just as easy to play Amiga games as any console with game ROMs.

An alternative would be to use **.adf** images as not all games may be available with WHDLoad support. For this, you can either put single-disk images in the root folder or in a dedicated adf directory, or multiple-disk games in separate folders. It's highly recommended to create **.m3u** playlist files for multi-disk images as this simplifies the disk swapping in the emulator. It's then this .m3u file that should be selected for launching the game.

Here's an example of what the file structure could look like:

```
~/ROMs/amiga/Multidisk/ZakMcKracken/ZakMcKracken (Disk 1 of 2).adf
~/ROMs/amiga/Multidisk/ZakMcKracken/ZakMcKracken (Disk 2 of 2).adf
~/ROMs/amiga/Robbeary.adf
~/ROMs/amiga/Dungeon Master.hdf
```

Advanced topics such as the need for the Amiga Kickstart ROMs to run Amiga games is beyond the scope of this guide, but the following page is recommended for reading more about how this setup can be achieved:

[https://github.com/libretro/libretro-uae/blob/master/README.md](https://github.com/libretro/libretro-uae/blob/master/README.md)

#### DOS / PC

The DOS (and PC) platform uses the DOSBox emulator and the recommended approach here is to keep the directory structure intact, just as if running the game on a real DOS computer. So this means one folder per game in ES. It's also recommended to set the metadata field **Count as game** to off for all files but the actual file used to launch the game (the binary or a .bat batch file). This is done so that the game counter on the system view screen correctly reflects the number of games you have installed. It's also possible to mark files and subdirectories as hidden to avoid seeing them in ES. Both of these fields can be set using the metadata editor. The metadata field **Sortname** can also be set to sort the files in any preferred order without changing their names. For example, simply putting a '1' as sortname will sort the entry above all entries starting with a character.

Apart from this the game should work as normal. The game folders can be scraped so that it looks nice when browsing the list of games, but make sure to also scrape the files used to launch the games, otherwise the entries in the collections **Last played**, **Favorites** and **All games** as well as any custom collections will miss the game metadata and game media. If you don't have these collections activated, then this can of course be skipped.

#### Ports

Ports are not really executed using emulators, but is instead software running natively on your operating system. The easiest way to handle these is to add a simple shell script or batch file where you can customize the exact launch parameters for the game.

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

You don't need to set execution permissions for these scripts, ES will run them anyway.

#### Lutris

Lutris runs only on Unix so it's only present as a placeholder in the es_systems.cfg templates for macOS and Windows.

These games are executed via the Lutris binary (well it's actually a Python script), and you simply create a shell script per game with the following contents:

`lutris lutris:rungame/<game name>`

You can see the list of installed games by running the command `lutris --list-games`.

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

You don't need to set execution permissions for these scripts, ES will run them anyway.

As an alternative, you can add the Lutris games to the Ports game system, if you prefer to not separate them. The instructions above are identical in this case except that the shell scripts should be located inside the **ports** directory rather than inside the **lutris** directory.

#### Steam

For steam, it's recommended to put shell scripts/batch files directly in the root folder, where the file names of these scripts correspond to the game names.

Add the following information to each file:

`steam steam://rungameid/<game ID>`

Here's an example for the game Broforce:

`steam steam://rungameid/274190`

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

In order to use the default es_systems.cfg file, for Unix and Windows you need to make sure that the emulator binary directory is in the operating system's path variable. On Unix systems this is normally not an issue as a package manager has typically been used to install the emulator, and there is a standardized directory structure. But for Windows you may need to add the emulator directory to your %PATH% variable manually. If on Windows, a simple test is to open a command window and type the name of the emulator and press enter, if the emulator is not found, then EmulationStation won't find it either and you need to update your %PATH% variable.

As an alternative, if the emulator is not in your search path, you can either hardcode an absolute path in es_systems.cfg or use the %ESPATH% variable to set the emulator path relative to the EmulationStation location. Again, please refer to the INSTALL.md document on details regarding this.

For macOS the emulator directory is normally not an issue as there is a somehow standardized installation structure, and the es_systems.cfg template for this operating systems has absolute paths defined for the emulators.

## Scraping

Scraping means downloading metadata and game media files (images and videos) for the games in your collections.

EmulationStation Desktop Edition supports the two scrapers ScreenScraper.fr and TheGamesDB.net. In general TheGamesDB supports less formats and less systems, but in some areas such PC gaming, the quality is better and sometimes ScreenScraper is missing some specific information such as release dates where TheGamesDB may be able to fill in the gaps.

Here is an overview of what's supported by ES and these scrapers:

| Media type or option     | ScreenScraper | TheGamesDB |
| :----------------------- | :-----------: | :--------: |
| Region (EU/JP/US/SS/WOR) | Yes           | No         |
| Language (EN/WOR)        | Yes           | No         |
| Game names               | Yes           | Yes        |
| Ratings                  | Yes           | No         |
| Other game metadata      | Yes           | Yes        |
| Videos                   | Yes           | No         |
| Screenshots              | Yes           | Yes        |
| Box covers (2D)          | Yes           | Yes        |
| Marquees/wheels          | Yes           | Yes        |
| 3D boxes                 | Yes           | No         |

The category **Other game metadata** includes Description, Release date, Developer, Publisher, Genre and Players.

There are two approaches to scraping, either for a single game from the metadata editor, or for many games and systems using the multi-scraper.

### Single-game scraper

The single-game scraper is launched from the metadata editor. You navigate to a game, open the game options menu, choose **Edit this game's metadata** and then select the **Scrape** button. The metadata editor is explained in more depth later in this guide so it won't be covered here.

### Multi-scraper

The multi-scraper is launched from the main menu, it's the first option on the menu actually. Here you can configure a number of scraping options, all which are explained in more depth below when covering the main menu entries.

**_Tip: Pressing the 's' key on the keyboard is a quick-skip shortcut rather than having to navigate to the 'Skip' button and selecting it._**

### Scraping process

The process of scraping games is basically identical between the single-game scraper and the multi-scraper. You're presented with the returned scraper results, and you're able to refine the search if the scraper could not find your game. Sometimes just removing some extra characters such as disk information or other data from the search name yields a better result.

In general the actual file name of the game is used for the scraper, the exception being MAME/arcade games when using TheGamesDB, as the MAME names are then expanded to the full game names.

Hopefully the scraping process should be self-explanatory once you try it in ES.

### Manually copying game media files

If you already have a library of game media (images and videos) you can manually copy it into ES.

The default directory is ~/.emulationstation/downloaded_media/\<game system\>/\<media type\>/

For example on Unix:

`/home/myusername/.emulationstation/downloaded_media/c64/screenshots/`

For example on macOS:

`/Users/myusername/.emulationstation/downloaded_media/c64/screenshots/`

For example on Windows:

`C:\Users\Myusername\.emulationstation\downloaded_media\c64\screenshots\`

The media type directories are:

* 3dboxes
* covers
* marquees
* screenshots
* videos

The files must correspond directly to the game file. For example the following game:

`~/ROMs/c64/Multidisk/Last Ninja 2/Last Ninja 2.m3u`

Must have the filename:

`~/.emulationstation/downloaded_media/c64/screenshots/Last Ninja 2.jpg`

JPG and PNG file formats and file extensions are supported.

Remember that on Unix files are case sensitive, and as well the file extensions must be in lower case, i.e. .png intead of .PNG or .Png or the file won't be found.

As an alternative, you can also locate your game media in the ROM directory. This is explained below in this guide under the option **Display game media from ROM directories**. This is however not recommended and the built-in scraper will never save any game media to this folder structure.

Note that it's possible to change the game media directory from within ES, see the option **Game media directory** detailed below.


## Main menu

This menu can be accessed from both the main screen and from the gamelist views. It contains the scraper, the quit menu as well as the application settings. When it comes to saving settings, this is done automatically when navigating back from a screen, or when closing the menu altogether.

Here is a breakdown of the main menu entires:

### Scraper

Contains the various options for the scraper, which is used to download metadata and images for your games.

**Scrape from**

Scraper service selection, currently ScreenScraper.fr and TheGamesDB.net are supported.

**Filter**

Criteria for what games to include in the scraping. It can be set to 'All games', 'Favorite games', 'No metadata', 'No game image', 'No game video' or 'Folders only'.

**Systems**

A selection of which systems to scrape for. It's possible to automatically scrape several or all systems in one go.

#### Account settings

Setup of ScreenScraper account.

**Use this account for ScreenScraper**

Whether to use the account that has been setup here. If this is disabled, the username and password configured on this screen will be ignored during scraping. This can be useful if you have scraping issues and want to check whether it's related to your account or if it's a general problem. Note that screenscraper.fr does not seem to return a proper error message regarding incorrect username and password, but starting ES with the --debug flag will indicate in the log file whether the username was included in the server response.

**ScreenScraper username**

Username as registered on screenscraper.fr.

**ScreenScraper password**

The password as registered on screenscraper.fr. Note that the password is masked using asterisks on this screen, and the password input field will be blank when attempting to update an existing password. Entering a new password will of course work, and it will be saved accordingly. Be aware though that the es_settings.cfg file contains the password in clear text.

#### Content settings

Describes the content types to include in the scraping. Most users will probably not need to adjust so many of these.

**Scrape game names**

Whether to scrape the names of the games. This does not affect the actual files on the filesystem and is only used for viewing and sorting purposes. The downloaded media files are also matched against the actually game file on the filesystem, not against this name.

**Scrape ratings** _(ScreenScraper only)_

Currently only supported by ScreenScraper.

**Scrape other metadata**

This includes the game description, release date, developer, publisher, genre and the number of players.

**Scrape videos** _(ScreenScraper only)_

Videos of actual gameplay.

**Scrape screenshot images**

Screenshot images of actual gameplay.

**Scrape box cover images**

Cover art.

**Scrape marquee (wheel) images**

Logotype for the game, is used primarily for the Video view style.

**Scrape 3D box images** _(ScreenScraper only)_

These images are currently unused, but will be used for future versions of ES so it's recommended to keep this option ticked.

#### Other settings

Various scraping settings. Most users will probably not need to adjust so many of these.

**Region** _(ScreenScraper only)_

The region to scrape for, affects game names.

**Language** _(ScreenScraper only)_

Currently only English or World are supported, not really significant at the moment.

**Overwrite files and data**

Affects both overwriting of metadata as well as actual game media files on the filesystem.

**Search using metadata name**

By default ES will perform scraper searches based on the game name that has been manually set in the metadata editor, or that has been previously scraped. If you prefer to search using the physical name of the game file or directory, then turn off this option. The default game name will correspond to the name of the physical file or directory, so for the first scraping of any given game, this option makes no difference.

Note that when using TheGamesDB as scraper service for arcade games (MAME/Neo Geo), the short MAME name will always be expanded to the full game name as this scraper does not properly support searches using MAME names. Also note that you need to save the game name in the metadata editor before you can use it for scraping.

**Interactive mode**

If turned off, the scraping will be fully automatic and will not stop on multiple results or on missing games.

**Auto-accept single game matches**

Used in conjunction with interactive mode, to not having to confirm every single game if a single matched is returned from the scraper service.

**Respect per-file scraper exclusions**

It's possible to set a flag per file to indicate that the file should be excluded from the multi-scraper. With this flag it's possible to override this setting and scrape all files anyway.

**Exclude folders recursively**

If this settings is enabled and a directory has its flag set to be excluded from the scraping, then the entire folder contents are skipped when running the multi-scraper.

**Scrape actual folders**

Enabling this option causes folders themselves to be included in the scraping. This is useful when games are grouped into folders that should themselves be scraped. For instance for DOS games or any multi-disk games where there is a folder for each game.

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

**Menu opening effect** _(OpenGL renderer only)_

Animation to play when opening the main menu or the game options menu. Can be set to _scale-up_, _fade-in_ or _none_.

**Display carousel transitions**

Whether to perform an animation when transitioning between systems in the system view.

**Render scanlines for gamelist videos** _(OpenGL renderer only)_

Whether to use a shader to render scanlines for videos in the gamelist view. The effect is usually pretty subtle as the video is normally renderered in a limited size in the GUI, and the scanlines are sized relative to the video window size.

**Sort folders on top of gamelists**

Whether to place all folders on top of the gamelists. If done so, the folders will not be part of the quick selector index, meaning they can no longer be quick-jumped to. Also, if this option is enabled, folders marked as favorites will not be sorted above non-favorite folders.

**Sort favorite games above non-favorites**

Whether to sort your favorite games above your other games in the gamelists.

**Add star markings to favorite games**

With this setting enabled, there is a star symbol added at the beginning of the game name in the gamelist views. It's strongly recommended to keep this setting enabled if the option to sort favorite games above non-favorites has been enabled. If not, favorite games would be sorted on top of the gamelist with no visual indication that they are favorites, which would be very confusing.

**Enable shortcut to toggle favorites**

This setting enables the 'Y' button for quickly toggling a game as favorite. Although this may be convenient at times, it's also quite easy to accidentally remove a favorite tagging of a game when using the application more casually. As such it could sometimes make sense to disable this functionality. It's of course still possible to mark a game as favorite using the metadata editor when this setting is disabled. For additional restrictions, the application can be set to Kid or Kiosk mode as is explained elsewhere in this document. Note that this setting does not affect the functionality to use the 'Y' button to add games to custom collections.

**Enable gamelist filters**

Activating or deactivating the ability to filter your gamelists. This can normally be left enabled.

**Enable quick system select**

If activated, it will be possible to jump between gamelists using the Left and Right buttons without having to first go back to the system view.

**Display on-screen help**

Activating or deactivating the built-in help systems that provides contextual information regarding button usage.

**Play videos immediately (override theme)**

Some themes (including rbsimple-DE) display the game images briefly before playing the game videos. This setting forces the videos to be played immediately, regardless of the configuration in the theme. Note though that if there is a video available for a game, but no images, the video will always start to play immediately no matter the theme configuration or whether this settings has been enabled or not.

**Show start menu in kid mode**

Hiding or showing the menu when the UI mode is set to Kid.

#### Screensaver settings

Settings for the built-in screensaver.

**Start screensaver after (minutes)**

After how many minutes to start the screensaver. If set to 0 minutes, the timer will be deactivated and the screensaver will never start automatically. It's however still possible to start the screensaver manually in this case, assuming the _Enable screensaver controls_ setting is enabled. Note that while any menu is open, the screensaver will not start.

**Screensaver type**

The screensaven type to use; _Dim_, _Black_, _Slideshow_ or _Video_.

**Enable screensaver controls**

This includes the ability to start the screensaver manually, but also to browse Left and Right between images or videos, and to launch the game shown by the screensaver using the A button.

#### Slideshow screensaver settings

Options specific to the slideshow screensaver.

**Swap images after (seconds)**

For how long to display images before changing to the next game. Allowed range is between 5 and 120 seconds.

**Stretch images to screen resolution**

This will fill the entire screen surface but will possibly break the aspect ratio of the image.

**Render scanlines** _(OpenGL renderer only)_

Whether to use a shader to render scanlines on top of the images.

**Use custom images**

Using this option, it's possible to use custom images instead of random images from the game library.

**Custom image directory recursive search**

Whether to search the custom image directory recursively.

**Custom image directory**

The directory for the custom images.

#### Video screensaver settings

Options specific to the video screensaver.

**Swap videos after (seconds)**

For how long to play videos before changing to the next game. Allowed range is between 5 and 120 seconds.

**Show game info overlay**

This will display an overlay on top of the videos, showing the game name and game system name.

**Stretch videos to screen resolution**

This will fill the entire screen surface but will possibly break the aspect ratio of the video.

**Play audio for screensaver video files**

Muting or playing the audio.

**Render scanlines** _(OpenGL renderer only)_

Whether to use a shader to render scanlines for the videos.

**Render blur** _(OpenGL renderer only)_

Whether to use a shader to render a slight blur which somewhat simulates a well-used CRT monitor.


### Sound settings

General sound settings.

**System volume**

As the name implies, this sets the overall system volume and not the volume specifically for ES.

**Play audio for video files in gamelist views**

With this turned off, audio won't play for you game videos in the gamelists.

**Enable navigation sounds**

Enable or disable navigation sounds throughout the application. Sounds are played when browsing systems and lists, starting games, adding and removing games as favorites etc. The sounds can be customized per theme, but if the theme does not support navigation sounds, ES will fall back to built-in sounds.


### Game collection settings

Handles collections, which are built using the games already present for your game systems. _(Details on how this works are discussed later in this guide.)_

**Finish editing _'COLLECTION NAME'_ collection**

Self explanatory. This menu entry is only visible when editing a collection.

**Automatic game collections**

This opens a screen that lets you enable or disable the automatic game collections _All games_, _Favorites_ and _Last played_.

**Custom game collections**

This lets you create your own custom game collections.

**Create new custom collection from theme**

If the theme set in use provides themes for custom collections, then this entry can be selected here. For example, there could be themes for _"Fighting games"_ or _"Driving games"_ etc. The default rbsimple-DE theme set does not provide such themes for custom collections and in general it's not recommended to use this approach, as is explained [later](USERGUIDE.md#custom-collections) in this guide. This menu entry is not visible if the theme does not have any available themes to use for custom collections.

**Create new custom collection**

This lets you create a completely custom collection with a name that you choose.

**Delete custom collection**

This permanently deletes a custom collection, including its configuration file on the file system. A list of available collections is shown, and a confirmation dialog is displayed before committing the actual deletion. Only one collection at a time can be deleted.

**Sort favorites on top for custom collections**

Whether to sort your favorite games above your other games. This is disabled by default, as for collections you probably want to be able to mix all games regardless of whether they are favorites or not.

**Display star markings for custom collections**

With this option enabled, there is a star marking added to each favorite game name. It works identically to the setting 'Add star markings to favorite games' but is applied specifically to custom collections. It's disabled by default.

**Group unthemed custom collections**

With this enabled, if you have created custom collections and there is no theme support for the names you've selected, the collections will be grouped in a general collection which is correctly themed. It's strongly recommended to keep this option enabled as otherwise your collections would be completely unthemed which doesn't make much sense.

**Show system names in collections**

Enables the system name to be shown in square brackets after the game name, for example "CONTRA [NES]" or "DOOM [DOS]". It's recommended to keep this option enabled.

### Other settings

These are mostly technical settings.

**VRAM limit**

The amount of video RAM to use for the application. Defaults to 128 MiB which seems to work fine most of the time. The allowed range is 80 to 1024 MiB. If you try to set it lower or higher than this by passing such values as command line parameters or edit the es_settings.cfg file manually, ES will log a warning and automatically adjust the value within the allowable range.

**Fullscreen mode (requires restart) - Unix only**

This gives you a choice between Normal and Borderless modes. With the borderless being more seamless as the ES window will always stay on top of other windows so the taskbar will not be visible when launching and returning from games. It will however break the alt-tab application switching of your window manager. For normal fullscreen mode, if a lower resolution than the screen resolution has been set via the --resolution command line argument, ES will render in full screen at the lower resolution. If a higher resolution than the screen resolution has been set, ES will run in a window. For the borderless mode, any changes to the resolution will make ES run in a window.

**When to save metadata**

The metadata for a game is updated both by scraping and modifying data in the metadata editor, but also when launching a game, as the play count and last played date is then updated. This setting enables you to define when to write such metadata changes to the gamelist.xml files. Setting the option to "Never" will disable writing to these files altogether, except for some special conditions such as when a game is manually deleted using the metadata editor, or when scraping using the multi-scraper (the multi-scraper will always save any updates immediately to the gamelist.xml files). In theory "On exit" will give some performance gains, but it's normally recommended to leave the setting at its default value which is "Always". Note that with the settings set to "Never", any updates such as the last played date will still be shown on screen, however during the next application startup, any values previously saved to the gamelist.xml files will be read in again. As well, when changing this setting to "Always" from either of the two other options, any pending changes will be immediately written to the gamelist.xml files.

**Game media directory**

Here you can override the directory to your game media, i.e. the game images and videos. The default location is "~/.emulationstation/downloaded_media".

**Per game launch command override**

If enabled, you can override the launch command defined in es_systems.cfg on a per-game basis. It's only recommended to disable this option for testing purposes, such as when a game won't start and you're unsure if it's your custom launch command that causes the problem.

**Show hidden files and folders (requires restart)**

Allows hiding of hidden files, which on Unix means files and directories starting with a dot, and on Windows it's directories and files set as hidden using an NTFS option. Not to be confused with the next option which hides files based on metadata configuration within ES.

**Show hidden games (requires restart)**

You can mark games as hidden in the metadata editor, which is useful for instance for DOS games where you may not want to see some batch files and executables inside ES. This is entirely different that the previous option as this is based on metadata configured within ES and the previous options relates to files that are hidden on the operating system level.

**Enable custom event scripts**

It's possible to trigger custom scripts for a number of actions in ES. _(Details on how this works are discussed later in this guide.)_

**Only show roms from gamelist.xml files**

If enabled, only ROMs that have metadata saved to the gamelist.xml files will be shown in ES. This option is intended primarily for testing and debugging purposes so it should normally not be enabled.

**Display game media from ROM directories**

Using this option, you can place game images and videos in the ROM directory tree. The media files are searched inside the directory "\<ROM directory\>/\<system name\>/images/" and "\<ROM directory\>/\<system name\>/videos/" and the filenames must be the same as the ROM names, followed by a dash and the media type. For example "~/ROMs/nes/images/Contra-screenshot.jpg", "~/ROMs/nes/images/Contra-marquee.jpg" and "~/ROMs/nes/videos/Contra-video.jpg". This option is mostly intended for legacy purposes, if you have an existing game collection with this media setup that you would like to open in ES. The scraper will never save files to this directory structure and will instead use the standard media directory logic. It's recommended to keep this option disabled unless you really need it since it slows down the application somewhat.

**Display GPU statistics overlay**

Displays the framerate and VRAM statistics as an overlay. You normally never need to use this. **Note:** As of version 1.0.0 the VRAM usage statistics is not accurate; this issue will be addressed in future ES versions.

**Show 'Reboot System' menu entry - Unix and Windows only**

Gives the ability to hide the "Reboot system" entry on the quit menu. Anyone who has accidentally rebooted a system from such a menu will appreciate this.

**Show 'Power Off System' menu entry - Unix and Windows only**

Gives the ability to hide the "Power off system" entry on the quit menu. Anyone who has accidentally powered off a system from such a menu will appreciate this.


### Configure input

Gives you the possibility to reconfigure you devices or configure additional devices. This procedure is explained earlier in this guide.


### Quit

The menu where you quit ES, or reboot or power off your system.

**Quit emulationstation**

If the option _"When to save metadata"_ has been set to _"On exit"_, the gamelist.xml files will be updated at this point.

**Reboot system - Unix and Windows only**

Can be disabled, meaning the entry will not show up at all.

**Power off system - Unix and Windows only**

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

Choosing this entry opens a separate screen where it's possible to apply a filter for the gamelists, which is persistent throughout the program session, or until the filter is reset. The option to reset all filters is also shown on this separate screen.

The following filters can be applied:

**Text Filter (Game Name)**

**Favorites**

**Genre**

**Players**

**Publisher / Developer**

**Rating**

**Kidgame**

**Completed**

**Broken**

**Hidden**

With the exception of the text filter, all available filter values are assembled from metadata from the actual gamelist, so if there for instance are no games marked as completed, the Completed filter will only have the selectable option 'False', i.e. 'True' will be missing.

Be aware that although folders can have most of the metadata values set, the filters are only applied to files (this is also true for the text/game name filter). So if you for example set a filter to only display your favorite games, any folder that contains a favorite game will be displayed, and other folders which are themselves marked as favorites but that do not contain any favorite games will be hidden.

The filters are always applied for the complete game system, including all folder contents.

### Edit this game's metadata / Edit this folder's metadata

This opens the metadata editor, which will be described in detail below.

### Add/remove games to this game collection

This is only shown if the system is a collection. This will also be described in more detail below.

### Finish editing _'COLLECTION NAME'_ collection

This menu entry is only visible when editing the collection.


## Metadata editor

In the metadata editor, you can modify the metadata for a game, scrape for game info and media files and delete media files and gamelist entries, or the entire game.

### Metadata entries

The following entries can be modified (note that some of these are not available for folders, only for game files):

**Name**

This is the game that will be shown when browsing the gamelist. If no sortname has been defined, the games are sorted using this field.

**Sortname** _(files only)_

This entry makes it possible to change the sorting of a game without having to change its name. For instance it can be used to sort _"Mille Miglia"_ as _"1000 Miglia"_ or _"The Punisher"_ as _"Punisher, The"_. Be aware though that the 'Jump to...' quick selector will base its index on the first character of the sortname if it exists for a game, which could be slightly confusing in some instances when quick jumping in the gamelist.

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

**Kidgame** _(files only)_

A flag to mark whether the game is suitable for children. This will be applied as a filter when starting ES in 'Kid mode'.

**Hidden**

A flag to indicate that the game is hidden. If the corresponding option has been set in the main menu, the game will not be shown. Useful for example for DOS games to hide batch scripts, configuration tools etc. If a file or folder is flagged as hidden but the correponding option to hide hidden games has not been set, then the opacity of the text will be lowered significantly to make it clear that it's a hidden game.

**Broken/not working**

A flag to indicate whether the game is broken. Useful for MAME games for instance where future releases may make the game functional.

**Exclude from game counter** _(files only)_

A flag to indicate whether the game should be excluded from being counted. It's only used for the game system counter on the main screen, but is quite useful for multi-file games such as multi-disk Amiga or Commodore 64 games, or for DOS games for configuration executables that you want to keep in ES and therefore can't hide. Games that have this flag set will have a lower opacity in the gamelists, making them easy to spot.

**Exclude from multi-scraper**

Whether to exclude the file from the multi-scraper. This is quite useful in order to avoid scraping all the disks for multi-disk games for example. There is an option in the scraper settings to ignore this flag, but by default the scraper will respect it. Note that the manual single-file scraper will work regardless of whether this flag is set or not.

**Hide metadata fields**

This option will hide most metadata fields in the gamelist view. The intention is to be able to hide the fields for situations such as general folders (Multi-disk, Cartridges etc.) and configuration files and similar (e.g. SETUP.EXE or INSTALL.BAT for DOS games). It could also be used on the game files for multi-disk games where perhaps only the .m3u playlist should have any metadata values. The only fields shown with this option enabled are the game name and description. Using the description it's possible to write some comments regarding the file or folder, should you want to. It's still possible to display game images and videos with this setting enabled.

**Launch command** _(files only)_

Here you can override the launch command for the game, for example to use a different emulator than the default for the game system. Very useful for MAME/arcade games.

**Play count** _(files only)_

A statistics counter that counts how many times you're played the game. You normally don't need to touch this, but if you want to, the possibility is there.

### Buttons

For game files, there will be five buttons displayed on the bottom of the metadata editor window, and for folders there will be four. These are their functions:

**Scrape**

Opens the single-game scraper, which is explained earlier in this guide.

**Save**

Saves any changes and closes the window. If no changes have been done, it simply closes the window.

**Cancel**

Cancels any changes and closes the window. If no changes have been done, it simply closes the window.

**Clear**

This will remove any media files for the game file or folder and also remove its entry from the gamelist.xml file. The actual game file or folder will however _not_ be deleted. A prompt will be shown asking for confirmation.

**Delete** _(Files only)_

This will remove the actual game file, its gamelist.xml entry, its entry in any custom collections and its media files. A prompt will be shown asking for confirmation. The deletion of folders is not supported as that would potentially be a bit dangerous, instead use the appropriate operating system tools to handle deletion of folders.


## Screensaver

There is a screensaver built into ES with four different behaviours: _Dim_, _Black_, _Slideshow_ and _Video_.

There are numerous options for the screensaver, refer to the Main menu section above to find out about them.

The _Dim_ screensaver simply dims and desaturates the current view and _Black_ will show a black screen. The _Slideshow_ and _Video_ screensavers are a bit more interesting as they can display images and videos from your game collection. (In addition to this, Slideshow can be configured to only show images from a specified directory).

If the option **Screensaver controls** has been activated, you can manually toggle the screensaver from the system view by pressing the 'Select' key. In addition to this the controls will allow you to jump to a new random image video or by using the left and right buttons on your keyboard or controller, and it allows you to actually launch the game just shown by pressing the 'A' button.


## Game collections

ES provides two types of collections, automatic collections as well as custom collections defined by the user. Each collection can be individually enabled or disabled in the main menu.

Collections are, as the name implies, only collections of games already present games from your game systems and any given game can be part of as many collections as you want.

There are multiple settings for the game collections, but these are covered above in the 'Main menu' section so that information won't be repeated here.

### Automatic collections

These are **All games**, **Favorites** and **Last played**. The 'All games' collection simply groups all your game system into one big list, 'Favorites' combines all your games marked as favorites from all your game systems, and 'Last played' is a list of the 50 last games you have launched.

These automatic collections can be individually enabled or disabled by going to the main menu, selecting **Game collection settings** and then **Automatic game collections**.

Note that you should only enable these collections if you really need them as they slow down the application quite significantly. By default these collections are therefore disabled.

### Custom collections

These are collections that you create yourself. Example of such collections could be grouping in genres like _Shoot em up_, _Fighting games_ etc. or perhaps a time period like '1980s', '1990s' and so on.

If the theme set supports it, you can create a custom collection directly from a theme. However, rbsimple-DE does not provide such themes as it's believed that grouping them together in a dedicated **Collections** system is a more elegant solution. Especially since the theme set would need to ship with an almost endless amount of collection themes for whatever categories the users would like to use for their game collections.

So if you have enabled the option **Group unthemed custom collections** (it's enabled by default), any collections you add will show up in the special **Collections** system. Here you can access them just as you would access folders inside a regular game system. The amount of games per collection is shown in the description, and the game media for a random game is displayed each time you browse through the list.

To create a custom collection, go to 'Game collection settings' in the main menu and choose 'Create new custom collection'.

Select a name and press enter, let's use the name '1980s' for this example.

The collection will now be added and the collection edit mode will be automatically selected. You can now add games to your collection by navigating to any game system and adding a game by pressing the 'Y' key, just as you would normally do to flag the game as a favorite. Any number of games from any of your game systems can be added to your collection.

Removing games works in the same way, just press 'Y' to remove it if it's already present in your collection. You can do this either from the game system where the game was added, or from the collection itself.

Note that only files can be part of collections, not folders.

During the time that the collection is being edited, any game that is already part of the collection will be marked with a leading tick mark in their game name to make it easy to see which games have already been added.

When you are done adding games, you can either open the main menu and go to 'Game collection settings' and select the 'Finish editing '1980s' collection' or you can open the game options menu and select the same option there. The latter works from within any game system, you don't need to navigate back to the collection that you're editing.

You can later add additional games to the collection by navigating to it, bringing up the game options menu and choosing 'Add/remove games to this game collection'.

The way that custom collection are implemented is very simple. There is a folder for the collections in `~/.emulationstation/collections` with a separate file for each collection.

For this example a file will have been created named `~/.emulationstation/collections/custom-1980s.cfg`.

The file contents is simply a list of ROM files, such as the following:

```
%ROMPATH%/c64/multidisk/Last Ninja 2/Last Ninja 2.m3u
%ROMPATH%/nes/Legend of Zelda, The.zip
```

Any changes to custom collections (for example adding or removing a game) will be immediately written to the corresponding collection configuration file.

Note that if you for example copy or migrate a collection from a previous version of EmulationStation or if you're setting up EmulationStation Desktop Edition on a new computer, even though you copy the files into the collections directory, they will not show up in the application. You always need to enable the collection in the menu. ES looks inside the es_settings.cfg file during startup to see which collections should be shown.

If you're migrating from a previous version of EmulationStation that has absolute paths in the collection files, these will be rewritten with the %ROMPATH% variable the first time you make a change to the collection.


## Themes

ES is fully themeable, and although the application ships with the comprehensive rbsimple-DE theme set, you can replace it with a number of themes available from various locations on the Internet.

>>>
Somewhat confusingly the term 'theme' and 'theme set' are used to refer to the same thing. The correct term for what you apply to the application to achieve a different look is actually 'theme set' as it's a collection of a number of themes for a number of game systems. The supplied rbsimple-DE is an example of such a theme set. But in this guide and in other EmulationStation resources on the Internet, the term 'theme' is often used to refer to the same thing as a 'theme set'.
>>>

Note that this Desktop Edition fork adds additional features to the themes and more still will be added in the future. This means that you may not get the full benefit of the application if you're using a different theme set. But effort is spent trying to make sure that the application is backwards compatible with the available themes used by other ES versions, even with the limited functionality.

Themes are most easily installed in your ES home directory, i.e. `~/.emulationstation/themes`. By just adding the theme sets there, one folder each, they will be found by ES during startup and you're given an option to choose which one to use from the 'UI Settings' on the main menu.

Note that although you can put additional themes in your ES home directory, the default rbsimple-DE theme is located in your installation folder. For example this could be something like `/usr/local/share/emulationstation/themes` on Unix, `/Applications/EmulationStation.app/Contents/Resources/themes` on macOS or `C:\Program Files\EmulationStation\themes` on Windows.

Note: If you would like to customize the rbsimple-DE theme, simply make a copy of the complete rbsimple-DE directory to ~/.emulationstation/themes and then that copy of the theme will take precedence over the one in the application installation directory.

In this example, we've downloaded the Carbon and Fundamental themes and uncompressed them to the ES folder:

```
~/.emulationstation/themes/es-theme-carbon
~/.emulationstation/themes/es-theme-fundamental
```

You will now have three entries for the Theme set selector in the UI settings menu, i.e. rbsimple-DE, es-theme-carbon and es-theme-fundamental.

Here are some resources where additional theme sets can be downloaded.

https://aloshi.com/emulationstation#themes

https://github.com/RetroPie

https://gitlab.com/recalbox/recalbox-themes

https://wiki.batocera.org/themes


## Custom event scripts

There are numerous locations throughout ES where custom scripts will be executed if the option to do so has been enabled in the settings. By default it's deactivated so be sure to enable it to use this feature.

The setup for this is a bit technical, so please refer to the [INSTALL.md](INSTALL.md) document to see how it's configured.


## Portable installation (Windows only)

On Windows, ES can be installed to and run from a removable media device such as a USB memory stick. Together with games and emulators this makes for a fully portable retro gaming solution. The setup is somewhat technical, please refer to the [INSTALL.md](INSTALL.md) document to see how it's configured.


## Command line arguments

Please refer to the [INSTALL.md](INSTALL.md#command-line-arguments) document for a list of the command line arguments per operating system.


## Supported game systems

For details regarding the systems such as which emulator or core is setup as default or which file extensions are supported, refer to the **es_systems.cfg** templates [es_systems.cfg_unix](resources/templates/es_systems.cfg_unix), [es_systems.cfg_macos](resources/templates/es_systems.cfg_macos) and [es_systems.cfg_windows](resources/templates/es_systems.cfg_windows).

**Note:** The following list is what the default es_systems.cfg files and the rbsimple-DE theme supports. This theme is very comprehensive, so if you're using another theme, it may be that some or many of these systems are not supported. EmulationStation will still work but the game system will not be themed which looks very ugly.

The column **Game system name** corresponds to the directory where you should put your game files, e.g. `~/ROMs/c64` or `~/ROMs/megadrive`.

Regional differences are handled by simply using the game system name corresponding to your region. For example for Sega Mega Drive, _megadrive_ would be used by most people in the world, although people from North America would use _genesis_ instead. The same is true for _pcengine_ vs _tg16_ etc. This only affects the theme selection and the corresponding theme graphics, the same emulator and scraper settings are still used for the regional variants although that can of course be modified in the es_systems.cfg file if you wish to.

Sometimes the name of the console is (more or less) the same for multiple regions, and in those circumstances the region has been added as a suffix to the game system name. For example 'na' for North America has been added to `snes` (Super Nintendo), as this is the minority region corresponding to around 7,5% of the world population. The same goes for Japan, as in `megacd` and `megacdjp`. Again, this only affects the theme and theme graphics.

| Game system name      | Full name                                      | Recommended game setup               |
| :-------------------- | :--------------------------------------------- | :----------------------------------- |
| 3do                   | 3DO                                            |                                      |
| ags                   | Adventure Game Studio                          |                                      |
| amiga                 | Commodore Amiga                                | WHDLoad hard disk image in .hdf or .hdz format, or diskette image in .adf format (with .m3u playlist if multi-disk) |
| amiga600              | Commodore Amiga 600                            | WHDLoad hard disk image in .hdf or .hdz format, or diskette image in .adf format (with .m3u playlist if multi-disk) |
| amiga1200             | Commodore Amiga 1200                           | WHDLoad hard disk image in .hdf or .hdz format, or diskette image in .adf format (with .m3u playlist if multi-disk) |
| amigacd32             | Commodore Amiga CD32                           |                                      |
| amstradcpc            | Amstrad CPC                                    |                                      |
| apple2                | Apple II                                       |                                      |
| apple2gs              | Apple IIGS                                     |                                      |
| arcade                | Arcade                                         | Single archive file following MAME name standard in root folder |
| astrocade             | Bally Astrocade                                |                                      |
| atari2600             | Atari 2600                                     |                                      |
| atari5200             | Atari 5200                                     |                                      |
| atari7800             | Atari 7800 ProSystem                           |                                      |
| atari800              | Atari 800                                      |                                      |
| atarijaguar           | Atari Jaguar                                   |                                      |
| atarijaguarcd         | Atari Jaguar CD                                |                                      |
| atarilynx             | Atari Lynx                                     |                                      |
| atarist               | Atari ST                                       |                                      |
| atarixe               | Atari XE                                       |                                      |
| atomiswave            | Atomiswave                                     |                                      |
| bbcmicro              | BBC Micro                                      |                                      |
| c64                   | Commodore 64                                   | Single disk, tape or cartridge image in root folder and/or multi-disk images in separate folder |
| cavestory             | Cave Story (NXEngine)                          |                                      |
| cdtv                  | Commodore CDTV                                 |                                      |
| channelf              | Fairchild Channel F                            |                                      |
| coco                  | Tandy Color Computer                           |                                      |
| coleco                | ColecoVision                                   |                                      |
| daphne                | Daphne Arcade Laserdisc Emulator               |                                      |
| desktop               | Desktop applications                           |                                      |
| doom                  | Doom                                           |                                      |
| dos                   | DOS (PC)                                       | In separate folder (one folder per game, with complete file structure retained) |
| dragon32              | Dragon 32                                      |                                      |
| dreamcast             | Sega Dreamcast                                 |                                      |
| famicom               | Nintendo Family Computer                       | Single archive or ROM file in root folder |
| fba                   | Final Burn Alpha                               | Single archive file following MAME name standard |
| fbneo                 | FinalBurn Neo                                  | Single archive file following MAME name standard |
| fds                   | Nintendo Famicom Disk System                   |                                      |
| gameandwatch          | Nintendo Game and Watch                        |                                      |
| gamegear              | Sega Game Gear                                 |                                      |
| gamecube              | Nintendo GameCube                              |                                      |
| gb                    | Nintendo Game Boy                              |                                      |
| gba                   | Nintendo Game Boy Advance                      |                                      |
| gbc                   | Nintendo Game Boy Color                        |                                      |
| genesis               | Sega Genesis                                   | Single archive or ROM file in root folder |
| gx4000                | Amstrad GX4000                                 |                                      |
| intellivision         | Mattel Electronics Intellivision               |                                      |
| chailove              | ChaiLove game engine                           |                                      |
| kodi                  | Kodi home theatre software                     |                                      |
| lutris                | Lutris open gaming platform (Unix only)        | Shell script in root folder          |
| lutro                 | Lutro game engine                              |                                      |
| macintosh             | Apple Macintosh                                |                                      |
| mame                  | Multiple Arcade Machine Emulator               | Single archive file following MAME name standard in root folder |
| mame-advmame          | AdvanceMAME                                    | Single archive file following MAME name standard in root folder |
| mame-libretro         | Multiple Arcade Machine Emulator               | Single archive file following MAME name standard in root folder |
| mame-mame4all         | MAME4ALL                                       | Single archive file following MAME name standard in root folder |
| mastersystem          | Sega Master System                             |                                      |
| megacd                | Sega Mega-CD                                   |                                      |
| megacdjp              | Sega Mega-CD (Japan)                           |                                      |
| megadrive             | Sega Mega Drive                                | Single archive or ROM file in root folder |
| mess                  | Multi Emulator Super System                    |                                      |
| moonlight             | Moonlight game streaming                       |                                      |
| msx                   | MSX                                            |                                      |
| msx1                  | MSX1                                           |                                      |
| msx2                  | MSX2                                           |                                      |
| naomi                 | Sega NAOMI                                     |                                      |
| n64                   | Nintendo 64                                    | Single archive or ROM file in root folder |
| nds                   | Nintendo DS                                    |                                      |
| neogeo                | Neo Geo                                        | Single archive file following MAME name standard |
| neogeocd              | Neo Geo CD                                     |                                      |
| nes                   | Nintendo Entertainment System                  | Single archive or ROM file in root folder |
| ngp                   | Neo Geo Pocket                                 |                                      |
| ngpc                  | Neo Geo Pocket Color                           |                                      |
| odyssey2              | Magnavox Odyssey2                              |                                      |
| openbor               | OpenBOR game engine                            |                                      |
| oric                  | Tangerine Computer Systems Oric                |                                      |
| palm                  | Palm OS                                        |                                      |
| pc                    | IBM PC                                         | In separate folder (one folder per game, with complete file structure retained) |
| pcengine              | NEC PC Engine                                  | Single archive or ROM file in root folder |
| pcenginecd            | NEC PC Engine CD                               |                                      |
| pcfx                  | NEC PC-FX                                      |                                      |
| pokemini              | Nintendo Pokémon Mini                          |                                      |
| ports                 | Ports                                          | Shell/batch script in separate folder (possibly combined with game data) |
| ps2                   | Sony PlayStation 2                             |                                      |
| psp                   | PlayStation Portable                           |                                      |
| psvita                | PlayStation Vita                               |                                      |
| psx                   | Sony PlayStation 1                             |                                      |
| residualvm            | ResidualVM game engine                         |                                      |
| samcoupe              | SAM Coupé                                      |                                      |
| satellaview           | Nintendo Satellaview                           |                                      |
| saturn                | Sega Saturn                                    |                                      |
| scummvm               | ScummVM game engine                            | In separate folder (one folder per game, with complete file structure retained) |
| sega32x               | Sega Mega Drive 32X                            | Single archive or ROM file in root folder |
| sega32xjp             | Sega Super 32X (Japan)                         | Single archive or ROM file in root folder |
| sega32xna             | Sega Genesis 32X (North America)               | Single archive or ROM file in root folder |
| segacd                | Sega CD                                        |                                      |
| sg-1000               | Sega SG-1000                                   |                                      |
| snes                  | Nintendo SNES (Super Nintendo)                 | Single archive or ROM file in root folder |
| snesna                | Nintendo SNES (Super Nintendo) (North America) | Single archive or ROM file in root folder |
| solarus               | Solarus game engine                            |                                      |
| spectravideo          | Spectravideo                                   |                                      |
| steam                 | Valve Steam                                    | Shell/batch script in root folder    |
| stratagus             | Stratagus game engine                          |                                      |
| sufami                | Bandai SuFami Turbo                            |                                      |
| supergrafx            | NEC SuperGrafx                                 |                                      |
| thomson               | Thomson TO/MO series                           |                                      |
| tg16                  | NEC TurboGrafx-16                              |                                      |
| tg-cd                 | NEC TurboGrafx-CD                              |                                      |
| ti99                  | Texas Instruments TI-99                        |                                      |
| trs-80                | Tandy TRS-80                                   |                                      |
| uzebox                | Uzebox                                         |                                      |
| vectrex               | Vectrex                                        |                                      |
| videopac              | Philips Videopac G7000 (Magnavox Odyssey2)     |                                      |
| virtualboy            | Nintendo Virtual Boy                           |                                      |
| wii                   | Nintendo Wii                                   |                                      |
| wiiu                  | Nintendo Wii U                                 |                                      |
| wonderswan            | Bandai WonderSwan                              |                                      |
| wonderswancolor       | Bandai WonderSwan Color                        |                                      |
| x68000                | Sharp X68000                                   |                                      |
| xbox                  | Microsoft Xbox                                 |                                      |
| xbox360               | Microsoft Xbox 360                             |                                      |
| zmachine              | Infocom Z-machine                              |                                      |
| zx81                  | Sinclair ZX81                                  |                                      |
| zxspectrum            | Sinclair ZX Spectrum                           |                                      |

Building
========

EmulationStation-DE uses some C++11 code, which means you'll need to use a compiler that supports that. \
GCC is recommended although other compilers should hopefully work fine as well.

The code has a few dependencies. For building, you'll need CMake and development packages for SDL2, FreeImage, FreeType, libVLC, cURL and RapidJSON.

**On Debian/Ubuntu:**
All of the required packages can be easily installed with `apt-get`:
```
sudo apt-get install libsdl2-dev libfreeimage-dev libfreetype6-dev libcurl4-openssl-dev rapidjson-dev
  libasound2-dev libgl1-mesa-dev build-essential cmake fonts-droid-fallback libvlc-dev
  libvlccore-dev vlc-bin
```
**On Fedora:**
For this operating system, use `dnf` (with rpmfusion activated) :
```
sudo dnf install SDL2-devel freeimage-devel freetype-devel curl-devel
  alsa-lib-devel mesa-libGL-devel cmake
  vlc-devel rapidjson-devel
```

To checkout the source, run the following:

```
git clone https://gitlab.com/leonstyhre/emulationstation-de
```

Then generate the Makefile and build the software like this:

```
cd emulationstation-de
cmake -DOpenGL_GL_PREFERENCE=GLVND .
make
```

NOTE: to generate a `Debug` build on Unix/Linux, run this instead:
```
cmake -DOpenGL_GL_PREFERENCE=GLVND -DCMAKE_BUILD_TYPE=Debug .
make
```
Keep in mind though that a debug version will be much slower due to all compiler optimizations being disabled.

**On Windows:**

[CMake](http://www.cmake.org/cmake/resources/software.html)

[SDL2](http://www.libsdl.org/release/SDL2-devel-2.0.8-VC.zip)

[FreeImage](http://downloads.sourceforge.net/freeimage/FreeImage3154Win32.zip)

[FreeType2](http://download.savannah.gnu.org/releases/freetype/freetype-2.4.9.tar.bz2) (you'll need to compile)

[cURL](http://curl.haxx.se/download.html) (you'll need to compile or get the pre-compiled DLL version)

[RapisJSON](https://github.com/tencent/rapidjson) (you need to add `include/rapidsjon` to your include path)

Remember to copy the necessary .DLL files into the same folder as the executable: probably FreeImage.dll, freetype6.dll, SDL2.dll, libcurl.dll, and zlib1.dll. Exact list depends on if you built your libraries in "static" mode or not.


Configuring
===========


**~/.emulationstation/es_systems.cfg:**

EmulationStation Desktop Edition ships with a comprehensive `es_systems.cfg` configuration file, and as the logic is to use a `%ROMPATH%` variable to locate the ROM files (with a corresponding setting in `es_settings.cfg`), normally you shouldn't need to modify this file to the same extent as previous versions of EmulationStation. Still, see below in this document on how to adjust the es_systems.cfg file if required.

Upon first startup of the application, if there is no es_systems.cfg file present, it will be copied from the template subdirectory inside the resources directory. This directory is located in the installation path of the application, for instance `/usr/local/share/emulationstation/resources/templates`.

The template file will be copied to `~/.emulationstation/es_systems.cfg`.  `~` is `$HOME` on Linux, and `%HOMEPATH%` on Windows.

**~/.emulationstation/es_settings.cfg:**

When ES is first run, an example configuration file will be created as `~/.emulationstation/es_settings.cfg`.  `~` is `$HOME` on Linux, and `%HOMEPATH%` on Windows. \
This file contains all the settings supported by ES, at their default values. Normally you shouldn't need to modify this file manually, instead you should be able to use the menu inside ES to update all the necessary settings.

The exception would be the ROMDirectory setting as ES won't start if no ROM files are found.

**Setting the ROM directory:**

By default, ES looks in `~/ROMs` for the ROM files, where they are expected to be grouped into directories corresponding to the game systems, for example:

```
user@computer:~ROMs$ ls -1
arcade
megadrive
pcengine
```

However, if you've saved your ROMs to another directory, you need to configure the ROMDirectory setting in es_settings.cfg.\
Here's an example:

`<string name="ROMDirectory" value="~/games/roms" />`

Keep in mind though that you still need to group the ROMs into directories corresponding to the system names. Well at least if you want to use the default `es_systems.cfg` file. See below how to customize that file, which gives you full control over the location of the ROMs.

**Keep in mind that you'll have to set up your emulator separately from EmulationStation!**

**~/.emulationstation/es_input.cfg:**

When you first start EmulationStation, you will be prompted to configure an input device. The process is thus:

1. Hold a button on the device you want to configure.  This includes the keyboard.

2. Press the buttons as they appear in the list.  Some inputs can be skipped by holding any button down for a few seconds (e.g. page up/page down).

3. You can review your mappings by pressing up and down, making any changes by pressing A.

4. Choose "SAVE" to save this device and close the input configuration screen.

The new configuration will be added to the `~/.emulationstation/es_input.cfg` file.

**Both new and old devices can be (re)configured at any time by pressing the Start button and choosing "CONFIGURE INPUT".**  From here, you may unplug the device you used to open the menu and plug in a new one, if necessary.  New devices will be appended to the existing input configuration file, so your old devices will retain their configuration.

**If your controller stops working, you can delete the `~/.emulationstation/es_input.cfg` file to make the input configuration screen re-appear on the next run.**


**Command line arguments:**

You can use `--help` or `-h` to view a list of command line options. Briefly outlined here:

```
--resolution [width] [height]   Try to force a particular resolution
--gamelist-only                 Skip automatic game ROM search, only read from gamelist.xml
--ignore-gamelist               Ignore the gamelist files (useful for troubleshooting)
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
--force-kid                     Force the UI mode to Kid
--force-kiosk                   Force the UI mode to Kiosk
--force-disable-filters         Force the UI to ignore applied filters in gamelist
--home [path]                   Directory to use as home path
--version, -v                   Displays version information
--help, -h                      Summon a sentient, angry tuba
```

As long as ES hasn't frozen, you can always press F4 to close the application.

As you can see above, you can override the home directory path using the `--home` flag. So by running for instance the command `emulationstation --home ~/games/emulation`, ES will use `~/games/emulation/.emulationstation` as its base directory.


Writing an es_systems.cfg
=========================

The es_systems.cfg file contains the system configuration data for EmulationStation, written in XML format. \
This tells EmulationStation what systems you have, what platform they correspond to (for scraping), and where the games are located.

ES will only check in your home directory for an es_systems.cfg file, for example `~/.emulationstation/es_systems.cfg`.

The order EmulationStation displays systems reflects the order you define them in. In the case of the default es_systems.cfg file, the systems are listed in alphabetical order.

**NOTE:** A system *must* have at least one game present in its "path" directory, or ES will ignore it! If no valid systems are found, ES will report an error and quit!

Here's an overview of the file layout:

```xml
<?xml version="1.0"?>
<!-- This is the EmulationStation-DE game systems configuration file. -->
<systemList>
    <!-- Here's an example system to get you started. -->
    <system>
        <!-- A short name, used internally. -->
        <name>snes</name>

        <!-- A "pretty" name, displayed in the menus and such. This one is optional. -->
        <fullname>Super Nintendo Entertainment System</fullname>

        <!-- The path to start searching for ROMs in. '~' will be expanded to $HOME or %HOMEPATH%, depending on platform.
        The optional %ROMPATH% variable will expand to the path defined for the setting ROMDirectory in es_settings.cfg.
        All subdirectories (and non-recursive links) will be included. -->
        <path>%ROMPATH%/snes</path>

        <!-- A list of extensions to search for, delimited by any of the whitespace characters (", \r\n\t").
        You MUST include the period at the start of the extension! It's also case sensitive. -->
        <extension>.smc .SMC .sfc .SFC .swc .SWC .fig .FIG .bs .BS .bin .BIN .mgd .MGD .7z .7Z .zip .ZIP</extension>

        <!-- The command executed when a game is launched. A few special variables are replaced if found in a command, like %ROM% (see below).
        This example would run RetroArch with the the snes9x_libretro core. -->
        <command>retroarch -L ~/.config/retroarch/cores/snes9x_libretro.so %ROM%</command>

        <!-- The platform(s) to use when scraping. You can see the full list of accepted platforms in src/PlatformIds.cpp.
        It's case sensitive, but everything is lowercase. This tag is optional.
        You can use multiple platforms too, delimited with any of the whitespace characters (", \r\n\t"), e.g.: "megadrive, genesis" -->
        <platform>snes</platform>

        <!-- The theme to load from the current theme set. See THEMES.md for more information. -->
        <theme>snes</theme>
    </system>
</systemList>
```

The following variables are expanded by ES for the `command` tag:

`%ROM%` - Replaced with absolute path to the selected ROM, with most Bash special characters escaped with a backslash.

`%BASENAME%` - Replaced with the "base" name of the path to the selected ROM. For example, a path of `/foo/bar.rom`, this tag would be `bar`. This tag is useful for setting up AdvanceMAME.

`%ROM_RAW%`	- Replaced with the unescaped, absolute path to the selected ROM.  If your emulator is picky about paths, you might want to use this instead of %ROM%, but enclosed in quotes.

For the `path` tag, the following variable is expanded by ES:

`%ROMPATH%` - Replaced with the path defined for the setting ROMDirectory in `es_settings.cfg`.


gamelist.xml
============

The gamelist.xml file for a system defines metadata for games, such as a name, description, release date, and rating.

As of the fork to EmulationStation Desktop Edition, game media information no longer needs to be defined in the gamelist.xml files. Instead the application will look for any media matching the ROM filename. The media path where to look for game art is configurable either manually in `es_settings.cfg` or via the GUI. If configured manually in es_settings.cfg, it looks something like this:

`<string name="MediaDirectory" value="~/games/images/emulationstation" />`

The default game media directory is `~/.emulationstation/downloaded_media`.

If at least one game in a system has an image (mix image, screenshot or box cover), ES will use the detailed view for that system (which displays metadata alongside the game list).

*You can use ES's [scraping](http://en.wikipedia.org/wiki/Web_scraping) tools to avoid creating a gamelist.xml by hand.*  There are two ways to run the scraper:

* **If you want to scrape multiple games:** press start to open the menu and choose the "SCRAPER" option.  Adjust your settings and press "START".
* **If you just want to scrape one game:** find the game on the game list in ES and press select.  Choose "EDIT THIS GAME'S METADATA" and then press the "SCRAPE" button at the bottom of the metadata editor.

You can also edit metadata within ES by using the metadata editor - just find the game you wish to edit on the gamelist, press Select, and choose "EDIT THIS GAME'S METADATA."

The switch `--ignore-gamelist` can be used to ignore the gamelist and force ES to use the non-detailed view.

If you're writing a tool to generate or parse gamelist.xml files, you should check out [GAMELISTS.md](GAMELISTS.md) for more detailed documentation.


Themes
======

EmulationStation is not intended to be used without a theme. The default theme 'rbsimple-DE' is included in the emulationstation-de repository.

For additional themes, the following resources are recommended:

https://aloshi.com/emulationstation#themes

https://github.com/RetroPie

https://gitlab.com/recalbox/recalbox-themes

https://wiki.batocera.org/themes

For information on how to make your own theme or edit an existing one, read [THEMES.md](THEMES.md).

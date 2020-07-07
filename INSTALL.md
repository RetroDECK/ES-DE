EmulationStation Desktop Edition - Installation
===============================================

**Note:** This is a quite technical document intended for those that are interested in compiling EmulationStation from source code, or would like to customize the configuration. If you just want to start using the software, check out the [User Guide](USERGUIDE.md) instead!


Building
========

The code has a few dependencies. For building, you'll need CMake and development packages for cURL, FreeImage, FreeType, libVLC, pugixml, SDL2 and RapidJSON.

**On Debian/Ubuntu:**
All of the required packages can be easily installed with `apt-get`:
```
sudo apt-get install build-essential cmake libsdl2-dev libfreeimage-dev libfreetype6-dev libcurl4-openssl-dev libpugixml-dev rapidjson-dev libasound2-dev libvlc-dev libgl1-mesa-dev
```

**On Fedora:**
For this operating system, use `dnf` (with rpmfusion activated) :
```
sudo dnf install cmake SDL2-devel freeimage-devel freetype-devel curl-devel rapidjson-devel alsa-lib-devel vlc-devel mesa-libGL-devel
```

To clone the source repository, run the following:

```
git clone https://gitlab.com/leonstyhre/emulationstation-de
```

Then generate the Makefile and build the software:

```
cd emulationstation-de
cmake .
make
```

Note: To generate a `Debug` build on Unix/Linux, run this instead:
```
cmake -DCMAKE_BUILD_TYPE=Debug .
make
```
Keep in mind though that a debug version will be much slower due to all compiler optimizations being disabled.

Running multiple compile jobs in parallel is a good thing as it speeds up the build time a lot (scaling almost linearly). Here's an example telling make to run 6 parallel jobs:

```
make -j6
```

By default EmulationStation will install under `/usr/local` but this can be changed by setting the `CMAKE_INSTALL_PREFIX` variable.\
The following example will build the application for installtion under `/opt`:

```
cmake -DCMAKE_INSTALL_PREFIX=/opt .
```

It's important to know that this is not only the directory used by the install script, the CMAKE_INSTALL_PREFIX variable also modifies code inside ES used to locate the required program resources. So it's necessary that the install prefix corresponds to the location where the application will actually be installed.

**Compilers:**

Both Clang/LLVM and GCC work fine for building ES.

The Microsoft Visual C++ compiler (MSVC) could maybe work as well, there are some old settings in CMakeLists.txt for it. Try it if you want, but I recommend Clang or GCC instead of this legacy compiler.

I did some small benchmarks comparing Clang to GCC with the ES codebase (as of writing it's year 2020) and it's pretty interesting.

Advantages with Clang (vs GCC):
* 10% smaller binary size for a release build
* 17% smaller binary size for a debug build
* 2% faster compile time for a release build
* 16% faster compile time for a debug build
* 4% faster application startup time for a debug build

Advantage with GCC (vs Clang):
* 1% faster application startup time for a release build

*Release build: Optimizations enabled, debug info disabled, binary stripped.* \
*Debug build: Optimizations disabled, debug info enabled, binary not stripped.*

This Clang debug build is LLVM "native", i.e. intended to be debugged using the LLVM project debugger LLDB. The problem is that this is still not well integrated with VSCodium that I use for development so I need to keep using GDB. But this is problematic as the libstd++ data required by GDB is missing in the binary, making it impossible to see the values of for instance `std::string` variables.

It's possible to activate the additional debug info needed by GDB by using the flag `-D_GLIBCXX_DEBUG`. I've added this to CMakeLists.txt when using Clang, but this bloats the binary and makes the code much slower. Actually, instead of a 4% faster application startup, it's now 36% slower! The same goes for the binary size, instead of 17% smaller it's now 17% larger.

So overall Clang is interesting and perhaps even a better compiler than GCC, but it needs better integration with VSCodium before it's really usable. For macOS it seems as if Clang is the preferred compiler though, so it's good that ES now fully supports it. (It took quite some effort to get all the compile errors and compile warnings sorted out.)

It's by the way very easy to switch between LLVM and GCC using Ubuntu, just use the `update-alternatives` command:

```
user@computer:~$ sudo update-alternatives --config c++
[sudo] password for user:
There are 2 choices for the alternative c++ (providing /usr/bin/c++).

  Selection    Path              Priority   Status
------------------------------------------------------------
* 0            /usr/bin/g++       20        auto mode
  1            /usr/bin/clang++   10        manual mode
  2            /usr/bin/g++       20        manual mode

Press <enter> to keep the current choice[*], or type selection number: 1
update-alternatives: using /usr/bin/clang++ to provide /usr/bin/c++ (c++) in manual mode
```

Following this, just re-run cmake and make and the binary should be built by Clang instead.

**Installing:**

Installing the software requires root permissions, the following command will install all the required application files:

```
sudo make install
```

Assuming the default installation prefix `/usr/local` has been used, this is the directory structure for the installation:

```
/usr/local/bin/emulationstation
/usr/local/share/emulationstation/LICENSES
/usr/local/share/emulationstation/resources
/usr/local/share/emulationstation/themes
```

**Note:** The resources directory is critical, without it the application won't start.

ES will look in the following two locations for the resources:

* `[HOME]/.emulationstation/resources/`
* `[INSTALL PREFIX]/share/emulationstation/resources/`

The home directory will always take precedence, so any resources located there will override the ones in the installation path. It's not recommended to override any resources since they are by nature static. But using the ability to locate resource files in the home directory is very useful when combined with the command line option `--home` (see below), as a fully portable version of EmulationStation can then be created on a USB memory stick or similar.

Anyway, after compiling the application, either run `make install` or copy or symlink the resources directory to `~/.emulationstation/resources`.

`cp -R resources ~/.emulationstation/` \
or \
`ln -s $(pwd)/resources ~/.emulationstation/`

The same goes for the `themes` directory. Although ES can start without a theme, it would be pretty pointless as the application would barely be usable.

**Creating .deb and .rpm packages:**

Creation of Debian .deb packages are enabled by default, simply run `cpack` to generate the package:

```
user@computer:~/emulationstation-de$ cpack
CPack: Create package using DEB
CPack: Install projects
CPack: - Run preinstall target for: emulationstation-de
CPack: - Install project: emulationstation-de []
CPack: Create package
CPackDeb: - Generating dependency list
CPack: - package: /home/user/emulationstation-de/emulationstation-de-1.0.0.deb generated.
```

The package can now be installed using a package manager, for example `dpkg`.

For RPM packages, change the comments in the `CMakeLists.txt` accordingly, from:

```
#SET(CPACK_GENERATOR "RPM")
SET(CPACK_GENERATOR "DEB")
```
to:
```
SET(CPACK_GENERATOR "RPM")
#SET(CPACK_GENERATOR "DEB")
```

Then simply run `cpack`.

To be able to generate RPM packages on a Debian system such as Ubuntu, install the `rpm` package first:

```
sudo apt-get install rpm
```


### On Windows:

This is a strange legacy operating system. However it's still popular, so we need to support it.

I did a brief evaluation of the Microsoft Visual C++ compiler (MSVC) but as far as I'm concerned it's an abomination so I won't cover it here and it won't be supported.

At the moment I have only built the software using GCC on Windows, but I may try to get Clang/LLVM working at a later date.

Anyway, here's a brief summary of how to get a build environment up and running on Windows.

**Install Git, CMake, MinGW and your code editor:**

[Git](https://gitforwindows.org)

[CMake](https://cmake.org/download)

[MinGW](https://gnutoolchains.com/mingw64)

Make a copy of `mingw64/bin/mingw32-make` to `make` just for convenience and make sure that the necessary paths are defined for the PATH environmental variable.

I won't get into the details on how to configure Git, but there are many resources available online to support with this. The `Git Bash` shell is very useful though as it's somewhat reproducing a Unix environment using MinGW/MSYS.

Install your editor of choice. As for VSCodium it's unfortunately broken or crippled under Windows, making some important extensions impossible to install. VSCode can however be used instead.

It's strongly recommended to set line breaks to Unix-style (linefeed only) directly in the editor, although it can also be configured in Git for conversion during commit. The source code for EmulationStation-DE only uses Unix-style line breaks.

**Enable pretty printing for GDB:**

This is useful for displaying `std::string` values for example.

Adjust your paths accordingly, the below are just examples of course.

Save a file to
C:/Programming/mingw64/bin/pp.gdb with the following contents:

```
python
import sys
sys.path.insert(0, 'c:/Programming/mingw64/share/gcc-9.1.0/python/libstdcxx/v6')
from printers import register_libstdcxx_printers
register_libstdcxx_printers (None)
end
```

If using VSCode, add the following line to launch.json:

`"miDebuggerArgs": "-x c:/programming/mingw64/bin/pp.gdb",`

An equivalent setup should be possible on other code editors as well.

Note that most GDB builds for Windows have broken Python support so that pretty printing won't work. The MinGW installation recommended in the previous step works fine though.

**Download the dependency packages:**

[FreeImage](https://sourceforge.net/projects/freeimage)

[cURL](https://curl.haxx.se/download.html)

[SDL2](https://www.libsdl.org/download-2.0.php)

[libVLC](https://ftp.lysator.liu.se/pub/videolan/vlc)

Uncompress the files to a suitable directory, for example C:/Programming/Dependencies/

The following packages are not readily available for Windows, so clone the repos and build them yourself:

[FreeType](https://www.freetype.org)
```
git clone git://git.savannah.gnu.org/freetype/freetype2.git
git checkout VER-2-10-2
mkdir build
cmake -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=ON ..
make
```

[pugixml](https://pugixml.org)
```
git clone git://github.com/zeux/pugixml
git checkout v1.10
cmake -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=ON .
make
```

As for RapidJSON, you don't need to compile it, you just need the include files:

[RapidJSON](http://rapidjson.org)
```
git clone git://github.com/Tencent/rapidjson
git checkout v1.1.0
```

**Clone the EmulationStation-DE repository:**

This works the same as in Unix, just run the following:

```
git clone https://gitlab.com/leonstyhre/emulationstation-de
```

**Setup the include directories:**

As there is no standardized include directory structure in Windows and no package manager, you need to provide the include files manually.

Make a directory in your build environment tree, for instance under `C:/Programming/include`.

Copy the include files from cURL, FreeImage, FreeType, pugixml, RapidJSON, SDL2 and VLC to this directory.
It should then look something like this:

```
$ ls -1 include/
curl/
FreeImage.h
freetype/
ft2build.h
pugiconfig.hpp
pugixml.hpp
rapidjson/
SDL2/
vlc/
```

**Copy the required DLL files to the EmulationStation build directory:**

As there's no package manager in Windows and no way to handle dependencies, we need to ship all the required shared libraries with the application.

Copy the following files to the `emulationstation-de` build directory. Most of them will come from the packages that were provided in the previous steps of this guide:

```
FreeImage.dll
libcrypto-1_1-x64.dll    (from the OpenSSL package, located in Git MinGW/MSYS under /mingw/bin/)
libcurl-x64.dll
libfreetype.dll
libgcc_s_seh-1.dll    (located in Git MinGW/MSYS under /mingw/bin/)
libpugixml.dll
libssl-1_1-x64.dll    (from the OpenSSL package, located in Git MinGW under /mingw/bin/)
libstdc++-6.dll
libvlc.dll
libvlccore.dll
libwinpthread-1.dll    (located in Git MinGW under /mingw/bin/)
SDL2.dll
libSDL2main.a
```

The files from the MinGW installation must correspond to the version used to compile the binary.

*So if the MinGW installation is upgraded to a newer version or so, make sure to copy the .dll files again, overwriting the old ones.*

**Building the application:**

For a release build:

```
cmake -G "MinGW Makefiles" -DWIN32_INCLUDE_DIR=../include .
```

Or for a debug build:

```
cmake -G "MinGW Makefiles" -DWIN32_INCLUDE_DIR=../include -DCMAKE_BUILD_TYPE=Debug .
```

For some reason defining the '../include' path doesn't work when running CMake from PowerShell (and no, changing to backslash doesn't help). Instead use Bash, by running from a `Git Bash` shell.

The make command works fine directly in PowerShell though so it can be run from the VSCode terminal.

Running `make -j6` (or whatever number of parallel jobs you prefer) should now build the binary.

Note that compilation time is much longer than on Unix, and linking time is excessive for a debug build. The debug binary is also much larger than on Unix.

A worthwhile endeavour could be to setup a cross-compilation environment using WLS/WLS2 (Linux), but I have not tried it.


**Creating an NSIS installer:**

To create an NSIS installer (Nullsoft Scriptable Install System) you need to first install the NSIS creation tool:

[NSIS](https://nsis.sourceforge.io/Download)

Simply install the application using it's installer.

After the installation has been completed, go to the emulationstation-de directory and run cpack to generate the NSIS installer:

```
$ cpack
CPack: Create package using NSIS
CPack: Install projects
CPack: - Run preinstall target for: emulationstation-de
CPack: - Install project: emulationstation-de []
CPack: Create package
CPack: - package: C:/Programming/emulationstation-de/emulationstation-de-1.0.0-win64.exe generated.
```


Configuring EmulationStation-DE
===============================


**~/.emulationstation/es_systems.cfg:**

EmulationStation Desktop Edition ships with a comprehensive `es_systems.cfg` configuration file, and as the logic is to use a `%ROMPATH%` variable to locate the ROM files (with a corresponding setting in `es_settings.cfg`), normally you shouldn't need to modify this file to the same extent as previous versions of EmulationStation. Still, see below in this document on how to adjust the es_systems.cfg file if required.

Upon first startup of the application, if there is no es_systems.cfg file present, it will be copied from the template subdirectory inside the resources directory. This directory is located in the installation path of the application, for instance `/usr/local/share/emulationstation/resources/templates`.

The template file will be copied to `~/.emulationstation/es_systems.cfg`. \
`~` is `$HOME` on Linux, and `%HOMEPATH%` on Windows.

**~/.emulationstation/es_settings.cfg:**

When ES is first run, a configuration file will be created as `~/.emulationstation/es_settings.cfg`.

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

Keep in mind though that you still need to group the ROMs into directories corresponding to the system names. Well at least if you want to use the default es_systems.cfg file. See below how to customize that file, which gives you full control over the location of the ROMs.

**Keep in mind that you have to set up your emulator separately from EmulationStation!**

**~/.emulationstation/es_input.cfg:**

When you first start EmulationStation, you will be prompted to configure an input device. The process is thus:

1. Hold a button on the device you want to configure.  This includes the keyboard.

2. Press the buttons as they appear in the list.  Some inputs can be skipped by holding any button down for a few seconds (e.g. page up/page down).

3. You can review your mappings by pressing up and down, making any changes by pressing A.

4. Choose "SAVE" to save this device and close the input configuration screen.

The new configuration will be added to the `~/.emulationstation/es_input.cfg` file.

**Both new and old devices can be (re)configured at any time by pressing the Start button and choosing "CONFIGURE INPUT".**  From here, you may unplug the device you used to open the menu and plug in a new one, if necessary.  New devices will be appended to the existing input configuration file, so your old devices will retain their configuration.

**If your controller stops working, you can delete the `~/.emulationstation/es_input.cfg` file to make the input configuration screen re-appear on the next run.**


Command line arguments
======================

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

**Note:** A system *must* have at least one game present in its `path` directory, or ES will ignore it! If no valid systems are found, ES will report an error and quit.

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
        This example would run RetroArch with the the snes9x_libretro core.
        If there are spaces in the path or file name, you must enclose them in quotation marks, for example:
        retroarch -L "~/my configs/retroarch/cores/snes9x_libretro.so" %ROM% -->
        <command>retroarch -L ~/.config/retroarch/cores/snes9x_libretro.so %ROM%</command>

        <!-- This is an example for Windows. The .exe extension is optional and both forward slashes and backslashes are allowed as
        directory separators. As there is no standardized installation directory structure for this operating system, the %EMUPATH%
        variable is used here to find the cores relative to the RetroArch binary. This binary must be in the PATH environmental variable
        or otherwise the complete path to the retroarch.exe file needs to be defined. Batch scripts (.bat) are also supported. -->
        <command>retroarch.exe -L %EMUPATH%\cores\snes9x_libretro.dll %ROM%</command>

        <!-- Another example for Windows. As can be seen here, the absolut path to the emulator has been defined, and there are spaces
        in the directory name, so it needs to be surrounded by quotation marks. As well the quotation marks are needed around the core
        configuration as the %EMUPATH% will expand to the path of the emulator binary, which will of course also include the spaces. -->
        <command>"C:\My Games\RetroArch\retroarch.exe" -L "%EMUPATH%\cores\snes9x_libretro.dll" %ROM%</command>

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

`%EMUPATH%` - Replaced with the path to the emulator binary. This is expanded either using the PATH environmental variable of the operating system, or if an absolute emulator path is defined, this will be used instead. This variable is mostly useful to define the emulator core path for Windows, as this operating system does not have a standardized program installation directory structure.

For the `path` tag, the following variable is expanded by ES:

`%ROMPATH%` - Replaced with the path defined for the setting ROMDirectory in `es_settings.cfg`.


gamelist.xml
============

The gamelist.xml file for a system defines metadata for games, such as a name, description, release date, and rating.

As of the fork to EmulationStation Desktop Edition, game media information no longer needs to be defined in the gamelist.xml files. Instead the application will look for any media matching the ROM filename. The media path where to look for game art is configurable either manually in `es_settings.cfg` or via the GUI. If configured manually in es_settings.cfg, it looks something like this:

`<string name="MediaDirectory" value="~/games/images/emulationstation" />`

The default game media directory is `~/.emulationstation/downloaded_media`.

*You can use ES's [scraping](http://en.wikipedia.org/wiki/Web_scraping) tools to avoid creating a gamelist.xml by hand.*  There are two ways to run the scraper:

* **If you want to scrape multiple games:** press the Start button to open the menu and choose the "SCRAPER" option.  Adjust your settings and press "START".
* **If you just want to scrape one game:** find the game on the game list in ES and press the Select button.  Choose "EDIT THIS GAME'S METADATA" and then press the "SCRAPE" button at the bottom of the metadata editor.

You can also edit metadata within ES by using the metadata editor - just find the game you wish to edit on the gamelist, press Select, and choose "EDIT THIS GAME'S METADATA."

The switch `--ignore-gamelist` can be used to ignore the gamelists upon start of the application.

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

# EmulationStation Desktop Edition - Installation and configuration

**Note:** This is a quite technical document intended for those that are interested in compiling EmulationStation from source code, or would like to customize the configuration. If you just want to start using the software, check out the [User Guide](USERGUIDE.md) instead.

Table of contents:

[[_TOC_]]

## Development Environment

EmulationStation-DE is developed and compiled using both Clang/LLVM and GCC on Unix, Clang/LLVM on macOS and GCC (MinGW) on Windows. I'm intending to get Clang/LLVM working on Windows as well.

There are more details regarding compilers later in this document.

Any code editor can be used of course, but I recommend [VSCode](https://code.visualstudio.com).


## Building on Unix

The code has a few dependencies. For building, you'll need CMake and development packages for cURL, FreeImage, FreeType, libVLC, pugixml, SDL2 and RapidJSON.

**On Debian/Ubuntu:**
All of the required packages can be easily installed with apt-get:
```
sudo apt-get install build-essential cmake libsdl2-dev libfreeimage-dev libfreetype6-dev libcurl4-openssl-dev libpugixml-dev rapidjson-dev libasound2-dev libvlc-dev libgl1-mesa-dev
```

**On Fedora:**
For this operating system, use dnf (with rpmfusion activated) :
```
sudo dnf install cmake SDL2-devel freeimage-devel freetype-devel curl-devel rapidjson-devel alsa-lib-devel vlc-devel mesa-libGL-devel
```

**Cloning and compiling:**

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

To create a debug build, run this instead:
```
cmake -DCMAKE_BUILD_TYPE=Debug .
make
```
Keep in mind though that a debug version will be much slower due to all compiler optimizations being disabled.

To create a profiling build (optimized with debug symbols), run this:
```
cmake -DCMAKE_BUILD_TYPE=Profiling .
make
```

You can then profile the code with Valgrind:
```
valgrind --tool=callgrind ./emulationstation
```

The output file can be loaded into a tool such as KCachegrind for data analysis.

To check for memory leaks, the following command is useful:

```
valgrind --tool=memcheck --leak-check=full ./emulationstation
```

Note that you can also profile either a normal build or a debug build, but it's normally recommended to use the profiling build as it's compiled with optimizations while retaining the debug symbols. But if you need to alternate between Valgrind and the normal debugger the optimizations can be very annoying and therefore a normal debug build could be recommended in that instance.

Another useful tool is **scan-build**, assuming you use Clang/LLVM. This is a static analyzer that runs during compilation to provide a very helpful HTML report of potential bugs (well it should be actual bugs but some false positives could be included). You need to run it for both the cmake and make steps, here's an example:

```
scan-build cmake -DCMAKE_BUILD_TYPE=Debug .
scan-build make -j6
```

You open the report with the **scan-view** command which lets you browse it using your web browser. Note that the compilation time is much higher when using the static analyzer compared to a normal compilation. As well this tool generates a lot of extra files and folders in the build tree, so it may make sense to run it on a separate copy of the ES source folder to avoid having to clean up all this extra data when the analysis has been completed.

To build ES with CEC support, add the corresponding option, for example:

```
cmake -DCMAKE_BUILD_TYPE=Debug -DCEC=on .
make
```
I have however not been able to test the CEC support and I'm not entirely sure how it's supposed to work.

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

This Clang debug build is LLVM "native", i.e. intended to be debugged using the LLVM project debugger LLDB. The problem is that this is still not well integrated with VSCode that I use for development so I need to keep using GDB. But this is problematic as the libstd++ data required by GDB is missing in the binary, making it impossible to see the values of for instance `std::string` variables.

It's possible to activate the additional debug info needed by GDB by using the flag `-D_GLIBCXX_DEBUG`. I've added this to CMakeLists.txt when using Clang, but this bloats the binary and makes the code much slower. Actually, instead of a 4% faster application startup, it's now 36% slower! The same goes for the binary size, instead of 17% smaller it's now 17% larger.

I'm expecting this to be resolved in the near future though, and as I think Clang is an interesting compiler, I use it as the default when working on the project (I sometimes test with GCC to make sure that it still builds the software correctly).

It's by the way very easy to switch between LLVM and GCC using Ubuntu, just use the `update-alternatives` command:

```
myusername@computer:~$ sudo update-alternatives --config c++
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
/usr/local/man/man6/emulationstation.6.gz
/usr/local/share/applications/emulationstation.desktop
/usr/local/share/emulationstation/LICENSE
/usr/local/share/emulationstation/licenses/*
/usr/local/share/emulationstation/resources/*
/usr/local/share/emulationstation/themes/*
/usr/local/share/pixmaps/emulationstation.svg
```

ES will look in the following locations for the resources, in the listed order:

* `[HOME]/.emulationstation/resources/`
* `[INSTALL PREFIX]/share/emulationstation/resources/`
* `[ES EXECUTABLE DIRECTORY]/resources/`

**Note:** The resources directory is critical, without it the application won't start.

And it will look in the following locations for the themes, also in the listed order:

* `[HOME]/.emulationstation/themes/`
* `[INSTALL PREFIX]/share/emulationstation/themes/`
* `[ES EXECUTABLE DIRECTORY]/themes/`

A theme is not mandatory to start the application, but ES will be basically useless without it.

The home directory will always take precedence, and any resources or themes located there will override the ones in the installation path or in the path of the ES executable.

**Creating .deb and .rpm packages:**

Creation of Debian .deb packages is enabled by default, simply run `cpack` to generate the package:

```
myusername@computer:~/emulationstation-de$ cpack
CPack: Create package using DEB
CPack: Install projects
CPack: - Run preinstall target for: emulationstation-de
CPack: - Install project: emulationstation-de []
CPack: Create package
CPackDeb: - Generating dependency list
CPack: - package: /home/myusername/emulationstation-de/emulationstation-de-1.0.0.deb generated.
```

The package can now be installed using a package manager, for example `dpkg`.

For RPM packages, remove the comment in the `CMakeLists.txt` accordingly, from:

```
#SET(CPACK_GENERATOR "RPM")
```
to:
```
SET(CPACK_GENERATOR "RPM")
```

Then simply run `cpack`.

To be able to generate RPM packages on a Debian system such as Ubuntu, install the `rpm` package first:

```
sudo apt-get install rpm
```


## Building on macOS

EmulationStation for macOS is built using Clang/LLVM which is the default compiler for this operating system. It's pretty straightforward to build software on this OS. Although it's a bizarre Unix variant with a very strange window manager, it is still a proper system with good tools. The main deficiency is that there is no native package manager, but as there are several third party package managers available, this can be partly compensated for. The use of one of them, `Homebrew`, is detailed below.

As for code editing, I use [VSCode](https://code.visualstudio.com). I suppose Xcode could be used instead but I have no experience with this tool and no interest in it as I want to use the same tools for all the operating systems that I develop on.

**Setting up the build tools:**

Install the Command Line Tools which include Clang/LLVM, Git, make etc. Simply open a terminal and enter the command `clang`. This will open a dialog that will let you download and install the tools.

Following this, install the Homebrew package manager which will simplify the rest of the installation greatly. Install it by runing the following in a terminal window:
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
```
Be aware though that Homebrew is really slow as it often compiles the packages from source code, so you need some patience when using it.

**Package installation with Homebrew:**

Install cmake, which will also install pkg-config that we need as well:

```
brew install cmake
```

Following this, install the dependency packages needed by ES:
```
brew install sdl2 freeimage freetype pugixml rapidjson
```
Curl could optionally be installed too, but normally the version shipped with macOS is fine to use.

Finally, install VLC/libVLC:
```
brew cask install vlc
```

**Some additional/optional steps:**

Enable developer mode to avoid annoying password requests when attaching the debugger to a process:
```
sudo /usr/sbin/DevToolsSecurity --enable
```
It makes me wonder who designed this functionality and what was their thinking, but a simple command is enough to not having to ponder this any further.

If required, define SDKROOT. This is only needed if during compilation you get error messages regarding missing include files. Running the following will properly setup the development environment paths:
```
export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
```

I suppose you should add this to your .bash_profile or similar, but I didn't have to do this step so I'm not sure.

**Cloning and compiling:**

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

To generate a debug build, run this instead:
```
cmake -DCMAKE_BUILD_TYPE=Debug .
make
```
Keep in mind though that a debug version will be much slower due to all compiler optimizations being disabled.

Running `make -j6` (or whatever number of parallel jobs you prefer) speeds up the compilation time if you have cores to spare.

After building ES and trying to execute the application, there could be issues with finding the dynamic link libraries for VLC as these are not installed into a standard location but rather into the /Applications folder. As such, you may need to set the DYLD_LIBRARY_PATH environmental variable to find the VLC libraries. Note that this is not intended or required for the release build that will be shipped in a DMG installer or if you manually install ES using 'make install'. It's only needed to be able to run the ES binary from the build directory. The following will of course only be active during your session, and you need to set the variable for each terminal window that you want to start ES from, unless you add it to your shell profile file:

```export DYLD_LIBRARY_PATH=/Applications/VLC.app/Contents/MacOS/lib```

**Note:** According to the SDL documentation, there could be issues with attempting to run ES from the build directory when using a High DPI display as the required NSHighResolutionCapable key is not set as there is no Info.plist file available. In this case, doing a 'make install' and running ES from the installation folder would solve the problem. I've been unable to verify if this is really required though as I don't have the necessary hardware to test it on.

**Installing:**

As macOS does not have any package manager which would have handled the library dependencies, we need to bundle the required shared libraries with the application. Copy the following .dylib files from their respective installation directories to the emulationstation-de build directory:

```
libSDL2-2.0.0.dylib
libcurl.4.dylib
libfreeimage.dylib
libfreetype.6.dylib
libvlc.dylib
libvlccore.dylib
```

Note that the filenames could be slightly different depending on what versions you have installed on your system.

In addition to these, you need to create a `plugins` directory and copy over the following VLC libraries, which are normally located in `/Applications/VLC.app/Contents/MacOS/plugins/`:

```
libaudio_format_plugin.dylib
libauhal_plugin.dylib
libavcodec_plugin.dylib
libconsole_logger_plugin.dylib
libfilesystem_plugin.dylib
libfreetype_plugin.dylib
libswscale_plugin.dylib
libtrivial_channel_mixer_plugin.dylib
libvmem_plugin.dylib
libwave_plugin.dylib
libx264_plugin.dylib
libx265_plugin.dylib
```

If you only want to build EmulationStation to be used on your own computer, there's the option to skip the entire bundling of the libraries and use the ones already installed using Homebrew, meaning you can skip the previous .dylib copying. To do so, run CMake with the following option:

```
cmake -DAPPLE_SKIP_INSTALL_LIBS=ON .
```

**Note:** This also affects the .dmg package generation using cpack, so if this options is enabled, the package will be unusable for everyone but yourself as the required libraries will not be bundled with the application.

On macOS you can then install the application as a normal user, i.e. no root privileges are required. Simply run the following to install the application to `/Applications/EmulationStation Desktop Edition.app`:

```
make install
```

This will be the directory structure for the installation:

```
/Applications/EmulationStation Desktop Edition.app/Contents/Info.plist
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/EmulationStation
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libSDL2-2.0.0.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libcurl.4.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libfreeimage.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libfreetype.6.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libvlc.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libvlccore.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/plugins/*
/Applications/EmulationStation Desktop Edition.app/Contents/Resources/EmulationStation-DE.icns
/Applications/EmulationStation Desktop Edition.app/Contents/Resources/LICENSE
/Applications/EmulationStation Desktop Edition.app/Contents/Resources/licenses/*
/Applications/EmulationStation Desktop Edition.app/Contents/Resources/resources/*
/Applications/EmulationStation Desktop Edition.app/Contents/Resources/themes/*
```

ES will look in the following locations for the resources, in the listed order:

* `[HOME]/.emulationstation/resources/`
* `[ES EXECUTABLE DIRECTORY]/../Resources/resources/`
* `[ES EXECUTABLE DIRECTORY]/resources/`

**Note:** The resources directory is critical, without it the application won't start.

And it will look in the following locations for the themes, also in the listed order:

* `[HOME]/.emulationstation/themes/`
* `[ES EXECUTABLE DIRECTORY]/../Resources/themes/`
* `[ES EXECUTABLE DIRECTORY]/themes/`

A theme is not mandatory to start the application, but ES will be basically useless without it.

The home directory will always take precedence, and any resources or themes located there will override the ones in the installation path or in the path of the ES executable.

**Creating a .dmg installer:**

Simply run `cpack` to build a .dmg disk image/installer:

```
myusername@computer:~/emulationstation-de$ cpack
CPack: Create package using Bundle
CPack: Install projects
CPack: - Run preinstall target for: emulationstation-de
CPack: - Install project: emulationstation-de []
CPack: Create package
CPack: - package: /Users/myusername/emulationstation-de/EmulationStation-DE-1.0.0.dmg generated.
```

**Special considerations regarding run-paths:**

Even after considerable effort I've been unable to make CMake natively set correct rpaths for the EmulationStation binary on macOS. Therefore a hack/workaround is in place that uses install_name_tool to change absolute paths to rpaths for most of the bundled libraries.

This is certainly not perfect as the versions of the libraries are hardcoded inside es-app/CMakeLists.txt. Therefore always check that all the rpaths are set correctly if you intend to create a .dmg image that will be used on other computers than your own.

Simply run `otool -L EmulationStation` and verify that the result looks something like this:

```
EmulationStation:
	@rpath/libcurl.4.dylib (compatibility version 7.0.0, current version 8.0.0)
	@rpath/libfreeimage.dylib (compatibility version 3.0.0, current version 3.18.0)
	@rpath/libfreetype.6.dylib (compatibility version 24.0.0, current version 24.2.0)
	@rpath/libSDL2-2.0.0.dylib (compatibility version 13.0.0, current version 13.0.0)
	/System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa (compatibility version 1.0.0, current version 22.0.0)
	@rpath/libvlc.dylib (compatibility version 12.0.0, current version 12.0.0)
	/System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL (compatibility version 1.0.0, current version 1.0.0)
	/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)
```

If any of the lines that should start with @rpath instead has an absolute path, then you have a problem and need to modify the install_name_tools parameters in es-app/CMakeLists.txt, for example:

`/usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib (compatibility version 13.0.0, current version 13.0.0)`

This is the section that would need to be modified:

```
add_custom_command(TARGET EmulationStation POST_BUILD COMMAND ${CMAKE_INSTALL_NAME_TOOL}
        -change /usr/lib/libcurl.4.dylib @rpath/libcurl.4.dylib
        -change /usr/local/opt/freeimage/lib/libfreeimage.dylib @rpath/libfreeimage.dylib
        -change /usr/local/opt/freetype/lib/libfreetype.6.dylib @rpath/libfreetype.6.dylib
        -change /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib @rpath/libSDL2-2.0.0.dylib
        $<TARGET_FILE:EmulationStation>)
```


## Building on Windows

This is a strange legacy operating system. However it's still popular, so we need to support it.

I did a brief evaluation of the Microsoft Visual C++ compiler (MSVC) but as far as I'm concerned it's an abomination so I won't cover it here and it won't be supported.

The OpenGL library shipped with Windows is unfortunately garbage, so the OpenGL Extension Wrangler (GLEW) library needs to be used as well to provide shader support. Its installation is explained below.

At the moment I have only built the software using GCC on Windows, but I aim to get Clang/LLVM working at a later date.

Anyway, here's a (not so) brief summary of how to get a build environment up and running on Windows.

**Install Git, CMake, MinGW and your code editor:**

[https://gitforwindows.org](https://gitforwindows.org)

[https://cmake.org/download](https://cmake.org/download)

[https://gnutoolchains.com/mingw64](https://gnutoolchains.com/mingw64)

Make a copy of `mingw64/bin/mingw32-make` to `make` just for convenience and make sure that the necessary paths are defined for the PATH environmental variable.

I won't get into the details on how to configure Git, but there are many resources available online to support with this. The `Git Bash` shell is very useful though as it's somewhat reproducing a Unix environment using MinGW/MSYS.

Install your editor of choice, I use [VSCode](https://code.visualstudio.com).

It's strongly recommended to set line breaks to Unix-style (line feed only) directly in the editor, although it can also be configured in Git for conversion during commit. The source code for EmulationStation-DE only uses Unix-style line breaks.

**Enable pretty printing for GDB:**

This is useful for displaying `std::string` values for example.

Adjust your paths accordingly, the below are just examples of course.

Save a file to
C:\Programming\mingw64\bin\pp.gdb with the following contents:

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

Note that most GDB builds for Windows have broken Python support so that pretty printing won't work. The MinGW installation recommended in the previous step should work fine though.

**Download the dependency packages:**

FreeImage\
[https://sourceforge.net/projects/freeimage](https://sourceforge.net/projects/freeimage)

cURL\
[https://curl.haxx.se/download.html](https://curl.haxx.se/download.html)

SDL2\
[https://www.libsdl.org/download-2.0.php](https://www.libsdl.org/download-2.0.php)

libVLC\
[https://ftp.lysator.liu.se/pub/videolan/vlc](https://ftp.lysator.liu.se/pub/videolan/vlc)

Uncompress the files for the above packages to a suitable directory, for example C:\Programming\Dependencies\

GLEW\
[http://glew.sourceforge.net](http://glew.sourceforge.net)

This library needs to be compiled from source as the pre-built libraries don't seem to work with GCC. The GitHub repo seems to be somewhat broken as well, therefore the manual download is required. It's recommended to get the source in zip format and uncompress it to the same directory as the other libraries listed above.

Now simply build the required glew32.dll library:

```
unzip glew-2.1.0.zip
cd glew-2.1.0
make
```
You will probably see a huge amount of compile warnings, and the glewinfo.exe tool may fail to build, but we don't need it so it's not an issue.

The following packages are not readily available for Windows either, so clone the repos and build them yourself:

[FreeType](https://www.freetype.org)
```
git clone git://git.savannah.gnu.org/freetype/freetype2.git
git checkout VER-2-10-2
mkdir build
cd build
cmake -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=ON ..
make
```

[pugixml](https://pugixml.org)
```
git clone git://github.com/zeux/pugixml
cd pugixml
git checkout v1.10
cmake -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=ON .
make
```

As for RapidJSON, you don't need to compile it, you just need the include files:

[RapidJSON](http://rapidjson.org)
```
git clone git://github.com/Tencent/rapidjson
cd rapidjson
git checkout v1.1.0
```

**Clone the EmulationStation-DE repository:**

This works the same as in Unix or macOS, just run the following:

```
git clone https://gitlab.com/leonstyhre/emulationstation-de
```

**Setup the include directories:**

As there is no standardized include directory structure in Windows, you need to provide the include files manually.

Make a directory in your build environment tree, for instance under C:\Programming\include\

Copy the include files from cURL, FreeImage, FreeType, GLEW, pugixml, RapidJSON, SDL2 and VLC to this directory.
It should then look something like this:

```
$ ls -1 include/
curl/
FreeImage.h
freetype/
ft2build.h
GL/
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
glew32.dll
libcrypto-1_1-x64.dll    (from the OpenSSL package, located in Git MinGW/MSYS under /mingw/bin/)
libcurl-x64.dll
libfreetype.dll
libgcc_s_seh-1.dll    (located in Git MinGW/MSYS under /mingw/bin/)
libpugixml.dll
libSDL2main.a
libssl-1_1-x64.dll    (from the OpenSSL package, located in Git MinGW under /mingw/bin/)
libstdc++-6.dll
libvlc.dll
libvlccore.dll
libwinpthread-1.dll    (located in Git MinGW under /mingw/bin/)
SDL2.dll
```

The files from the MinGW installation must correspond to the version used to compile the binary.

*So if the MinGW installation is upgraded to a newer version or so, make sure to copy the .dll files again, overwriting the old ones.*

In addition to these, you need to copy some libraries from the VLC `plugins` folder to be able to play video files. There is a subdirectory structure under the plugins folder and although there is no requirement to retain these as libVLC apparently looks recursively for the required .dll files, it still makes it a bit more tidy to keep the folder names for each type of plugin. The CMake install script will however copy all the contents of this plugins folder regardless of whether subdirectories are in use or not.

It's a bit tricky to know which libraries are really needed. But as the plugins directory is around 120 MB (as of VLC version 3.0.11), we definitely only want to copy the files we need.

The following files seem to be required to play most video and audio formats (place them in `emulationstation-de\plugins\`):

```
access\libfilesystem_plugin.dll
audio_filter\libaudio_format_plugin.dll
audio_filter\libtrivial_channel_mixer_plugin.dll
audio_output\libwaveout_plugin.dll
codec\libavcodec_plugin.dll
codec\libx264_plugin.dll
codec\libx265_plugin.dll
logger\libconsole_logger_plugin.dll
text_renderer\libfreetype_plugin.dll
video_chroma\libswscale_plugin.dll
video_output\libvmem_plugin.dll
```

The combined size of these files is around 24 MB which is more reasonable.

**Building the application:**

For a release build:

```
cmake -G "MinGW Makefiles" -DWIN32_INCLUDE_DIR=../include .
```

Or for a debug build:

```
cmake -G "MinGW Makefiles" -DWIN32_INCLUDE_DIR=../include -DCMAKE_BUILD_TYPE=Debug .
```

For some reason defining the '../include' path doesn't work when running CMake from PowerShell (and no, changing to backslash doesn't help). Instead use Bash, by running from a Git Bash shell.

The make command works fine directly in PowerShell though so it can be run from the VSCode terminal.

Running `make -j6` (or whatever number of parallel jobs you prefer) should now build the binary.

Note that compilation time is much longer than on Unix or macOS, and linking time is excessive for a debug build (around 10 times longer on my computer). The debug binary is also much larger than on Unix.

**Creating an NSIS installer:**

To create an NSIS installer (Nullsoft Scriptable Install System) you need to first install the NSIS creation tool:

NSIS\
[https://nsis.sourceforge.io/Download](https://nsis.sourceforge.io/Download)

Simply install the application using its installer.

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

The default installation directory suggested by the installer is `C:\Program Files\EmulationStation`. However this can of course be changed by the user.

ES will look in the following locations for the resources, in the listed order:

* `[HOME]\.emulationstation\resources\`
* `[ES EXECUTABLE DIRECTORY]\resources\`

**Note:** The resources directory is critical, without it the application won't start.

And it will look in the following locations for the themes, also in the listed order:

* `[HOME]\.emulationstation\themes\`
* `[ES EXECUTABLE DIRECTORY]\themes\`

The theme is not mandatory to start the application, but ES will be basically useless without it.

So the home directory will always take precedence, and any resources or themes located there will override the ones in the path of the ES executable.

**Setting up a portable installation:**

It's possible to easily create a portable installation of ES for Windows, for example on a USB memory stick.

For the sake of this example, let's assume that the removable media has the device name `f:\`.

* Copy the EmulationStation installation directory to f:\
* Create the directory `ES-Home` directly under f:\
* Copy your game ROMs into `f:\ES-Home\ROMs`
* Copy your emulators to f:\ (such as the RetroArch directory)
* Create the file `start_es.bat` directly under f:\

Add the following lines to the start_es.bat file:
```
@echo off
EmulationStation\emulationstation.exe --home %CD:~0,3%ES-Home
```

The contents of f:\ should now look something like this:
```
EmulationStation
ES-Home
RetroArch
start_es.bat

```

Now run the batch script, ES should start and ask you to configure any attached controllers. Following this, check that everything works as expected, i.e. the gamelists are properly populated etc.

You can optionally skip the configuration of the controllers by copying any existing `es_input.cfg` file to `f:\ES-Home\.emulationstation\es_input.cfg`.

Exit ES and modify the file `f:\ES-Home\.emulationstation\es_systems.cfg` to point to the emulators on the portable media.

Example change, from this:
```
<command>retroarch.exe -L "%EMUPATH%\cores\snes9x_libretro.dll" %ROM%</command>
```

To this:
```
<command>%ESPATH%\..\RetroArch\retroarch.exe -L "%EMUPATH%\cores\snes9x_libretro.dll" %ROM%</command>
```

The %ESPATH% variable is explained later in this document.

Following this, optionally copy any existing gamelists and game media files to the removable media. By default these files should be located here:

`f:\ES-Home\.emulationstation\gamelists\` \
`f:\ES-Home\.emulationstation\downloaded_media\`

You now have a fully functional portable emulator installation!

The portable installation works exactly as a normal installation, i.e. you can use the built-in scraper, edit metadata etc.


## Configuration

**~/.emulationstation/es_systems.cfg:**

EmulationStation Desktop Edition ships with a comprehensive `es_systems.cfg` configuration file, and as the logic is to use a `%ROMPATH%` variable to locate the ROM files (with a corresponding setting in `es_settings.cfg`), normally you shouldn't need to modify this file to the same extent as previous versions of EmulationStation. Still, see below in this document on how to adjust the es_systems.cfg file if required.

Upon first startup of the application, if there is no es_systems.cfg file present, it will be copied from the template subdirectory inside the resources directory. This directory is located in the installation path of the application, for instance `/usr/local/share/emulationstation/resources/templates` on Unix, `/Applications/EmulationStation.app/Contents/Resources/resources/templates` on macOS and `C:\Program Files\EmulationStation\resources\templates` on Windows.

The template file will be copied to `~/.emulationstation/es_systems.cfg`. \
`~` is `$HOME` on Unix and macOS, and `%HOMEPATH%` on Windows.

**Note:** Keep in mind that you have to set up your emulators separately from EmulationStation, the es_systems.cfg file assumes that your emulator environment is properly configured.

On Unix it's assumed that RetroArch is using the default configuration directory location, i.e. the cores should be located in ~/.config/retroarch/cores. If you've installed RetroArch via a Snap package, make a symlink from the Snap .config directory:

```
ln -s ~/snap/retroarch/current/.config/retroarch ~/.config/
```


**~/.emulationstation/es_settings.cfg:**

When ES is first run, a configuration file will be created as `~/.emulationstation/es_settings.cfg`.

This file contains all the settings supported by ES, at their default values. Normally you shouldn't need to modify this file manually, instead you should be able to use the menu inside ES to update all the necessary settings.

For the ROM directory, you can either change it manually in es_settings.cfg, or use the dialog that is shown on application startup to change the path to your liking.

**Setting the ROM directory in es_settings.cfg:**

**Note:** This complete configuration step can normally be skipped as you're presented with a dialog to change the ROM directory upon application startup if no game files are found.

By default, ES looks in `~/ROMs` for the ROM files, where they are expected to be grouped into directories corresponding to the game systems, for example:

```
myusername@computer:~ROMs$ ls -1
arcade
megadrive
pcengine
```

However, if you've saved your ROMs to another directory, you need to configure the ROMDirectory setting in es_settings.cfg.\
Here's an example:

`<string name="ROMDirectory" value="~/games/ROMs" />`

Keep in mind though that you still need to group the ROMs into directories corresponding to the \<path\> tags in es_systems.cfg.

There is also support to add the variable %ESPATH% to the ROM directory setting, this will expand to the path where the ES executable is started from. You would normally not need this, but the option is there, should you require it for some reason.

Here is such an example:

`<string name="ROMDirectory" value="%ESPATH%/../ROMs" />`

**~/.emulationstation/es_input.cfg:**

You normally don't need to modify this file manually as it's created by the built-in input configuration step. This procedure is detailed in the [User Guide](USERGUIDE.md#input-device-configuration).

If your controller and keyboard stop working, you can delete the `~/.emulationstation/es_input.cfg` file to make the input configuration screen re-appear on the next run.


## Command line arguments

You can use `--help` or `-h` to view a list of command line options, as shown here.

### Unix

```
--resolution [width] [height]   Try to force a particular resolution
--gamelist-only                 Skip automatic game ROM search, only read from gamelist.xml
--ignore-gamelist               Ignore the gamelist files (useful for troubleshooting)
--show-hidden-files             Show hidden files and folders
--show-hidden-games             Show hidden games
--no-exit                       Don't show the exit option in the menu
--no-splash                     Don't show the splash screen
--debug                         Print debug information
--windowed                      Windowed mode, should be combined with --resolution
--fullscreen-normal             Normal fullscreen mode
--fullscreen-borderless         Borderless fullscreen mode (always on top)
--vsync [1/on or 0/off]         Turn vsync on or off (default is on)
--max-vram [size]               Max VRAM to use (in mebibytes) before swapping
--gpu-statistics                Display framerate and VRAM usage overlay
--force-full                    Force the UI mode to Full
--force-kid                     Force the UI mode to Kid
--force-kiosk                   Force the UI mode to Kiosk
--force-input-config            Force configuration of input device
--home [path]                   Directory to use as home path
--version, -v                   Displays version information
--help, -h                      Summon a sentient, angry tuba
```

### macOS

```
--resolution [width] [height]   Try to force a particular resolution
--gamelist-only                 Skip automatic game ROM search, only read from gamelist.xml
--ignore-gamelist               Ignore the gamelist files (useful for troubleshooting)
--show-hidden-files             Show hidden files and folders
--show-hidden-games             Show hidden games
--no-exit                       Don't show the exit option in the menu
--no-splash                     Don't show the splash screen
--debug                         Print debug information
--vsync [1/on or 0/off]         Turn vsync on or off (default is on)
--max-vram [size]               Max VRAM to use (in mebibytes) before swapping
--gpu-statistics                Display framerate and VRAM usage overlay
--force-full                    Force the UI mode to Full
--force-kid                     Force the UI mode to Kid
--force-kiosk                   Force the UI mode to Kiosk
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
--no-exit                       Don't show the exit option in the menu
--no-splash                     Don't show the splash screen
--debug                         Print debug information
--vsync [1/on or 0/off]         Turn vsync on or off (default is on)
--max-vram [size]               Max VRAM to use (in mebibytes) before swapping
--gpu-statistics                Display framerate and VRAM usage overlay
--force-full                    Force the UI mode to Full
--force-kid                     Force the UI mode to Kid
--force-kiosk                   Force the UI mode to Kiosk
--force-input-config            Force configuration of input device
--home [path]                   Directory to use as home path
--version, -v                   Displays version information
--help, -h                      Summon a sentient, angry tuba
```

As long as ES hasn't frozen, you can always press F4 to close the application.

As you can see above, you can override the home directory path using the `--home` flag. So by running for instance the command `emulationstation --home ~/games/emulation`, ES will use `~/games/emulation/.emulationstation` as its base directory.


## es_systems.cfg

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
        This example for Unix would run RetroArch with the the snes9x_libretro core.
        If there are spaces in the path or file name, you must enclose them in quotation marks, for example:
        retroarch -L "~/my configs/retroarch/cores/snes9x_libretro.so" %ROM% -->
        <command>retroarch -L ~/.config/retroarch/cores/snes9x_libretro.so %ROM%</command>

        <!-- This is an example for macOS. It uses the %EMUPATH% variable to point to the RetroArch cores relative to the emulator binary.
        As there is a somehow standardized installation structure for this operating system, an absolute path is defined for the emulator. -->
        <command>/Applications/RetroArch.app/Contents/MacOS/RetroArch -L %EMUPATH%/../Resources/cores/snes9x_libretro.dylib %ROM%</command>

        <!-- This is an example for Windows. The .exe extension is optional and both forward slashes and backslashes are allowed as
        directory separators. As there is no standardized installation directory structure for this operating system, the %EMUPATH%
        variable is used here to find the cores relative to the RetroArch binary. The emulator binary must be in the PATH environmental variable
        or otherwise the complete path to the retroarch.exe file needs to be defined. Batch scripts (.bat) are also supported. -->
        <command>retroarch.exe -L %EMUPATH%\cores\snes9x_libretro.dll %ROM%</command>

        <!-- Another example for Windows. As can be seen here, the absolute path to the emulator has been defined, and there are spaces
        in the directory name, so it needs to be surrounded by quotation marks. As well the quotation marks are needed around the core
        configuration as the %EMUPATH% will expand to the path of the emulator binary, which will of course also include the spaces. -->
        <command>"C:\My Games\RetroArch\retroarch.exe" -L "%EMUPATH%\cores\snes9x_libretro.dll" %ROM%</command>

        <!-- An example for use in a portable emulator installation, for instance on a USB memory stick. The %ESPATH% variable is
        expanded to the directory of the ES executable. -->
        <command>"%ESPATH%\..\RetroArch\retroarch.exe" -L "%ESPATH%\..\RetroArch\cores\snes9x_libretro.dll" %ROM%</command>

        <!-- The platform(s) to use when scraping. You can see the full list of supported platforms in src/PlatformIds.cpp.
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

`%ESPATH%` - Replaced with the path to the EmulationStation binary. Mostly useful for portable emulator installations, for example on a USB memory stick.

For the `path` tag, the following variable is expanded by ES:

`%ROMPATH%` - Replaced with the path defined for the setting ROMDirectory in `es_settings.cfg`.


## gamelist.xml

The gamelist.xml file for a system defines metadata for games, such as a name, description, release date, and rating.

As of the fork to EmulationStation Desktop Edition, game media information no longer needs to be defined in the gamelist.xml files. Instead the application will look for any media matching the ROM filename. The media path where to look for game art is configurable either manually in `es_settings.cfg` or via the GUI. If configured manually in es_settings.cfg, it looks something like this:

`<string name="MediaDirectory" value="~/games/images/emulationstation" />`

The default game media directory is `~/.emulationstation/downloaded_media`.

You can use ES's [scraping](http://en.wikipedia.org/wiki/Web_scraping) tools to populate the gamelist.xml files. There are two ways to run the scraper:

* **If you want to scrape multiple games:** press the Start button to open the menu and choose the "SCRAPER" option.  Adjust your settings and press "START".
* **If you just want to scrape one game:** find the game on the game list in ES and press the Select button.  Choose "EDIT THIS GAME'S METADATA" and then press the "SCRAPE" button at the bottom of the metadata editor.

You can also edit metadata within ES by using the metadata editor - just find the game you wish to edit on the gamelist, press Select, and choose "EDIT THIS GAME'S METADATA."

The switch `--ignore-gamelist` can be used to ignore the gamelists upon start of the application.

**File location and structure:**

ES checks for the `gamelist.xml` files in the user's home directory:

`~/.emulationstation/gamelists/[SYSTEM_NAME]/gamelist.xml`

The home directory can be overridden using the `--home` option from the command line, see above in this document for details regarding that.

An example gamelist.xml:

```xml
<gameList>
	<game>
		<path>./mm2.nes</path>
		<name>Mega Man 2</name>
		<desc>Mega Man 2 is a classic NES game which follows Mega Man as he murders eight robot masters in cold blood.</desc>
	</game>
</gameList>
```

Everything is enclosed in a `<gameList>` tag.  The information for each game or folder is enclosed in a corresponding tag (`<game>` or `<folder>`).  Each piece of metadata is encoded as a string.

**gamelist.xml reference:**

There are a few types of metadata:

* `string` - just text.
* `float` - a floating-point decimal value (written as a string).
* `integer` - an integer value (written as a string).
* `datetime` - a date and, potentially, a time.  These are encoded as an ISO string, in the following format: "%Y%m%dT%H%M%S%F%q".  For example, the release date for Chrono Trigger is encoded as "19950311T000000" (no time specified).

Some metadata is also marked as "statistic" - these are kept track of by ES and do not show up in the metadata editor. They are shown in certain views (for example, the detailed view and the video view both show `lastplayed`, although the label can be disabled by the theme).

**\<game\>**

* `name` - string, the displayed name for the game.
* `desc` - string, a description of the game.  Longer descriptions will automatically scroll, so don't worry about size.
* `rating` - float, the rating for the game, expressed as a floating point number between 0 and 1. ES will round fractional values to half-stars.
* `releasedate` - datetime, the date the game was released.  Displayed as date only, time is ignored.
* `developer` - string, the developer for the game.
* `publisher` - string, the publisher for the game.
* `genre` - string, the (primary) genre for the game.
* `players` - integer, the number of players the game supports.
* `favorite` - bool, indicates whether the game is a favorite.
* `completed`- bool, indicates whether the game has been completed.
* `broken` - bool, indicates a game that doesn't work (useful for MAME).
* `kidgame` - bool, indicates whether the game is suitable for children, used by the `kid' UI mode.
* `playcount` - integer, the number of times this game has been played.
* `lastplayed` - statistic, datetime, the last date and time this game was played.
* `sortname` - string, used in sorting the gamelist in a system, instead of `name`.
* `launchcommand` - optional tag that is used to override the emulator and core settings on a per-game basis.

**\<folder\>**
* `name` - string, the displayed name for the folder.
* `desc` - string, the description for the folder.
* `developer` - string, developer(s).
* `publisher` - string, publisher(s).
* `genre` - string, genre(s).
* `players` - integer, the number of players the game supports.


**Additional gamelist.xml information:**

* If a value matches the default for a particular piece of metadata, ES will not write it to the gamelist.xml (for example, if `genre` isn't specified, ES won't write an empty genre tag)

* A `game` can actually point to a folder/directory if the folder has a matching extension

* `folder` metadata will only be used if a game is found inside of that folder

* ES will keep entries for games and folders that it can't find the game/ROM files for

* The switch `--gamelist-only` can be used to skip automatic searching, and only display games defined in the system's gamelist.xml

* The switch `--ignore-gamelist` can be used to ignore the gamelist upon start of the application


## Custom event scripts

There are numerous locations throughout ES where custom scripts will be executed if the option to do so has been enabled in the settings. You'll find the option on the Main menu under **Other settings**. By default it's deactivated so be sure to enable it to use this feature.

The approach is quite straightforward, ES will look for any files inside a script directory that correspond to the event that is triggered and execute all files located there.

As an example, let's create a log that will record the start and end time for each game we play, letting us see how much time we spend on retro-gaming.

**Note: The following example is for Unix systems, but it works the same in Windows (though .bat batch files are then used instead of shell scripts).**

The events when the game starts and ends are called **game-start** and **game-end** respectively. Finding these event names is easily achieved by starting ES with the **--debug** flag. If this is done, all attempts to execute custom event scripts will be logged to es_log.txt, including the event names.

So let's create the folders for these events in the scripts directory. The location of this directory is **~/.emulationstation/scripts/**.

After creating the directories, we need to create the scripts to log the actual game launch and game ending. The parameters that are passed to the scripts varies depending on the type of event, but for these events the two parameters are the absolute path to the game file, and the game name as shown in the gamelist.

Let's name the start script **game_start_logging.sh** with the following contents:

```
#!/bin/bash
TIMESTAMP=$(date +%Y-%m-%d' '%H:%M:%S)
echo Starting game "\""${2}"\"" "(\""${1}"\")" at $TIMESTAMP >> ~/.emulationstation/game_playlog.txt
```

And let's name the end script **game_end_logging.sh** with the following contents:

```
#!/bin/bash
TIMESTAMP=$(date +%Y-%m-%d' '%H:%M:%S)
echo "Ending game  " "\""${2}"\"" "(\""${1}"\")" at $TIMESTAMP >> ~/.emulationstation/game_playlog.txt
```

After creating the two scripts, you should have something like this on the filesystem:

```
~/.emulationstation/scripts/game-start/game_start_logging.sh
~/.emulationstation/scripts/game-end/game_end_logging.sh
```

Don't forget to make the scripts executable (e.g. 'chmod 755 ./game_start_logging.sh').

If we now start ES with the debug flag and launch a game, something like the following should show up in es_log.txt:

```
Aug 05 14:19:24 Debug:  Scripting::fireEvent(): game-start "/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip" "The Legend Of Zelda"
Aug 05 14:19:24 Debug:  Executing: /home/myusername/.emulationstation/scripts/game-start/game_start_logging.sh "/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip" "The Legend Of Zelda"
.
.
Aug 05 14:27:15 Debug:  Scripting::fireEvent(): game-end "/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip" "The Legend Of Zelda"
Aug 05 14:27:15 Debug:  Executing: /home/myusername/.emulationstation/scripts/game-end/game_end_logging.sh "/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip" "The Legend Of Zelda"

```

Finally after running some games, ~/.emulationstation/game_playlog.txt should contain something like the following:

```
Starting game "The Legend Of Zelda" ("/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip") at 2020-08-05 14:19:24
Ending game   "The Legend Of Zelda" ("/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip") at 2020-08-05 14:27:15
Starting game "Quake" ("/home/myusername/ROMs/ports/Quakespasm/quakespasm.sh") at 2020-08-05 14:38:46
Ending game   "Quake" ("/home/myusername/ROMs/ports/Quakespasm/quakespasm.sh") at 2020-08-05 15:13:58
Starting game "Pirates!" ("/home/myusername/ROMs/c64/Multidisk/Pirates/Pirates!.m3u") at 2020-08-05 15:15:24
Ending game   "Pirates!" ("/home/myusername/ROMs/c64/Multidisk/Pirates/Pirates!.m3u") at 2020-08-05 15:17:11
```
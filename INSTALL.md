# EmulationStation Desktop Edition (ES-DE) v2.0 - Building and advanced configuration

Table of contents:

[[_TOC_]]

## Development environment

ES-DE is developed and compiled using Clang/LLVM and GCC on Unix, Clang/LLVM on macOS and MSVC and GCC (MinGW) on Windows.

CMake is the build system for all the supported operating systems, used in conjunction with `make` on Unix and macOS and `nmake` and `make` on Windows. Xcode on macOS or Visual Studio on Windows are not required for building ES-DE and they have not been used during the development. The only exception is notarization of codesigned macOS packages which require the `altool` and `stapler` binaries that come bundled with Xcode.

For automatic code formatting [clang-format](https://clang.llvm.org/docs/ClangFormat.html) is used.

Any code editor can be used of course, but I recommend [VSCode](https://code.visualstudio.com).

## Building on Unix

There are some dependencies that need to be fulfilled in order to build ES-DE. These are detailed per operating system below.

**Debian/Ubuntu**

All of the required packages can be installed with apt-get:
```
sudo apt-get install build-essential clang-format git cmake libsdl2-dev libavcodec-dev libavfilter-dev libavformat-dev libavutil-dev libfreeimage-dev libfreetype6-dev libcurl4-openssl-dev libpugixml-dev libasound2-dev libgl1-mesa-dev
```

**Fedora**

On Fedora you first need to install the RPM Fusion repository:

```
sudo dnf install \
https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm \
https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm
```

Then you can use dnf to install all the required packages:
```
sudo dnf install gcc-c++ clang-tools-extra cmake libasan rpm-build SDL2-devel ffmpeg-devel freeimage-devel freetype-devel curl-devel pugixml-devel alsa-lib-devel mesa-libGL-devel
```

**Manjaro**

Use pacman to install all the required packages:

```
sudo pacman -S gcc clang make cmake pkgconf sdl2 ffmpeg freeimage freetype2 pugixml
```

**Raspberry Pi OS (Raspian)**

All of the required packages can be installed with apt-get:
```
sudo apt-get install clang-format cmake libsdl2-dev libavcodec-dev libavfilter-dev libavformat-dev libavutil-dev libfreeimage-dev libcurl4-gnutls-dev libpugixml-dev
```

To build with CEC support you also need to install these packages:
```
sudo apt-get install libcec-dev libp8-platform-dev
```

The Raspberry Pi 4/400 is the minimum recommended version and earlier boards have not been tested. The GPU memory should be set to at least 256 MB using `raspi-config` and the GL driver must be set to `GL (Fake KMS)` or the performance will be horrible.

Note that low-level ALSA sound support has been removed from ES-DE which means that a sound server like PulseAudio or PipeWire is required. Likewise a display server (Xorg or Wayland) is required, direct framebuffer access is not supported.

Only the OpenGL ES 3.0 renderer works on Raspberry Pi and it's enabled by default.

**FreeBSD**

Use pkg to install the dependencies:
```
pkg install llvm-devel git pkgconf cmake sdl2 ffmpeg freeimage pugixml
```

Clang/LLVM and curl should already be included in the base OS installation.

**NetBSD**

Use pkgin to install the dependencies:
```
pkgin install clang git cmake pkgconf SDL2 ffmpeg4 freeimage pugixml
```

NetBSD ships with GCC by default, and although you should be able to use Clang/LLVM, it's probably easier to just stick to the default compiler environment. The reason why the clang package needs to be installed is to get clang-format onto the system.

**OpenBSD**

Use pkg_add to install the dependencies:
```
pkg_add clang-tools-extra cmake pkgconf sdl2 ffmpeg freeimage
```

In the same manner as for FreeBSD, Clang/LLVM and curl should already be installed by default.

Pugixml does exist in the package collection but somehow this version is not properly detected by CMake, so you need to compile this manually as well:

```
git clone https://github.com/zeux/pugixml.git
cd pugixml
git checkout v1.10
cmake .
make
make install
```

**Cloning and compiling ES-DE**

To clone the source repository, run the following:

```
git clone https://gitlab.com/es-de/emulationstation-de.git
```

Then generate the Makefile and build the software:

```
cd emulationstation-de
cmake .
make
```

By default the application updater will be built which checks for new releases on startup, to disable this functionality run the following:
```
cmake -DAPPLICATION_UPDATER=off .
make
```

Note that the application updater is always disabled when building for the AUR, RetroDECK, Raspberry Pi or BSD Unix.

By default the master branch will be used, which is where development takes place. To instead build a stable release, switch to the `stable-x.x` branch for the version, for example:

```
cd emulationstation-de
git checkout stable-1.2
cmake .
make
```

To create a debug build, run this:
```
cmake -DCMAKE_BUILD_TYPE=Debug .
make
```
Keep in mind that a debug version will be much slower due to all compiler optimizations being disabled.

To create a profiling build (optimized with debug symbols), run this:
```
cmake -DCMAKE_BUILD_TYPE=Profiling .
make
```

To enable AddressSanitizer which helps with identifying memory issues like corruption bugs and buffer overflows, build with the ASAN option:
```
cmake -DCMAKE_BUILD_TYPE=Debug -DASAN=on .
make
```
Due to buggy AMD GPU drivers it could be a good idea to use the `LSAN_suppressions` file included in the repository to avoid reports of a lot of irrelevant issue, for example:
```
LSAN_OPTIONS="suppressions=tools/LSAN_suppressions" ./emulationstation --debug --resolution 2560 1440
```

This applies to LeakSanitizer specifically, which is integrated into AddressSanitizer.

To enable ThreadSanitizer which helps with identifying data races and other thread-related issues, build with the TSAN option:
```
cmake -DCMAKE_BUILD_TYPE=Debug -DTSAN=on .
make
```

It could also be a good idea to use the `TSAN_suppressions` file included in the repository to suppress issues reported by some third party libraries, for example:
```
TSAN_OPTIONS="suppressions=tools/TSAN_suppressions" ./emulationstation --debug --resolution 2560 1440
```

To enable UndefinedBehaviorSanitizer which helps with identifying bugs that may otherwise be hard to find, build with the UBSAN option:
```
cmake -DCMAKE_BUILD_TYPE=Debug -UBSAN=on .
make
```

To get stack traces printed as well, set this environment variable:
```
export UBSAN_OPTIONS=print_stacktrace=1
```

These tools aren't very useful without debug symbols so only use them for a Debug or Profiling build. Clang and GCC support all three tools. Note that ASAN and TSAN can't be combined.

As for advanced debugging, Valgrind is a very powerful and useful tool which can analyze many aspects of the application. Be aware that some of the Valgrind tools should be run with an optimized build, and some with optimizations turned off. Refer to the Valgrind documentation for more information.

The most common tool is Memcheck to check for memory leaks, which you run like this:

```
valgrind --tool=memcheck --leak-check=full --log-file=../valgrind_run_01 ./emulationstation
```

There are numerous flags that can be used, for example this will also track reachable memory which could indicate further leaks:
```
valgrind --tool=memcheck --leak-check=full --track-origins=yes --show-reachable=yes --log-file=../valgrind_run_01 ./emulationstation
```

Another helpful tool is the Callgrind call-graph analyzer:
```
valgrind --tool=callgrind --log-file=../valgrind_run_01 ./emulationstation
```

The output file can be loaded into an application such as KCachegrind for data analysis.



Yet another very useful Valgrind tool is the Massif heap profiler, which can be run like this:
```
valgrind --tool=massif --massif-out-file=../massif.out.01 ./emulationstation
```

The output file can be loaded into an application such as Massif-Visualizer for analysis.

Another useful tool is `scan-build`, assuming you use Clang/LLVM. This is a static analyzer that runs during compilation to provide a very helpful HTML report of potential bugs (well it should be actual bugs but some false positives could be included). You need to run it for both the cmake and make steps, here's an example:

```
scan-build cmake -DCMAKE_BUILD_TYPE=Debug .
scan-build make -j6
```

You open the report with the `scan-view` command which lets you read it using your web browser. Note that the compilation time is much longer when using the static analyzer compared to a normal build. As well this tool generates a lot of extra files and folders in the build tree, so it may make sense to run it in a separate copy of the source folder to avoid having to clean up all this extra data when the analysis has been completed.

An even more advanced static analyzer is `clang-tidy`, to use it first make sure it's installed on your system and then run the following:
```
cmake -DCLANG_TIDY=on .
```

Even though many irrelevant checks are filtered out via the configuration, this will still likely produce a quite huge report (at least until most of the recommendations have been implemented). In the same manner as for scan-view, the compilation time is much longer when using this static analyzer compared to a normal build.

To build ES-DE with experimental FFmpeg video hardware decoding support, enable the following option:

```
cmake -DVIDEO_HW_DECODING=on .
make
```

To build ES-DE with CEC support, enable the corresponding option, for example:

```
cmake -DCEC=on .
make
```
You will most likely need to install additional packages to get this to build. On Debian-based systems these are _libcec-dev_ and _libp8-platform-dev_. Note that the CEC support is currently untested.

To build with the GLES 3.0 renderer, run the following:
```
cmake -DGLES=on .
make
```

This renderer is generally only needed on the Raspberry Pi and the desktop OpenGL renderer should otherwise be used.

Running multiple compile jobs in parallel is a good thing as it speeds up the build time a lot (scaling almost linearly). Here's an example telling make to run 6 parallel jobs:

```
make -j6
```

By default ES-DE will install under /usr on Linux, /usr/pkg on NetBSD and /usr/local on FreeBSD and OpenBSD although this can be changed by setting the `CMAKE_INSTALL_PREFIX` variable.

The following example will build the application for installtion under /opt:

```
cmake -DCMAKE_INSTALL_PREFIX=/opt .
```

It's important to understand that this is not only the directory used by the install script, the CMAKE_INSTALL_PREFIX variable also modifies code inside ES-DE used to locate the required program resources. So it's necessary that the install prefix corresponds to the location where the application will actually be installed.

On Linux, if you're not building a package and instead intend to install using `make install` it's recommended to set the installation prefix to /usr/local instead of /usr.

**Compilers**

Both Clang/LLVM and GCC work fine for building ES-DE, and on Ubuntu it's easy to switch between the two using `update-alternatives`:

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

**Installing**

Installing the software requires root permissions, the following command will install all the required application files:

```
sudo make install
```

Assuming the default installation prefix /usr has been used, this is the directory structure for the installation:

```
/usr/bin/emulationstation
/usr/share/man/man6/emulationstation.6.gz
/usr/share/applications/emulationstation.desktop
/usr/share/emulationstation/LICENSE
/usr/share/emulationstation/licenses/*
/usr/share/emulationstation/resources/*
/usr/share/emulationstation/themes/*
/usr/share/pixmaps/emulationstation.svg
```

However, when installing manually instead of building a package, it's recommended to change the install prefix to /usr/local instead of /usr.

Be aware that if using the GNOME desktop environment, /usr/share/pixmaps/emulationstation.svg must exist in order for the ES-DE icon to be shown in the Dash and task switcher.

ES-DE will look in the following locations for application resources, in the listed order:

* \<home\>/.emulationstation/resources/
* \<install prefix\>/share/emulationstation/resources/
* \<ES-DE executable directory\>/resources/

The resources directory is critical, without it the application won't start.

As well the following locations will be searched for themes, also in the listed order:

* \<home\>/.emulationstation/themes/
* \<install prefix\>/share/emulationstation/themes/
* \<ES-DE executable directory\>/themes/

A theme is not mandatory to start the application, but ES-DE will be basically useless without it.

As indicated above, the home directory will always take precedence and any resources or themes located there will override the ones in the installation path, or in the path of the ES-DE executable.

**Creating .deb and .rpm packages**

Creation of Debian .deb packages is enabled by default, simply run `cpack` to generate the package:

```
myusername@computer:~/emulationstation-de$ cpack
CPack: Create package using DEB
CPack: Install projects
CPack: - Run preinstall target for: emulationstation-de
CPack: - Install project: emulationstation-de []
CPack: Create package
CPackDeb: - Generating dependency list
CPack: - package: /home/myusername/emulationstation-de/emulationstation-de-1.2.0-x64.deb generated.
```

You may want to check that the dependencies look fine, as they're (mostly) automatically generated by CMake:

```
dpkg -I ./emulationstation-de-1.2.0-x64.deb
```

The package can now be installed using a package manager, for example apt:

```
sudo apt install ./emulationstation-de-1.2.0-x64.deb
```

To build an RPM package instead, set the flag LINUX_CPACK_GENERATOR to RPM when running cmake, for example:

```
cmake -DLINUX_CPACK_GENERATOR=RPM .
```

Then simply run `cpack`:

```
myusername@computer:~/emulationstation-de$ cpack
CPack: Create package using RPM
CPack: Install projects
CPack: - Run preinstall target for: emulationstation-de
CPack: - Install project: emulationstation-de []
CPack: Create package
CPackRPM: Will use GENERATED spec file: /home/myusername/emulationstation-de/_CPack_Packages/Linux/RPM/SPECS/emulationstation-de.spec
CPack: - package: /home/myusername/emulationstation-de/emulationstation-de-1.2.0-x64.rpm generated.
```

On Fedora, you need to install rpmbuild before this command can be run:
```
sudo dnf install rpm-build
```

After the package generation you can check that the metadata looks fine using the `rpm` command:
```
rpm -qi ./emulationstation-de-1.2.0-x64.rpm
```

To see the automatically generated dependencies, run this:
```
rpm -q --requires ./emulationstation-de-1.2.0-x64.rpm
```

And of course, you can also install the package:
```
sudo dnf install ./emulationstation-de-1.2.0-x64.rpm
```

**Creating an AppImage**

The process to create a Linux AppImage is completely automated. You simply run the AppImage creation script, which has to be executed from the root of the repository:

```
tools/create_AppImage.sh
```

This script has only been tested on Ubuntu (20.04 LTS and 22.04 LTS) and it's recommended to go for an older operating system when building the AppImage to achieve compatibility with a large number of distributions. This does come with some sacrifices though, such as the use of an older SDL version which may not support the latest game controllers.

The script will delete CMakeCache.txt and run cmake with the BUNDLED_CERTS option, as otherwise scraping wouldn't work on Fedora (and probably not on openSUSE and a few other distributions as well).

After creating the AppImage it's recommended to delete CMakeCache.txt manually so the BUNDLED_CERTS option is not accidentally enabled when building the other packages.

To build the Steam Deck-specific AppImage, run the following:
```
tools/create_AppImage_SteamDeck.sh
```

This is similar to the regular AppImage but does not build with the BUNDLED_CERTS option and changes some settings like the VRAM limit.

Both _appimagetool_ and _linuxdeploy_ are required for the build process but they will be downloaded automatically by the script if they don't exist. So to force an update to the latest build tools, delete these two AppImages prior to running the build script.

## Building on macOS

ES-DE for macOS is built using Clang/LLVM which is the default compiler for this operating system. It's pretty straightforward to build software on this OS. The main problem is that there is no native package manager, but as there are several third party package managers available, this can be partly compensated for. The use of one of them, [Homebrew](https://brew.sh), is detailed below.

**Setting up the build tools**

Install the Command Line Tools which include Clang/LLVM, Git, make etc. Simply open a terminal and enter the command `clang`. This will open a dialog that will let you download and install the tools.

Following this, install the Homebrew package manager which will simplify the installation of some additional required packages. Run the following in a terminal window:
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

If running on an M1 Mac, you also need to add the following to your `~/.zshrc` shell profile file:
```
export PATH=/opt/homebrew/bin:$PATH
```

**Package installation with Homebrew**

Install the required tools:

```
brew install clang-format cmake pkg-config nasm yasm
```

**Developer mode**

Enable developer mode to avoid annoying password requests when attaching the debugger to a process:
```
sudo /usr/sbin/DevToolsSecurity --enable
```

**Cloning and compiling**

To clone the source repository, run the following:

```
git clone https://gitlab.com/es-de/emulationstation-de.git
```

On macOS all dependencies are built in-tree in the `external` directory tree. There are two scripts in the tools directory that automate this entirely and they are executed such as this:

```
cd emulationstation-de
tools/macOS_dependencies_setup.sh
tools/macOS_dependencies_build.sh
```
This can take quite a while as multiple packages are downloaded and compiled. It's important to not have any of the dependency libraries installed using Homebrew as they will interfere with the in-tree build.

Re-running macOS_dependencies_setup.sh will delete and download all dependencies again, and re-running macOS_dependencies_build.sh will clean and rebuild all packages. If you want to recompile a single package, make sure to first set the MACOSX_DEPLOYMENT_TARGET variable:
```
export MACOSX_DEPLOYMENT_TARGET=10.14
```

Then manually recompile the package, for example:
```
cd external/pugixml
rm CMakeCache.txt
cmake .
make clean
make -j4
cp libpugixml.a ../..
```

After all dependencies have been built, generate the Makefile and build ES-DE:

```
cmake .
make
```

By default the master branch will be used, which is where development takes place. To instead build a stable release, switch to the `stable-x.x` branch for the version, for example:

```
cd emulationstation-de
git checkout stable-1.2
cmake .
make
```

To generate a debug build, run this:
```
cmake -DCMAKE_BUILD_TYPE=Debug .
make
```
Keep in mind that the debug version will be much slower due to all compiler optimizations being disabled.

To enable AddressSanitizer which helps with identifying memory issues like corruption bugs and buffer overflows, build with the ASAN option:
```
cmake -DCMAKE_BUILD_TYPE=Debug -DASAN=on .
make
```

To enable ThreadSanitizer which helps with identifying data races for multi-threaded code, build with the TSAN option:
```
cmake -DCMAKE_BUILD_TYPE=Debug -DTSAN=on .
make
```

To enable UndefinedBehaviorSanitizer which helps with identifying bugs that may otherwise be hard to find, build with the UBSAN option:
```
cmake -DCMAKE_BUILD_TYPE=Debug -UBSAN=on .
make
```

To get stack traces printed as well, set this environment variable:
```
export UBSAN_OPTIONS=print_stacktrace=1
```

These tools aren't very useful without debug symbols so only use them for a Debug or Profiling build. Note that ASAN and TSAN can't be combined.

Specifically on macOS it seems as if AddressSanitizer generates a lot of false positives regarding container overflows, so it may be necessary to ignore these:
```
export ASAN_OPTIONS=detect_container_overflow=0
```

Running `make -j6` (or whatever number of parallel jobs you prefer) speeds up the compilation time if you have cores to spare.

Running ES-DE from the build directory may be a bit flaky as there is no Info.plist file available which is required for setting the proper window mode and such. It's therefore recommended to run the application from the installation directory for any more in-depth testing. But normal debugging can of course be done from the build directory.

**Building for the M1 (ARM) processor**

The build steps detailed above should in theory work identically on an M1 processor but possibly some of the dependencies will not build correctly and may need manual patching. Cross-compiling using an Intel processor has been attempted but failed due to multiple issues with dependencies refusing to build.

**Code signing**

Due to the Apple notarization requirement implemented as of macOS 10.14.5 a build with simple code signing is needed for versions up to 10.13 and another build with both code signing and notarization is required for version 10.14 and higher.

macOS code signing is beyond the scope of this document, but the CMake option MACOS_CODESIGN_IDENTITY is used to specify the code signing certificate identity, for example:
```
cmake -DMACOS_CODESIGN_IDENTITY="My Name" .
```

Assuming the code signing ceritificate is properly setup in Keychain Access, the process will be automatic and the resulting DMG package can be notarized as-is. For some reason code signing fails if run via an SSH session, so in order for the cpack command to succeed it needs to run from a terminal window started via the GUI.

**Legacy build**

Normally ES-DE is meant to be built for macOS 10.14 and higher, but a legacy build for earlier operating system versions can be enabled. This has been tested with a minimum version of 10.11. It's unclear if it works with even older macOS releases.

To enable a legacy build, change the CMAKE_OSX_DEPLOYMENT_TARGET variable in CMakeLists.txt from 10.14 to whatever version you would like to build for. This will disable Hardened Runtime if signing is enabled and it will add "legacy" to the DMG installer filename when running CPack. It will also enable the bundled TLS/SSL certificates. As these older macOS releases are no longer receiving patches from Apple, certificates have likely expired meaning the scraper would not work if the bundled certificates were not used.

You also need to modify es-app/assets/EmulationStation-DE_Info.plist and set the key SMinimumSystemVersion to the version you're building for. And finally CMAKE_OSX_DEPLOYMENT_TARGET needs to be updated in tools/macOS_dependencies_build.sh. This script then needs to be executed to rebuild all dependencies for the configured macOS version.

**Installing**

As macOS does not have any package manager which would have handled the library dependencies, we need to bundle the required shared libraries with the application. This is almost completely automated by the build scripts.

You can install the application as a normal user, i.e. no root privileges are required. Simply run the following:

```
make install
```

This will be the directory structure for the installation:

```
/Applications/EmulationStation Desktop Edition.app/Contents/Info.plist
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/EmulationStation
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libSDL2-2.0.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libavcodec.58.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libavfilter.7.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libavformat.58.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libavutil.56.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libfdk-aac.2.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libfreetype.6.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libpostproc.55.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libswresample.3.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libswscale.5.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libvorbis.0.4.9.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libvorbisenc.2.0.12.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/plugins/*
/Applications/EmulationStation Desktop Edition.app/Contents/Resources/EmulationStation-DE.icns
/Applications/EmulationStation Desktop Edition.app/Contents/Resources/LICENSE
/Applications/EmulationStation Desktop Edition.app/Contents/Resources/licenses/*
/Applications/EmulationStation Desktop Edition.app/Contents/Resources/resources/*
/Applications/EmulationStation Desktop Edition.app/Contents/Resources/themes/*
```

ES-DE will look in the following locations for application resources, in the listed order:

* \<home\>/.emulationstation/resources/
* \<ES-DE executable directory\>/../Resources/resources/
* \<ES-DE executable directory\>/resources/

The resources directory is critical, without it the application won't start.

As well the following locations will be searched for themes, also in the listed order:

* \<HOME\>/.emulationstation/themes/
* \<ES-DE executable directory\>/../Resources/themes/
* \<ES-DE executable directory\>/themes/

A theme is not mandatory to start the application, but ES-DE will be basically useless without it.

As indicated above, the home directory will always take precedence and any resources or themes located there will override the ones in the path of the ES-DE executable.

**Creating a .dmg installer**

Simply run `cpack` to build a .dmg disk image/installer:

```
myusername@computer:~/emulationstation-de$ cpack
CPack: Create package using Bundle
CPack: Install projects
CPack: - Run preinstall target for: emulationstation-de
CPack: - Install project: emulationstation-de []
CPack: Create package
CPack: - package: /Users/myusername/emulationstation-de/EmulationStation-DE-1.2.0-x64.dmg generated.
```

## Building on Windows

Both MSVC and MinGW (GCC) work fine for building ES-DE on Windows.

Although I would prefer to exclude support for MSVC, this compiler simply works much better when developing as it's much faster than MinGW when linking debug builds and when actually debugging. But for release builds MinGW is very fast and ES-DE starts around 18% faster when built with MinGW, meaning this compiler probably generates more efficient code overall. As well MSVC requires a lot more DLL files to be distributed with the application and the console window is always displayed on startup for some reason.

For these reasons I think MSVC makes sense for development and MinGW for the releases.

**MSVC setup**

Install Git for Windows: \
[https://gitforwindows.org](https://gitforwindows.org)

Download the Visual Studio Build Tools (choose Visual Studio Community edition): \
[https://visualstudio.microsoft.com/downloads](https://visualstudio.microsoft.com/downloads)

It seems as if Microsoft has dropped support for installing the Build Tools without the Visual Studio IDE, at least I've been unable to find a way to exclude it. But I just pretend it's not installed and use VSCode instead which works perfectly fine.

During installation, choose the Desktop development with C++ workload with the following options (version details may differ):

```
MSVC v143 - VS 2022 C++ x64/x86 build tools (Latest)
Windows 10 SDK
Just-In-Time debugger
C++ AddressSanitizer
```

If you will only use MSVC and not MinGW, then also add this option:
```
C++ CMake tools for Windows
```

If not installing the CMake version supplied by Microsoft, you need to make sure that you have a recent version on your machine or CMake will not be able to detect MSVC correctly.

As well you may need to install the latest version of Microsoft Visual C++ Redistributable which can be downloaded here:\
https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redis


The way the MSVC environment works is that a specific developer shell is provided where the build environment is properly configured. You open this from the Start menu via `Visual Studio 2022` -> `Visual Studio tools` -> `VC` -> `x64 Native Tools Command Prompt for VS 2022 Current`.

It's important to choose the x64-specific shell and not the x86 variant, as ES-DE will only compile as a 64-bit application.

**MinGW (GCC) setup**

Download the following packages and install them:

[https://gitforwindows.org](https://gitforwindows.org)

[https://cmake.org/download](https://cmake.org/download)

Download the _MinGW-w64 based_ version of GCC: \
[https://jmeubank.github.io/tdm-gcc](https://jmeubank.github.io/tdm-gcc)

After installation, make a copy of `TDM-GCC-64\bin\mingw32-make` to `make` just for convenience.

Version 9.2.0 of MinGW has been confirmed to work fine, but 10.3.0 appears broken as it causes huge performance problems for the FFmpeg function avfilter_graph_free() with execution times going from milliseconds to hundreds of milliseconds or even seconds.

Note that most GDB builds for Windows have broken Python support so that pretty printing won't work. The recommended MinGW distribution should work fine though.

**Other preparations**

In order to get clang-format onto the system you need to download and install Clang: \
[https://releases.llvm.org](https://releases.llvm.org)

Just run the installer and make sure to select the option _Add LLVM to the system PATH for current user_.

Install your editor of choice, I use [VSCode](https://code.visualstudio.com).

Configure Git. I won't get into the details on how this is done, but there are many resources available online to support with this. The `Git Bash` shell shipped with Git for Windows is very useful though as it's somewhat reproducing a Unix environment using MinGW/MSYS2.

It's strongly recommended to set line breaks to Unix-style (line feed only) directly in the editor. But if not done, lines breaks will anyway be converted when running clang-format on the code, as explained [here](INSTALL.md#using-clang-format-for-automatic-code-formatting).

In the descriptions below it's assumed that all build steps for MinGW/GCC will be done in the Git Bash shell, and all the build steps for MSVC will be done in the MSVC developer console (x64 Native Tools Command Prompt for VS).

**Cloning and setting up dependencies**

To clone the source repository, run the following:

```
git clone https://gitlab.com/es-de/emulationstation-de.git
```

By default the master branch will be used, which is where development takes place. To instead build a stable release, switch to the `stable-x.x` branch for the version, for example:

```
cd emulationstation-de
git checkout stable-1.2
```

On Windows all dependencies are kept in-tree in the `external` directory tree. Most of the libraries can be downloaded in binary form, but a few need to be built from source code. There are four scripts in the tools directory that automate this entirely. Two of them are used for the MSVC compiler and the other two for MinGW.

For MSVC, you run them like this:
```
cd emulationstation-de
tools\Windows_dependencies_setup_MSVC.bat
tools\Windows_dependencies_build_MSVC.bat
```

And for MinGW like the following:
```
cd emulationstation-de
tools/Windows_dependencies_setup_MinGW.sh
tools/Windows_dependencies_build_MinGW.sh
```

Re-running the setup script will delete and download all dependencies again, and re-running the build script will clean and rebuild from scratch. You can of course also manually recompile an individual library if needed.

The setup scripts for both MSVC and MinGW will download and launch an installer for OpenSSL for Windows if this has not already been installed on the build machine. Just run through the installer using the default settings and everything should work fine.

Following these preparations, ES-DE should be ready to be compiled.

**Building ES-DE using MSVC**

For a release build:

```
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release .
nmake
```

Or for a debug build:
```
cmake -G "NMake Makefiles" .
nmake
```

For some annoying reason MSVC is the only compiler that creates a debug build by default and where you need to explicitly set the build type to Release.

To enable AddressSanitizer which helps with identifying memory issues like corruption bugs and buffer overflows, build with the ASAN option:
```
cmake -G "NMake Makefiles" -DASAN=on .
nmake
```

ThreadSanitizer and UndefinedBehaviorSanitizer aren't available for the MSVC compiler.

There are a number of compiler warnings for the bundled rlottie library when building with MSVC. Unfortunately these need to be resolved upstream, but everything should still work fine so the warnings can be ignored for now.

Unfortunately nmake does not support parallel compiles so it's very slow. There are third party solutions to get multi-core building working with MSVC, but I've not investigated this in depth.

Be aware that MSVC links against different VC++ libraries for debug and release builds (e.g. MSVCP140.dll or MSVCP140d.dll), so any NSIS package made from a debug build will not work on computers without the MSVC development environment installed.

**Building ES-DE using MinGW**

For a release build:

```
cmake -G "MinGW Makefiles" .
make
```

Or for a debug build:

```
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug .
make
```

Unfortunately AddressSanitizer, ThreadSanitizer and UndefinedBehaviorSanitizer do not seem to be supported with MinGW.

You run a parallel build using multiple CPU cores with the `-j` flag, for example, `make -j6`.

Note that compilation time is much longer than on Unix or macOS, and linking is incredibly slow for a debug build (around 10 times longer compared to Linux). The debug binary is also much larger than on Unix.

**TLS/SSL certificates**

On Windows the certificates supplied with the operating system will not be utilized, instead TLS/SSL certificates bundled with ES-DE will be used.

**Running with OpenGL software rendering**

If you are running Windows in a virtualized environment such as QEMU-KVM that does not support HW accelerated OpenGL, you can install the Mesa3D for Windows library, which can be downloaded at [https://fdossena.com/?p=mesa/index.frag](https://fdossena.com/?p=mesa/index.frag).

You simply extract the opengl32.dll file into the ES-DE directory and this will enable the llvmpipe renderer. The performance will be terrible of course, but everything should work and it should be good enough for test building on Windows without having to reboot your computer to a native Windows installation. (Note that you may need to copy opengl32.dll to your RetroArch installation directory as well to get the emulators to work somehow correctly.)

Obviously this library is only intended for development and will not be shipped with ES-DE.

**Creating an NSIS installer**

To create an NSIS installer (Nullsoft Scriptable Install System) you need to first install the NSIS creation tool:

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
CPack: - package: C:/Programming/emulationstation-de/EmulationStation-DE-1.2.0-x64.exe generated.
```

The default installation directory suggested by the installer is `C:\Program Files\EmulationStation-DE` but this can of course be changed by the user.

ES-DE will look in the following locations for application resources, in the listed order:

* \<home\>\\.emulationstation\resources\
* \<ES-DE executable directory\>\resources\

The resources directory is critical, without it the application won't start.

As well the following locations will be searched for themes, also in the listed order:

* \<home\>\\.emulationstation\themes\
* \<ES-DE executable directory\>\themes\

A theme is not mandatory to start the application, but ES-DE will be basically useless without it.

As indicated above, the home directory will always take precedence and any resources or themes located there will override the ones in the path of the ES-DE executable.

## Using clang-format for automatic code formatting

The entire ES-DE codebase is formatted using clang-format and all new code must be formatted using this tool before being committed.

There is a style configuration file named .clang-format located directly at the root of the repository which contains the formatting rules. How to install clang-format is detailed per operating system earlier in this document.

There are two ways to run the tool, from the command line or from inside an editor such as VSCode.

To format a file from the command line, simply run:

```clang-format -i <source file>```

The -i flag will make an inplace edit of the file.

Alternatively the `tools/reformat_codebase.sh` script can be executed to format the entire codebase in one go.

But the recommended approach is to run clang-format from within the editor. If using VSCode, there is an extension available named Clang-Format. After installing this, simply open a source file, right click and choose `Format Document` or use the applicable keyboard shortcut. The first time you do this, you will have to make a choice to perform the formatting using clang-format. The rest should be completely automatic.

Whatever you do, don't set up your editor to run clang-format on commit because if something goes wrong (which has happened in the past) you will potentially commit a lot of garbage which could take some effort to clean up. Adding to this, string literals can get strange formatting from time to time and there are occasionally clang-format bugs that may cause other problems. So always review the formatted code manually before commit.

In some instances you may want to avoid getting code formatted, and you can accomplish this by simply enclosing the lines with the two comments "clang-format off" and "clang-format on", such as this:

```c++
// clang-format off
CollectionSystemDecl systemDecls[] = {
//  Type                 Name                Long name       Theme folder           isCustom
    {AUTO_ALL_GAMES,     "all",              "all games",    "auto-allgames",       false},
    {AUTO_LAST_PLAYED,   "recent",           "last played",  "auto-lastplayed",     false},
    {AUTO_FAVORITES,     "favorites",        "favorites",    "auto-favorites",      false},
    {CUSTOM_COLLECTION,  myCollectionsName,  "collections",  "custom-collections",  true }
};
// clang-format on
```

Adding a comment on its own line will also prevent some formatting such as turning short functions and lambda expressions into single lines. For this function such a comment has been added:

```c++
const std::string FileData::get3DBoxPath() const
{
    // Return path to the 3D box image.
    return getMediafilePath("3dboxes", "3dbox");
}
```

If the comment was omitted, the function would get formatted like this:

```c++
const std::string FileData::get3DBoxPath() const { return getMediafilePath("3dboxes", "3dbox"); }
```

Adding comments (even empty ones) can also force line breaks to avoid ugly formatting such as this:

```c++
for (auto it = mCursorStackHistory.begin(); it != mCursorStackHistory.end();
        it++) {
    if (std::find(listEntries.begin(), listEntries.end(), *it) !=
        listEntries.end()) {
        newCursor = *it;
        mCursorStackHistory.erase(it);
        break;
    }
}
```

A comment at the right place produces this much nicer formatting:
```c++
for (auto it = mCursorStackHistory.begin(); // Line break.
        it != mCursorStackHistory.end(); it++) {
    if (std::find(listEntries.begin(), listEntries.end(), *it) !=
        listEntries.end()) {
        newCursor = *it;
        mCursorStackHistory.erase(it);
        break;
    }
}
```

Of course you would like to get the code formatted according to the clang-format rules in most instances, these workaround are only meant for rare exceptions. Some compromises are necessary when auto-formatting code, at least with clang-format in its current state.

## CA certificates and MAME ROM information

**CA certificates**

There are some files shipped with ES-DE that need to be pulled from external resources, the first one being the CA certificate bundle to get TLS/SSL support working on Windows.

The CA certificates shipped with ES-DE come directly from the curl project but they're originally supplied by the Mozilla foundation. See [https://wiki.mozilla.org/CA](https://wiki.mozilla.org/CA) for more information about this certificate bundle.

The latest version can be downloaded from [https://curl.se/docs/caextract.html](https://curl.se/docs/caextract.html)

After downloading the file, rename it from `cacert.pem` to `curl-ca-bundle.crt` and move it to the certificates directory i.e.:

```
emulationstation-de/resources/certificates/curl-ca-bundle.crt
```

**MAME ROM info**

ES-DE automatically identifies and excludes MAME BIOS and device files, as well as translating the short MAME ROM names to their full game names. This is done using information from the MAME driver file shipped with the official MAME distribution. The file needs to be converted to an internal format used by ES-DE as the original file is huge and most of the information is not required.

To get hold of the driver file, go to [https://www.mamedev.org/release.php](https://www.mamedev.org/release.php) and select the Windows version, but only download the driver information in XML format and not MAME itself. This file will be named something like `mame0226lx.zip` and unzipping it will give you a filename such as `mame0226.xml`.

Move the XML driver file to the resources/MAME directory and then convert it to the ES-DE internal files:

```
cd emulationstation-de/resources/MAME
mv mamebioses.xml mamebioses.xml_OLD
mv mamedevices.xml mamedevices.xml_OLD
../../tools/mame_create_index_files.sh mame0226.xml
mv mamebioses.xml mamebioses.xml_NEW
mv mamedevices.xml mamedevices.xml_NEW
../../tools/mame_merge_index_files.sh mamebioses.xml_OLD mamebioses.xml_NEW mamebioses.xml
../../tools/mame_merge_index_files.sh mamedevices.xml_OLD mamedevices.xml_NEW mamedevices.xml
diff mamebioses.xml mamebioses.xml_OLD
diff mamedevices.xml mamedevices.xml_OLD
rm *NEW *OLD mame0226.xml
```

You need `xmlstarlet` installed for these scripts to work.

The diff command is used to do a sanity check that the changes look reasonable before deleting the old files. This is an example for the BIOS file when going from driver version 0.221 to 0.226:
```
diff mamebioses.xml mamebioses.xml_OLD
1c1
< <!-- Last updated with information from MAME driver file mame0226.xml -->
---
> <!-- Last updated with information from MAME driver file mame0221.xml -->
51d50
< <bios>kpython</bios>
```

You can also use git for this comparison of course, which may actually provide a clearer visualization of the changes:
```
git diff mamebioses
git diff mamedevices
```

The reason to not simply replace the BIOS and devices files with the new version is that we want to retain entries from all older MAME versions as otherwise older ROM sets used on older MAME versions would have missing information. This is so as the MAME project sometimes removes older entries when they're reorganizing the ROM sets. By merging the files we retain backward compatibility but still support the latest MAME version. To clarify, this of course does not affect the emulation itself, but rather the filtering of BIOS and device files inside ES-DE. The mamenames.xml file containing the translation of MAME ROM names to the full game names does not suffer from this problem as it's cumulative, which is why it is simply overwritten.

## Configuration

**~/.emulationstation/es_settings.xml**

When ES-DE is first started, a configuration file will be created as `~/.emulationstation/es_settings.xml`

This file will contain all supported settings at their default values. Normally you shouldn't need to modify this file manually, instead you should be able to use the menu inside ES-DE to update all the necessary settings.

**Setting the ROM directory in es_settings.xml**

This complete configuration step can normally be skipped as you're presented with a dialog to change the ROM directory upon application startup if no game files are found.

By default, ES-DE looks in `~/ROMs` for the ROM files, where they are expected to be grouped into directories corresponding to the game systems, for example:

```
myusername@computer:~ROMs$ ls -1
c64
megadrive
pcengine
```

However, if you've saved your ROMs to another directory, you need to configure the ROMDirectory setting in es_settings.xml.\
Here's an example:

```xml
<string name="ROMDirectory" value="~/games/ROMs" />
```

Keep in mind that you still need to group the ROMs into directories corresponding to the `<path>` tags in es_systems.xml.

There is also support to add the variable %ESPATH% to the ROM directory setting, this will expand to the path where the ES-DE executable is started from. You should normally not need this, but the option is there for special cases. For example:

```xml
<string name="ROMDirectory" value="%ESPATH%/../ROMs" />
```

**~/.emulationstation/es_input.xml**

As ES-DE auto-configures the keyboard and controllers, neither the input configuration step or manual adjustments to the es_input.xml file should normally be needed. Actually, unless the button layout has been customized using the input configurator, the es_input.xml file will not even exist.

But if you have customized your button layout and your controller or keyboard stop working, you can delete the `~/.emulationstation/es_input.xml` file to remove the customizations, or you can start ES-DE with the `--force-input-config` command line option to make the input configurator appear.

The input configuration is described in the [User guide](USERGUIDE.md#input-device-configuration).

## Command line options

You can use **--help** or **-h** to view the list of command line options, as shown here.

```
--display [1 to 4]                    Display/monitor to use
--resolution [width] [height]         Application resolution
--screenoffset [horiz.] [vert.]       Offset screen contents within application window
--screenrotate [0, 90, 180 or 270]    Rotate screen contents within application window
--fullscreen-padding [1/on or 0/off]  Padding if --resolution is lower than display resolution
--vsync [1/on or 0/off]               Turn VSync on or off (default is on)
--max-vram [size]                     Max VRAM to use (in mebibytes) before swapping
--anti-aliasing [0, 2 or 4]           Set MSAA anti-aliasing to disabled, 2x or 4x
--no-splash                           Don't show the splash screen during startup
--no-update-check                     Don't check for application updates during startup
--gamelist-only                       Skip automatic game ROM search, only read from gamelist.xml
--ignore-gamelist                     Ignore the gamelist.xml files
--show-hidden-files                   Show hidden files and folders
--show-hidden-games                   Show hidden games
--force-full                          Force the UI mode to Full
--force-kiosk                         Force the UI mode to Kiosk
--force-kid                           Force the UI mode to Kid
--force-input-config                  Force configuration of input devices
--create-system-dirs                  Create game system directories
--home [path]                         Directory to use as home path
--debug                               Print debug information
--version, -v                         Display version information
--help, -h                            Summon a sentient, angry tuba
```

_The --anti-aliasing option is not available if ES-DE is built using the OpenGL ES renderer and the --no-update-check option is not available for builds where the application updater is disabled._

As you can see above, you can override the home directory path using the `--home` flag. So by running for instance the command `emulationstation --home ~/games/emulation`, ES-DE will use `~/games/emulation/.emulationstation` as its application home directory. Be aware that this option completely replaces what is considered the home directory, meaning the default ROM directory ~/ROMs would be resolved to ~/games/emulation/ROMs. The same is true for the emulator core locations if es_find_rules.xml is configured to look for them relative to the home directory. So of course RetroArch and other emulators would also need to be configured to use ~/games/emulation as its base directory in this instance.

Setting --resolution to a lower or higher value than the display resolution will add a border to the application window. The exception is if defining a lower resolution than the display resolution in combination with the --fullscreen-padding flag as this will pad the screen contents on a black background. This can be combined with the --screenoffset option for exact positioning on displays where bezels or similar may obstruct part of the viewable area.

The --no-update-check option only disabled the application updater for the current startup. To permanently disable this functionality use the _Check for application updates_ option in the _Other settings_ menu. The command line option is primarily intended for the unlikely event that the application updater breaks the application and makes it impossible to start.

Running with the --create-system-dirs option will generate all the game system directories in the ROMs folder. This is equivalent to starting ES-DE with no game ROMs present and pressing the _Create directories_ button. Detailed output for the directory creation will be available in es_log.txt and the application will quit immediately after the directories have been created.

For the following options, the es_settings.xml file is immediately updated/saved when passing the parameter:
```
--display
--screenrotate
--max-vram
--anti-aliasing
--gamelist-only
--show-hidden-files
--show-hidden-games
```

As well, passing the option --ignore-gamelist will disable the ParseGamelistOnly setting controlled by --gamelist-only and immediately save the es_settings.xml file. If passing both the --ignore-gamelist and --gamelist-only parameters then --ignore-gamelist will take precedence and --gamelist-only will be ignored.

The --ignore-gamelist option is only active during the program session and is not saved to es_settings.xml. But --gamelist-only is however saved, so in order to return to the normal operation of parsing the gamelist.xml files after starting ES-DE with the --gamelist-only option, you will need to disable the setting _Only show ROMs from gamelist.xml files_ in the _Other settings_ menu (or manually change the ParseGamelistOnly entry in es_settings.xml).

## Settings not configurable via the GUI

There are some settings which are not configurable via the GUI as modifying these should normally not be required. To still change these, edit the es_settings.xml file directly.

**DebugSkipInputLogging**

Enabling this will skip all input event logging (button and key presses). Default value is false.

**DebugSkipMissingThemeFiles**

Enabling this will skip all debug messages about missing files when loading a theme set. Default value is false.

**DebugSkipMissingThemeFilesCustomCollections**

Enabling this will skip all debug messages about missing files specifically for custom collections when loading a theme set. Note that DebugSkipMissingThemeFiles takes precedence, so if that setting is set to true then the DebugSkipMissingThemeFilesCustomCollections setting will be ignored. Default value is true.

**LegacyGamelistFileLocation**

As of ES-DE 2.0.0 any gamelist.xml files stored in the game system directories (e.g. under `~/ROMs/`) will not get loaded, they are instead required to be placed in the `~/.emulationstation/gamelists/` directory tree. By setting this option to `true` it's however possible to retain the old behavior of first looking for gamelist.xml files in the system directories on startup. Note that even if this setting is enabled ES-DE will still always create new gamelist.xml files under `~/.emulationstation/gamelists/` which was the case also for the 1.x.x releases.

**LottieMaxFileCache**

Sets the maximum per-file animation cache for Lottie animations. Minimum value is 0 MiB and maximum value is 1024 MiB. Default value is 150 MiB.

**LottieMaxTotalCache**

Sets the maximum total animation cache for Lottie animations. Minimum value is 0 MiB and maximum value is 4096 MiB. Default value is 1024 MiB.

**OpenGLVersion**

If using the regular desktop OpenGL renderer, the allowed values are 3.3 (default on all builds except the Steam Deck), 4.2 and 4.6 (default on the Steam Deck). If using the OpenGL ES renderer, the allowed values are 3.0 (default), 3.1 and 3.2.

**ScraperConnectionTimeout**

Sets the server connection timeout for the scraper. Minimum value is 0 seconds (infinity) and maximum value is 300 seconds. Default value is 30 seconds.

**ScraperTransferTimeout**

Sets the transfer timeout per HTTPS request. Minimum value is 0 seconds (infinity) and maximum value is 300 seconds. Default value is 120 seconds.

**UIMode_passkey**

The passkey to use to change from the _Kiosk_ or _Kid_ UI modes to the _Full_ UI mode.

## es_systems.xml

The es_systems.xml file contains the game systems configuration data for ES-DE, written in XML format. This defines the system name, the full system name, the ROM path, the allowed file extensions, the launch command, the platform (for scraping) and the theme to use.

ES-DE ships with a comprehensive `es_systems.xml` file and most users will probably never need to make any customizations. But there may be special circumstances such as wanting to use different emulators for some game systems or perhaps to add additional systems altogether.

To accomplish this, ES-DE supports customizations via a separate es_systems.xml file that is to be placed in the `custom_systems` folder in the application home directory, i.e. `~/.emulationstation/custom_systems/es_systems.xml`. (The tilde symbol `~` translates to `$HOME` on Unix and macOS, and to `%HOMEPATH%` on Windows unless overridden via the --home command line option.)

This custom file functionality is designed to be complementary to the bundled es_systems.xml file, meaning you should only add entries to the custom configuration file for game systems that you actually want to add or override. So to for example customize a single system, this file should only contain a single `<system>` tag. The structure of the custom file is identical to the bundled file with the exception of an additional optional tag named `<loadExclusive/>`. If this is placed in the custom es_systems.xml file, ES-DE will not load the bundled file. This is normally not recommended and should only be used for special situations. At the end of this section you can find an example of a custom es_systems.xml file.

The bundled es_systems.xml file is located in the resources directory that is part of the application installation. For example this could be `/usr/share/emulationstation/resources/systems/unix/es_systems.xml` on Unix, `/Applications/EmulationStation Desktop Edition.app/Contents/Resources/resources/systems/macos/es_systems.xml` on macOS or `C:\Program Files\EmulationStation-DE\resources\systems\windows\es_systems.xml` on Windows. The actual location may differ from these examples of course, depending on where ES-DE has been installed.

If you're using the AppImage release of ES-DE then the bundled es_systems.xml file is embedded in the AppImage together with the rest of the resources.

It doesn't matter in which order you define the systems as they will be sorted by the `<fullname>` tag or by the optional `<systemsortname>` tag when displayed inside the application. But it's still a good idea to add the systems in alphabetical order to make the configuration file easier to maintain.

Note that the `<systemsortname>` tags are sorted in [lexicographic order](https://en.wikipedia.org/wiki/Lexicographic_order) so 11 will be sorted above 2 but 002 will be sorted above 011.

Wildcards are supported for emulator binaries, but not for directories:
```xml
<!-- This is supported, first matching file will be selected -->
<command>~/Applications/yuzu*.AppImage %ROM%</command>
<!-- This is also supported -->
<command>~/Applications/yuzu*.App* %ROM%</command>
<!-- This is NOT supported -->
<command>~/App*/yuzu*.AppImage %ROM%</command>
```

There is a special case when it comes to file extensions where it's possible to use extensionless files if required. To accomplish this simply add a dot (.) to the list of extensions in the `<extension>` tag. Obviously this makes it impossible to use the _directories interpreted as files_ functionality as there is no file extension, but apart from that everything should work the same as for regular files.

Keep in mind that you have to set up your emulators separately from ES-DE as the es_systems.xml file assumes that your emulator environment is properly configured.

Below is an overview of the file layout with various examples. For the command tag, the newer es_find_rules.xml logic described later in this document removes the need for most of the legacy options, but they are still supported for special configurations and for backward compatibility with old configuration files.

```xml
<?xml version="1.0"?>
<!-- This is the ES-DE game systems configuration file. -->
<systemList>
    <!-- Any tag not explicitly described as optional in the description is mandatory.
    If omitting a mandatory tag, ES-DE will skip the system entry during startup. -->
    <system>
        <!-- A short name. Although there can be multiple identical <name> tags in the file, upon successful loading of a system,
        any succeeding entries with identical <name> tags will be skipped. Multiple identical name tags is only required for very
        special situations so it's normally recommended to keep this tag unique. -->
        <name>snes</name>

        <!-- The full system name, used for sorting the systems, for selecting the systems to multi-scrape etc. -->
        <fullname>Nintendo SNES (Super Nintendo)</fullname>

        <!-- By default the systems are sorted by their full names, but this can be overridden by setting the optional
        <systemsortname> tag to an arbitrary value. As far as sorting is concerned, the effect will be identical to
        changing the <fullname> tag. Apart for system sorting, this tag has no effect and its actual value will not
        be displayed anywhere within the appliction. Note that the sorting is done in lexicographic order. -->
        <systemsortname>Super Nintendo</systemsortname>

        <!-- The path to look for ROMs in. '~' will be expanded to $HOME or %HOMEPATH%, depending on the operating system.
        The optional %ROMPATH% variable will expand to the path defined in the setting ROMDirectory in es_settings.xml.
        All subdirectories (and non-recursive links) will be included. -->
        <path>%ROMPATH%/snes</path>

        <!-- A list of extensions to search for, delimited by any of the whitespace characters (", \r\n\t"). Extensions are
        case sensitive and they must begin with a dot. It's also possible to add just a dot to include extensionless files. -->
        <extension>.smc .SMC .sfc .SFC .swc .SWC .fig .FIG .bs .BS .bin .BIN .mgd .MGD .7z .7Z .zip .ZIP</extension>

        <!-- The command executed when a game is launched. Various variables are replaced if found for a command tag as explained below.
        This example for Unix uses the %EMULATOR_ and %CORE_ variables which utilize the find rules defined in the es_find_rules.xml
        file. This is the recommended way to configure the launch command. -->
        <command>%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/snes9x_libretro.so %ROM%</command>

        <!-- It's possible to define alternative emulators by adding additional command tags for a system. When doing this, the
        "label" attribute is mandatory for all tags. It's these labels that will be shown in the user interface when selecting the
        alternative emulator either system-wide or per game. The first row will be the default emulator. -->
        <command label="Snes9x - Current">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/snes9x_libretro.so %ROM%</command>
        <command label="Snes9x 2010">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/snes9x2010_libretro.so %ROM%</command>
        <command label="bsnes">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/bsnes_libretro.so %ROM%</command>
        <command label="bsnes-mercury Accuracy">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/bsnes_mercury_accuracy_libretro.so %ROM%</command>
        <command label="Beetle Supafaust">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/mednafen_supafaust_libretro.so %ROM%</command>
        <command label="Mesen-S">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/mesen-s_libretro.so %ROM%</command>

        <!-- This example for Unix will search for RetroArch in the PATH environment variable and it also has an absolute path to
        the snes9x_libretro core, If there are spaces in the path or filename, you must enclose them in quotation marks, such as
        retroarch -L "~/my configs/retroarch/cores/snes9x_libretro.so" %ROM% -->
        <command>retroarch -L ~/.config/retroarch/cores/snes9x_libretro.so %ROM%</command>

        <!-- This example for Unix combines the two rules above to search for RetroArch in the PATH environment variable but uses
        the find rules for the emulator cores. -->
        <command>retroarch -L %CORE_RETROARCH%/snes9x_libretro.so %ROM%</command>

        <!-- This example for Unix uses a wildcard to find the first matching RPCS3 AppImage in the ~/Applications directory.
        This is useful as AppImages often have version information embedded in the filename that may change when upgrading the package. -->
        <command label="RPCS3 (Standalone)">~/Applications/rpcs3*.AppImage --no-gui %ROM%</command>

        <!-- This is an example for macOS, which is very similar to the Unix example above except using an absolute path to the emulator. -->
        <command>/Applications/RetroArch.app/Contents/MacOS/RetroArch -L %CORE_RETROARCH%/snes9x_libretro.dylib %ROM%</command>

        <!-- This is an example for Windows. The .exe extension is optional and both forward slashes and backslashes are allowed as
        directory separators. As there is no standardized installation directory structure for this operating system, the %EMUPATH%
        variable is used here to find the cores relative to the RetroArch binary. The emulator binary must be in the PATH environment
        variable or otherwise the complete path to the retroarch.exe file needs to be defined. Batch scripts (.bat) are also supported. -->
        <command>retroarch.exe -L %EMUPATH%\cores\snes9x_libretro.dll %ROM%</command>

        <!-- Another example for Windows. As can be seen here, the absolute path to the emulator has been defined, and there are spaces
        in the directory name, so it needs to be surrounded by quotation marks. Quotation marks around the %EMUPATH% entry are optional
        but for this example they're added. -->
        <command>"C:\My Games\RetroArch\retroarch.exe" -L "%EMUPATH%\cores\snes9x_libretro.dll" %ROM%</command>

        <!-- An example for use in a portable Windows emulator installation, for instance on a USB memory stick. The %ESPATH% variable is
        expanded to the directory of the ES-DE executable. -->
        <command>"%ESPATH%\RetroArch\retroarch.exe" -L "%ESPATH%\RetroArch\cores\snes9x_libretro.dll" %ROM%</command>

        <!-- An example of setting the start directory to the directory of the emulator binary, which is required for standalone MAME
        on Windows. The %ROMPATH% variable is also used as this emulator needs to receive the ROM directory and game file separately. -->
        <command label="MAME (Standalone)">%HIDEWINDOW% %EMULATOR_MAME% %STARTDIR%=%EMUDIR% -rompath %ROMPATH%\arcade %BASENAME%</command>

        <!-- The equivalent setup of standalone MAME for Unix. If not existing, the start directory will be created on game launch. -->
        <command label="MAME (Standalone)">%EMULATOR_MAME% %STARTDIR%=~/.mame -rompath %ROMPATH%/arcade %BASENAME%</command>

        <!-- An example on Unix which launches either a .desktop file or a shell script. This is for example used by the ports system.
        The %RUNINBACKGROUND% variable does exactly what it sounds like, it keeps ES-DE running in the background while the game is
        launched. This is required for launching Steam games properly as well as for some other systems. -->
        <command>%RUNINBACKGROUND% %ENABLESHORTCUTS% %EMULATOR_OS-SHELL% %ROM%</command>

        <!-- The equivalent configuration as above, but for Windows.
        The optional %HIDEWINDOW% variable is used to hide the console window which would otherwise be visible when launching games
        and %ESCAPESPECIALS% escapes the characters &()^=;, that cmd.exe can't otherwise handle. -->
        <command>%HIDEWINDOW% %ESCAPESPECIALS% %RUNINBACKGROUND% cmd.exe /C %ROM%</command>

        <!-- The platform(s) to use when scraping. You can see the full list of supported platforms in es-app/src/PlatformId.cpp.
        The entry is case insensitive as it will be converted to lower case during startup.
        This tag is optional but scraper searches for the system will be inaccurate if it's left out.
        You can use multiple platforms too, delimited with any of the whitespace characters (", \r\n\t"), e.g. "megadrive, genesis". -->
        <platform>snes</platform>

        <!-- The theme to load from the current theme set. See THEMES.md for more information.
        This tag is optional and if it doesn't exist, ES-DE will attempt to find a theme with the same name as the system name.
        If no such match is made, the system will be unthemed.
        It's recommended to include this tag even if it's just to clarify that the theme should correspond to the system name. -->
        <theme>snes</theme>
    </system>
</systemList>
```

The following variable is expanded for the `path` tag:

`%ROMPATH%` - Replaced with the path defined in the setting ROMDirectory in es_settings.xml.

The following variables are expanded for the `command` tag:

`%ROM%` - Replaced with the absolute path to the selected ROM, with most special characters escaped with a backslash.

`%ROMRAW%`	- Replaced with the unescaped, absolute path to the selected ROM.  If your emulator is picky about paths, you might want to use this instead of %ROM%, but enclosed in quotes.

`%ROMPATH%` - Replaced with the path defined in the setting ROMDirectory in es_settings.xml. If combined with a path that contains blankspaces, then it must be surrounded by quotation marks, for example `%ROMPATH%"\Arcade Games"`. Note that the quotation mark must be located before the directory separator in this case.

`%BASENAME%` - Replaced with the "base" name of the path to the selected ROM. For example the path `/foo/bar.rom` would end up as the value `bar`

`%FILENAME%` - Replaced with the filename of the selected ROM. For example the path `/foo/bar.rom` would end up as the value `bar.rom`

`%STARTDIR%` - The directory to start in when launching the emulator. Must be defined as a pair separated by an equal sign. This is normally not required, but some emulators and game engines like standalone MAME and OpenBOR will not work properly unless you're in the correct directory when launching a game. Either an absolute path can be used, such as `%STARTDIR%=C:\Games\mame` or some variables are available that provide various functions. The `%EMUDIR%` variable can be used to start in the directory where the emulator binary is located, i.e. `%STARTDIR%=%EMUDIR%`, the `%GAMEDIR%` variable can be used to start in the directory where the game file is located, i.e. `%STARTDIR%=%GAMEDIR%` and the `%GAMEENTRYDIR%` variable can be used which works identically to `%GAMEDIR%` with the exception that it will interpret the actual game entry as the start directory. This is useful in very rare situations like for the EasyRPG Player where the game directories are interpreted as files but where the game engine must still be started from inside the game directory. If an absolute path is set that contains blankspaces, then it must be surrounded by quotation marks, for example `%STARTDIR%="C:\Retro games\mame"`. If the directory defined by this variable does not exist, it will be created on game launch. The variable can be placed anywhere in the launch command if the %EMULATOR_ variable is used, otherwise it has to be placed after the emulator binary.

`%INJECT%` - This allows the injection of launch arguments or environment variables stored in a text file on the filesystem. The %INJECT% variable must be defined as a pair separated by an equal sign, for example `%INJECT%=game.commands`. The `%BASENAME%` variable can also be used in conjunction with this variable, such as `%INJECT%=%BASENAME%.commands`. By default a path relative to the game file will be assumed but it's also possible to use an absolute path or the ~ (tilde) symbol which will expand to the home directory. If a path contains spaces it needs to be surrounded by quotation marks, such as `%INJECT%="C:\My games\ROMs\daphne\%BASENAME%.daphne\%BASENAME%.commands"`. The variable can be placed anywhere in the launch command and the file contents will be injected at that position. The specified file is optional, if it does not exist, is empty, or if there are insufficient permissions to read the file, then it will simply be skipped. For safety reasons the injection file can only have a maximum size of 4096 bytes and if it's larger than this it will be skipped and a warning will be written to es_log.txt.

`%EMUPATH%` - Replaced with the path to the emulator binary. This variable is used for manually specifying emulator core locations, and a check for the existence of the core file will be done on game launch and an error displayed if it can't be found. Normally %EMUPATH% should not be used as the %CORE_ variable is the recommended method for defining core locations.

`%EMUDIR%` - Replaced with the path to the emulator binary. This is a general purpose variable as opposed to %EMUPATH% which is intended specifically for core locations.

`%GAMEDIR%` - Replaced with the path to the game.

`%ESPATH%` - Replaced with the path to the ES-DE binary. Mostly useful for portable emulator installations, for example on a USB memory stick.

`%EMULATOR_` - This utilizes the emulator find rules as defined in `es_find_rules.xml`. This is the recommended way to configure the launch command. The find rules are explained in more detail below.

`%CORE_` - This utilizes the core find rules as defined in `es_find_rules.xml`. This is the recommended way to configure the launch command.

`%RUNINBACKGROUND%` - When this variable is present, ES-DE will continue to run in the background while a game is launched. This will also prevent the gamelist video from playing, the screensaver from starting, and the game name and game description from scrolling. This functionality is required for some systems such as Valve Steam. The variable can be placed anywhere in the launch command.

`%HIDEWINDOW%` - This variable is only available on Windows and is used primarily for hiding console windows when launching scripts (used for example by Steam games and source ports). If not defining this, the console window will be visible when launching games. The variable can be placed anywhere in the launch command.

`%ESCAPESPECIALS%` - This variable is only available on Windows and is used to escape the characters &()^=;, for the %ROM% variable, which would otherwise make binaries like cmd.exe fail when launching scripts or links. The variable can be placed anywhere in the launch command.

`%ENABLESHORTCUTS%` - This variable is only available on Unix and macOS and is used to enable shortcuts to games and applications. On Unix these come in the form of .desktop files and ES-DE has a simple parser which essentially extracts the command defined in the Exec key and then executes it. Although some basic file structure checks are performed, the actual command listed with the Exec key is blindly executed. In addition to this the variables %F, %f, %U and %u are removed from the Exec key entry. On macOS shortcuts in the form of .app directories and alias files are executed using the `open -W -a` command. This makes it possible to launch shortcuts to emulators and applications like Steam as well as aliases for any application. However the latter need to be renamed to the .app file extension or it won't work. When a file is matching the .desktop or .app extension respectively, the emulator command defined using the %EMULATOR% variable will be stripped. An %EMULATOR% entry is however still required for the %ENABLESHORTCUTS% variable to work as the intention is to combine shortcuts with the ability to launch shell scripts without having to setup alternative emulators. The %ROM% variable is expanded to the command to execute when using %ENABLESHORTCUTS%, which also means that this variable has to be used, and for example %ROMRAW% will not work.

Here are some additional real world examples of system entries, the first one for Unix:

```xml
<system>
    <name>dos</name>
    <fullname>DOS (PC)</fullname>
    <path>%ROMPATH%/dos</path>
    <extension>.bat .BAT .com .COM .conf .CONF .cue .CUE .exe .EXE .iso .ISO .7z .7Z .zip .ZIP</extension>
    <command label="DOSBox-Core">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/dosbox_core_libretro.so %ROM%</command>
    <command label="DOSBox-Pure">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/dosbox_pure_libretro.so %ROM%</command>
    <command label="DOSBox-SVN">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/dosbox_svn_libretro.so %ROM%</command>
    <platform>dos</platform>
    <theme>dos</theme>
</system>
```

Then one for macOS:

```xml
<system>
    <name>n64</name>
    <fullname>Nintendo 64</fullname>
    <path>%ROMPATH%/n64</path>
    <extension>.n64 .N64 .v64 .V64 .z64 .Z64 .bin .BIN .u1 .U1 .7z .7Z .zip .ZIP</extension>
    <command>%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/parallel_n64_libretro.dylib %ROM%</command>
    <platform>n64</platform>
    <theme>n64</theme>
</system>
```

And finally one for Windows:

```xml
<system>
    <name>pcengine</name>
    <fullname>NEC PC Engine</fullname>
    <path>%ROMPATH%\pcengine</path>
    <extension>.bin .BIN .ccd .CCD .chd .CHD .cue .CUE .img .IMG .iso .ISO .m3u .M3U .pce .PCE .sgx .SGX .toc .TOC .7z .7Z .zip .ZIP</extension>
    <command label="Beetle PCE">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%\mednafen_pce_libretro.dll %ROM%</command>
    <command label="Beetle PCE FAST">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%\mednafen_pce_fast_libretro.dll %ROM%</command>
    <platform>pcengine</platform>
    <theme>pcengine</theme>
</system>
```

As well, here's an example for Unix of a custom es_systems.xml file placed in ~/.emulationstation/custom_systems/ that overrides a single game system from the bundled configuration file:
```xml
<?xml version="1.0"?>
<!-- This is a custom ES-DE game systems configuration file for Unix -->
<systemList>
    <system>
        <name>nes</name>
        <fullname>Nintendo Entertainment System</fullname>
        <path>%ROMPATH%/nes</path>
        <extension>.nes .NES .zip .ZIP</extension>
        <command>/usr/games/fceux %ROM%</command>
        <platform>nes</platform>
        <theme>nes</theme>
    </system>
</systemList>
```

If adding the `<loadExclusive/>` tag to the file, the bundled es_systems.xml file will not be processed. For this example it wouldn't be a very good idea as NES would then be the only platform that could be used in ES-DE.

```xml
<?xml version="1.0"?>
<!-- This is a custom ES-DE game systems configuration file for Unix -->
<loadExclusive/>
<systemList>
    <system>
        <name>nes</name>
        <fullname>Nintendo Entertainment System</fullname>
        <path>%ROMPATH%/nes</path>
        <extension>.nes .NES .zip .ZIP</extension>
        <command>/usr/games/fceux %ROM%</command>
        <platform>nes</platform>
        <theme>nes</theme>
    </system>
</systemList>
```

Here is yet another example with the addition of the `snes` system where some file extensions and alternative emulator entries have been removed, and the full name and sorting have been modified.

```xml
<?xml version="1.0"?>
<!-- This is a custom ES-DE game systems configuration file for Unix -->
<systemList>
    <system>
        <name>nes</name>
        <fullname>Nintendo Entertainment System</fullname>
        <path>%ROMPATH%/nes</path>
        <extension>.nes .NES .zip .ZIP</extension>
        <command>/usr/games/fceux %ROM%</command>
        <platform>nes</platform>
        <theme>nes</theme>
    </system>
    <system>
        <name>snes</name>
        <fullname>Super Nintendo</fullname>
        <systemsortname>Nintendo SNES (Super Nintendo)</systemsortname>
        <path>%ROMPATH%/snes</path>
        <extension>.smc .SMC .sfc .SFC .swc .SWC .bin .BIN .mgd .MGD .7z .7Z .zip .ZIP</extension>
        <command label="Snes9x - Current">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/snes9x_libretro.so %ROM%</command>
        <command label="Snes9x 2010">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/snes9x2010_libretro.so %ROM%</command>
        <command label="bsnes">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/bsnes_libretro.so %ROM%</command>
        <platform>snes</platform>
        <theme>snes</theme>
    </system>
</systemList>
```

## es_find_rules.xml

This file makes it possible to define rules for where to search for the emulator binaries and emulator cores.

The file is located in the resources directory in the same location as the es_systems.xml file, but a customized copy can be placed in ~/.emulationstation/custom_systems, which will override the bundled file.

Here's an example es_find_rules.xml file for Unix (this is not the complete file shipped with ES-DE as that would be too large to include here):
```xml
<?xml version="1.0"?>
<!-- This is the ES-DE find rules configuration file for Unix -->
<ruleList>
    <emulator name="RETROARCH">
        <rule type="systempath">
            <entry>retroarch</entry>
            <entry>org.libretro.RetroArch</entry>
            <entry>RetroArch-Linux-x86_64.AppImage</entry>
        </rule>
        <rule type="staticpath">
            <entry>/var/lib/flatpak/exports/bin/org.libretro.RetroArch</entry>
            <entry>~/.local/share/flatpak/exports/bin/org.libretro.RetroArch</entry>
            <entry>~/Applications/RetroArch-Linux-x86_64.AppImage</entry>
            <entry>~/.local/bin/RetroArch-Linux-x86_64.AppImage</entry>
            <entry>~/bin/RetroArch-Linux-x86_64.AppImage</entry>
        </rule>
    </emulator>
    <core name="RETROARCH">
        <rule type="corepath">
            <!-- Snap package -->
            <entry>~/snap/retroarch/current/.config/retroarch/cores</entry>
            <!-- Flatpak package -->
            <entry>~/.var/app/org.libretro.RetroArch/config/retroarch/cores</entry>
            <!-- AppImage and compiled from source -->
            <entry>~/.config/retroarch/cores</entry>
            <!-- Ubuntu and Linux Mint repository -->
            <entry>/usr/lib/x86_64-linux-gnu/libretro</entry>
            <!-- Fedora repository -->
            <entry>/usr/lib64/libretro</entry>
            <!-- Manjaro repository -->
            <entry>/usr/lib/libretro</entry>
            <!-- FreeBSD and OpenBSD repository -->
            <entry>/usr/local/lib/libretro</entry>
            <!-- NetBSD repository -->
            <entry>/usr/pkg/lib/libretro</entry>
        </rule>
    </core>
    <emulator name="DOSBOX_STAGING">
        <!-- DOS emulator DOSBox Staging -->
        <rule type="systempath">
            <entry>dosbox-staging</entry>
            <entry>io.github.dosbox-staging</entry>
        </rule>
        <rule type="staticpath">
            <entry>/var/lib/flatpak/exports/bin/io.github.dosbox-staging</entry>
        </rule>
    </emulator>
    <emulator name="YUZU">
        <!-- Nintendo Switch emulator Yuzu -->
        <rule type="systempath">
            <entry>yuzu</entry>
            <entry>org.yuzu_emu.yuzu</entry>
            <entry>yuzu.AppImage</entry>
        </rule>
        <rule type="staticpath">
            <entry>/var/lib/flatpak/exports/bin/org.yuzu_emu.yuzu</entry>
            <entry>~/Applications/yuzu*.AppImage</entry>
            <entry>~/.local/bin/yuzu*.AppImage</entry>
            <entry>~/bin/yuzu*.AppImage</entry>
        </rule>
    </emulator>
</ruleList>
```

It's pretty straightforward, there are currently four rules supported for finding emulators, `winregistrypath`, `winregistryvalue`, `systempath` and `staticpath` and there is one rule supported for finding the emulator cores, `corepath`.

Of these, `winregistrypath` and `winregistryvalue` are only available on Windows, and attempting to use the rule on any other operating system will generate a warning in the log file when processing the es_find_rules.xml file.

The `name` attribute must correspond to the command tags in es_systems.xml, take for example this line:

```xml
<command label="DOSBox-Core">%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/dosbox_core_libretro.so %ROM%</command>
```

Here %EMULATOR_ and %CORE_ are followed by the string RETROARCH which corresponds to the name attribute in es_find_rules.xml. The name is case sensitive but it's recommended to use uppercase names to make the variables feel consistent (%EMULATOR_retroarch% doesn't look so pretty).

Of course this makes it possible to add any number of emulators to the configuration file.

The `winregistrypath` rule searches the Windows Registry "App Paths" keys for the emulators defined in the `<entry>` tags. If for example this tag is set to `retroarch.exe`, a search will be performed for the key `SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\retroarch.exe`. HKEY_CURRENT_USER is tried first, and if no key is found there, HKEY_LOCAL_MACHINE is tried as well. In addition to this, ES-DE will check that the binary defined in the default value for the key actually exists. If not, it will proceed with the next rule. Be aware that the App Paths keys are added by the emulators during their installation, and although RetroArch does add this key, not all emulators do.

The `winregistryvalue` rule will search for the specific registry value, and if it exists, it will use that value as the path to the emulator binary. HKEY_CURRENT_USER will be tried first, followed by HKEY_LOCAL_MACHINE. In the same manner as `winregistrypath`, ES-DE will check that the binary defined in the registry value actually exists. If not, it will proceed with the next rule. For example, if setting the `<entry>` tag for this rule to `SOFTWARE\Valve\Steam\SteamExe`, the emulator binary would be set to `c:\program files (x86)\steam\steam.exe`, assuming that's where Steam has been installed. As this rule can be used to query any value in the Registry, it's a quite powerful tool to locate various emulators and applications. In addition to this it's posssible to append an arbitrary string to the key value if it's found and use that as the emulator binary path. This is accomplished by using the pipe sign, so for example the entry `SOFTWARE\PCSX2\Install_Dir|\pcsx2.exe` will look for the key `SOFTWARE\PCSX2\Install_Dir` and if it's found it will take the value of that key and append the string `\pcsx2.exe` to it. This could for example result in `C:\Program Files (x86)\PCSX2\pcsx2.exe`. Also for this setup, ES-DE will check that the emulator binary actually exists, or it will proceed to the next rule.

The other rules are probably self-explanatory with `systempath` searching the PATH environment variable for the binary names defined by the `<entry>` tags and `staticpath` defines absolute paths to the emulators. For staticpath, the actual emulator binary must be included in the entry tag. Wildcards (*) are supported for the emulator binary, but not for directories. Wildcards are very useful for AppImages which often embed version information into the filenames. Note that if multiple files match a wildcard pattern, the first file returned by the operating system will be selected.

```xml
<rule type="staticpath">
    <!-- This is supported, first matching file will be selected -->
    <entry>~/Applications/yuzu*.AppImage</entry>
    <!-- This is also supported -->
    <entry>~/Applications/yuzu*.App*</entry>
    <!-- This is NOT supported -->
    <entry>~/App*/yuzu*.AppImage</entry>
</rule>
```

There is also support for substituting the emulator binary in a staticpath rule with an explicit command. To accomplish this add a pipe (|) character after the emulator entry followed by the command to execute. This is for example useful for Flatpaks when you want to check the presence of a package while still launching a specific command inside the package using the --command option. For example:

``` xml
<rule type="staticpath">
    <entry>/var/lib/flatpak/exports/bin/com.github.AmatCoder.mednaffe|flatpak run --command=mednafen com.github.AmatCoder.mednaffe</entry>
    <entry>~/.local/share/flatpak/exports/bin/com.github.AmatCoder.mednaffe|flatpak run --command=mednafen com.github.AmatCoder.mednaffe</entry>
</rule>
```

This will execute the regular logic of checking if the Mednaffe Flatpak is installed but will actually run the command after the pipe character when launching the game. Note that no checks or controls are in place for the explicitly defined command, it's just blindly executed.

The winregistrypath rules are always processed first, followed by winregistryvalue, then systempath and finally staticpath. This is done regardless of which order they are defined in the es_find_rules.xml file.

As for `corepath` this rule is simply a path to search for the emulator core.

Each rule supports multiple entry tags which are tried in the order that they are defined in the file.

The %ESPATH% and %ROMPATH% variables can be used inside the staticpath rules and the %ESPATH% and %EMUPATH% variables can be used inside the corepath rules.

The tilde symbol `~` is supported for the staticpath and corepath rules and will expand to the user home directory. Be aware that if ES-DE has been started with the --home command line option, the home directory is considered to be whatever path was passed as an argument to that option. The same is true if using a portable.txt file.

All these options combined makes it possible to create quite powerful find rules.

For reference, here are also example es_find_rules.xml files for macOS and Windows:

```xml
<?xml version="1.0"?>
<!-- This is the ES-DE find rules configuration file for macOS -->
<ruleList>
    <emulator name="RETROARCH">
        <rule type="staticpath">
            <entry>/Applications/RetroArch.app/Contents/MacOS/RetroArch</entry>
        </rule>
    </emulator>
    <core name="RETROARCH">
        <rule type="corepath">
            <!-- RetroArch >= v1.9.2 -->
            <entry>~/Library/Application Support/RetroArch/cores</entry>
            <!-- RetroArch < v1.9.2 -->
            <entry>/Applications/RetroArch.app/Contents/Resources/cores</entry>
        </rule>
    </core>
    <emulator name="DOSBOX-STAGING">
        <!-- DOS emulator DOSBox Staging -->
        <rule type="staticpath">
            <entry>/Applications/dosbox-staging.app/Contents/MacOS/dosbox</entry>
            <entry>/opt/homebrew/bin/dosbox-staging</entry>
            <entry>/usr/local/bin/dosbox-staging</entry>
        </rule>
    </emulator>
    <emulator name="MUPEN64PLUS">
        <!-- Nintendo 64 emulator Mupen64Plus -->
        <rule type="staticpath">
            <entry>/Applications/mupen64plus.app/Contents/MacOS/mupen64plus</entry>
            <entry>/usr/local/bin/mupen64plus</entry>
        </rule>
    </emulator>
    <emulator name="PCSX2">
        <!-- Sony PlayStation 2 emulator PCSX2 -->
        <rule type="staticpath">
            <entry>/Applications/PCSX2.app/Contents/MacOS/PCSX2</entry>
        </rule>
    </emulator>
</ruleList>
```

```xml
<?xml version="1.0"?>
<!-- This is the ES-DE find rules configuration file for Windows -->
<ruleList>
    <emulator name="RETROARCH">
        <rule type="winregistrypath">
            <!-- Check for an App Paths entry in the Windows Registry -->
            <entry>retroarch.exe</entry>
        </rule>
        <rule type="systempath">
            <!-- This requires that the user has manually updated the Path variable -->
            <entry>retroarch.exe</entry>
        </rule>
        <rule type="staticpath">
            <!-- Some reasonable installation locations as fallback -->
            <entry>C:\RetroArch-Win64\retroarch.exe</entry>
            <entry>C:\RetroArch\retroarch.exe</entry>
            <entry>~\AppData\Roaming\RetroArch\retroarch.exe</entry>
            <entry>C:\Program Files\RetroArch-Win64\retroarch.exe</entry>
            <entry>C:\Program Files\RetroArch\retroarch.exe</entry>
            <entry>C:\Program Files (x86)\RetroArch-Win64\retroarch.exe</entry>
            <entry>C:\Program Files (x86)\RetroArch\retroarch.exe</entry>
            <!-- Steam release at some default locations -->
            <entry>C:\Program Files (x86)\Steam\steamapps\common\RetroArch\retroarch.exe</entry>
            <entry>D:\Program Files (x86)\Steam\steamapps\common\RetroArch\retroarch.exe</entry>
            <entry>C:\Program Files\Steam\steamapps\common\RetroArch\retroarch.exe</entry>
            <entry>D:\Program Files\Steam\steamapps\common\RetroArch\retroarch.exe</entry>
            <!-- Portable installation -->
            <entry>%ESPATH%\Emulators\RetroArch-Win64\retroarch.exe</entry>
            <entry>%ESPATH%\Emulators\RetroArch\retroarch.exe</entry>
            <entry>%ESPATH%\RetroArch-Win64\retroarch.exe</entry>
            <entry>%ESPATH%\RetroArch\retroarch.exe</entry>
            <entry>%ESPATH%\..\RetroArch-Win64\retroarch.exe</entry>
            <entry>%ESPATH%\..\RetroArch\retroarch.exe</entry>
        </rule>
    </emulator>
    <core name="RETROARCH">
        <rule type="corepath">
            <entry>%EMUPATH%\cores</entry>
        </rule>
    </core>
    <emulator name="PCSX2">
        <!-- Sony PlayStation 2 emulator PCSX2 -->
        <rule type="winregistryvalue">
            <entry>SOFTWARE\PCSX2\Install_Dir|\pcsx2.exe</entry>
        </rule>
        <rule type="systempath">
            <entry>pcsx2.exe</entry>
        </rule>
        <rule type="staticpath">
            <entry>C:\Program Files (x86)\PCSX2\pcsx2.exe</entry>
            <entry>D:\Program Files (x86)\PCSX2\pcsx2.exe</entry>
            <entry>%ESPATH%\Emulators\PCSX2\pcsx2.exe</entry>
            <entry>%ESPATH%\PCSX2\pcsx2.exe</entry>
            <entry>%ESPATH%\..\PCSX2\pcsx2.exe</entry>
        </rule>
    </emulator>
    <emulator name="YUZU">
        <!-- Nintendo Switch emulator Yuzu -->
        <rule type="systempath">
            <entry>yuzu.exe</entry>
        </rule>
        <rule type="staticpath">
            <entry>~\AppData\Local\yuzu\yuzu-windows-msvc\yuzu.exe</entry>
            <entry>%ESPATH%\Emulators\yuzu\yuzu-windows-msvc\yuzu.exe</entry>
            <entry>%ESPATH%\yuzu\yuzu-windows-msvc\yuzu.exe</entry>
            <entry>%ESPATH%\..\yuzu\yuzu-windows-msvc\yuzu.exe</entry>
        </rule>
    </emulator>
</ruleList>
```

## gamelist.xml

The gamelist.xml file for a system defines the metadata for its entries, such as the game names, descriptions, release dates and ratings.

As of the fork to EmulationStation Desktop Edition, game media information no longer needs to be defined in the gamelist.xml files. Instead the application will look for any media matching the ROM filename. The media path where to look for game media is configurable either manually in `es_settings.xml` or via the GUI. If configured manually in es_settings.xml, it looks something like this:

```xml
<string name="MediaDirectory" value="~/games/media/emulationstation" />
```

There is also support to add the variable %ESPATH% to the media directory setting, this will expand to the path where the ES-DE executable is started from. You should normally not need this, but the option is there for special cases. For example:

```xml
<string name="MediaDirectory" value="%ESPATH%/../downloaded_media" />
```

The default media directory is `~/.emulationstation/downloaded_media`

You can use ES-DE's scrapers to populate the gamelist.xml files, or manually update individual entries using the metadata editor. All of this is explained in the [User guide](USERGUIDE.md).

The gamelist.xml files are searched for in the ES-DE home directory, i.e. `~/.emulationstation/gamelists/<system name>/gamelist.xml`

For example:

```
~/.emulationstation/gamelists/c64/gamelist.xml
~/.emulationstation/gamelists/megadrive/gamelist.xml
```

**gamelist.xml file structure**

An example gamelist.xml entry for the Commodore 64 game Popeye:

```xml
<?xml version="1.0"?>
<gameList>
    <game>
        <path>./cartridge/Popeye/Popeye.crt</path>
        <name>Popeye</name>
        <desc>Popeye is a conversion of the arcade action/platform game.</desc>
        <rating>0.7</rating>
        <releasedate>19860101T000000</releasedate>
        <developer>Parker Brothers</developer>
        <publisher>Nintendo</publisher>
        <genre>Action</genre>
        <players>1-2</players>
        <favorite>true</favorite>
    </game>
</gameList>
```

Everything is enclosed in a `<gameList>` tag, and the information for each game or folder is enclosed in a corresponding `<game>` or `<folder>` tag. Each piece of metadata is encoded as a string.

**gamelist.xml reference**

There are a few different data types for the metadata which the string values in the gamelist.xml files are converted to during file loading:

* `string` - just text
* `float` - a floating-point decimal value (written as a string)
* `integer` - an integer value (written as a string)
* `datetime` - a date and optionally a time encoded as an ISO string in the format "%Y%m%dT%H%M%S",  for example "19950311T000000"
* `bool` - a true or false value

Some metadata is also marked as "statistic", these are kept track of by ES-DE and do not show up in the metadata editor. They are shown in certain views (for example, the detailed view and the video view both show `lastplayed`, although the label can be disabled by the theme).

There are two basic categories of metadata, `game` and `folders` and the metdata tags for these are described in detail below.

**\<game\>**
* `path` - string, the path to the game file, either relative to the %ROMPATH% variable or as an absolute path on the filesystem
* `name` - string, the displayed name for the game
* `sortname` - string, used for sorting the system gamelist, instead of using `name`
* `collectionsortname` - string, used for sorting the gamelist for custom collections, instead of using `name` or `sortname`
* `desc` - string, a description of the game, longer descriptions will automatically scroll, so don't worry about the size
* `rating` - float, the rating for the game, expressed as a floating point number between 0 and 1. Fractional values will be rounded to 0.1 increments (half-stars) during processing
* `releasedate` - datetime, the date the game was released, displayed as date only, time is ignored
* `developer` - string, the developer for the game
* `publisher` - string, the publisher for the game
* `genre` - string, the genre(s) for the game
* `players` - integer, the number of players the game supports
* `favorite` - bool, indicates whether the game is a favorite
* `completed` - bool, indicates whether the game has been completed
* `kidgame` - bool, indicates whether the game is suitable for children, as used by the `Kid' UI mode
* `hidden` - bool, indicates whether the game is hidden
* `broken` - bool, indicates a game that doesn't work (useful for MAME)
* `nogamecount` - bool, indicates whether the game should be excluded from the game counter and the automatic and custom collections
* `nomultiscrape` - bool, indicates whether the game should be excluded from the multi-scraper
* `hidemetadata` - bool, indicates whether to hide most of the metadata fields when displaying the game in the gamelist view
* `playcount` - integer, the number of times this game has been played
* `controller` - string, used to display controller badges
* `altemulator` - string, overrides the emulator/launch command on a per game basis
* `lastplayed` - statistic, datetime, the last date and time this game was played

For folders, most of the fields are identical although some are removed. In the list below, the fields with identical function compared to the game files described above have been left without a description.

**\<folder\>**
* `path`
* `name`
* `desc`
* `rating`
* `releasedate`
* `developer`
* `publisher`
* `genre`
* `players`
* `favorite`
* `completed`
* `hidden`
* `broken`
* `nomultiscrape`
* `hidemetadata`
* `controller`
* `folderlink` - string, points to a file inside the game's folder structure that will be launched instead of entering the folder
* `lastplayed` - statistic, datetime, for folders this is inherited by the latest game file launched inside the folder.

**Additional gamelist.xml information**

* If a value matches the default for a particular piece of metadata, ES-DE will not write it to the gamelist.xml file (for example, if `genre` isn't specified, an empty genre tag will not be written)

* A `game` can actually point to a folder/directory if the folder has a matching extension, although this is exceedingly rare

* The `folder` metadata will only be used if a game file is found inside that folder, as empty folders will be skipped during startup even if they have metadata values defined for themselves

* ES-DE will keep entries for games and folders that it can't find the game/ROM files for, i.e. it will not clean up the gamelist.xml files automatically when game files are missing

* The switch `--gamelist-only` can be used to skip automatic searching, and only display games defined in the gamelist.xml files

* The switch `--ignore-gamelist` can be used to ignore the gamelist upon start of the application (mostly useful for debugging purposes)

## Debug mode

By passing the --debug command line option, ES-DE will increase the logging to include a lot of additional debug output which is useful both for development and in order to pinpoint issues as a user.
In addition to this extra logging, a few key combinations are enabled when in debug mode. These are useful both for working on ES-DE itself as well as for theme developers.

**Ctrl + i**

This will draw a semi-transparent red frame behind all image and animation components. It will also draw a green frame around the carousel and grid.

**Ctrl + t**

This will draw a semi-transparent blue frame around all text components. It will also draw a green frame around the textlist.

**Ctrl + g**

This option only applies to menus, where it will render a grid on the user interace. Note that any open screen needs to be closed and reopened again after using the key combination in order for it to have any effect.

**Ctrl + r**

This will reload either a single gamelist or all gamelists depending on where you're located when entering the key combination (go to the system view to make a complete reload). Very useful for theme development as any changes to the theme files will be activated without requiring an application restart. Note that the menu needs to be closed for this key combination to have any effect.

By default all controller input (keyboard and controller button presses) will be logged when the --debug flag has been passed. To disable the input logging, the setting DebugSkipInputLogging kan be set to false in the es_settings.xml file. There is no menu entry to change this as it's intended for developers and not for end users.

## Adding custom controller profiles

Before attempting to add a custom profile for your controller you need to check whether there is device driver support for it in your operating system. If the controller works in other applications and games, then proceed with the instructions below, but if it doesn't work anywhere else then chances are very low that you can get it to work in ES-DE.

ES-DE uses the [SDL](https://www.libsdl.org) (Simple DirectMedia Layer) library to handle controller input, so in order for a controller to work in ES-DE, it has to be supported by SDL. There is however a possibility to add custom controller profiles to SDL which in some cases could enable devices in ES-DE that would otherwise not be supported. This is generally a temporary solution though, as controller support is constantly getting improved natively in SDL. As a first step it's therefore recommended to open a request at the SDL [issue tracker](https://github.com/libsdl-org/SDL/issues) to have your specific controller added to a future SDL release.

Assuming the controller works in other applications than ES-DE, you can attempt to add a custom profile by creating the file `~/.emulationstation/es_controller_mappings.cfg` and enter the appropriate configuration inside this file.

The required format is described here:\
https://github.com/gabomdq/SDL_GameControllerDB

The really blunt approach is to copy the entire content of the following file into es_controller_mappings.cfg: \
https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt

But just do this as a first step to see whether you controller gets enabled. If it does, then you should remove all entries that are not relevant. That is important as this file will take precedence over the built-in controller profiles in the SDL library, so any future controller bug fixes and similar would not apply. In the past the gamecontrollerdb.txt file has also included some invalid configuration entries, so even though it may make your controller work, it could actually break some other controllers that you may want to use now or in the future.

Therefore only keep the entries in the es_controller_mappings.cfg file that are relevant for your devices. You can find each relevant controller GUID by starting ES-DE and then looking in the ~/.emulationstation/es_log.txt file. You should see entries such as the following:
```
May 16 18:26:17 Info:   Added controller with custom configuration: "X360 Controller" (GUID: 030000005e0400008e02000010010000, instance ID: 0, device index: 0)
```

It's the GUID that is the key, and it's the lines matching these IDs that you want to retain inside the es_controller_mappings.cfg file. All other rows can be deleted.

Even if pasting the entire content of gamecontrollerdb.txt into the es_controller_mappings.cfg file did not enable your controller, all hope is not lost. You may still be able to create your own custom controller entry, but doing that is beyond the scope of this document and you would have to look into the instructions at the SDL_GameControllerDB URL mentioned above.

As a final note, this configuration file can also be used for the opposite purpose, i.e. to blacklist devices so that they will not work inside ES-DE. Some wireless controllers with buggy drivers will register as two devices meaning every button press will be registered twice. In this situation, blacklisting one of these entries will make the controller behave correctly. Although it's possible to enable the _Only accept input from first controller_ menu option as a workaround, this will completely ignore all other controllers which may not be what you want. To blacklist a controller, follow the same procedure described above but leave the button configuration blank for the GUID entry.

## Portable installation on Windows

_As there is a preconfigured portable release available for Windows, this section is mostly relevant for understanding how the setup works, as well as to provide information on how to make customizations._

It's possible to easily create a portable installation of ES-DE on Windows, for example to place on a USB memory stick.

For this example, let's assume that the removable media has the device name `F:\`

These are the steps to perform:

* Install ES-DE
* Copy the EmulationStation-DE installation directory to F:\
* Create a directory named F:\EmulationStation-DE\Emulators
* Copy your emulator directories to F:\EmulationStation-DE\Emulators\
* Copy your ROMs directory to F:\EmulationStation-DE\
* Create an empty file named portable.txt in F:\EmulationStation-DE\

You should end up with something like this:
```
F:\EmulationStation-DE\
F:\EmulationStation-DE\Emulators\dosbox-staging\
F:\EmulationStation-DE\Emulators\RetroArch-Win64\
F:\EmulationStation-DE\Emulators\RPCS3\
F:\EmulationStation-DE\Emulators\xemu\
F:\EmulationStation-DE\ROMs\
F:\EmulationStation-DE\portable.txt
```

This is just an example as you may of course not use these specific emulators. There are also many more emulators supported as detailed in the `es_find_rules.xml` configuration file. As well there will be many more files and directories than those listed above inside the F:\EmulationStation-DE folder.

How the portable setup works is that when ES-DE finds a file named portable.txt in its executable directory, it will by default locate the .emulationstation directory directly inside this folder. It's also possible to modify portable.txt with a path relative to the ES-DE executable directory. For instance if two dots `..` are placed inside the portable.txt file, then the .emulationstation directory will be located in the parent folder, which would be directly under F:\ for this example.

If the --home command line parameter is passed when starting ES-DE, that will override the portable.txt file.

Start ES-DE from the F:\ device and check that everything works as expected. Just be aware that some emulators may not play that well with a portable setup and may store their configuration files in your home directory (probably on your C: drive) or at some other location. So when using the portable installation on another computer you may need to perform some additional emulator-specific setup.

Following this, optionally copy any existing gamelist.xml files, game media files etc. to the removable media. For example:

```
F:\EmulationStation-DE\.emulationstation\collections\
F:\EmulationStation-DE\.emulationstation\downloaded_media\
F:\EmulationStation-DE\.emulationstation\gamelists\
```

You could alternatively copy over your entire .emulationstation directory, but in this case make sure that you have no settings in es_settings.xml that point to a specific location on your local filesystem, such as the game ROMs or game media directories.

You now have a fully functional portable retrogaming installation!

The portable installation works exactly as a normal installation, i.e. you can use the built-in scraper, edit metadata, launch games etc.

Just make sure to not place the portable installation on a network share that uses the Microsoft SMB protocol and run it from there as this will lead to unacceptably poor application performance.

## Custom event scripts

There are numerous locations throughout ES-DE where custom scripts can be executed if the option to do so has been enabled in the settings. You'll find the option _Enable custom event scripts_ on the Main menu under _Other settings_. By default this setting is deactivated so make sure to enable it to use this feature.

The approach is quite straightforward, ES-DE will look for any files inside a script directory that corresponds to the event that is triggered and will then execute all these files. If you want to have the scripts executed in a certain order you can name them accordingly as they will be sorted and executed in lexicographic order. The sorting is case-sensitive on Unix/Linux and case-insensitive on macOS and Windows. ES-DE will wait for each script to finish its execution before moving on to the next one, so the application will suspend briefly when whatever the script is doing is executing. If you want to avoid this you can setup a wrapper script that executes another script outside the ES-DE scripts directory as a background process. Refer to your operating system documentation on how to accomplish this.

There are up to four parameters that will be passed to these scripts, as detailed below:

| Event                    | Parameters*                                        | Description                                                                 |
| :----------------------- | :------------------------------------------------- | :-------------------------------------------------------------------------- |
| startup                  |                                                    | Application startup                                                         |
| quit                     |                                                    | Application quit/shutdown                                                   |
| reboot                   |                                                    | System reboot (quit event triggered as well)                                |
| poweroff                 |                                                    | System power off (quit event triggered as well)                             |
| config-changed           |                                                    | On saving application settings or controller configuration                  |
| settings-changed         |                                                    | On saving application settings (config-changed event triggered as well)     |
| controls-changed         |                                                    | On saving controller configuration (config-changed event triggered as well) |
| theme-changed            | New theme name, old theme name                     | When manually changing theme sets in the UI Settings menu                   |
| game-start               | ROM path, game name, system name, system full name | On game launch                                                              |
| game-end                 | ROM path, game name, system name, system full name | On game end (or on application wakeup if running in the background)         |
| screensaver-start        | _timer_ or _manual_                                | Screensaver started via timer or manually                                   |
| screensaver-end          | _cancel_ or _game-jump_ or _game-start_            | Screensaver ended via cancellation, jump to game or start/launch of game    |

***)** Parameters in _italics_ are literal strings.

We'll go through two examples:
* Creating a log file that will record the start and end time for each game we play, letting us see how much time we spend on retro-gaming
* Changing the system resolution when launching and returning from a game in order to run the emulator at a lower resolution than ES-DE

The following examples are for Unix systems, but it works the same way on macOS and Windows (although .bat batch files are used on Windows instead of shell scripts and any spaces in the parameters are not escaped as is the case on Unix and macOS).

As can be seen in the table above, the events executed when a game starts and ends are named _game-start_ and _game-end_

So let's create the folders for these events in the scripts directory. The location is `~/.emulationstation/scripts`

**Game log**

After creating the directories, we need to create the scripts to log the actual game launch and game ending. The parameters that are passed to the scripts vary depending on the type of event, but for these events the four parameters are the absolute path to the game file, the game name as shown in the gamelist view, the system name and the full system name.

Let's name the start script `game_start_logging.sh` with the following contents:

```
#!/bin/bash
TIMESTAMP=$(date +%Y-%m-%d' '%H:%M:%S)
echo Starting game "\""${2}"\"" "\""${4}"\"" "(\""${1}"\")" at $TIMESTAMP >> ~/.emulationstation/game_playlog.txt
```

And let's name the end script `game_end_logging.sh` with the following contents:

```
#!/bin/bash
TIMESTAMP=$(date +%Y-%m-%d' '%H:%M:%S)
echo "Ending game  " "\""${2}"\"" "\""${4}"\"" "(\""${1}"\")" at $TIMESTAMP >> ~/.emulationstation/game_playlog.txt
```

After creating the two scripts, you should have something like this on the filesystem:

```
~/.emulationstation/scripts/game-start/game_start_logging.sh
~/.emulationstation/scripts/game-end/game_end_logging.sh
```

Don't forget to make the scripts executable (e.g. "chmod 755 ./game_start_logging.sh").

If we now start ES-DE with the debug flag and launch a game, something like the following should show up in es_log.txt:

```
Aug 05 14:19:24 Debug:  Scripting::fireEvent(): game-start "/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip" "The Legend Of Zelda" "nes" "Nintendo Entertainment System"
Aug 05 14:19:24 Debug:  Executing: /home/myusername/.emulationstation/scripts/game-start/game_start_logging.sh "/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip" "The Legend Of Zelda" "nes" "Nintendo Entertainment System"
.
.
Aug 05 14:27:15 Debug:  Scripting::fireEvent(): game-end "/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip" "The Legend Of Zelda" "nes" "Nintendo Entertainment System" ""
Aug 05 14:27:15 Debug:  Executing: /home/myusername/.emulationstation/scripts/game-end/game_end_logging.sh "/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip" "The Legend Of Zelda" "nes" "Nintendo Entertainment System"

```

Finally after running some games, ~/.emulationstation/game_playlog.txt should contain something like the following:

```
Starting game "The Legend Of Zelda" "Nintendo Entertainment System" ("/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip") at 2020-08-05 14:19:24
Ending game   "The Legend Of Zelda" "Nintendo Entertainment System" ("/home/myusername/ROMs/nes/Legend\ of\ Zelda,\ The.zip") at 2020-08-05 14:27:15
Starting game "Quake" "Ports" ("/home/myusername/ROMs/ports/Quakespasm/quakespasm.sh") at 2020-08-05 14:38:46
Ending game   "Quake" "Ports" ("/home/myusername/ROMs/ports/Quakespasm/quakespasm.sh") at 2020-08-05 15:13:58
Starting game "Pirates!" "Commodore 64" ("/home/myusername/ROMs/c64/Multidisk/Pirates/Pirates!.m3u") at 2020-08-05 15:15:24
Ending game   "Pirates!" "Commodore 64" ("/home/myusername/ROMs/c64/Multidisk/Pirates/Pirates!.m3u") at 2020-08-05 15:17:11
```

**Resolution changes**

The same directories are used as for the above example with the game log.

First create the game start script, let's name it `set_resolution_1080p.sh` with the following contents:

```
#!/bin/sh
xrandr -s 1920x1080
```

Then create the end script, which we'll name `set_resolution_4K.sh`:

```
#!/bin/sh
xrandr -s 3840x2160
sleep 0.3
xdotool search --class emulationstation windowactivate
```

The last two lines are optional, they're used to set the focus back to ES-DE in case you're running attention-seeking applications such as Kodi which may steal focus after resolution changes. You may need to adjust the sleep time to get this to work reliably though, as the timing may differ between different computers and graphics drivers.

After creating the two scripts, you should have something like this on the filesystem:

```
~/.emulationstation/scripts/game-start/set_resolution_1080p.sh
~/.emulationstation/scripts/game-end/set_resolution_4K.sh
```

Don't forget to make the scripts executable (e.g. "chmod 755 ./set_resolution_1080p.sh").

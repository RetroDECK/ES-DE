# EmulationStation Desktop Edition (ES-DE) v1.2 (development version) - Building and advanced configuration

**Note:** This is a quite technical document intended for those that are interested in compiling ES-DE from source code, or would like to customize the configuration. If you just want to start using the software, check out [USERGUIDE-DEV.md](USERGUIDE-DEV.md) instead.

Also note that this document is only relevant for the current ES-DE development version, if you would like to see the documentation for the latest stable release, refer to [INSTALL.md](INSTALL.md) instead.

Table of contents:

[[_TOC_]]

## Development Environment

ES-DE is developed and compiled using Clang/LLVM and GCC on Unix, Clang/LLVM on macOS and MSVC and GCC (MinGW) on Windows.

CMake is the build system for all the supported operating systems, used in conjunction with `make` on Unix and macOS and `nmake` and `make` on Windows. Xcode on macOS or Visual Studio on Windows are not required for building ES-DE and they have not been used during the development. The only exception is notarization of codesigned macOS packages which require the `altool` and `stapler` binaries that come bundled with Xcode.

For automatic code formatting [clang-format](https://clang.llvm.org/docs/ClangFormat.html) is used.

Any code editor can be used of course, but I recommend [VSCode](https://code.visualstudio.com).


## Building on Unix

There are some dependencies that need to be fulfilled in order to build ES-DE. These are detailed per operating system below.

**Debian/Ubuntu**

All of the required packages can be installed with apt-get:
```
sudo apt-get install build-essential clang-format git cmake libsdl2-dev libavcodec-dev libavfilter-dev libavformat-dev libavutil-dev libfreeimage-dev libfreetype6-dev libcurl4-openssl-dev libpugixml-dev rapidjson-dev libasound2-dev libgl1-mesa-dev
```

If building with the optional VLC video player, the following packages are also needed:
```
sudo apt-get install vlc libvlc-dev
```

**Fedora**

Use dnf to install all the required packages:
```
sudo dnf install gcc-c++ clang-tools-extra cmake SDL2-devel ffmpeg-devel freeimage-devel freetype-devel curl-devel pugixml-devel rapidjson-devel alsa-lib-devel mesa-libGL-devel
```

If building with the VLC video player, add the RPM Fusion repository.

Go to [https://rpmfusion.org/Configuration](https://rpmfusion.org/Configuration) and download the .rpm package for the free repository. Then install it, followed by VLC:

```
sudo dnf install ./rpmfusion-free-release-33.noarch.rpm
sudo dnf install vlc vlc-devel
```

**Manjaro**

Use pacman to install all the required packages:

```
sudo pacman -S gcc clang make cmake pkgconf sdl2 ffmpeg freeimage freetype2 pugixml rapidjson
```

If building with the optional VLC video player, the following package is also needed:
```
sudo pacman -S vlc
```

**Raspberry Pi OS (Raspian)**

Note: The Raspberry Pi 4 is the minimum recommended model to use with ES-DE. As this type of device is quite weak and because the FFmpeg video player does not support hardware decoding on this platform, it's strongly adviced to build with the VLC player, which is hardware accelerated.

All of the required packages can be installed with apt-get:
```
sudo apt-get install clang-format cmake libsdl2-dev libavcodec-dev libavfilter-dev libavformat-dev libavutil-dev libfreeimage-dev libcurl4-gnutls-dev libpugixml-dev rapidjson-dev
```

If building with the optional VLC video player, the following packages are also needed:
```
sudo apt-get install vlc libvlc-dev
```

To build with CEC support you also need to install these packages:
```
sudo apt-get install libcec-dev libp8-platform-dev
```

**FreeBSD**

Use pkg to install the dependencies:
```
pkg install git pkgconf cmake sdl2 ffmpeg freeimage pugixml rapidjson
```

If building with the optional VLC video player, the following package is also needed:
```
pkg install vlc
```

Clang/LLVM and cURL should already be included in the base OS installation.

**NetBSD**

Use pkgin to install the dependencies:
```
pkgin install clang git cmake pkgconf SDL2 ffmpeg4 freeimage pugixml rapidjson
```

If building with the optional VLC video player, the following package is also needed:
```
pkgin vlc
```

NetBSD ships with GCC by default, and although you should be able to use Clang/LLVM, it's probably easier to just stick to the default compiler environment. The reason why the clang package needs to be installed is to get clang-format onto the system.

**OpenBSD**

Use pkg_add to install the dependencies:
```
pkg_add clang-tools-extra cmake pkgconf sdl2 ffmpeg freeimage
```

If building with the optional VLC video player, the following package is also needed:
```
pkg_add vlc
```

In the same manner as for FreeBSD, Clang/LLVM and cURL should already be installed by default.

RapidJSON is not part of the OpenBSD ports/package collection as of v6.8, so you need to compile it yourself. At the time of writing, the latest release v1.1.0 does not compile on OpenBSD, so you need to use the master branch:

```
git clone https://github.com/Tencent/rapidjson.git
cd rapidjson
cmake .
make
make install
```

Pugixml does exist in the package collection but somehow this version is not properly detected by CMake, so you need to compile this manually as well:

```
git clone git://github.com/zeux/pugixml.git
cd pugixml
git checkout v1.10
cmake .
make
make install
```

**Cloning and compiling ES-DE**

To clone the source repository, run the following:

```
git clone https://gitlab.com/leonstyhre/emulationstation-de.git
```

Then generate the Makefile and build the software:

```
cd emulationstation-de
cmake .
make
```

By default the master branch will be used, which is where development takes place. To instead build a stable release, switch to the `stable-x.x` branch for the version, for example:

```
cd emulationstation-de
git checkout stable-1.1
cmake .
make
```

To build ES-DE with the VLC video player in addition to the default FFmpeg player, enable the VLC_PLAYER option, for example:
```
cmake -DVLC_PLAYER=on .
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

As for more advanced debugging, Valgrind is a very powerful and useful tool which can analyze many aspects of the application. Be aware that some of the Valgrind tools should be run with an optimized build, and some with optimizations turned off. Refer to the Valgrind documentation for more information.

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

To build ES-DE with CEC support, enable the corresponding option, for example:

```
cmake -DCEC=on .
make
```
You will most likely need to install additional packages to get this to build. On Debian-based systems these are _libcec-dev_ and _libp8-platform-dev_. Note that the CEC support is currently untested.

To build with the GLES renderer, run the following:
```
cmake -DGLES=on .
make
```
The GLES renderer is quite limited as there is no shader support for it, so ES-DE will definitely not look as pretty as when using the default OpenGL renderer. When building on a Raspberry Pi, the GLES renderer will be automatically selected.

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

Both Clang/LLVM and GCC work fine for building ES-DE.

I did some small benchmarks comparing Clang 10.0 to GCC 9.3.0 with the ES-DE v1.1 codebase on an Intel Xeon W-2245 @ 3.90GHz running Kubuntu 20.04.2 LTS and it's pretty interesting.

Advantages with Clang (vs GCC):
* 8% smaller binary size for a release build
* 31% smaller binary size for a debug build
* 16% faster compile time for a release build
* 25% faster compile time for a debug build
* 13% faster application startup time for a release build
* 4% faster application startup time for a debug build

*Release build: Optimizations enabled, debug info disabled, binary stripped.* \
*Debug build: Optimizations disabled, debug info enabled, binary not stripped.*

This Clang debug build is LLVM "native", i.e. intended to be debugged using the LLVM project debugger LLDB. The problem is that this is still not well integrated with VSCode that I use for development so I need to keep using GDB. But this is problematic as the libstd++ data required by GDB is missing in the binary, making it impossible to see the values of for instance std::string variables.

It's possible to activate the additional debug info needed by GDB by using the flag `-D_GLIBCXX_DEBUG`. I've added this to CMakeLists.txt when using Clang, but this bloats the binary and makes the code much slower. Actually, instead of a 4% faster application startup, it's now 25% slower. The same goes for the binary size, instead of 31% smaller it's now 5% larger. The compilation time is still less than GCC but only by 10% instead of 25%.

But I'm expecting this issue to be resolved in the future so the workaround can be removed.

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
CPack: - package: /home/myusername/emulationstation-de/emulationstation-de-1.1.0-x64.deb generated.
```

You may want to check that the dependencies look fine, as they're (mostly) automatically generated by CMake:

```
dpkg -I ./emulationstation-de-1.1.0-x64.deb
```

The package can now be installed using a package manager, for example apt:

```
sudo apt install ./emulationstation-de-1.1.0-x64.deb
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
CPack: - package: /home/myusername/emulationstation-de/emulationstation-de-1.1.0-x64.rpm generated.
```

On Fedora, you need to install rpmbuild before this command can be run:
```
sudo dnf install rpm-build
```

After the package generation you can check that the metadata looks fine using the `rpm` command:
```
rpm -qi ./emulationstation-de-1.1.0-x64.rpm
```

To see the automatically generated dependencies, run this:
```
rpm -q --requires ./emulationstation-de-1.1.0-x64.rpm
```

And of course, you can also install the package:
```
sudo dnf install ./emulationstation-de-1.1.0-x64.rpm
```

## Building on macOS

EmulationStation for macOS is built using Clang/LLVM which is the default compiler for this operating system. It's pretty straightforward to build software on this OS. The main problem is that there is no native package manager, but as there are several third party package managers available, this can be partly compensated for. The use of one of them, [Homebrew](https://brew.sh), is detailed below.

As for code editing, I use [VSCode](https://code.visualstudio.com). I suppose Xcode could be used instead but I have no experience with this tool and no interest in it as I want to use the same tools for all the operating systems that I develop on.

**Setting up the build tools:**

Install the Command Line Tools which include Clang/LLVM, Git, make etc. Simply open a terminal and enter the command `clang`. This will open a dialog that will let you download and install the tools.

Following this, install the Homebrew package manager which will greatly simplify the rest of the installation. Install it by runing the following in a terminal window:
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
```
Be aware that Homebrew can be really slow, especially when it compiles packages from source code.

**Package installation with Homebrew:**

Install the required tools and dependencies:

```
brew install clang-format cmake pkg-config nasm fdk-aac libvpx sdl2 freeimage freetype pugixml rapidjson
```

If building with the optional VLC video player, then run this as well:

```
brew install --cask vlc
```

**Compiling FFmpeg:**

The FFmpeg build distributed via Homebrew has a lot of dependencies we don't need, and which would make it very difficult to package the application. Instead we will build this software with only some limited options:

```
git clone https://github.com/FFmpeg/FFmpeg.git
cd FFmpeg
git checkout n4.4
./configure --prefix=/usr/local --enable-gpl --enable-nonfree --enable-shared --enable-libfdk-aac --enable-libvpx
make
sudo make install
```

**Some additional/optional steps:**

Enable developer mode to avoid annoying password requests when attaching the debugger to a process:
```
sudo /usr/sbin/DevToolsSecurity --enable
```
It makes me wonder who designed this functionality and what was their thinking, I've never seen anything like this on any of the other systems I've been developing on.

If required, define SDKROOT. This is only needed if during compilation you get error messages regarding missing include files. Running the following will properly setup the development environment paths:
```
export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
```

I suppose you should add this to your shell profile file or similar, but I didn't have to do this step so I'm not sure.

**Cloning and compiling:**

To clone the source repository, run the following:

```
git clone https://gitlab.com/leonstyhre/emulationstation-de.git
```

Then generate the Makefile and build the software:

```
cd emulationstation-de
cmake .
make
```

By default the master branch will be used, which is where development takes place. To instead build a stable release, switch to the `stable-x.x` branch for the version, for example:

```
cd emulationstation-de
git checkout stable-1.1
cmake .
make
```

To build ES-DE with the VLC video player in addition to the default FFmpeg player, enable the VLC_PLAYER option:
```
cmake -DVLC_PLAYER=on .
make
```

To generate a debug build, run this:
```
cmake -DCMAKE_BUILD_TYPE=Debug .
make
```
Keep in mind that the debug version will be much slower due to all compiler optimizations being disabled.

Running `make -j6` (or whatever number of parallel jobs you prefer) speeds up the compilation time if you have cores to spare.

After building ES-DE and trying to execute the application, there could be issues with finding the dynamic link libraries for VLC (assuming VLC was enabled for the build) as these are not installed into a standard location but rather into the /Applications folder. As such, you may need to set the DYLD_LIBRARY_PATH environmental variable to find the VLC libraries. Note that this is not intended or required for the release build that will be shipped in a DMG installer or if you manually install ES-DE using _make install_. It's only needed to be able to run the binary from the build directory. You should add this to your shell profile file to avoid having to set it each time you open a new terminal window:
```
export DYLD_LIBRARY_PATH=/Applications/VLC.app/Contents/MacOS/lib
```

Running ES-DE from the build directory may be a bit flaky as there is no Info.plist file available which is required for setting the proper window mode and such. It's therefore recommended to run the application from the installation directory for any more in-depth testing. But normal debugging can of course be done from the build directory.

Be aware that the approach taken for macOS has the limitation that you can't build for previous operating system versions. You can certainly set CMAKE_OSX_DEPLOYMENT_TARGET to whatever version you like, but the problem is that the Homebrew libraries will most likely not work on earlier macOS versions. In theory this can be worked around by building all these libraries yourself with a lower deployment target, but it's hardly worth the effort. It's better to build on the lowest OS version that should be supported and rely on forward compatibility.

**Code signing:**

Due to the Apple notarization requirement implemented as of macOS 10.14.5 a build with simple code signing is needed for versions up to 10.13 and another build with both code signing and notarization is required for version 10.14 and higher.

macOS code signing is beyond the scope of this document, but the option MACOS_CODESIGN_IDENTITY is used to specify the code signing certificate identity, for example:
```
cmake -DMACOS_CODESIGN_IDENTITY="My Name" .
```

Assuming the code signing ceritificate is properly setup in Keychain Access, the process will be automatic and the resulting DMG package can be notarized as-is.

**Legacy build:**

Normally ES-DE is meant to be built for macOS 10.14 and higher, but a legacy build for earlier operating system versions can be enabled. This has been tested with a minimum version of 10.11. It's unclear if it works with even older macOS versions.

To enable a legacy build, change the CMAKE_OSX_DEPLOYMENT_TARGET variable in CMakeLists.txt from 10.14 to whatever version you would like to build for. This will disable Hardened Runtime if signing is enabled and it will add 'legacy' to the DMG installer file name when running CPack.

You also need to modify es-app/assets/EmulationStation-DE_Info.plist and set the key SMinimumSystemVersion to the version you're building for.

Due to issues with getting FFmpeg to compile on some older macOS versions, ES-DE v1.0.1 is the last version where a legacy build has been tested.

**Installing:**

As macOS does not have any package manager which would have handled the library dependencies, we need to bundle the required shared libraries with the application. Copy the following .dylib files from their respective installation directories to the emulationstation-de build directory:

```
libavcodec.58.dylib
libavfilter.7.dylib
libavformat.58.dylib
libavutil.56.dylib
libpostproc.55.dylib
libswresample.3.dylib
libswscale.5.dylib
libfdk-aac.2.dylib
libSDL2-2.0.0.dylib
libfreeimage.dylib
libfreetype.6.dylib
libpng16.16.dylib
libvlc.dylib            (only if building with the VLC video player)
libvlccore.dylib        (only if building with the VLC video player)
```

All except the VLC libraries should be located in /usr/local/lib. The VLC libraries should be located in /Applications/VLC.app/Contents/MacOS/lib/

Note that the filenames could be slightly different depending on what versions you have installed on your system.

After copying the libraries to the build directory, set their permissions like this:
```
chmod 755 ./*.dylib
```

There are some secondary internal dependencies between some of these library files, and these are baked into the files as absolute paths. As such we need to rewrite these to rpaths (relative paths) which is done using the install_name_tool command.

A script is available to automate this: `tools/macOS_change_dylib_rpaths.sh`

Simply run the following:
```
cd emulationstation-de
tools/macOS_change_dylib_rpaths.sh
Found file libfreetype.6.dylib - changing to rpaths
Found file libavcodec.58.dylib - changing to rpaths
Found file libavfilter.7.dylib - changing to rpaths
Found file libavformat.58.dylib - changing to rpaths
Found file libpostproc.55.dylib - changing to rpaths
Found file libswresample.3.dylib - changing to rpaths
Found file libswscale.5.dylib - changing to rpaths
```

Verify that it worked as expected by running the otool command, for example `otool -L libfreetype.6.dylib` should show something like the following:

```
libfreetype.6.dylib:
	/usr/local/opt/freetype/lib/libfreetype.6.dylib (compatibility version 24.0.0, current version 24.2.0)
	/usr/lib/libbz2.1.0.dylib (compatibility version 1.0.0, current version 1.0.5)
	@rpath/libpng16.16.dylib (compatibility version 54.0.0, current version 54.0.0)
	/usr/lib/libz.1.dylib (compatibility version 1.0.0, current version 1.2.5)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)
```

It's unclear why the first line shows a reference to itself, and this line apparently can't be modified using the install_name_tool command. It doesn't matter though and the application will work fine even if this path does not exist on the system.

You of course only need to change the absolute paths to rpaths once, well at least until you replace the libraries in case of moving to a newer version or so.

In addition to these libraries, if building with the optional VLC video player, you need to create a `plugins` directory and copy over the following libraries, which are normally located in `/Applications/VLC.app/Contents/MacOS/plugins/`:

```
libadummy_plugin.dylib
libamem_plugin.dylib
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

On macOS you can install the application as a normal user, i.e. no root privileges are required. Simply run the following:

```
make install
```

This will be the directory structure for the installation (the VLC-related files are optional):

```
/Applications/EmulationStation Desktop Edition.app/Contents/Info.plist
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/EmulationStation
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libSDL2-2.0.0.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libavcodec.58.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libavfilter.7.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libavformat.58.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libavutil.56.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libfdk-aac.2.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libfreeimage.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libfreetype.6.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libpng16.16.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libpostproc.55.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libswresample.3.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libswscale.5.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libvlc.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libvlccore.dylib
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

**Creating a .dmg installer:**

Simply run `cpack` to build a .dmg disk image/installer:

```
myusername@computer:~/emulationstation-de$ cpack
CPack: Create package using Bundle
CPack: Install projects
CPack: - Run preinstall target for: emulationstation-de
CPack: - Install project: emulationstation-de []
CPack: Create package
CPack: - package: /Users/myusername/emulationstation-de/EmulationStation-DE-1.1.0-x64.dmg generated.
```

Generating .dmg installers on older version of macOS seems to make them forward compatible to a pretty good extent, for instance building on El Capitan seems to generate an application that is usable on Catalina and Big Sur. The other way around is not true due to the use of dependencies from the Homebrew repository.

**Special considerations regarding run-paths:**

Even after considerable effort I've been unable to make CMake natively set correct rpaths for the EmulationStation binary on macOS. Therefore a hack/workaround is in place that uses install_name_tool to change absolute paths to rpaths for most of the bundled libraries.

This is certainly not perfect as the versions of the libraries are hardcoded inside es-app/CMakeLists.txt. Therefore always check that all the rpaths are set correctly if you intend to create a .dmg image that will be used on other computers than your own.

Simply run `otool -L EmulationStation` and verify that the result looks something like this:

```
./EmulationStation:
        /usr/lib/libcurl.4.dylib (compatibility version 7.0.0, current version 9.0.0)
        @rpath/libavcodec.58.dylib (compatibility version 58.0.0, current version 58.134.100)
        @rpath/libavfilter.7.dylib (compatibility version 7.0.0, current version 7.110.100)
        @rpath/libavformat.58.dylib (compatibility version 58.0.0, current version 58.76.100)
        @rpath/libavutil.56.dylib (compatibility version 56.0.0, current version 56.70.100)
        @rpath/libfreeimage.dylib (compatibility version 3.0.0, current version 3.18.0)
        @rpath/libfreetype.6.dylib (compatibility version 24.0.0, current version 24.4.0)
        @rpath/libSDL2-2.0.0.dylib (compatibility version 15.0.0, current version 15.0.0)
        /System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa (compatibility version 1.0.0, current version 23.0.0)
        @rpath/libvlc.dylib (compatibility version 12.0.0, current version 12.0.0)
        /System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL (compatibility version 1.0.0, current version 1.0.0)
        /usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 400.9.4)
        /usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1252.250.1)
```

If any of the lines that should start with @rpath instead has an absolute path, then you have a problem and need to modify the install_name_tools parameters in es-app/CMakeLists.txt.

This is what an incorrect line would look like:

`/usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib (compatibility version 13.0.0, current version 13.0.0)`


## Building on Windows

Both MSVC and MinGW (GCC) work fine for building ES-DE on Windows.

Although I would prefer to exclude support for MSVC, this compiler simply works much better when developing as it's much faster than MinGW when linking debug builds and when actually debugging. But for release builds MinGW is very fast and ES-DE starts around 18% faster when built with MinGW meaning this compiler probably generates more efficient code overall. As well MSVC requires a lot more junk DLL files to be distributed with the application so it's not a good candidate for the final release build.

For this reason I think MSVC makes sense for development and MinGW for the releases.

**MSVC setup:**

Install Git for Windows: \
[https://gitforwindows.org](https://gitforwindows.org)

Download the Visual Studio Build Tools (choose Visual Studio Community edition): \
[https://visualstudio.microsoft.com/downloads](https://visualstudio.microsoft.com/downloads)

It seems as if Microsoft has dropped support for installing the Build Tools without the Visual Studio IDE, at least I've been unable to find a way to exclude it. But I just pretend it's not installed and use VSCode instead which works perfectly fine.

During installation, choose the Desktop development with C++ workload with the following options (version details excluded):

```
MSVC and x64/x86 build tools
Windows 10 SDK
Just-In-Time debugger
C++ CMake tools for Windows
```

If you intend to use both MinGW and MSVC on the same machine, it's probably better to exclude CMake and install it manually as described in the MinGW setup instructions below.

The way the MSVC environment works is that a specific developer shell is provided where the build environment is properly configured. You open this from the Start menu via `Visual Studio 2019` -> `Visual Studio tools` -> `VC` -> `x64 Native Tools Command Prompt for VS 2019`.

It's very important to choose the x64-specific shell and not the x86 variant, as ES-DE will only compile as a 64-bit application.

**MinGW (GCC) setup:**

Download the following packages and install them:

[https://gitforwindows.org](https://gitforwindows.org)

[https://cmake.org/download](https://cmake.org/download)

[https://jmeubank.github.io/tdm-gcc](https://jmeubank.github.io/tdm-gcc)

After installation, make a copy of `TDM-GCC-64\bin\mingw32-make` to `make` just for convenience.

Note that most GDB builds for Windows have broken Python support so that pretty printing won't work. The recommended MinGW distribution should work fine though.

**Other preparations:**

In order to get clang-format onto the system you need to download and install Clang: \
[https://llvm.org/builds](https://llvm.org/builds)

Just run the installer and make sure to select the option _Add LLVM to the system PATH for current user_.

Install your editor of choice, I use [VSCode](https://code.visualstudio.com).

Configure Git. I won't get into the details on how this is done, but there are many resources available online to support with this. The `Git Bash` shell shipped with Git for Windows is very useful though as it's somewhat reproducing a Unix environment using MinGW/MSYS2.

It's strongly recommended to set line breaks to Unix-style (line feed only) directly in the editor. But if not done, lines breaks will anyway be converted when running clang-format on the code, as explained [here](INSTALL-DEV.md#using-clang-format-for-automatic-code-formatting).

In the descriptions below it's assumed that all build steps for MinGW/GCC will be done in the Git Bash shell, and all the build steps for MSVC will be done in the MSVC developer console (x64 Native Tools Command Prompt for VS).


**Download the dependency packages:**

FFmpeg (choose the n4.4 package with win64-gpl-shared in the filename, the snapshot version will not work) \
[https://github.com/BtbN/FFmpeg-Builds/releases](https://github.com/BtbN/FFmpeg-Builds/releases)

FreeImage (binary distribution) \
[https://sourceforge.net/projects/freeimage](https://sourceforge.net/projects/freeimage)

cURL (Windows 64 bit binary, select the MinGW version even if using MSVC) \
[https://curl.haxx.se/download.html](https://curl.haxx.se/download.html)

SDL2 (development libraries, MinGW or VC/MSVC) \
[https://www.libsdl.org/download-2.0.php](https://www.libsdl.org/download-2.0.php)

libVLC (both win64 binary and source distributions)  - optional, if building with the VLC video player\
[https://ftp.lysator.liu.se/pub/videolan/vlc](https://ftp.lysator.liu.se/pub/videolan/vlc)

Uncompress the files from the above packages to a suitable directory, for example `C:\Programming\Dependencies`

Append `_src` or something appropriate to the VLC source distribution directory as it has the same name as the binary distribution.

GLEW\
[http://glew.sourceforge.net](http://glew.sourceforge.net)

If using MinGW, this library needs to be compiled from source as the pre-built libraries don't seem to work with GCC. The GitHub repo seems to be somewhat broken as well, therefore the manual download is required. It's recommended to get the source in zip format and uncompress it to the same directory as the other libraries listed above.

Then simply build the required glew32.dll library:

```
unzip glew-2.1.0.zip
cd glew-2.1.0
make
```
You will probably see a huge amount of compiler warnings, and the glewinfo.exe tool may fail to build, but we don't need it so it's not an issue.

If using MSVC, simply download the binary distribution of GLEW.

The following packages are not readily available for Windows, so clone the repos and build them yourself:

[FreeType](https://www.freetype.org)
```
git clone git://git.savannah.gnu.org/freetype/freetype2.git
cd freetype2
git checkout VER-2-10-4
mkdir build
cd build
```

MSVC:
```
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON ..
nmake
```

MinGW:
```
cmake -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=ON ..
make
```

[pugixml](https://pugixml.org)
```
git clone git://github.com/zeux/pugixml.git
cd pugixml
git checkout v1.10
```

MSVC:

```
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON .
nmake
```

MinGW:
```
cmake -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=ON .
make
```

[RapidJSON](http://rapidjson.org)

For RapidJSON you don't need to compile, you just need the include files:

```
git clone git://github.com/Tencent/rapidjson.git
cd rapidjson
git checkout v1.1.0
```



**Clone the ES-DE repository:**

This works the same as on Unix or macOS, just run the following:

```
git clone https://gitlab.com/leonstyhre/emulationstation-de.git
```

By default the master branch will be used, which is where development takes place. To instead build a stable release, switch to the `stable-x.x` branch for the version, for example:

```
cd emulationstation-de
git checkout stable-1.1
```

**Setup the include directories:**

As there is no standardized include directory structure in Windows, you need to provide the include files manually.

Make a directory in your build environment tree, for instance under `C:\Programming\include`

Copy the include files for cURL, FFmpeg, FreeImage, FreeType, GLEW, pugixml, RapidJSON, SDL2 and optionally VLC to this directory.

You may need to create the SDL2 directory manually and copy the header files there.

For FFmpeg, copy the directories libavcodec, libavfilter, libavformat and libavutil.

It should look something like this:

```
$ ls -1 include/
curl/
FreeImage.h
freetype/
ft2build.h
GL/
libavcodec/
libavfilter/
libavformat/
libavutil/
pugiconfig.hpp
pugixml.hpp
rapidjson/
SDL2/
vlc/            (only if building with the VLC video player)
```

**Copy the required library files to the ES-DE build directory:**

As there's no package manager in Windows and no way to handle dependencies, we need to ship all the required shared libraries with the application.

Copy the files to the `emulationstation-de` build directory. Most of them will come from the packages that were provided in the previous steps of this guide.

**Required files for MSVC:**
```
avcodec-58.dll
avcodec.lib
avfilter.lib
avfilter-7.dll
avformat-58.dll
avformat.lib
avutil-56.dll
avutil.lib
postproc-55.dll
swresample-3.dll
swresample.lib
swscale-5.dll
swscale.lib
FreeImage.dll
FreeImage.lib
freetype.dll
freetype.lib
glew32.dll
glew32.lib
libcurl-x64.dll
libcrypto-1_1-x64.dll     (from the OpenSSL package, located in Git MinGW/MSYS2 under \mingw64\bin)
libssl-1_1-x64.dll        (from the OpenSSL package, located in Git MinGW under \mingw64\bin)
libvlc.dll                (only if building with the VLC video player)
libvlccore.dll            (only if building with the VLC video player)
pugixml.dll
pugixml.lib
SDL2.dll
SDL2.lib
SDL2main.lib
MSVCP140.dll              (from Windows\System32)
VCOMP140.DLL              (from Windows\System32)
VCRUNTIME140.dll          (from Windows\System32)
VCRUNTIME140_1.dll        (from Windows\System32)
```

In addition to these files, you need libcurl-x64.lib and libvlc.lib (if building with the VLC video player), but these are not available for download so you need to generate them yourself from their corresponding DLL files. Do this inside the respective library directory and not within the emulationstation-de folder.

libcurl-x64.lib:
```
dumpbin /exports libcurl-x64.dll > exports.txt
echo LIBRARY libcurl-x64 > libcurl-x64.def
echo EXPORTS >> libcurl-x64.def
for /f "skip=19 tokens=4" %A in (exports.txt) do echo %A >> libcurl-x64.def
lib /def:libcurl-x64.def /out:libcurl-x64.lib /machine:x64
```

libvlc.lib:
```
dumpbin /exports libvlc.dll > exports.txt
echo LIBRARY libvlc > libvlc.def
echo EXPORTS >> libvlc.def
for /f "skip=19 tokens=4" %A in (exports.txt) do echo %A >> libvlc.def
lib /def:libvlc.def /out:libvlc.lib /machine:x64
```

**Required files for MinGW:**

```
avcodec-58.dll
avfilter-7.dll
avformat-58.dll
avutil-56.dll
postproc-55.dll
swresample-3.dll
swscale-5.dll
FreeImage.dll
glew32.dll
libcrypto-1_1-x64.dll     (from the OpenSSL package, located in Git MinGW/MSYS2 under \mingw64\bin)
libcurl-x64.dll
libfreetype.dll
libpugixml.dll
libSDL2main.a
libssl-1_1-x64.dll        (from the OpenSSL package, located in Git MinGW under \mingw64\bin)
libvlc.dll                (only if building with the VLC video player)
libvlccore.dll            (only if building with the VLC video player)
SDL2.dll
vcomp140.dll              (From Visual C++ Redistributable for Visual Studio 2015, 32-bit version)
```

**Additional files for both MSVC and MinGW if building with the VLC video player:**

In addition to the files above, you need to copy some libraries from the VLC `plugins` folder. This contains a subdirectory structure but there is no requirement to retain this as libVLC apparently looks recursively for the .dll files.

The following libraries seem to be required to play most video and audio formats:

```
access\libfilesystem_plugin.dll
audio_filter\libaudio_format_plugin.dll
audio_filter\libtrivial_channel_mixer_plugin.dll
audio_output\libadummy_plugin.dll
audio_output\libamem_plugin.dll
audio_output\libdirectsound_plugin.dll
audio_output\libmmdevice_plugin.dll
audio_output\libwasapi_plugin.dll
audio_output\libwaveout_plugin.dll
codec\libavcodec_plugin.dll
codec\libx264_plugin.dll
codec\libx265_plugin.dll
logger\libconsole_logger_plugin.dll
video_chroma\libswscale_plugin.dll
video_output\libvmem_plugin.dll
```

The reason to not simply copy all plugins is that the combined size of these is around 120 MB (as of VLC version 3.0.11) and the size of the selected files listed above is around 22 MB, which is more reasonable.

Place the files in the `emulationstation-de\plugins\` directory.

**Building ES-DE using MSVC:**

There is a bug in libVLC when building using MSVC, so three lines need to be commented out from `libvlc_media.h`. The compiler error messages will provide you with the line numbers, but they involve the callback `libvlc_media_read_cb`.

After doing this, ES-DE should build correctly.

For a release build:

```
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DWIN32_INCLUDE_DIR=../include .
nmake
```

Or for a debug build:
```
cmake -G "NMake Makefiles" -DWIN32_INCLUDE_DIR=../include .
nmake
```

For some annoying reason MSVC is the only compiler that creates a debug build by default and where you need to explicitly set the build type to Release.

Unfortunately nmake does not support parallel compiles so it's very slow. There are third party solutions to get multi-core building working with MSVC, but I've not investigated this in depth.

Be aware that MSVC links against different VC++ libraries for debug and release builds (e.g. MSVCP140.dll or MSVCP140d.dll), so any NSIS package made from a debug build will not work on computers without the MSVC development environment installed.

To build ES-DE with the VLC video player in addition to the default FFmpeg player, enable the VLC_PLAYER option, for example:
```
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DWIN32_INCLUDE_DIR=../include -DVLC_PLAYER=on .
make
```

**Building ES-DE using MinGW:**

For a release build:

```
cmake -G "MinGW Makefiles" -DWIN32_INCLUDE_DIR=../include .
make
```

Or for a debug build:

```
cmake -G "MinGW Makefiles" -DWIN32_INCLUDE_DIR=../include -DCMAKE_BUILD_TYPE=Debug .
make
```

For some reason defining the `../include` path doesn't work when running CMake from PowerShell (and no, changing to backslash doesn't help). Instead use Bash, by running from a Git Bash shell.

The make command works fine directly in PowerShell though so it can be run from the VSCode terminal.

To build ES-DE with the VLC video player in addition to the default FFmpeg player, enable the VLC_PLAYER option, for example:
```
cmake -G "MinGW Makefiles" -DWIN32_INCLUDE_DIR=../include -DVLC_PLAYER=on .
make
```

You run a parallel build using multiple CPU cores with the `-j` flag, for example, `make -j6`.

Note that compilation time is much longer than on Unix or macOS, and linking time is unendurable for a debug build (around 10 times longer on my computer compared to Linux). The debug binary is also much larger than on Unix.


**Running with OpenGL software rendering:**

If you are running Windows in a virtualized environment such as QEMU-KVM that does not support HW accelerated OpenGL, you can install the Mesa3D for Windows library, which can be downloaded at [https://fdossena.com/?p=mesa/index.frag](https://fdossena.com/?p=mesa/index.frag).

You simply extract the opengl32.dll file into the ES-DE directory and this will enable the llvmpipe renderer. The performance will be terrible of course, but everything should work and it should be good enough for test building on Windows without having to reboot your computer to a native Windows installation. (Note that you may need to copy opengl32.dll to your RetroArch installation directory as well to get the emulators to work somehow correctly.)

Obviously this library is only intended for development and will not be shipped with ES-DE.

**Creating an NSIS installer:**

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
CPack: - package: C:/Programming/emulationstation-de/EmulationStation-DE-1.1.0-x64.exe generated.
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

But the recommended approach is to run clang-format from within the editor. If using VSCode, there is an extension available named Clang-Format. After installing this, simply open a source file, right click and choose `Format Document` or use the applicable keyboard shortcut. The first time you do this, you will have to make a choice to perform the formatting using clang-format. The rest should be completely automatic.

In some instances you may want to avoid getting code formatted, and you can accomplish this by simply enclosing the lines with the two comments "clang-format off" and "clang-format on", such as this:

```c++
// clang-format off
CollectionSystemDecl systemDecls[] = {
//  Type                  Name                Long name       Theme folder           isCustom
    { AUTO_ALL_GAMES,     "all",              "all games",    "auto-allgames",       false },
    { AUTO_LAST_PLAYED,   "recent",           "last played",  "auto-lastplayed",     false },
    { AUTO_FAVORITES,     "favorites",        "favorites",    "auto-favorites",      false },
    { CUSTOM_COLLECTION,  myCollectionsName,  "collections",  "custom-collections",  true  }
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

**CA certificates:**

There are some files shipped with ES-DE that need to be pulled from external resources, the first one being the CA certificate bundle to get TLS/SSL support working on Windows.

The CA certificates shipped with ES-DE come directly from the curl project but they're originally supplied by the Mozilla foundation. See [https://wiki.mozilla.org/CA](https://wiki.mozilla.org/CA) for more information about this certificate bundle.

The latest version can be downloaded from [https://curl.se/docs/caextract.html](https://curl.se/docs/caextract.html)

After downloading the file, rename it from `cacert.pem` to `curl-ca-bundle.crt` and move it to the certificates directory i.e.:

```
emulationstation-de/resources/certificates/curl-ca-bundle.crt
```

**MAME ROM info:**

ES-DE automatically identifies and excludes MAME BIOS and device files, as well as translating the short MAME ROM names to their full game names. This is done using information from the MAME driver file shipped with the official MAME distribution. The file needs to be converted to an internal format used by ES-DE as the original file is huge and most of the information is not required.

To get hold of the driver file, go to [https://www.mamedev.org/release.php](https://www.mamedev.org/release.php) and select the Windows version, but only download the driver information in XML format and not MAME itself. This file will be named something like `mame0226lx.zip` and unzipping it will give you a file name such as `mame0226.xml`.

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

**~/.emulationstation/es_settings.xml:**

When ES-DE is first started, a configuration file will be created as `~/.emulationstation/es_settings.xml`

This file will contain all supported settings at their default values. Normally you shouldn't need to modify this file manually, instead you should be able to use the menu inside ES-DE to update all the necessary settings.

**Setting the ROM directory in es_settings.xml:**

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

**~/.emulationstation/es_input.xml:**

As ES-DE auto-configures the keyboard and controllers, neither the input configuration step or manual adjustments to the es_input.xml file should normally be needed. Actually, unless the button layout has been customized using the input configurator, the es_input.xml file will not even exist.

But if you have customized your button layout and your controller or keyboard stop working, you can delete the `~/.emulationstation/es_input.xml` file to remove the customizations, or you can start ES-DE with the `--force-input-config` command line option to make the input configurator appear.

The input configuration is described in the [User guide](USERGUIDE-DEV.md#input-device-configuration).


## Command line options

You can use **--help** or **-h** to view the list of command line options, as shown here.

### Unix

```
--display [index 1-4]           Display/monitor to use
--resolution [width] [height]   Application resolution
--windowed                      Windowed mode, should be combined with --resolution
--fullscreen-normal             Normal fullscreen mode
--fullscreen-borderless         Borderless fullscreen mode (always on top)
--vsync [1/on or 0/off]         Turn VSync on or off (default is on)
--max-vram [size]               Max VRAM to use (in mebibytes) before swapping
--no-splash                     Don't show the splash screen during startup
--gamelist-only                 Skip automatic game ROM search, only read from gamelist.xml
--ignore-gamelist               Ignore the gamelist files (useful for troubleshooting)
--show-hidden-files             Show hidden files and folders
--show-hidden-games             Show hidden games
--force-full                    Force the UI mode to Full
--force-kiosk                   Force the UI mode to Kiosk
--force-kid                     Force the UI mode to Kid
--force-input-config            Force configuration of input device
--home [path]                   Directory to use as home path
--debug                         Print debug information
--version, -v                   Display version information
--help, -h                      Summon a sentient, angry tuba
```

### macOS and Windows

```
--display [index 1-4]           Display/monitor to use
--resolution [width] [height]   Application resolution
--vsync [1/on or 0/off]         Turn VSync on or off (default is on)
--max-vram [size]               Max VRAM to use (in mebibytes) before swapping
--no-splash                     Don't show the splash screen during startup
--gamelist-only                 Skip automatic game ROM search, only read from gamelist.xml
--ignore-gamelist               Ignore the gamelist files (useful for troubleshooting)
--show-hidden-files             Show hidden files and folders
--show-hidden-games             Show hidden games
--force-full                    Force the UI mode to Full
--force-kiosk                   Force the UI mode to Kiosk
--force-kid                     Force the UI mode to Kid
--force-input-config            Force configuration of input device
--home [path]                   Directory to use as home path
--debug                         Print debug information
--version, -v                   Display version information
--help, -h                      Summon a sentient, angry tuba
```

As you can see above, you can override the home directory path using the `--home` flag. So by running for instance the command `emulationstation --home ~/games/emulation`, ES-DE will use `~/games/emulation/.emulationstation` as its application home directory. Be aware that this option completely replaces what is considered the home directory, meaning the default ROM directory ~/ROMs would be resolved to ~/games/emulation/ROMs. The same is true for the emulator core locations if es_find_rules.xml is configured to look for them relative to the home directory. So of course RetroArch and other emulators would also need to be configured to use ~/games/emulation as its base directory in this instance.

For the following options, the es_settings.xml file is immediately updated/saved when passing the parameter:
```
--display
--fullscreen-normal
--fullscreen-borderless
--max-vram
--show-hidden-files
--show-hidden-games
```


## es_systems.xml

The es_systems.xml file contains the system configuration data for ES-DE, written in XML format. This defines the system name, the full system name, the ROM path, the allowed file extensions, the launch command, the platform (for scraping) and the theme to use.

ES-DE ships with a comprehensive `es_systems.xml` configuration file and normally you shouldn't need to modify this. However there may be special circumstances such as wanting to use alternative emulators for some game systems or perhaps you need to add additional systems altogether.

To make a customized version of the systems configuration file, it first needs to be copied to `~/.emulationstation/custom_systems/es_systems.xml`. (The tilde symbol `~` translates to `$HOME` on Unix and macOS, and to `%HOMEPATH%` on Windows unless overridden using the --home command line option.)

The bundled es_systems.xml file is located in the resources directory that is part of the application installation. For example this could be `/usr/share/emulationstation/resources/systems/unix/es_systems.xml` on Unix, `/Applications/EmulationStation Desktop Edition.app/Contents/Resources/resources/systems/macos/es_systems.xml` on macOS or `C:\Program Files\EmulationStation-DE\resources\systems\windows\es_systems.xml` on Windows. The actual location may differ from these examples of course, depending on where ES-DE has been installed.

Note that when copying the bundled es_systems.xml file to ~/.emulationstation/custom_systems/, it will completely replace the default file processing. So when upgrading to future ES-DE versions, any modifications such as additional game systems will not be enabled until the customized configuration file has been manually updated.

It doesn't matter in which order you define the systems as they will be sorted by the full system name inside the application, but it's still probably a good idea to add them in alphabetical order to make the file easier to maintain.

Keep in mind that you have to set up your emulators separately from ES-DE as the es_systems.xml file assumes that your emulator environment is properly configured.

Below is an overview of the file layout with various examples. For the command tag, the newer es_find_rules.xml logic described later in this document removes the need for most of the legacy options, but they are still supported for special configurations and for backward compatibility with old configuration files.

For a real system entry there can of course not be multiple entries for the same tag such as the multiple \<command\> entries listed here.

```xml
<?xml version="1.0"?>
<!-- This is the ES-DE game systems configuration file. -->
<systemList>
    <!-- Any tag not explicitly described as optional in the description is mandatory.
    If omitting a mandatory tag, ES-DE will skip the system entry during startup. -->
    <system>
        <!-- A short name, used internally. -->
        <name>snes</name>

        <!-- The full system name, used for sorting the systems, for selecting the systems to multi-scrape etc. -->
        <fullname>Nintendo SNES (Super Nintendo)</fullname>

        <!-- The path to look for ROMs in. '~' will be expanded to $HOME or %HOMEPATH%, depending on the operating system.
        The optional %ROMPATH% variable will expand to the path defined in the setting ROMDirectory in es_settings.xml.
        All subdirectories (and non-recursive links) will be included. -->
        <path>%ROMPATH%/snes</path>

        <!-- A list of extensions to search for, delimited by any of the whitespace characters (", \r\n\t").
        You must include the period at the start of the extension and it's also case sensitive. -->
        <extension>.smc .SMC .sfc .SFC .swc .SWC .fig .FIG .bs .BS .bin .BIN .mgd .MGD .7z .7Z .zip .ZIP</extension>

        <!-- The command executed when a game is launched. A few special variables are replaced if found for a command tag (see below).
        This example for Unix uses the %EMULATOR_ and %CORE_ variables which utilize the find rules defined in the es_find_rules.xml
        file. This is the recommended way to configure the launch command. -->
        <command>%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/snes9x_libretro.so %ROM%</command>

        <!-- This example for Unix will search for RetroArch in the PATH environment variable and it also has an absolute path to
        the snes9x_libretro core, If there are spaces in the path or file name, you must enclose them in quotation marks, such as
        retroarch -L "~/my configs/retroarch/cores/snes9x_libretro.so" %ROM% -->
        <command>retroarch -L ~/.config/retroarch/cores/snes9x_libretro.so %ROM%</command>

        <!-- This example for Unix combines the two rules above to search for RetroArch in the PATH environment variable but uses
        the find rules for the emulator cores. -->
        <command>retroarch -L %CORE_RETROARCH%/snes9x_libretro.so %ROM%</command>

        <!-- This is an example for macOS, which is very similar to the Unix example above except using an absolute path to the emulator. -->
        <command>/Applications/RetroArch.app/Contents/MacOS/RetroArch -L %CORE_RETROARCH%/snes9x_libretro.dylib %ROM%</command>

        <!-- This is an example for Windows. The .exe extension is optional and both forward slashes and backslashes are allowed as
        directory separators. As there is no standardized installation directory structure for this operating system, the %EMUPATH%
        variable is used here to find the cores relative to the RetroArch binary. The emulator binary must be in the PATH environmental
        variable or otherwise the complete path to the retroarch.exe file needs to be defined. Batch scripts (.bat) are also supported. -->
        <command>retroarch.exe -L %EMUPATH%\cores\snes9x_libretro.dll %ROM%</command>

        <!-- Another example for Windows. As can be seen here, the absolute path to the emulator has been defined, and there are spaces
        in the directory name, so it needs to be surrounded by quotation marks. Quotation marks around the %EMUPATH% entry are optional
        but in this example they're added. -->
        <command>"C:\My Games\RetroArch\retroarch.exe" -L "%EMUPATH%\cores\snes9x_libretro.dll" %ROM%</command>

        <!-- An example for use in a portable Windows emulator installation, for instance on a USB memory stick. The %ESPATH% variable is
        expanded to the directory of the ES-DE executable. -->
        <command>"%ESPATH%\RetroArch\retroarch.exe" -L "%ESPATH%\RetroArch\cores\snes9x_libretro.dll" %ROM%</command>

        <!-- An example on Unix which launches a script, this is for example used by source ports, Steam games etc. -->
        <command>bash %ROM%</command>

        <!-- The equivalent configuration as above, but for Windows.
        The optional %HIDEWINDOW% variable is used to hide the console window, which would otherwise be visible when launching the game. -->
        <command>%HIDEWINDOW% cmd /C %ROM%</command>

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

`%BASENAME%` - Replaced with the "base" name of the path to the selected ROM. For example, a path of `/foo/bar.rom`, this tag would be `bar`. This tag is useful for setting up AdvanceMAME.

`%EMUPATH%` - Replaced with the path to the emulator binary. This is expanded using either the PATH environmental variable of the operating system, or using an absolute emulator path if this has been defined.

`%ESPATH%` - Replaced with the path to the ES-DE binary. Mostly useful for portable emulator installations, for example on a USB memory stick.

`%EMULATOR_` - This utilizes the emulator find rules as defined in `es_find_rules.xml`. This is the recommended way to configure the launch command. The find rules are explained in more detail below.

`%CORE_` - This utilizes the core find rules as defined in `es_find_rules.xml`. This is the recommended way to configure the launch command.

`%HIDEWINDOW%` - This variable is only available on Windows and is used primarily for hiding console windows when launching scripts (used for example by Steam games and source ports). If not defining this, the console window will be visible when launching the game. It needs to be placed first in the command tag.

Here are some additional real world examples of system entries, the first one for Unix:

```xml
  <system>
    <name>dos</name>
    <fullname>DOS (PC)</fullname>
    <path>%ROMPATH%/dos</path>
    <extension>.bat .BAT .com .COM .conf .CONF .cue .CUE .exe .EXE .iso .ISO .7z .7Z .zip .ZIP</extension>
    <command>%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/dosbox_core_libretro.so %ROM%</command>
    <platform>dos</platform>
    <theme>dos</theme>
  </system>
```

Then one for macOS:

```xml
  <system>
    <name>nes</name>
    <fullname>Nintendo Entertainment System</fullname>
    <path>%ROMPATH%/nes</path>
    <extension>.nes .NES .unf .UNF .unif .UNIF .7z .7Z .zip .ZIP</extension>
    <command>%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/nestopia_libretro.dylib %ROM%</command>
    <platform>nes</platform>
    <theme>nes</theme>
  </system>
```

And finally one for Windows:

```xml
  <system>
    <name>sega32x</name>
    <fullname>Sega Mega Drive 32X</fullname>
    <path>%ROMPATH%\sega32x</path>
    <extension>.bin .BIN .gen .GEN .smd .SMD .md .MD .32x .32X .cue .CUE .iso .ISO .sms .SMS .68k .68K .7z .7Z .zip .ZIP</extension>
    <command>%EMULATOR_RETROARCH% -L %CORE_RETROARCH%\picodrive_libretro.dll %ROM%</command>
    <platform>sega32x</platform>
    <theme>sega32x</theme>
  </system>
```

## es_find_rules.xml

This file makes it possible to define rules for where to search for the emulator binaries and emulator cores.

The file is located in the resources directory in the same location as the es_systems.xml file, but a customized copy can be placed in ~/.emulationstation/custom_systems, which will override the bundled file.

Here's an example es_find_rules.xml file for Unix:
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
      <entry>~/Applications/RetroArch-Linux-x86_64.AppImage</entry>
      <entry>~/.local/bin/RetroArch-Linux-x86_64.AppImage</entry>
      <entry>~/bin/RetroArch-Linux-x86_64.AppImage</entry>
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
      <entry>~/Applications/yuzu.AppImage</entry>
      <entry>~/.local/bin/yuzu.AppImage</entry>
      <entry>~/bin/yuzu.AppImage</entry>
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
</ruleList>
```

It's pretty straightforward, there are currently three rules supported for finding emulators, `winregistrypath`, `systempath` and `staticpath` and there is one rule supported for finding the emulator cores, `corepath`.

Of these, `winregistrypath` is only available on Windows, and attempting to use the rule on any other operating system will generate a warning in the log file when processing the es_find_fules.xml file.

The `name` attribute must correspond to the command tags in es_systems.xml, take for example this line:

```xml
<command>%EMULATOR_RETROARCH% -L %CORE_RETROARCH%/dosbox_core_libretro.so %ROM%</command>
```

Here %EMULATOR_ and %CORE_ are followed by the string RETROARCH which corresponds to the name attribute in es_find_rules.xml. The name is case sensitive but it's recommended to use uppercase names to make the variables feel consistent (%EMULATOR_retroarch% doesn't look so pretty).

Of course this makes it possible to add any number of emulators to the configuration file.

The `winregistrypath` rule searches the Windows Registry "App Paths" keys for the emulators defined in the `<entry>` tags. If for example this tag is set to `retroarch.exe`, the key `SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\retroarch.exe` will be searched for. HKEY_CURRENT_USER is tried first, and if no key is found there, HKEY_LOCAL_MACHINE is tried as well. In addition to this, ES-DE will check that the binary defined in the key actually exists, and if not, it will proceed with the next rule. Be aware that the App Paths keys are added by the emulators during their installation, and although RetroArch does add this key, not all emulators do.


The other rules are probably self-explanatory with `systempath` searching the PATH environment variable for the binary names defined by the `<entry>` tags and `staticpath` defines absolute paths to the emulators. For staticpath, the actual emulator binary must be included in the entry tag.

The winregistrypath rules are always processed first, followed by systempath and then staticpath. This is done regardless of which order they are defined in the es_find_rules.xml file.

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
      <!-- Portable installation -->
      <entry>%ESPATH%\RetroArch-Win64\retroarch.exe</entry>
      <entry>%ESPATH%\RetroArch\retroarch.exe</entry>
      <entry>%ESPATH%\..\RetroArch-Win64\retroarch.exe</entry>
      <entry>%ESPATH%\..\RetroArch\retroarch.exe</entry>
    </rule>
  </emulator>
  <emulator name="YUZU">
    <!-- Nintendo Switch emulator Yuzu -->
    <rule type="systempath">
      <entry>yuzu.exe</entry>
    </rule>
    <rule type="staticpath">
      <entry>~\AppData\Local\yuzu\yuzu-windows-msvc\yuzu.exe</entry>
      <!-- Portable installation -->
      <entry>%ESPATH%\yuzu\yuzu-windows-msvc\yuzu.exe</entry>
      <entry>%ESPATH%\..\yuzu\yuzu-windows-msvc\yuzu.exe</entry>
    </rule>
  </emulator>
  <core name="RETROARCH">
    <rule type="corepath">
      <entry>%EMUPATH%\cores</entry>
    </rule>
  </core>
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

You can use ES-DE's scrapers to populate the gamelist.xml files, or manually update individual entries using the metadata editor. All of this is explained in the [User guide](USERGUIDE-DEV.md).

The gamelist.xml files are searched for in the ES-DE home directory, i.e. `~/.emulationstation/gamelists/<system name>/gamelist.xml`

For example:

```
~/.emulationstation/gamelists/c64/gamelist.xml
~/.emulationstation/gamelists/megadrive/gamelist.xml
```

**gamelist.xml file structure:**

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

**gamelist.xml reference:**

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
* `sortname` - string, used in sorting the gamelist in a system, instead of `name`
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
* `launchcommand` - string, overrides the emulator and core settings on a per-game basis
* `playcount` - integer, the number of times this game has been played
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
* `lastplayed` - statistic, for folders this is inherited by the latest game file launched inside the folder.

**Additional gamelist.xml information:**

* If a value matches the default for a particular piece of metadata, ES-DE will not write it to the gamelist.xml file (for example, if `genre` isn't specified, an empty genre tag will not be written)

* A `game` can actually point to a folder/directory if the folder has a matching extension, although this is exceedingly rare

* The `folder` metadata will only be used if a game file is found inside that folder, as empty folders will be skipped during startup even if they have metadata values defined for themselves

* ES-DE will keep entries for games and folders that it can't find the game/ROM files for, i.e. it will not clean up the gamelist.xml files automatically when game files are missing

* The switch `--gamelist-only` can be used to skip automatic searching, and only display games defined in the gamelist.xml files

* The switch `--ignore-gamelist` can be used to ignore the gamelist upon start of the application (mostly useful for debugging purposes)


## Debug mode

By passing the --debug command line option, ES-DE will increase the logging to include a lot of additional debug output which is useful both for development and in order to pinpoint issues as a user.
In addition to this extra logging, a few key combinations are enabled when in debug mode. These are useful both for working on ES-DE itself as well as for theme developers.

**Ctrl + g**

This will render a grid on the user interface, most notably in the menus, showing the layout of all the GUI elements. Note that any open screen needs to be closed and reopened again after using the key combination in order for it to have any effect.

**Ctrl + i**

This will draw a semi-transparent red frame behind all image elements.

**Ctrl + t**

This will draw a semi-transparent blue frame around all text elements.

**Ctrl + r**

This will reload either a single gamelist or all gamelists depending on where you're located when entering the key combination (go to the system view to make a complete reload). Very useful for theme development as any changes to the theme files will be activated without requiring an application restart. Note that the menu needs to be closed for this key combination to have any effect.

By default all controller input (keyboard and controller button presses) will be logged when the --debug flag has been passed. To disable the input logging, the setting DebugSkipInputLogging kan be set to false in the es_settings.xml file. There is no menu entry to change this as it's intended for developers and not for end users.


## Portable installation on Windows

It's possible to easily create a portable installation of ES-DE on Windows, for example to place on a USB memory stick.

For the sake of this example, let's assume that the removable media has the device name `F:\`

These are the steps to perform:

* Copy the EmulationStation-DE installation directory to F:\
* Copy your emulator directories to F:\EmulationStation-DE\
* Copy your ROMs directory to F:\EmulationStation-DE\
* Create an empty file named portable.txt in F:\EmulationStation-DE\

You should end up with something like this:
```
F:\EmulationStation-DE\
F:\EmulationStation-DE\RetroArch-Win64\
F:\EmulationStation-DE\yuzu\
F:\EmulationStation-DE\ROMs\
F:\EmulationStation-DE\portable.txt
```

(Yuzu is an optional Nintendo Switch emulator.)

Of course there will be many more files and directories from the normal installation than those listed above.

How this works is that when ES-DE finds a file named portable.txt in its executable directory, it will by default locate the .emulationstation directory directly in this folder. It's also possible to modify portable.txt with a path relative to the ES-DE executable directory. For instance if two dots `..` are placed inside the portable.txt file, then the .emulationstation directory will be located in the parent folder, which would be directly under F:\ in this example.

If the --home command line parameter is passed when starting ES-DE, that will override the portable.txt file.

The emulators that will be automatically searched for by ES-DE are (relative to the EmulationStation-DE directory):

```
RetroArch-Win64\retroarch.exe
RetroArch\retroarch.exe
yuzu\yuzu-windows-msvc\yuzu.exe
..\RetroArch-Win64\retroarch.exe
..\RetroArch\retroarch.exe
..\yuzu\yuzu-windows-msvc\yuzu.exe
```

If you want to place your emulators elsewhere, you need to create a customized es_find_rules.xml file, which is explained earlier in this document.

Start ES-DE from the F:\ device and check that everything works as expected.

Following this, optionally copy any existing gamelist.xml files, game media files etc. to the removable media. For example:

```
F:\EmulationStation-DE\.emulationstation\gamelists\
F:\EmulationStation-DE\.emulationstation\downloaded_media\
```

You now have a fully functional portable retro gaming installation!

The portable installation works exactly as a normal installation, i.e. you can use the built-in scraper, edit metadata etc.


## Custom event scripts

There are numerous locations throughout ES-DE where custom scripts will be executed if the option to do so has been enabled in the settings. You'll find the option on the Main menu under `Other settings`. By default it's deactivated so be sure to enable it to use this feature.

The approach is quite straightforward, ES-DE will look for any files inside a script directory that corresponds to the event that is triggered and will then execute all these files.

We'll go through two examples:
* Creating a log file that will record the start and end time for each game we play, letting us see how much time we spend on retro-gaming
* Changing the system resolution when launching and returning from a game in order to run the emulator at a lower resolution than ES-DE

**Note:** The following examples are for Unix systems, but it works the same way on macOS (which is also Unix after all), and on Windows (although .bat batch files are then used instead of shell scripts and any spaces in the parameters are not escaped as is the case on Unix).

The events executed when a game starts and ends are called `game-start` and `game-end` respectively. Finding these event names is easily achieved by starting ES-DE with the `--debug` flag. If this is done, all attempts to execute custom event scripts will be logged to es_log.txt, including the event names.

So let's create the folders for these events in the scripts directory. The location is `~/.emulationstation/scripts`

**Game log:**

After creating the directories, we need to create the scripts to log the actual game launch and game ending. The parameters that are passed to the scripts varies depending on the type of event, but for these events the two parameters are the absolute path to the game file, and the game name as shown in the gamelist view.

Let's name the start script `game_start_logging.sh` with the following contents:

```
#!/bin/bash
TIMESTAMP=$(date +%Y-%m-%d' '%H:%M:%S)
echo Starting game "\""${2}"\"" "(\""${1}"\")" at $TIMESTAMP >> ~/.emulationstation/game_playlog.txt
```

And let's name the end script `game_end_logging.sh` with the following contents:

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

Don't forget to make the scripts executable (e.g. "chmod 755 ./game_start_logging.sh").

If we now start ES-DE with the debug flag and launch a game, something like the following should show up in es_log.txt:

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

**Resolution changes:**

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

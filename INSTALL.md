# EmulationStation Desktop Edition (ES-DE) - Building and advanced configuration

**Note:** This is a quite technical document intended for those that are interested in compiling ES-DE from source code, or would like to customize the configuration. If you just want to start using the software, check out the [User guide](USERGUIDE.md) instead.

Table of contents:

[[_TOC_]]

## Development Environment

ES-DE is developed and compiled using Clang/LLVM and GCC on Unix, Clang/LLVM on macOS and MSVC and GCC (MinGW) on Windows.

CMake is the build system for all the supported operating systems, used in conjunction with `make` on Unix and macOS and `nmake` and `make` on Windows. Xcode on macOS or Visual Studio on Windows are not required for building ES-DE and they have not been used during the development.

Any code editor can be used of course, but I recommend [VSCode](https://code.visualstudio.com).


## Building on Unix

The code has some dependencies. For building, you'll need CMake and development packages for cURL, FreeImage, FreeType, libVLC, pugixml, SDL2 and RapidJSON.

**Installing dependencies:**

**On Debian/Ubuntu**

All of the required packages can be installed with apt-get:
```
sudo apt-get install build-essential git cmake libsdl2-dev libfreeimage-dev libfreetype6-dev libcurl4-openssl-dev libpugixml-dev rapidjson-dev libasound2-dev vlc libvlc-dev libgl1-mesa-dev
```

**On Fedora**

First add the RPM Fusion repository which is required for VLC. Go to [https://rpmfusion.org/Configuration](https://rpmfusion.org/Configuration) and download the .rpm package for the free repository, then install it such as this:

```
sudo dnf install ./rpmfusion-free-release-33.noarch.rpm
```

Then use dnf to install all the required packages:
```
sudo dnf install gcc-c++ cmake SDL2-devel freeimage-devel freetype-devel curl-devel pugixml-devel rapidjson-devel alsa-lib-devel mesa-libGL-devel vlc vlc-devel
```

**On Manjaro**

Use pacman to install all the required packages:

```
pacman -S gcc make cmake pkgconf sdl2 freeimage freetype2 pugixml rapidjson vlc
```

**On FreeBSD**

Use pkg to install the dependencies:
```
pkg install git pkgconf cmake sdl2 freeimage pugixml rapidjson vlc
```

Clang/LLVM and cURL should already be installed in the base OS image.

**On NetBSD**

Use pkgin to install the dependencies:
```
pkgin install git cmake pkgconf SDL2 freeimage pugixml rapidjson vlc
```

NetBSD ships with GCC by default, and although you should be able to install and use Clang/LLVM, it's probably easier to just stick to the default compiler environment.

**On OpenBSD**

Use pkg_add to install the dependencies:
```
pkg_add cmake pkgconf sdl2 freeimage vlc
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

To create a debug build, run this instead:
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

You open the report with the `scan-view` command which lets you browse it using your web browser. Note that the compilation time is much higher when using the static analyzer compared to a normal compilation. As well this tool generates a lot of extra files and folders in the build tree, so it may make sense to run it in a separate copy of the source folder to avoid having to clean up all this extra data when the analysis has been completed.

To build ES with CEC support, add the corresponding option, for example:

```
cmake -DCMAKE_BUILD_TYPE=Debug -DCEC=on .
make
```
I have however not been able to test the CEC support and I'm not entirely sure how it's supposed to work.

To build with the GLES renderer, run the following:
```
cmake -DCMAKE_BUILD_TYPE=Debug -DGLES=on .
make
```
Note that the GLES renderer is quite limited as there is no shader support for it, so ES-DE will definitely not look as pretty as when using the default OpenGL renderer.

Running multiple compile jobs in parallel is a good thing as it speeds up the build time a lot (scaling almost linearly). Here's an example telling make to run 6 parallel jobs:

```
make -j6
```

By default ES-DE will install under /usr on Linux, /usr/pkg on NetBSD and /usr/local on FreeBSD and OpenBSD although this can be changed by setting the `CMAKE_INSTALL_PREFIX` variable.

The following example will build the application for installtion under /opt:

```
cmake -DCMAKE_INSTALL_PREFIX=/opt .
```

It's important to know that this is not only the directory used by the install script, the CMAKE_INSTALL_PREFIX variable also modifies code inside ES-DE used to locate the required program resources. So it's necessary that the install prefix corresponds to the location where the application will actually be installed.

On Linux, if you're not building a package and instead intend to install using `make install` it's recommended to set the installation prefix to /usr/local instead of /usr.

**Compilers**

Both Clang/LLVM and GCC work fine for building ES-DE.

I did some small benchmarks comparing Clang to GCC with the ES-DE codebase (as of writing it's year 2020) and it's pretty interesting.

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

This Clang debug build is LLVM "native", i.e. intended to be debugged using the LLVM project debugger LLDB. The problem is that this is still not well integrated with VSCode that I use for development so I need to keep using GDB. But this is problematic as the libstd++ data required by GDB is missing in the binary, making it impossible to see the values of for instance std::string variables.

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

ES-DE will look in the following locations for the resources, in the listed order:

* \<home\>/.emulationstation/resources/
* \<install prefix\>/share/emulationstation/resources/
* \<ES-DE executable directory\>/resources/

The resources directory is critical, without it the application won't start.

And it will look in the following locations for the themes, also in the listed order:

* \<home\>/.emulationstation/themes/
* \<install prefix\>/share/emulationstation/themes/
* \<ES-DE executable directory\>/themes/

A theme is not mandatory to start the application, but ES-DE will be basically useless without it.

The home directory will always take precedence, and any resources or themes located there will override the ones in the installation path or in the path of the ES-DE executable.

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
CPack: - package: /home/myusername/emulationstation-de/emulationstation-de-1.0.0-x64.deb generated.
```

You may like to check that the dependencies look fine, as they're automatically generated by CMake:

```
dpkg -I ./emulationstation-de-1.0.0-x64.deb
```

The package can now be installed using a package manager, for example apt:

```
sudo apt install ./emulationstation-de-1.0.0-x64.deb
```

For RPM packages, remove the comment in es-app/CMakeLists.txt accordingly, from:

```
#SET(CPACK_GENERATOR "RPM")
```
to:
```
SET(CPACK_GENERATOR "RPM")
```

Then simply run cpack.

On Fedora, you need to install rpmbuild before this command can be run though:
```
sudo dnf install rpm-build
```

After the package generation you can check that the metadata looks fine using this command:
```
rpm -qi ./emulationstation-de-1.0.0-x64.rpm
```

And to see the automatically generated dependency requirements, run this:
```
rpm -q --requires ./emulationstation-de-1.0.0-x64.rpm
```

And of course, you can also install the package:
```
sudo dnf install ./emulationstation-de-1.0.0-x64.rpm
```

## Building on macOS

EmulationStation for macOS is built using Clang/LLVM which is the default compiler for this operating system. It's pretty straightforward to build software on this OS. The main deficiency is that there is no native package manager, but as there are several third party package managers available, this can be partly compensated for. The use of one of them, [Homebrew](https://brew.sh), is detailed below.

As for code editing, I use [VSCode](https://code.visualstudio.com). I suppose Xcode could be used instead but I have no experience with this tool and no interest in it as I want to use the same tools for all the operating systems that I develop on.

**Setting up the build tools:**

Install the Command Line Tools which include Clang/LLVM, Git, make etc. Simply open a terminal and enter the command `clang`. This will open a dialog that will let you download and install the tools.

Following this, install the Homebrew package manager which will simplify the rest of the installation greatly. Install it by runing the following in a terminal window:
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
```
Be aware that Homebrew can be really slow, especially when it compiles packages from source code.

**Package installation with Homebrew:**

Install the required tools and dependencies:

```
brew install cmake pkg-config sdl2 freeimage freetype pugixml rapidjson
```

Curl could optionally be installed too, but normally the version shipped with macOS is fine to use.

Install VLC/libVLC as well:
```
brew install --cask vlc
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

To generate a debug build, run this instead:
```
cmake -DCMAKE_BUILD_TYPE=Debug .
make
```
Keep in mind though that a debug version will be much slower due to all compiler optimizations being disabled.

Running `make -j6` (or whatever number of parallel jobs you prefer) speeds up the compilation time if you have cores to spare.

After building ES-DE and trying to execute the application, there could be issues with finding the dynamic link libraries for VLC as these are not installed into a standard location but rather into the /Applications folder. As such, you may need to set the DYLD_LIBRARY_PATH environmental variable to find the VLC libraries. Note that this is not intended or required for the release build that will be shipped in a DMG installer or if you manually install ES-DE using 'make install'. It's only needed to be able to run the binary from the build directory. The following will of course only be active during your session, and you need to set the variable for each terminal window that you want to start ES-DE from, unless you add it to your shell profile file:

```
export DYLD_LIBRARY_PATH=/Applications/VLC.app/Contents/MacOS/lib
```

**Note:** According to the SDL documentation, there could be issues with attempting to run ES-DE from the build directory when using a High DPI display as the required NSHighResolutionCapable key is not set as there is no Info.plist file available. In this case, doing a 'make install' and running from the installation folder would solve the problem. I've been unable to verify if this is really required though.

Be aware that the approach taken for macOS has the limitation that you can't build for previous operating system versions. You can certainly set CMAKE_OSX_DEPLOYMENT_TARGET to whatever version you like, but the problem is that the Homebrew libraries will most likely not work on earlier macOS versions. In theory this can be worked around by building all these libraries yourself with a lower deployment target, but it's hardly worth the effort. It's better to build on the lowest OS version that should be supported and rely on forward compatibility.

**Code signing:**

Due to the Apple notarization requirement implemented as of macOS 10.14.5 a build with simple code signing is needed for versions up to 10.13 and another build with both code signing and notarization will be required for versions 10.14 and higher.

macOS code signing is beyond the scope of this document but a short summary is that signing works as intended and can be enabled by uncommenting the following lines in the es-app/CMakeLists.txt file:
```
# Uncomment the following lines and change to your certificate identity to enable code signing.
#set(CPACK_BUNDLE_APPLE_CERT_APP "Developer ID Application: <identity>")
#if(${CMAKE_OSX_DEPLOYMENT_TARGET} VERSION_GREATER 10.13)
#    set(CPACK_BUNDLE_APPLE_CODESIGN_PARAMETER "--deep --force --options runtime")
#endif()
```

**Legacy build:**

Normally ES-DE is meant to be built for macOS 10.14 and higher, but a legacy build for earlier operating system versions can be enabled. This has been tested with a minimum version of 10.11 and it's unclear if it works with even older macOS versions.

To enable a legacy build, change the CMAKE_OSX_DEPLOYMENT_TARGET variable in CMakeLists.txt from 10.14 to whatever version you would like to build for. This will disable Hardened Runtime if signing is enabled and it will add 'legacy' to the DMG installer file name when running CPack.

You also need to modify es-app/assets/EmulationStation-DE_Info.plist and set the key SMinimumSystemVersion to the version you're building for.

**Installing:**

As macOS does not have any package manager which would have handled the library dependencies, we need to bundle the required shared libraries with the application. Copy the following .dylib files from their respective installation directories to the emulationstation-de build directory:

```
libSDL2-2.0.0.dylib
libfreeimage.dylib
libfreetype.6.dylib
libpng16.16.dylib
libvlc.dylib
libvlccore.dylib
```

The first four would normally be installed by Homebrew under /usr/local/lib and the VLC libraries should be installed under /Applications/VLC.app/Contents/MacOS/lib/

Note that the filenames could be slightly different depending on what versions you have installed on your system.

For libfreetype there is a dependency on libpng and you need to rewrite the rpath to point to the local directory, otherwise any generated installation package will not work on other computers. Make sure that you have write permissions to libfreetype.6.dylib before attempting to run this:

```
cd emulationstation-de
install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @rpath/libpng16.16.dylib libfreetype.6.dylib
install_name_tool -add_rpath @executable_path libfreetype.6.dylib
```

Verify that it worked as expected by running `otool -L libfreetype.6.dylib` - you should see something like the following:

```
libfreetype.6.dylib:
	/usr/local/opt/freetype/lib/libfreetype.6.dylib (compatibility version 24.0.0, current version 24.2.0)
	/usr/lib/libbz2.1.0.dylib (compatibility version 1.0.0, current version 1.0.5)
	@rpath/libpng16.16.dylib (compatibility version 54.0.0, current version 54.0.0)
	/usr/lib/libz.1.dylib (compatibility version 1.0.0, current version 1.2.5)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)
```

You of course only need to run this once, well at least until you replace the library in case of moving to a newer version or so.

In addition to these libraries, you need to create a `plugins` directory and copy over the following VLC libraries, which are normally located in `/Applications/VLC.app/Contents/MacOS/plugins/`:

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

If you only want to build ES-DE to be used on your own computer, there's the option to skip the entire bundling of the libraries and use the ones already installed using Homebrew, meaning you can skip the previous .dylib copying. To do so, run CMake with the following option:

```
cmake -DAPPLE_SKIP_INSTALL_LIBS=ON .
```

This also affects the .dmg package generation using cpack, so if this option is enabled, the package will be unusable for anyone but yourself as the required libraries will not be bundled with the application.

On macOS you can install the application as a normal user, i.e. no root privileges are required. Simply run the following:

```
make install
```

This will be the directory structure for the installation:

```
/Applications/EmulationStation Desktop Edition.app/Contents/Info.plist
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/EmulationStation
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libSDL2-2.0.0.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libfreeimage.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libfreetype.6.dylib
/Applications/EmulationStation Desktop Edition.app/Contents/MacOS/libpng16.16.dylib
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

* \<home\>/.emulationstation/resources/
* \<ES-DE executable directory\>/../Resources/resources/
* \<ES-DE executable directory\>/resources/

**Note:** The resources directory is critical, without it the application won't start.

And it will look in the following locations for the themes, also in the listed order:

* \<HOME\>/.emulationstation/themes/
* \<ES-DE executable directory\>/../Resources/themes/
* \<ES-DE executable directory\>/themes/

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
CPack: - package: /Users/myusername/emulationstation-de/EmulationStation-DE-1.0.0-x64.dmg generated.
```

Generating .dmg installers on older version of macOS seems to make them forward compatible to a pretty good extent, for instance building on El Capitan seems to generate an application that is usable on Catalina and Big Sur. The other way around does however not seem to be true, which is quite unsurprising.

**Special considerations regarding run-paths:**

Even after considerable effort I've been unable to make CMake natively set correct rpaths for the EmulationStation binary on macOS. Therefore a hack/workaround is in place that uses install_name_tool to change absolute paths to rpaths for most of the bundled libraries.

This is certainly not perfect as the versions of the libraries are hardcoded inside es-app/CMakeLists.txt. Therefore always check that all the rpaths are set correctly if you intend to create a .dmg image that will be used on other computers than your own.

Simply run `otool -L EmulationStation` and verify that the result looks something like this:

```
EmulationStation:
	/usr/lib/libcurl.4.dylib (compatibility version 7.0.0, current version 8.0.0)
	@rpath/libfreeimage.dylib (compatibility version 3.0.0, current version 3.18.0)
	@rpath/libfreetype.6.dylib (compatibility version 24.0.0, current version 24.2.0)
	@rpath/libSDL2-2.0.0.dylib (compatibility version 13.0.0, current version 13.0.0)
	/System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa (compatibility version 1.0.0, current version 22.0.0)
	@rpath/libvlc.dylib (compatibility version 12.0.0, current version 12.0.0)
	/System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL (compatibility version 1.0.0, current version 1.0.0)
	/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)
```

If any of the lines that should start with @rpath instead has an absolute path, then you have a problem and need to modify the install_name_tools parameters in es-app/CMakeLists.txt.

This is what an incorrect line would look like:

`/usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib (compatibility version 13.0.0, current version 13.0.0)`

This is the section in es-app/CMakeLists.txt that would need to be modified:

```
add_custom_command(TARGET EmulationStation POST_BUILD COMMAND ${CMAKE_INSTALL_NAME_TOOL}
        -change /usr/local/opt/freeimage/lib/libfreeimage.dylib @rpath/libfreeimage.dylib
        -change /usr/local/opt/freetype/lib/libfreetype.6.dylib @rpath/libfreetype.6.dylib
        -change /usr/local/opt/libpng/lib/libpng16.16.dylib @rpath/libpng16.16.dylib
        -change /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib @rpath/libSDL2-2.0.0.dylib
        $<TARGET_FILE:EmulationStation>)
```


## Building on Windows

Both MSVC and MinGW (GCC) work fine for building ES-DE on Windows.

As much as I would like to exclude support for MSVC, this proprietary compiler simply works much better when developing as it's much faster than MinGW when linking debug builds and when actually debugging. For release builds though MinGW is very fast and it seems as if ES-DE starts around 18% faster when built with MinGW so this compiler probably generates more efficient code overall. As well MSVC requires a lot more junk DLL files to be distributed with the application so it's not a good candidate for the final release build.

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

If you intend to use both MinGW and MSVC on the machine, it's probably better to exclude CMake and install it manually as described in the MinGW setup instructions below.

The way the MSVC environment works is that a specific developer shell is provided where the build environment is properly configured. You open this from the Start menu via `Visual Studio 2019` -> `Visual Studio tools` -> `VC` -> `x64 Native Tools Command Prompt for VS 2019`.

It's very important to choose the x64-specific shell and not the x86 variant, as ES-DE will only compile as a 64-bit application.

**MinGW (GCC) setup:**

Download the following packages and install them:

[https://gitforwindows.org](https://gitforwindows.org)

[https://cmake.org/download](https://cmake.org/download)

[https://jmeubank.github.io/tdm-gcc](https://jmeubank.github.io/tdm-gcc)

After installation, make a copy of `TDM-GCC-64/bin/mingw32-make` to `make` just for convenience.

Note that most GDB builds for Windows have broken Python support so that pretty printing won't work. The recommended MinGW installation should work fine though.

**Other preparations:**

Install your editor of choice, I use [VSCode](https://code.visualstudio.com).

Configure Git. I won't get into the details on how it's done, but there are many resources available online to support with this. The `Git Bash` shell shipped with Git for Windows is very useful though as it's somewhat reproducing a Unix environment using MinGW/MSYS2.

It's strongly recommended to set line breaks to Unix-style (line feed only) directly in the editor, although it can also be configured in Git for conversion during commits. The source code for ES-DE only uses Unix-style line breaks.

In the description below it's assumed that all build steps for MinGW/GCC will be done in the Git Bash shell, and all the build steps for MSVC will be done in the MSVC developer console (x64 Native Tools Command Prompt for VS).


**Download the dependency packages:**

FreeImage (binary distribution) \
[https://sourceforge.net/projects/freeimage](https://sourceforge.net/projects/freeimage)

cURL (Windows 64 bit binary, the MinGW version even if using MSVC) \
[https://curl.haxx.se/download.html](https://curl.haxx.se/download.html)

SDL2 (development libraries, MinGW or VC/MSVC) \
[https://www.libsdl.org/download-2.0.php](https://www.libsdl.org/download-2.0.php)

libVLC (both win64 binary and source distributions) \
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

For RapidJSON, you don't need to compile it, you just need the include files:

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

**Setup the include directories:**

As there is no standardized include directory structure in Windows, you need to provide the include files manually.

Make a directory in your build environment tree, for instance under `C:\Programming\include`

Copy the include files from cURL, FreeImage, FreeType, GLEW, pugixml, RapidJSON, SDL2 and VLC to this directory.

You may need to create the SDL2 directory manually and copy the header files there.

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

**Copy the required library files to the ES-DE build directory:**

As there's no package manager in Windows and no way to handle dependencies, we need to ship all the required shared libraries with the application.

Copy the files to the `emulationstation-de` build directory. Most of them will come from the packages that were provided in the previous steps of this guide.

**Required files for MSVC:**
```
FreeImage.dll
FreeImage.lib
freetype.dll
freetype.lib
glew32.dll
glew32.lib
libcurl-x64.dll
libcrypto-1_1-x64.dll     (from the OpenSSL package, located in Git MinGW/MSYS2 under \mingw64\bin)
libssl-1_1-x64.dll        (from the OpenSSL package, located in Git MinGW under \mingw64\bin)
libvlc.dll
libvlccore.dll
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

In addition to these files, you need libcurl-x64.lib and libvlc.lib, but these are not available for download so you need to generate them yourself from their corresponding DLL files. Do this inside the respective library directory and not within the emulationstation-de folder.

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
FreeImage.dll
glew32.dll
libcrypto-1_1-x64.dll     (from the OpenSSL package, located in Git MinGW/MSYS2 under \mingw64\bin)
libcurl-x64.dll
libfreetype.dll
libpugixml.dll
libSDL2main.a
libssl-1_1-x64.dll        (from the OpenSSL package, located in Git MinGW under \mingw64\bin)
libvlc.dll
libvlccore.dll
SDL2.dll
vcomp140.dll              (From Visual C++ Redistributable for Visual Studio 2015, 32-bit version)
```

**Required files for both MSVC and MinGW:**

In addition to the files above, you need to copy some libraries from the VLC `plugins` folder to be able to play video files. There is a subdirectory structure under the plugins folder but there is no requirement to retain this as libVLC apparently looks recursively for the .dll files.

It's a bit tricky to know which libraries are really needed. But as the plugins directory is around 120 MB (as of VLC version 3.0.11), we definitely only want to copy the files we need.

The following files seem to be required to play most video and audio formats (place them in `emulationstation-de\plugins`):

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

The combined size of these files is around 22 MB which is more reasonable.

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

Unfortunately nmake does not support parallel compiles so it's very slow. There are third party solutions to get multi-core building working with MSVC, but I've not investigated this much. Embrace this as a retro experience as in the 1990s we normally just had a single core available in our computers.

Be aware that MSVC links against different VC++ libraries for debug and release builds (e.g. MSVCP140.dll or MSVCP140d.dll), so any NSIS package made from a debug build will not work on computers without the MSVC development environment installed.

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

You run a parallel build using multiple CPU cores with the `-j` flag, for example, `make -j6`.

Note that compilation time is much longer than on Unix or macOS, and linking time is unendurable for a debug build (around 10 times longer on my computer compared to Linux). The debug binary is also much larger than on Unix.


**Running with OpenGL software rendering:**

If you are running Windows in a virtualized environment such as QEMU-KVM that does not support HW accelerated OpenGL, you can install the Mesa3D for Windows library, which can be downloaded at [https://fdossena.com/?p=mesa/index.frag](https://fdossena.com/?p=mesa/index.frag).

You simply extract the opengl32.dll file into the ES-DE directory and this will enable the llvmpipe renderer. The performance will be terrible of course, but everything should work and it should be good enough for test building on Windows without having to reboot your computer to a native Windows installation. (Note that you may need to copy opengl32.dll to your RetroArch installation directory as well to get the emulators to work correctly.)

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
CPack: - package: C:/Programming/emulationstation-de/EmulationStation-DE-1.0.0-x64.exe generated.
```

The default installation directory suggested by the installer is `C:\Program Files\EmulationStation-DE` but this can of course be changed by the user.

ES will look in the following locations for the resources, in the listed order:

* \<home\>\\.emulationstation\resources\
* \<ES-DE executable directory\>\resources\

The resources directory is critical, without it the application won't start.

And it will look in the following locations for the themes, also in the listed order:

* \<home\>\\.emulationstation\themes\
* \<ES-DE executable directory\>\themes\

The theme is not mandatory to start the application, but ES-DE will be basically useless without it.

So the home directory will always take precedence, and any resources or themes located there will override the ones in the path of the ES executable.

**Setting up a portable installation:**

It's possible to easily create a portable installation of ES-DE for Windows, for example on a USB memory stick.

For the sake of this example, let's assume that the removable media has the device name `f:\`.

* Copy the EmulationStation-DE installation directory to f:\
* Create the directory `ES-DE_Home` directly under f:\
* Copy your game ROMs into `f:\ES-DE_Home\ROMs`
* Copy your emulators to f:\ (such as the RetroArch directory)
* Create the file `start_es.bat` directly under f:\

Add the following lines to the start_es.bat file:
```
@echo off
EmulationStation-DE\EmulationStation.exe --home %CD:~0,3%ES-DE_Home
```

The contents of f:\ should now look something like this:
```
EmulationStation-DE
ES-DE_Home
RetroArch
start_es.bat
```

Now run the batch script, ES should start and ask you to configure any attached controllers. Following this, check that everything works as expected, i.e. the gamelists are properly populated etc.

You can optionally skip the configuration of the controllers by copying any existing es_input.cfg file to `f:\ES-DE_Home\.emulationstation\`

Exit ES-DE and modify the file `f:\ES-DE_Home\.emulationstation\es_systems.cfg` to point to the emulators on the portable media.

For example, change from this:
```
<command>retroarch.exe -L %EMUPATH%\cores\snes9x_libretro.dll %ROM%</command>
```

To this:
```
<command>%ESPATH%\..\RetroArch\retroarch.exe -L %EMUPATH%\cores\snes9x_libretro.dll %ROM%</command>
```

The %ESPATH% variable is explained later in this document.

Following this, optionally copy any existing gamelist.xml and game media files to the removable media. By default these files should be located here:

```
f:\ES-DE_Home\.emulationstation\gamelists\
f:\ES-DE_Home\.emulationstation\downloaded_media\
```

You now have a fully functional portable retro gaming installation!

The portable installation works exactly as a normal installation, i.e. you can use the built-in scraper, edit metadata etc.


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

This is a bit tricky as the data needs to be converted to an internal format used by ES-DE. The original file is huge and most of the information is not required.

Go to [https://www.mamedev.org/release.php](https://www.mamedev.org/release.php) and select the Windows version, but only download the driver information in XML format and not MAME itself. This file will be named something like `mame0226lx.zip` and unzipping it will give you a file name such as `mame0226.xml`.

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

The reason to not simply replace the BIOS and devices files with the new version is that we want to retain entries from all older MAME versions as otherwise older ROM sets used on older MAME versions would have missing information. This is so as the MAME project sometimes removes older entries when they're reorganizing the ROM sets. By merging the files we retain backwards compatibility but still support the latest MAME version. To clarify, this of course does not affect the emulation itself, but rather the filtering of BIOS and device files inside ES-DE. The mamenames.xml file containing the translation of MAME ROM names to the full game names does not suffer from this problem as it's cumulative, which is why it is simply overwritten.


## Configuration

**~/.emulationstation/es_systems.cfg:**

ES-DE ships with a comprehensive `es_systems.cfg` configuration file, and as the logic is to use a `%ROMPATH%` variable to locate the ROM files (with a corresponding setting in `es_settings.cfg`), normally you shouldn't need to modify this file to the same extent as previous versions of EmulationStation. Still, see below in this document on how to adjust the es_systems.cfg file if required.

Upon first startup of the application, if there is no es_systems.cfg file present, it will be copied from the template subdirectory inside the resources directory. This is located in the installation path of the application, for instance `/usr/share/emulationstation/resources/templates` or `/usr/local/share/emulationstation/resources/templates` on Unix, `/Applications/EmulationStation Desktop Edition.app/Contents/Resources/resources/templates` on macOS and `C:\Program Files\EmulationStation-DE\resources\templates`on Windows.

The template file will be copied to `~/.emulationstation/es_systems.cfg`.

The tilde symbol `~` translates to `$HOME` on Unix and macOS, and to `%HOMEPATH%` on Windows.

Keep in mind that you have to set up your emulators separately from ES-DE as the es_systems.cfg file assumes that your emulator environment is properly configured.


**~/.emulationstation/es_settings.cfg:**

When ES-DE is first run, a configuration file will be created as `~/.emulationstation/es_settings.cfg`.

This file will contain all supported settings at their default values. Normally you shouldn't need to modify this file manually, instead you should be able to use the menu inside ES-DE to update all the necessary settings.

**Setting the ROM directory in es_settings.cfg:**

This complete configuration step can normally be skipped as you're presented with a dialog to change the ROM directory upon application startup if no game files are found.

By default, ES-DE looks in `~/ROMs` for the ROM files, where they are expected to be grouped into directories corresponding to the game systems, for example:

```
myusername@computer:~ROMs$ ls -1
c64
megadrive
pcengine
```

However, if you've saved your ROMs to another directory, you need to configure the ROMDirectory setting in es_settings.cfg.\
Here's an example:

`<string name="ROMDirectory" value="~/games/ROMs" />`

Keep in mind that you still need to group the ROMs into directories corresponding to the `<path>` tags in es_systems.cfg.

There is also support to add the variable %ESPATH% to the ROM directory setting, this will expand to the path where the ES executable is started from. You would normally not need this, but the option is there, should you require it for some reason.

Here is such an example:

`<string name="ROMDirectory" value="%ESPATH%/../ROMs" />`

**~/.emulationstation/es_input.cfg:**

You normally don't need to modify this file manually as it's created by the built-in input configuration step. This procedure is detailed in the [User guide](USERGUIDE.md#input-device-configuration).

If your controller and keyboard stop working, you can delete the `~/.emulationstation/es_input.cfg` file to make the input configuration screen re-appear on the next startup, or you can start ES-DE with the `--force-input-config` command line option.


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

As you can see above, you can override the home directory path using the `--home` flag. So by running for instance the command `emulationstation --home ~/games/emulation`, ES-DE will use `~/games/emulation/.emulationstation` as its base directory.

For the following options, the es_settings.cfg file is immediately updated/saved when passing the parameter:
```
--display
--fullscreen-normal
--fullscreen-borderless
--max-vram
--show-hidden-files
--show-hidden-games
```


## es_systems.cfg

The es_systems.cfg file contains the system configuration data for ES-DE, written in XML format. This defines the system name, the full system name, the ROM path, the allowed file extensions, the launch command, the platform (for scraping) and the theme to use.

ES-DE will only check in your home directory for an es_systems.cfg file, for example `~/.emulationstation/es_systems.cfg`, but if this file is not present, it will attempt to install it from the systems template directory as explained earlier in this document.

It doesn't matter in which order you define the systems as they will be sorted by the full system name inside the application, but it's still probably a good idea to add them in alphabetical order to make the file easier to maintain.

Below is an overview of the file layout with various examples. For a real system entry there can of course not be multiple entries for the same tag such as the multiple \<command\> entries listed here.

```xml
<?xml version="1.0"?>
<!-- This is the EmulationStation-DE game systems configuration file. -->
<systemList>
    <!-- Any tag not explicitly described as optional in the description is mandatory.
    If omitting a mandatory tag, ES-DE will skip the system entry during startup. -->
    <system>
        <!-- A short name, used internally. -->
        <name>snes</name>

        <!-- The full system name, used for sorting the systems, for selecting the systems to multi-scrape etc. -->
        <fullname>Super Nintendo Entertainment System</fullname>

        <!-- The path to look for ROMs in. '~' will be expanded to $HOME or %HOMEPATH%, depending on the operating system.
        The optional %ROMPATH% variable will expand to the path defined in the setting ROMDirectory in es_settings.cfg.
        All subdirectories (and non-recursive links) will be included. -->
        <path>%ROMPATH%/snes</path>

        <!-- A list of extensions to search for, delimited by any of the whitespace characters (", \r\n\t").
        You must include the period at the start of the extension and it's case sensitive. -->
        <extension>.smc .SMC .sfc .SFC .swc .SWC .fig .FIG .bs .BS .bin .BIN .mgd .MGD .7z .7Z .zip .ZIP</extension>

        <!-- The command executed when a game is launched. A few special variables are replaced if found for a command tag (see below).
        This example for Unix would run RetroArch with the the snes9x_libretro core, using an absolute path to the core.
        If there are spaces in the path or file name, you must enclose them in quotation marks, for example:
        retroarch -L "~/my configs/retroarch/cores/snes9x_libretro.so" %ROM% -->
        <command>retroarch -L ~/.config/retroarch/cores/snes9x_libretro.so %ROM%</command>

        <!-- This example for Unix searches the pre-configured core directories for the snes9x_libretro RetroArch core, see more
        info about this below. Spaces are not allowed in the core file names. -->
        <command>retroarch -L %COREPATH%/snes9x_libretro.so %ROM%</command>

        <!-- This is an example for macOS. It uses the %EMUPATH% variable to point to the RetroArch cores relative to the emulator binary.
        As there is a somehow standardized installation structure for this operating system, an absolute path is defined for the emulator. -->
        <command>/Applications/RetroArch.app/Contents/MacOS/RetroArch -L %EMUPATH%/../Resources/cores/snes9x_libretro.dylib %ROM%</command>

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
        expanded to the directory of the ES executable. -->
        <command>"%ESPATH%\..\RetroArch\retroarch.exe" -L "%ESPATH%\..\RetroArch\cores\snes9x_libretro.dll" %ROM%</command>

        <!-- The platform(s) to use when scraping. You can see the full list of supported platforms in es-app/src/PlatformId.cpp.
        The entry is case insensitive as it will be converted to lower case during startup.
        This tag is optional but the system can't be scraped if it's left out.
        You can use multiple platforms too, delimited with any of the whitespace characters (", \r\n\t"), e.g. "megadrive, genesis". -->
        <platform>snes</platform>

        <!-- The theme to load from the current theme set. See THEMES.md for more information.
        This tag is optional and if it doesn't exist, ES-DE will attempt to find a theme with the same name as the system name.
        If no such match is made, the system will be unthemed.
        It's strongly recommended to include this tag even if it's just to clarify that the theme should correspond to the system name. -->
        <theme>snes</theme>
    </system>
</systemList>
```

The following variable is expanded for the `path` tag:

`%ROMPATH%` - Replaced with the path defined in the setting ROMDirectory in es_settings.cfg.

The following variables are expanded for the `command` tag:

`%ROM%` - Replaced with the absolute path to the selected ROM, with most Bash special characters escaped with a backslash.

`%ROMRAW%`	- Replaced with the unescaped, absolute path to the selected ROM.  If your emulator is picky about paths, you might want to use this instead of %ROM%, but enclosed in quotes.

`%BASENAME%` - Replaced with the "base" name of the path to the selected ROM. For example, a path of `/foo/bar.rom`, this tag would be `bar`. This tag is useful for setting up AdvanceMAME.

`%EMUPATH%` - Replaced with the path to the emulator binary. This is expanded using either the PATH environmental variable of the operating system, or using an absolute emulator path if this has been defined.

`%ESPATH%` - Replaced with the path to the ES-DE binary. Mostly useful for portable emulator installations, for example on a USB memory stick.

`%COREPATH%` - The core file defined after this variable will be searched in each of the directories listed in the setting EmulatorCorePath in es_settings.cfg. This is done until the first match occurs, or until the directories are exhausted and no core file was found. This makes it possible to create a more general es_systems.cfg file but still support the variation between different operating systems and different types of emulator installations (e.g. installed via the OS repository, via Snap, compiled from source etc.). This is primarily intended for Unix using RetroArch but it can also be used on macOS and Windows and for other emulators that utilize discrete emulator cores. For macOS and Windows the EmulatorCorePath setting is blank by default, and for Unix it's set to the following value: `~/.config/retroarch/cores:~/snap/retroarch/current/.config/retroarch/cores:/usr/lib/x86_64-linux-gnu/libretro:/usr/lib64/libretro:/usr/lib/libretro:/usr/local/lib/libretro:/usr/pkg/lib/libretro`. Note that colons are used to separate the directories on Unix and macOS and that semicolons are used on Windows. This path setting can be changed from within the GUI, as described in the [User guide](USERGUIDE.md#other-settings-1). Never use quotation marks around the directories for this setting. It's strongly adviced to not add spaces to directory names, but if still done, ES-DE will handle this automatically by adding the appropriate quotation marks to the launch command. You can also use the %EMUPATH% and %ESPATH% variables within the EmulatorCorePath setting, which leads to quite flexible configuration options.

Here are some additional real world examples of system entries, the first one for Unix:

```xml
  <system>
    <name>dos</name>
    <fullname>DOS (PC)</fullname>
    <path>%ROMPATH%/dos</path>
    <extension>.bat .BAT .com .COM .exe .EXE .7z .7Z .zip .ZIP</extension>
    <command>retroarch -L %COREPATH%/dosbox_core_libretro.so %ROM%</command>
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
    <command>/Applications/RetroArch.app/Contents/MacOS/RetroArch -L %EMUPATH%/../Resources/cores/nestopia_libretro.dylib %ROM%</command>
    <platform>nes</platform>
    <theme>nes</theme>
  </system>
```

And one for Windows:

```xml
  <system>
    <name>sega32x</name>
    <fullname>Sega Mega Drive 32X</fullname>
    <path>%ROMPATH%\sega32x</path>
    <extension>.bin .BIN .gen .GEN .smd .SMD .md .MD .32x .32X .cue .CUE .iso .ISO .sms .SMS .68k .68K .7z .7Z .zip .ZIP</extension>
    <command>retroarch.exe -L %EMUPATH%\cores\picodrive_libretro.dll %ROM%</command>
    <platform>sega32x</platform>
    <theme>sega32x</theme>
  </system>

```


## gamelist.xml

The gamelist.xml file for a system defines the metadata for its entries, such as the game names, descriptions, release dates and ratings.

As of the fork to EmulationStation Desktop Edition, game media information no longer needs to be defined in the gamelist.xml files. Instead the application will look for any media matching the ROM filename. The media path where to look for game media is configurable either manually in `es_settings.cfg` or via the GUI. If configured manually in es_settings.cfg, it looks something like this:

```
<string name="MediaDirectory" value="~/games/images/emulationstation" />
```

The default directory is `~/.emulationstation/downloaded_media`

You can use ES-DE's scraping tools to populate the gamelist.xml files, or manually update individual entries using the metadata editor. All of this is explained in the [User guide](USERGUIDE.md).

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
* `datetime` - a date and, potentially, a time.  These are encoded as an ISO string, in the following format: "%Y%m%dT%H%M%S%F%q".  For example, the release date for Chrono Trigger is encoded as "19950311T000000" (no time specified)
* `bool` - a true or false value

Some metadata is also marked as "statistic" - these are kept track of by ES-DE and do not show up in the metadata editor. They are shown in certain views (for example, the detailed view and the video view both show `lastplayed`, although the label can be disabled by the theme).

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


## Custom event scripts

There are numerous locations throughout ES-DE where custom scripts will be executed if the option to do so has been enabled in the settings. You'll find the option on the Main menu under `Other settings`. By default it's deactivated so be sure to enable it to use this feature.

The approach is quite straightforward, ES-DE will look for any files inside a script directory that corresponds to the event that is triggered and will then execute all these files.

We'll go through two examples:
* Create a log file that will record the start and end time for each game we play, letting us see how much time we spend on retro-gaming
* Change the system resolution when launching and returning from a game in order to run the emulator at a lower resolution than ES-DE

**Note:** The following examples are for Unix systems, but it works the same way in macOS (which is also Unix after all), and on Windows (although .bat batch files are then used instead of shell scripts and any spaces in the parameters are not escaped as is the case on Unix).

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

Don't forget to make the scripts executable (e.g. 'chmod 755 ./game_start_logging.sh').

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

Don't forget to make the scripts executable (e.g. 'chmod 755 ./set_resolution_1080p.sh').

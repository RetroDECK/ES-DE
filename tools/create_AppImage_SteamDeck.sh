#!/usr/bin/bash
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  create_AppImage_SteamDeck.sh
#
#  Runs the complete process of building a Linux AppImage specific to the Valve Steam Deck.
#  The SDL library is also built and included in the AppImage.
#
#  This script has only been tested on Ubuntu 20.04 LTS.
#

# How many CPU threads to use for the compilation.
JOBS=4

SDL_RELEASE_TAG=release-2.24.1
SDL_SHARED_LIBRARY=libSDL2-2.0.so.0.2400.1

echo "Building Steam Deck AppImage..."

if [ ! -f .clang-format ]; then
  echo "You need to run this script from the root of the repository."
  exit
fi

if [ ! -f appimagetool-x86_64.AppImage ]; then
  echo -e "Can't find appimagetool-x86_64.AppImage, downloading the latest version...\n"
  wget "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
fi

chmod a+x appimagetool-x86_64.AppImage

if [ ! -f linuxdeploy-x86_64.AppImage ]; then
  echo -e "Can't find linuxdeploy-x86_64.AppImage, downloading the latest version...\n"
  wget "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
fi

chmod a+x linuxdeploy-x86_64.AppImage

if [ ! -f external/SDL/build/${SDL_SHARED_LIBRARY} ]; then
    echo
    echo "Building the SDL library..."
    cd external
    rm -rf SDL
    git clone https://github.com/libsdl-org/SDL.git
    cd SDL
    git checkout $SDL_RELEASE_TAG
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -S .. -B .
    make -j${JOBS}
    cd ../../..
else
    echo
    echo -e "The SDL library has already been built, skipping this step\n"
fi

rm -rf ./AppDir
mkdir AppDir

rm -f CMakeCache.txt
cmake -DAPPIMAGE_BUILD=on -DSTEAM_DECK=on .
make clean
make -j${JOBS}
make install DESTDIR=AppDir
cd AppDir
ln -s usr/bin/emulationstation AppRun
ln -s usr/share/pixmaps/emulationstation.svg .
ln -s usr/share/applications/org.es_de.emulationstation-de.desktop .
ln -s emulationstation.svg .DirIcon
cd usr/bin
ln -s ../share/emulationstation/resources .
ln -s ../share/emulationstation/themes .
cd ../../..

./linuxdeploy-x86_64.AppImage -l /lib/x86_64-linux-gnu/libOpenGL.so.0 -l /lib/x86_64-linux-gnu/libgio-2.0.so.0 --appdir AppDir
cp external/SDL/build/${SDL_SHARED_LIBRARY} AppDir/usr/lib/libSDL2-2.0.so.0
./appimagetool-x86_64.AppImage AppDir

mv EmulationStation_Desktop_Edition-x86_64.AppImage EmulationStation-DE-x64_SteamDeck.AppImage
echo -e "\nCreated AppImage EmulationStation-DE-x64_SteamDeck.AppImage"

#!/usr/bin/bash
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  create_AppImage_SteamDeck.sh
#
#  Runs the complete process of building a Linux AppImage specific to the Valve Steam Deck.
#
#  This script has only been tested on Ubuntu 20.04 LTS.
#

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

rm -rf ./AppDir
mkdir AppDir

rm -f CMakeCache.txt
cmake -DSTEAM_DECK=on .
make clean
make -j8
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
./appimagetool-x86_64.AppImage AppDir

mv EmulationStation_Desktop_Edition-x86_64.AppImage EmulationStation-DE-x64_SteamDeck.AppImage
echo -e "\nCreated AppImage EmulationStation-DE-x64_SteamDeck.AppImage"

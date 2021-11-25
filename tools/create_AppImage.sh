#!/usr/bin/bash
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  create_AppImage.sh
#
#  Runs the complete process of building a Linux AppImage.
#  The BUNDLED_CERTS option is very important as otherwise cURL will not work on all
#  distributions as for example Debian-based systems place the TLS certificates in a
#  different location under /etc than Fedora and openSUSE.
#
#  This script has only been tested on Ubuntu 20.04 LTS. It's recommended to only build
#  AppImages on this operating system for maximum compatibility.
#

echo "Building AppImage..."

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

rm CMakeCache.txt
cmake -DBUNDLED_CERTS=on .
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

VERSION=$(grep PROGRAM_VERSION_STRING es-app/src/EmulationStation.h | cut -f3 -d" " | sed s/\"//g)
mv EmulationStation_Desktop_Edition-x86_64.AppImage emulationstation-de-${VERSION}-x64.AppImage

echo -e "\nCreated AppImage emulationstation-de-${VERSION}-x64.AppImage"

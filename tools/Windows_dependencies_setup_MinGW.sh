#!/bin/sh
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  Windows_dependencies_setup_MinGW.sh
#
#  Downloads and prepares the external dependencies for building in-tree.
#  If the directories already exist they will be removed and the libraries will be downloaded again.
#
#  This script needs to run from the root of the repository.
#

if [ ! -f .clang-format ]; then
  echo "You need to run this script from the root of the repository."
  exit
fi

echo -e "Setting up dependencies in the ./external directory..."

cd external

echo -e "\nSetting up curl"
rm -rf curl*

curl -O https://curl.se/windows/dl-8.1.2_3/curl-8.1.2_3-win64-mingw.zip
unzip curl-8.1.2_3-win64-mingw.zip

if [ ! -d curl-8.1.2_3-win64-mingw ]; then
  echo "curl directory is missing, aborting."
  exit
fi

mv curl-8.1.2_3-win64-mingw curl

cp -p curl/bin/libcurl-x64.dll ..

echo -e "\nSetting up GLEW"
rm -rf glew*

curl -LO https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0.zip
unzip glew-2.1.0.zip

if [ ! -d glew-2.1.0 ]; then
  echo "GLEW directory is missing, aborting."
  exit
fi

mv glew-2.1.0 glew

echo -e "\nSetting up FreeType"
rm -rf freetype

git clone https://github.com/freetype/freetype.git

if [ ! -d freetype ]; then
  echo "FreeType directory is missing, aborting."
  exit
fi

cd freetype
git checkout VER-2-13-0
mkdir build
cd ..

echo -e "\nSetting up FreeImage"
rm -rf FreeImage

curl -LO https://downloads.sourceforge.net/project/freeimage/Binary%20Distribution/3.18.0/FreeImage3180Win32Win64.zip
unzip FreeImage3180Win32Win64.zip

if [ ! -d FreeImage ]; then
  echo "FreeImage directory is missing, aborting."
  exit
fi

cp -p FreeImage/Dist/x64/FreeImage.dll ..

echo -e "\nSetting up libgit2"
rm -rf libgit2

git clone https://github.com/libgit2/libgit2.git

if [ ! -d libgit2 ]; then
  echo "libgit2 directory is missing, aborting."
  exit
fi

cd libgit2
git checkout v1.6.4
mkdir build
cd ..

echo -e "\nSetting up Poppler"
rm -rf poppler*

curl -JO https://gitlab.com/es-de/emulationstation-de/-/package_files/83268133/download
unzip Poppler_Windows_MinGW_23.06.0-1.zip

if [ ! -d poppler ]; then
  echo "Poppler directory is missing, aborting."
  exit
fi

cp -p poppler/Library/bin/*.dll ../es-pdf-converter

echo -e "\nSetting up pugixml"
rm -rf pugixml

git clone https://github.com/zeux/pugixml.git

if [ ! -d pugixml ]; then
  echo "pugixml directory is missing, aborting."
  exit
fi

cd pugixml
git checkout v1.13
cd ..

echo -e "\nSetting up SDL"
rm -rf SDL2*

curl -O https://libsdl.org/release/SDL2-devel-2.26.5-mingw.tar.gz

tar xvzf SDL2-devel-2.26.5-mingw.tar.gz
# Needed due to some kind of file system race condition that sometimes occurs on Windows.
sleep 1

if [ ! -d SDL2-2.26.5 ]; then
  echo "SDL directory is missing, aborting."
  exit
fi

mv SDL2-2.26.5 SDL2

mv SDL2/x86_64-w64-mingw32/include/SDL2 SDL2/
cp -p SDL2/x86_64-w64-mingw32/lib/libSDL2main.a ..
cp -p SDL2/x86_64-w64-mingw32/bin/SDL2.dll ..

echo -e "\nSetting up FFmpeg"
rm -rf ffmpeg*

# This package should be available for download for two years.
curl -LO https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2022-09-30-12-41/ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1.zip
unzip ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1.zip

if [ ! -d ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1 ]; then
  echo "FFmpeg directory is missing, aborting."
  exit
fi

mv ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1 ffmpeg

cp -p ffmpeg/bin/avcodec-59.dll ..
cp -p ffmpeg/bin/avfilter-8.dll ..
cp -p ffmpeg/bin/avformat-59.dll ..
cp -p ffmpeg/bin/avutil-57.dll ..
cp -p ffmpeg/bin/postproc-56.dll ..
cp -p ffmpeg/bin/swresample-4.dll ..
cp -p ffmpeg/bin/swscale-6.dll ..

echo -e "\nSetting up OpenSSL"

if [ ! -f /c/Program\ Files/OpenSSL-Win64/libcrypto-1_1-x64.dll ]; then
  curl -O https://slproweb.com/download/Win64OpenSSL_Light-1_1_1m.exe
  # Run the installer.
  ./Win64OpenSSL_Light-1_1_1m.exe
fi

# Return to the root of the repository.
cd ..

cp -p /c/Program\ Files/OpenSSL-Win64/libcrypto-1_1-x64.dll .
cp -p /c/Program\ Files/OpenSSL-Win64/libssl-1_1-x64.dll .

echo -e "\nCopying DLL files from Windows\System32"

cp /c/Windows/System32/vcomp140.dll .

echo
echo "Done setting up all dependencies."

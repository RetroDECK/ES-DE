#!/usr/bin/bash
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  create_AppImage.sh
#
#  Runs the complete process of building a Linux AppImage.
#  The BUNDLED_CERTS option is very important as otherwise curl will not work on all
#  distributions as for example Debian-based systems place the TLS certificates in a
#  different location under /etc than Fedora and openSUSE.
#
#  The SDL library is also built and included in the AppImage.
#
#  This script has only been tested on Ubuntu 20.04 LTS and 22.04 LTS.
#

# How many CPU threads to use for the compilation.
JOBS=4

SDL_RELEASE_TAG=release-2.26.3
SDL_SHARED_LIBRARY=libSDL2-2.0.so.0.2600.3

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

if [ ! -f external/SDL/build/${SDL_SHARED_LIBRARY} ]; then
  echo
  echo "Building the SDL library..."
  cd external
  rm -rf SDL
  git clone https://github.com/libsdl-org/SDL.git
  cd SDL
  git checkout $SDL_RELEASE_TAG

# Temporary workaround until a proper fix has been implemented in the SDL library:
# https://github.com/libsdl-org/SDL/issues/7160
cat << EOF | git apply
diff --git a/src/video/SDL_video.c b/src/video/SDL_video.c
index 0809706c2..bbabfcaa7 100644
--- a/src/video/SDL_video.c
+++ b/src/video/SDL_video.c
@@ -4342,17 +4342,20 @@ SDL_GetWindowWMInfo(SDL_Window * window, struct SDL_SysWMinfo *info)
 void
 SDL_StartTextInput(void)
 {
+/*
     SDL_Window *window;
-
+*/
     /* First, enable text events */
     (void)SDL_EventState(SDL_TEXTINPUT, SDL_ENABLE);
     (void)SDL_EventState(SDL_TEXTEDITING, SDL_ENABLE);

     /* Then show the on-screen keyboard, if any */
+/*
     window = SDL_GetFocusWindow();
     if (window && _this && _this->ShowScreenKeyboard) {
         _this->ShowScreenKeyboard(_this, window);
     }
+*/

     /* Finally start the text input system */
     if (_this && _this->StartTextInput) {
EOF

  mkdir build
  cd build
  cmake -DCMAKE_BUILD_TYPE=Release -S .. -B .

  if [ $(grep PKG_PIPEWIRE_VERSION:INTERNAL= CMakeCache.txt) = "PKG_PIPEWIRE_VERSION:INTERNAL=" ]; then
    echo
    echo -e "The SDL library is not configured with PipeWire support, aborting."
    exit
  fi

  make -j${JOBS}
  cd ../../..
else
  echo
  echo -e "The SDL library has already been built, skipping this step\n"
fi

rm -rf ./AppDir
mkdir AppDir

rm -f CMakeCache.txt
cmake -DAPPIMAGE_BUILD=on -DBUNDLED_CERTS=on .
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

./linuxdeploy-x86_64.AppImage -l /lib/x86_64-linux-gnu/libOpenGL.so.0 -l /lib/x86_64-linux-gnu/libGLdispatch.so.0 -l /lib/x86_64-linux-gnu/libgio-2.0.so.0 --appdir AppDir
cp external/SDL/build/${SDL_SHARED_LIBRARY} AppDir/usr/lib/libSDL2-2.0.so.0
./appimagetool-x86_64.AppImage AppDir

#VERSION=$(grep PROGRAM_VERSION_STRING es-app/src/EmulationStation.h | cut -f3 -d" " | sed s/\"//g)
#mv EmulationStation_Desktop_Edition-x86_64.AppImage EmulationStation-DE-${VERSION}-x64.AppImage
#echo -e "\nCreated AppImage EmulationStation-DE-${VERSION}-x64.AppImage"

mv EmulationStation_Desktop_Edition-x86_64.AppImage EmulationStation-DE-x64.AppImage
echo -e "\nCreated AppImage EmulationStation-DE-x64.AppImage"

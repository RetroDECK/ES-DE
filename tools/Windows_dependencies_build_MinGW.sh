#!/bin/sh
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  Windows_dependencies_build_MinGW.sh
#
#  Builds the external dependencies in-tree.
#  The Windows_dependencies_setup_MinGW.sh script must have been executed prior to this.
#  All libraries will be recompiled from scratch every time.
#
#  This script needs to run from the root of the repository.
#

# How many CPU threads to use for the compilation.
JOBS=4

if [ ! -f .clang-format ]; then
  echo "You need to run this script from the root of the repository."
  exit
fi

cd external

if [ ! -d pugixml ]; then
  echo "You need to first run tools/Windows_dependencies_setup_MinGW.sh to download and configure the dependencies."
  exit
fi

echo "Building all dependencies in the ./external directory..."

echo -e "\nBuilding GLEW"

if [ ! -d glew-2.1.0 ]; then
  echo "GLEW directory is missing, aborting."
  exit
fi

cd glew-2.1.0
make clean
make -j${JOBS} 2>/dev/null
cp lib/glew32.dll ../..
cd ..

echo -e "\nBuilding FreeType"

if [ ! -d freetype/build ]; then
  echo "FreeType directory is missing, aborting."
  exit
fi

cd freetype/build
rm -f CMakeCache.txt
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON ..
make clean
make -j${JOBS}
strip libfreetype.dll
cp libfreetype.dll ../../..
cd ../..

echo -e "\nBuilding pugixml"

if [ ! -d pugixml ]; then
  echo "pugixml directory is missing, aborting."
  exit
fi

cd pugixml
rm -f CMakeCache.txt
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON .
make clean
make -j${JOBS}
strip libpugixml.dll
cp libpugixml.dll ../..
cd ..

echo
echo "Done building all dependencies."

#!/bin/sh
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  macOS_dependencies_build.sh
#
#  Builds the external dependencies in-tree.
#  The macOS_dependencies_setup.sh script must have been executed before this one.
#  All sources will be recompiled from scratch every time this script is run.
#  If manually compiling individual libraries, don't forget to set MACOSX_DEPLOYMENT_TARGET.
#
#  This script needs to run from the root of the repository.
#

export MACOSX_DEPLOYMENT_TARGET=10.14

# How many CPU threads to use for the compilation.
JOBS=4

if [ ! -f .clang-format ]; then
  echo "You need to run this script from the root of the repository."
  exit
fi

cd external

if [ ! -d FFmpeg ]; then
  echo "You need to first run tools/macOS_dependencies_setup.sh to download and configure the dependencies."
  exit
fi

echo "Building all dependencies in the ./external directory...\n"

export PKG_CONFIG_PATH=$(pwd)/../local_install/lib/pkgconfig

echo "Building libpng"

if [ ! -d libpng ]; then
  echo "libpng directory is missing, aborting."
  exit
fi

cd libpng
rm -f CMakeCache.txt
if [ $(uname -m) == "arm64" ]; then
  cmake -DCMAKE_BUILD_TYPE=Release -DPNG_SHARED=off -DPNG_ARM_NEON=off -DCMAKE_INSTALL_PREFIX=$(pwd)/../local_install .
else
  cmake -DCMAKE_BUILD_TYPE=Release -DPNG_SHARED=off -DCMAKE_INSTALL_PREFIX=$(pwd)/../local_install .
fi
make clean
make -j${JOBS}
make install
cd ..

echo
echo "\nBuilding FreeType"

if [ ! -d freetype/build ]; then
  echo "FreeType directory is missing, aborting."
  exit
fi

cd freetype/build
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=on -DBUILD_SHARED_LIBS=on -DCMAKE_MACOSX_RPATH=on -DCMAKE_INSTALL_PREFIX=$(pwd)/../../local_install -S .. -B .
make clean
make -j${JOBS}
cp libfreetype.6.19.0.dylib ../../../libfreetype.6.dylib
cd ../..

echo
echo "Building Fontconfig"

if [ ! -d fontconfig ]; then
  echo "fontconfig directory is missing, aborting."
  exit
fi

cd fontconfig
rm -rf builddir
meson setup --buildtype=release --prefix $(pwd)/../local_install builddir
cd builddir
meson compile
meson install
cp src/libfontconfig.1.dylib ../../../
cd ../..

echo
echo "Building libjpeg-turbo"

if [ ! -d libjpeg-turbo ]; then
  echo "libjpeg-turbo directory is missing, aborting."
  exit
fi

cd libjpeg-turbo/build
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(pwd)/../../local_install -B . -S ..
make clean
make -j${JOBS}
make install
cp libjpeg.62.4.0.dylib ../../../libjpeg.62.dylib
cd ../..

echo
echo "Building LibTIFF"

if [ ! -d libtiff ]; then
  echo "libtiff directory is missing, aborting."
  exit
fi

cd libtiff/build
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -Dtiff-tools=off -Dtiff-tests=off -Dtiff-contrib=off -Dtiff-docs=off -DCMAKE_INSTALL_PREFIX=$(pwd)/../../local_install -B . -S ..
make clean
make -j${JOBS}
make install
cp libtiff/libtiff.6.0.1.dylib ../../../libtiff.6.dylib
cd ../..

echo
echo "Building OpenJPEG"

if [ ! -d openjpeg ]; then
  echo "openjpeg directory is missing, aborting."
  exit
fi

cd openjpeg/build
rm -f CMakeCache.txt
PKG_CONFIG_PATH=$(pwd)/../local_install/lib/pkgconfig cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(pwd)/../../local_install -S .. -B .
make clean
make -j${JOBS}
make install
cp bin/libopenjp2.2.5.0.dylib ../../../libopenjp2.7.dylib
cd ../..

echo
echo "Building Poppler"

if [ ! -d poppler ]; then
  echo "poppler directory is missing, aborting."
  exit
fi

cd poppler/build
rm -f CMakeCache.txt
PKG_CONFIG_PATH=$(pwd)/../local_install/lib/pkgconfig cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(pwd)/../../local_install \
-DENABLE_UTILS=off -DBUILD_CPP_TESTS=off -DENABLE_LIBCURL=off -DRUN_GPERF_IF_PRESENT=off -DENABLE_QT5=off -DENABLE_QT6=off -DENABLE_BOOST=off -DENABLE_GLIB=off -DENABLE_NSS3=off -S .. -B .
make clean
make -j${JOBS}

# This will fail if there are spaces in the build path.
install_name_tool -change $(otool -L libpoppler.129.dylib | grep libfreetype | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libfreetype.6.dylib libpoppler.129.dylib
install_name_tool -change $(otool -L libpoppler.129.dylib | grep libfontconfig | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libfontconfig.1.dylib libpoppler.129.dylib

cp libpoppler.129.0.0.dylib ../../../libpoppler.129.dylib
cp cpp/libpoppler-cpp.0.11.0.dylib ../../../libpoppler-cpp.0.dylib
cd ../..

echo
echo "\nBuilding FreeImage"

if [ ! -d freeimage/FreeImage ]; then
  echo "FreeImage directory is missing, aborting."
  exit
fi

cd freeimage/FreeImage
make clean
make -j${JOBS}
cp libfreeimage.a ../../..
cd ../..

echo
echo "\nBuilding libgit2"

if [ ! -d libgit2/build ]; then
  echo "libgit2 directory is missing, aborting."
  exit
fi

cd libgit2/build
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF ..
make clean
make -j${JOBS}
cp libgit2.1.6.4.dylib ../../../libgit2.1.6.dylib
cd ../..

echo
echo "\nBuilding pugixml"

if [ ! -d pugixml ]; then
  echo "pugixml directory is missing, aborting."
  exit
fi

cd pugixml
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release .
make clean
make -j${JOBS}
cp libpugixml.a ../..
cd ..

echo
echo "\nBuilding SDL"

if [ ! -d SDL/build ]; then
  echo "SDL directory is missing, aborting."
  exit
fi

cd SDL/build
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -S .. -B .
make clean
make -j${JOBS}
cp libSDL2-2.0.0.dylib ../../..
cd ../..

echo
echo "\nBuilding libvpx"

if [ ! -d libvpx ]; then
  echo "libvpx directory is missing, aborting."
  exit
fi

cd libvpx
./configure --disable-examples --disable-docs --disable-tools --disable-unit-tests --enable-pic --enable-vp9-highbitdepth --prefix=$(pwd)/../local_install
make clean
make -j${JOBS}
make install
cd ..

echo
echo "\nBuilding Ogg"

if [ ! -d ogg ]; then
  echo "Ogg directory is missing, aborting."
  exit
fi

cd ogg
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(pwd)/../local_install .
make clean
make -j${JOBS}
make install
cd ..

echo
echo "\nBuilding Vorbis"

if [ ! -d vorbis ]; then
  echo "Vorbis directory is missing, aborting."
  exit
fi

cd vorbis
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=on -DCMAKE_MACOSX_RPATH=on -DCMAKE_INSTALL_PREFIX=$(pwd)/../local_install .
make clean
make -j${JOBS}
make install
cp lib/libvorbisenc.2.0.12.dylib ../..
cp lib/libvorbis.0.4.9.dylib ../..
cd ..

echo
echo "\nBuilding Opus"

if [ ! -d opus ]; then
  echo "Opus directory is missing, aborting."
  exit
fi

cd opus
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(pwd)/../local_install .
make clean
make -j${JOBS}
make install
cd ..

echo
echo "\nBuilding FFmpeg"

if [ ! -d FFmpeg ]; then
  echo "FFmpeg directory is missing, aborting."
  exit
fi

cd FFmpeg
PKG_CONFIG_PATH=$(pwd)/../local_install/lib/pkgconfig ./configure --prefix=/usr/local --enable-rpath --install-name-dir=@rpath --disable-doc --enable-gpl --enable-shared --enable-libvorbis --enable-libopus --enable-libvpx --enable-postproc

make clean
make -j${JOBS}
install_name_tool -rpath /usr/local/lib @executable_path libavcodec/libavcodec.59.dylib
cp libavcodec/libavcodec.59.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libavfilter/libavfilter.8.dylib
cp libavfilter/libavfilter.8.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libavformat/libavformat.59.dylib
cp libavformat/libavformat.59.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libavutil/libavutil.57.dylib
cp libavutil/libavutil.57.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libpostproc/libpostproc.56.dylib
cp libpostproc/libpostproc.56.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libswresample/libswresample.4.dylib
cp libswresample/libswresample.4.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libswscale/libswscale.6.dylib
cp libswscale/libswscale.6.dylib ../..

unset PKG_CONFIG_PATH

echo
echo "Done building all dependencies."

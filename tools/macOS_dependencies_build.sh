#!/bin/sh
#  SPDX-License-Identifier: MIT
#
#  ES-DE Frontend
#  macOS_dependencies_build.sh
#
#  Builds the external dependencies in-tree.
#  The macOS_dependencies_setup.sh script must have been executed before this one.
#  All sources will be recompiled from scratch every time this script is run.
#  If manually compiling individual libraries, don't forget to set MACOSX_DEPLOYMENT_TARGET.
#
#  This script needs to run from the root of the repository.
#

export MACOSX_DEPLOYMENT_TARGET=11.0

# How many CPU threads to use for the compilation.
JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 8)

if [ ! -f .clang-format ]; then
  echo "You need to run this script from the root of the repository."
  exit
fi

cd external

if [ ! -d FFmpeg ]; then
  echo "You need to first run tools/macOS_dependencies_setup.sh to download and configure the dependencies."
  exit
fi

echo "Building all dependencies in the ./external directory..."

export PKG_CONFIG_PATH=$(pwd)/../local_install/lib/pkgconfig

echo
echo "Building libiconv"

if [ ! -d libiconv ]; then
  echo "libiconv directory is missing, aborting."
  exit
fi

cd libiconv

./configure --enable-static=yes --enable-shared=no --prefix=$(pwd)/../local_install
make clean
make -j${JOBS}
make install
cd ..

echo
echo "Building gettext"

if [ ! -d gettext ]; then
  echo "gettext directory is missing, aborting."
  exit
fi

cd gettext

./configure --with-libiconv-prefix=$(pwd)/../local_install --prefix=$(pwd)/../local_install
make clean
make -j${JOBS}

cd gettext-runtime/intl/.libs
install_name_tool -id "@rpath/libintl.8.dylib" libintl.8.dylib
cp libintl.8.dylib ../../../../../
cd ../../../
make install
cd ..

echo
echo "Building ICU"

if [ ! -d icu/icu4c ]; then
  echo "icu/icu4c directory is missing, aborting."
  exit
fi

if [ ! -f icu/icu4c/source/icu_filters.json ]; then
  echo "icu/icu4c/source/icu_filters.json is missing, aborting."
  exit
fi

cd icu/icu4c/source
ICU_DATA_FILTER_FILE=icu_filters.json CXXFLAGS="-DUCONFIG_NO_COLLATION -DUCONFIG_NO_TRANSLITERATION" ./configure --disable-extras --disable-icuio --disable-samples --disable-tests
make clean
make -j${JOBS}
cd lib
install_name_tool -id "@rpath/libicudata.77.dylib" libicudata.77.1.dylib
install_name_tool -id "@rpath/libicui18n.77.dylib" libicui18n.77.1.dylib
install_name_tool -change $(otool -L libicui18n.77.1.dylib | grep libicuuc | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libicuuc.77.dylib libicui18n.77.1.dylib
install_name_tool -change $(otool -L libicui18n.77.1.dylib | grep libicudata | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libicudata.77.dylib libicui18n.77.1.dylib
install_name_tool -id "@rpath/libicuuc.77.dylib" libicuuc.77.1.dylib
install_name_tool -change $(otool -L libicuuc.77.1.dylib | grep libicudata | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libicudata.77.dylib libicuuc.77.1.dylib
cp libicudata.77.1.dylib ../../../../../libicudata.77.dylib
cp libicui18n.77.1.dylib ../../../../../libicui18n.77.dylib
cp libicuuc.77.1.dylib ../../../../../libicuuc.77.dylib
cd ../../../../

echo
echo "Building libpng"

if [ ! -d libpng ]; then
  echo "libpng directory is missing, aborting."
  exit
fi

cd libpng
rm -f CMakeCache.txt
if [ $(uname -m) == "arm64" ]; then
  cmake -DCMAKE_BUILD_TYPE=Release -DPNG_SHARED=off -DPNG_FRAMEWORK=off -DPNG_ARM_NEON=off -DCMAKE_INSTALL_PREFIX=$(pwd)/../local_install .
else
  cmake -DCMAKE_BUILD_TYPE=Release -DPNG_SHARED=off -DPNG_FRAMEWORK=off -DCMAKE_INSTALL_PREFIX=$(pwd)/../local_install .
fi
make clean
make -j${JOBS}
make install
cd ..

echo
echo "Building HarfBuzz"

if [ ! -d harfbuzz/build ]; then
  echo "harfbuzz directory is missing, aborting."
  exit
fi

cd harfbuzz/build
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DHB_BUILD_SUBSET=off ..
make clean
make -j${JOBS}
cp libharfbuzz.dylib ../../../
cd ../..

echo
echo "Building FreeType"

if [ ! -d freetype/build ]; then
  echo "FreeType directory is missing, aborting."
  exit
fi

cd freetype/build
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=on -DBUILD_SHARED_LIBS=on -DCMAKE_MACOSX_RPATH=on -DCMAKE_INSTALL_PREFIX=$(pwd)/../../local_install -S .. -B .
make clean
make -j${JOBS}
cp libfreetype.6.20.2.dylib ../../../libfreetype.6.dylib
cd ../..

echo
echo "Building Fontconfig"

if [ ! -d fontconfig ]; then
  echo "fontconfig directory is missing, aborting."
  exit
fi

cd fontconfig
rm -rf builddir
PKG_CONFIG_PATH=$(pwd)/../local_install/lib/pkgconfig meson setup --buildtype=release --prefix $(pwd)/../local_install builddir -Dcache-dir=false
cd builddir
meson compile

# This will fail if there are spaces in the build path.
cd src
install_name_tool -change $(otool -L libfontconfig.1.dylib | grep libfreetype | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libfreetype.6.dylib libfontconfig.1.dylib
cd ..

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
cp libtiff/libtiff.6.1.0.dylib ../../../libtiff.6.dylib
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
cp bin/libopenjp2.2.5.3.dylib ../../../libopenjp2.7.dylib
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
-DENABLE_UTILS=off -DBUILD_CPP_TESTS=off -DENABLE_LIBCURL=off -DRUN_GPERF_IF_PRESENT=off -DENABLE_QT5=off -DENABLE_QT6=off -DENABLE_BOOST=off \
-DENABLE_GLIB=off -DENABLE_NSS3=off -DENABLE_GPGME=off -DENABLE_LCMS=off -S .. -B .
make clean
make -j${JOBS}

# This will fail if there are spaces in the build path.
install_name_tool -change $(otool -L libpoppler.148.dylib | grep libfreetype | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libfreetype.6.dylib libpoppler.148.dylib
install_name_tool -change $(otool -L libpoppler.148.dylib | grep libfontconfig | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libfontconfig.1.dylib libpoppler.148.dylib

cp libpoppler.148.0.0.dylib ../../../libpoppler.148.dylib
cp cpp/libpoppler-cpp.2.1.0.dylib ../../../libpoppler-cpp.2.dylib
cd ../..

echo
echo "Building FreeImage"

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
echo "Building libgit2"

if [ ! -d libgit2/build ]; then
  echo "libgit2 directory is missing, aborting."
  exit
fi

cd libgit2/build
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF ..
make clean
make -j${JOBS}
cp libgit2.1.9.1.dylib ../../../libgit2.1.9.dylib
cd ../..

echo
echo "Building pugixml"

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
echo "Building SDL"

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
echo "Building Ogg"

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
echo "Building dav1d"

if [ ! -d dav1d ]; then
  echo "dav1d directory is missing, aborting."
  exit
fi

cd dav1d
rm -rf builddir
PKG_CONFIG_PATH=$(pwd)/../local_install/lib/pkgconfig meson setup --buildtype=release --prefix $(pwd)/../local_install builddir
cd builddir
meson compile
meson install
cp src/libdav1d.7.dylib ../../..
cd ../..

echo
echo "Building FFmpeg"

if [ ! -d FFmpeg ]; then
  echo "FFmpeg directory is missing, aborting."
  exit
fi

cd FFmpeg
PKG_CONFIG_PATH=$(pwd)/../local_install/lib/pkgconfig ./configure --prefix=/usr/local --enable-rpath --install-name-dir=@rpath --disable-doc --disable-lzma --enable-gpl --enable-shared --enable-libdav1d --enable-postproc

make clean
make -j${JOBS}
install_name_tool -rpath /usr/local/lib @executable_path libavcodec/libavcodec.61.dylib
install_name_tool -change $(otool -L libavcodec/libavcodec.61.dylib | grep libdav1d | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libdav1d.7.dylib libavcodec/libavcodec.61.dylib
cp libavcodec/libavcodec.61.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libavfilter/libavfilter.10.dylib
install_name_tool -change $(otool -L libavfilter/libavfilter.10.dylib | grep libdav1d | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libdav1d.7.dylib libavfilter/libavfilter.10.dylib
cp libavfilter/libavfilter.10.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libavformat/libavformat.61.dylib
install_name_tool -change $(otool -L libavformat/libavformat.61.dylib | grep libdav1d | cut -f1 -d' ' | sed 's/[[:blank:]]//g') @rpath/libdav1d.7.dylib libavformat/libavformat.61.dylib
cp libavformat/libavformat.61.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libavutil/libavutil.59.dylib
cp libavutil/libavutil.59.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libpostproc/libpostproc.58.dylib
cp libpostproc/libpostproc.58.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libswresample/libswresample.5.dylib
cp libswresample/libswresample.5.dylib ../..
install_name_tool -rpath /usr/local/lib @executable_path libswscale/libswscale.8.dylib
cp libswscale/libswscale.8.dylib ../..

unset PKG_CONFIG_PATH

echo
echo "Done building all dependencies."

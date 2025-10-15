#!/bin/bash
#  SPDX-License-Identifier: MIT
#
#  ES-DE Frontend
#  Emscripten_dependencies_build.sh
#
#  Builds the external dependencies in-tree.
#  The Emscripten_dependencies_setup.sh script must have been executed before this one.
#  All sources will be recompiled from scratch every time this script is run.
#
#  This script needs to run from the root of the ES-DE repository.
#  It's only intended to be used on Linux systems.
#

# NOTE: Several of the dependencies currently don't build correctly, it's unclear whether they
# actually support an Emscripten/WebAssembly build at all. These are the issues at the moment:
#
# ICU doesn't build
# Fontconfig doesn't build (possibly incorrect Meson configuration)
# Poppler doesn't build as Fontconfig is not available, and perhaps for more reasons
# OpenSSL doesn't build (unclear if it can be built using Emscripten at all)
# curl has no TLS/SSL support as there's no OpenSSL library (-DCURL_ENABLE_SSL=off option)
# ligbit2 has no HTTPS support (won't build without the -DUSE_HTTPS=off option)
# FFmpeg fails at the end of the build process

# Goto equivalent.
##### if false; then
##### fi # if false; then

# How many CPU threads to use for the compilation.
JOBS=$(nproc 2>/dev/null || echo 8)

if [ ! -f .clang-format ]; then
  echo "You need to run this script from the root of the repository."
  exit
fi

if [ -z ${EMSDK_NODE} ]; then
  echo "You need to initialize emsdk before running this script"
  exit
fi

cd external

if [ ! -d ffmpeg.wasm ]; then
  echo "You need to first run tools/Emscripten_dependencies_setup.sh to download and configure the dependencies."
  exit
fi

echo "Building all dependencies in the ./external directory..."

echo
echo "Building libiconv"

if [ ! -d libiconv ]; then
  echo "libiconv directory is missing, aborting."
  exit
fi

cd libiconv

emconfigure ./configure --host=wasm32-unknown-emscripten --enable-shared=no --enable-static=yes
make clean
make -j${JOBS}
cp lib/.libs/libiconv.a ../..
cd ..

echo
echo "Building gettext"

if [ ! -d gettext ]; then
  echo "gettext directory is missing, aborting."
  exit
fi

cd gettext
emconfigure ./configure ac_cv_have_decl_alarm=no gl_cv_func_sleep_works=no --host=wasm32-unknown-emscripten \
--enable-shared=no --enable-static=yes --disable-libasprintf --disable-java --disable-curses \
--disable-openmp --disable-native-java --disable-tools
make clean
make -j${JOBS}
cp gettext-runtime/intl/.libs/libgnuintl.a ../..

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

if [ ! -d icu_native/icu4c ]; then
  echo "icu_native/icu4c directory is missing, aborting."
  exit
fi

cd icu_native/icu4c/source
CXXFLAGS="-DUCONFIG_NO_COLLATION -DUCONFIG_NO_TRANSLITERATION" ./runConfigureICU Linux
make clean
make -j${JOBS}
cd ../../..

cd icu/icu4c/source

ICU_DATA_FILTER_FILE=icu_filters.json emconfigure ./configure --host=wasm32-unknown-emscripten \
--enable-static --disable-extras --disable-icuio --disable-tools --disable-samples --disable-tests \
--with-cross-build=$(pwd)/../../../icu_native/icu4c/source
make clean
make -j${JOBS}
cp lib/libicudata.a ../../../..
cp lib/libicui18n.a ../../../..
cp lib/libicuuc.a ../../../..

cd ../../..

echo
echo "Building zlib"

if [ ! -d zlib ]; then
  echo "zlib directory is missing, aborting."
  exit
fi

cd zlib/build

rm -f CMakeCache.txt
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=off -S .. -B .

make clean
make -j${JOBS}
cp ../zlib.h .
cp libz.a ../../..
cd ../..

echo
echo "Building libpng"

if [ ! -d libpng ]; then
  echo "libpng directory is missing, aborting."
  exit
fi

cd libpng

rm -f CMakeCache.txt
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DPNG_SHARED=off -DZLIB_INCLUDE_DIR=$(pwd)/../zlib/build -DZLIB_LIBRARY=$(pwd)/../zlib/build/libz.a .
make clean
make -j${JOBS}
cp libpng16.a ../..
cd ..

echo
echo "Building libjpeg-turbo"

if [ ! -d libjpeg-turbo ]; then
  echo "libjpeg-turbo directory is missing, aborting."
  exit
fi

cd libjpeg-turbo
rm -f CMakeCache.txt
emcmake emcmake cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_SHARED=off .
make clean
make -j${JOBS}
cp libjpeg.a ../..
cd ..

echo
echo "Building LibTIFF"

if [ ! -d libtiff ]; then
  echo "libtiff directory is missing, aborting."
  exit
fi

cd libtiff
rm -f CMakeCache.txt
emcmake emcmake cmake -DCMAKE_BUILD_TYPE=Release -Dtiff-tools=off -Dtiff-tests=off -Dtiff-contrib=off -Dtiff-docs=off -DBUILD_SHARED_LIBS=off .
make clean
make -j${JOBS}
cp libtiff/libtiff.a ../..
cd ..

echo
echo "Building OpenJPEG"

if [ ! -d openjpeg ]; then
  echo "openjpeg directory is missing, aborting."
  exit
fi

cd openjpeg/build
rm -f CMakeCache.txt
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=off -S .. -B .
make clean
make -j${JOBS}
cp bin/libopenjp2.a ../../..
cd ../..

echo
echo "Building HarfBuzz"

if [ ! -d harfbuzz/build ]; then
  echo "harfbuzz directory is missing, aborting."
  exit
fi

cd harfbuzz/build
rm -f CMakeCache.txt
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=off -DHB_BUILD_SUBSET=off -S .. -B .
make clean
make -j${JOBS}
cp libharfbuzz.a ../../..
cd ../..

echo
echo "Building FreeType"

if [ ! -d freetype/build ]; then
  echo "FreeType directory is missing, aborting."
  exit
fi

cd freetype/build
rm -f CMakeCache.txt
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=off -DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=on -S .. -B .
make clean
make -j${JOBS}
cp libfreetype.a  ../../..
cd ../..

echo
echo "Building Fontconfig"

if [ ! -d fontconfig ]; then
  echo "fontconfig directory is missing, aborting."
  exit
fi

cd fontconfig
rm -rf builddir

cat > emscripten_cross-compile.txt <<EOF
[binaries]
c = 'emcc'
cpp = 'em++'
ar = 'emar'
[host_machine]
system = 'emscripten'
cpu_family = 'wasm'
cpu = 'wasm'
endian = 'little'
EOF

meson setup --cross-file emscripten_cross-compile.txt --buildtype=release -Dtests=disabled -Dtools=disabled builddir
cd builddir
meson compile
cd ../..

echo
echo "Building Poppler"

if [ ! -d poppler ]; then
  echo "poppler directory is missing, aborting."
  exit
fi

cd poppler/build
rm -f CMakeCache.txt

emcmake cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_UTILS=off -DBUILD_CPP_TESTS=off -DENABLE_LIBCURL=off \
-DRUN_GPERF_IF_PRESENT=off -DENABLE_QT5=off -DENABLE_QT6=off -DENABLE_BOOST=off -DENABLE_GLIB=off \
-DENABLE_NSS3=off -DENABLE_GPGME=off -DENABLE_LCMS=off \
-DFREETYPE_INCLUDE_DIRS=$(pwd)/../../freetype/include -DFREETYPE_LIBRARY=$(pwd)/../../freetype/build/libfreetype.a \
-DFontconfig_INCLUDE_DIR=$(pwd)/../../fontconfig -DFontconfig_LIBRARY=$(pwd)/../../fontconfig/builddir/src/libfontconfig.a \
-DJPEG_INCLUDE_DIR=$(pwd)/../../libjpeg-turbo -DJPEG_LIBRARY=$(pwd)/../../libjpeg-turbo/libjpeg.a \
-DTIFF_INCLUDE_DIR=$(pwd)/../../libtiff/libtiff -DTIFF_LIBRARY=$(pwd)/../../libtiff/build/libtiff/libtiff.a \
-DIconv_INCLUDE_DIR=$(pwd)/../../libiconv/include -DIconv_LIBRARY=$(pwd)/../../libiconv//lib/libiconv.a \
-DZLIB_INCLUDE_DIR=$(pwd)/../../zlib/build -DZLIB_LIBRARY=$(pwd)/../../zlib/build/libz.a \
-DENABLE_LIBOPENJPEG=unmaintained -S .. -B .
make clean
make -j${JOBS}
cp libpoppler.a  ../../..
cp cpp/libpoppler-cpp.a ../../..
cd ../..

echo
echo "Building SDL"

if [ ! -d SDL/build ]; then
  echo "SDL directory is missing, aborting."
  exit
fi

cd SDL/build
rm -f CMakeCache.txt
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=off -DSDL_TESTS=off -S .. -B .
make clean
make -j${JOBS}
cp libSDL2.a ../../..
cd ../..

# echo
# echo "Building OpenSSL"

# if [ ! -d openssl ]; then
#   echo "OpenSSL directory is missing, aborting."
#   exit
# fi

# cd openssl

# ./config NO-EMSCRIPTEN-BUILD-TYPE --static --release
# make clean
# make -j${JOBS}
# cp *.a ../..
# cd ..

echo
echo "Building curl"

if [ ! -d curl ]; then
  echo "curl directory is missing, aborting."
  exit
fi

cd curl

rm -f CMakeCache.txt
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=off -DCURL_ENABLE_SSL=off -DCURL_USE_LIBPSL=off .
make clean
make -j${JOBS}
cp lib/libcurl.a ../..
cd ..

echo
echo "Building libgit2"

if [ ! -d libgit2/build ]; then
  echo "libgit2 directory is missing, aborting."
  exit
fi

cd libgit2/build
rm -f CMakeCache.txt
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=off -DBUILD_CLI=off -DBUILD_TESTS=off -DUSE_HTTPS=off -DUSE_NTLMCLIENT=off -S .. -B .
make clean
make -j${JOBS}
cp libgit2.a ../../..
cd ../..

echo
echo "Building pugixml"

if [ ! -d pugixml ]; then
  echo "pugixml directory is missing, aborting."
  exit
fi

cd pugixml
rm -f CMakeCache.txt
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=off .
make clean
make -j${JOBS}
cp libpugixml.a ../..
cd ..

echo
echo "Building FreeImage"

if [ ! -d FreeImage-CMake/FreeImage ]; then
  echo "FreeImage-CMake directory is missing, aborting."
  exit
fi

cd FreeImage-CMake/FreeImage

./clean.sh
rm -f *.a

emcmake cmake -DCMAKE_BUILD_TYPE=Release .
make clean
make -j${JOBS}
cp libFreeImage.a ../../..
cd ../..

echo
echo "Building FFmpeg"

if [ ! -d ffmpeg.wasm ]; then
  echo "ffmpeg.wasm directory is missing, aborting."
  exit
fi

cd ffmpeg.wasm
make prd-mt

cp libavcodec/libavcodec.a ../..
cp libavfilter/libavfilter.a ../..
cp libavformat/libavformat.a ../..
cp libavutil/libavutil.a ../..
cp libpostproc/libpostproc.a ../..
cp libswresample/libswresample.a ../..
cp libswscale/libswscale.a ../..
cp build/lib/*.a ../..

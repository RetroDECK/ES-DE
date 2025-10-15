#!/bin/sh
#  SPDX-License-Identifier: MIT
#
#  ES-DE Frontend
#  Emscripten_dependencies_setup.sh
#
#  Downloads and prepares the external dependencies for building in-tree.
#  If the directories already exist they will be removed and the source code will be downloaded again.
#
#  This script needs to run from the root of the ES-DE repository.
#  It's only intended to be used on Linux systems.
#

# Goto equivalent.
##### if false; then
##### fi # if false; then

if [ ! -f .clang-format ]; then
  echo "You need to run this script from the root of the repository."
  exit
fi

echo "Setting up Emscripten dependencies in the ./external directory..."

cd external

echo
echo "Setting up libiconv"
rm -rf libiconv*
curl -LO https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.18.tar.gz
tar xvzf libiconv-1.18.tar.gz

if [ ! -d libiconv-1.18 ]; then
  echo "libiconv directory is missing, aborting."
  exit
fi

mv libiconv-1.18 libiconv
rm libiconv-1.18.tar.gz

echo
echo "Setting up gettext"
rm -rf gettext*
curl -LO https://ftp.gnu.org/pub/gnu/gettext/gettext-0.24.tar.gz
tar xvzf gettext-0.24.tar.gz

if [ ! -d gettext-0.24 ]; then
  echo "gettext directory is missing, aborting."
  exit
fi

mv gettext-0.24 gettext
rm gettext-0.24.tar.gz

cd gettext
echo
echo Patching getlocalename_l-unsafe.c files
sed -i 's/ #error \"Please port gnulib getlocalename_l-unsafe.c.*/\/\/Patched out/' gettext-tools/gnulib-lib/getlocalename_l-unsafe.c
sed -i 's/ #error \"Please port gnulib getlocalename_l-unsafe.c.*/\/\/Patched out/' gettext-runtime/gnulib-lib/getlocalename_l-unsafe.c
sed -i 's/ #error \"Please port gnulib getlocalename_l-unsafe.c.*/\/\/Patched out/' gettext-runtime/intl/gnulib-lib/getlocalename_l-unsafe.c

cd ..

echo
echo "Setting up ICU"
rm -rf icu
rm -rf icu_native
git clone -n --filter=tree:0 https://github.com/unicode-org/icu.git
cd icu
git sparse-checkout set --no-cone icu4c
git checkout release-77-1
cd ..
cp -rp icu icu_native
cp ../es-app/assets/icu_filters.json icu/icu4c/source/

echo
echo "Setting up zlib"
rm -rf zlib
git clone https://github.com/madler/zlib

if [ ! -d zlib ]; then
  echo "zlib directory is missing, aborting."
  exit
fi

cd zlib
git checkout v1.3.1
mkdir build
cd ..

echo
echo "Setting up libpng"
rm -rf libpng code
git clone https://git.code.sf.net/p/libpng/code.git

if [ ! -d code ]; then
  echo "libpng directory is missing, aborting."
  exit
fi

mv code libpng
cd libpng
git checkout v1.6.47
cd ..

echo
echo "Setting up libjpeg-turbo"
rm -rf libjpeg-turbo
git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git

if [ ! -d libjpeg-turbo ]; then
  echo "libjpeg-turbo directory is missing, aborting."
  exit
fi

cd libjpeg-turbo
git checkout 3.0.1
cd ..

echo
echo "Setting up LibTIFF"
rm -rf libtiff
git clone https://gitlab.com/libtiff/libtiff.git

if [ ! -d libtiff ]; then
  echo "libtiff directory is missing, aborting."
  exit
fi

cd libtiff
git checkout v4.7.0
cd ..

echo
echo "Setting up OpenJPEG"
rm -rf openjpeg
git clone https://github.com/uclouvain/openjpeg.git

if [ ! -d openjpeg ]; then
  echo "openjpeg directory is missing, aborting."
  exit
fi

cd openjpeg
git checkout v2.5.0
mkdir build
cd ..

echo
echo "Setting up HarfBuzz"
rm -rf harfbuzz
git clone https://github.com/harfbuzz/harfbuzz.git

if [ ! -d harfbuzz ]; then
  echo "harfbuzz directory is missing, aborting."
  exit
fi

cd harfbuzz
git checkout 11.0.1
mkdir build
cd ..

echo
echo "Setting up FreeType"
rm -rf freetype
git clone https://github.com/freetype/freetype.git

if [ ! -d freetype ]; then
  echo "FreeType directory is missing, aborting."
  exit
fi

cd freetype
git checkout VER-2-13-3
mkdir build
cd ..

echo
echo "Setting up Fontconfig"
rm -rf fontconfig
git clone https://gitlab.freedesktop.org/fontconfig/fontconfig.git

if [ ! -d fontconfig ]; then
  echo "fontconfig directory is missing, aborting."
  exit
fi

cd fontconfig
git checkout 2.14.2
cd ..

echo
echo "Setting up Poppler"
rm -rf poppler
git clone https://gitlab.freedesktop.org/poppler/poppler.git

if [ ! -d poppler ]; then
  echo "poppler directory is missing, aborting."
  exit
fi

cd poppler
git checkout poppler-24.08.0
mkdir build
cd ..

echo
echo "Setting up SDL"
rm -rf SDL
git clone https://github.com/libsdl-org/SDL.git

if [ ! -d SDL ]; then
  echo "SDL directory is missing, aborting."
  exit
fi

cd SDL
git checkout release-2.32.2
ln -s include SDL2
mkdir build
cd ..

echo
echo "Setting up OpenSSL"
rm -rf openssl
git clone https://github.com/openssl/openssl.git

if [ ! -d openssl ]; then
  echo "OpenSSL directory is missing, aborting."
  exit
fi

cd openssl
git checkout openssl-3.4.1
cd ..

echo
echo "Setting up curl"
rm -rf curl
git clone https://github.com/curl/curl.git

if [ ! -d curl ]; then
  echo "curl directory is missing, aborting."
  exit
fi

cd curl
git checkout curl-8_13_0
cd ..

echo
echo "Setting up libgit2"
rm -rf libgit2
git clone https://github.com/libgit2/libgit2.git

if [ ! -d libgit2 ]; then
  echo "libgit2 directory is missing, aborting."
  exit
fi

cd libgit2
git checkout v1.9.0
mkdir build
find .. -name 'CMakeLists.txt' -exec sed -i 's|C_STANDARD 90|C_STANDARD 99|' {} \;
cd ..

echo
echo "Setting up pugixml"
rm -rf pugixml
git clone https://github.com/zeux/pugixml.git

if [ ! -d pugixml ]; then
  echo "pugixml directory is missing, aborting."
  exit
fi

cd pugixml
git checkout v1.15
cd ..

echo
echo "Setting up FreeImage"

rm -rf FreeImage-CMake
git clone https://github.com/Max-ChenFei/FreeImage-CMake.git

if [ ! -d FreeImage-CMake ]; then
  echo "FreeImage-CMake directory is missing, aborting."
  exit
fi

cd FreeImage-CMake/FreeImage
chmod +x clean.sh

tr -d '\r' < Source/LibJXR/jxrgluelib/JXRGlueJxr.c > Source/LibJXR/jxrgluelib/JXRGlueJxr.c_PATCH
mv Source/LibJXR/jxrgluelib/JXRGlueJxr.c_PATCH Source/LibJXR/jxrgluelib/JXRGlueJxr.c

cat << EOF | patch Source/LibJXR/jxrgluelib/JXRGlueJxr.c
--- JXRGlueJxr.c  2021-11-28 10:31:52.000000000 +0100
+++ JXRGlueJxr.c_macOS  2021-11-30 16:56:40.000000000 +0100
@@ -28,7 +28,7 @@
 //*@@@---@@@@******************************************************************
 #include <limits.h>
 #include <JXRGlue.h>
-
+#include <wchar.h>

 static const char szHDPhotoFormat[] = "<dc:format>image/vnd.ms-photo</dc:format>";
 const U32 IFDEntryTypeSizes[] = { 0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8 };
EOF

tr -d '\r' < Source/LibJXR/image/decode/segdec.c > Source/LibJXR/image/decode/segdec.c_PATCH
mv Source/LibJXR/image/decode/segdec.c_PATCH Source/LibJXR/image/decode/segdec.c

cat << EOF | patch Source/LibJXR/image/decode/segdec.c
--- segdec.c    2021-11-30 15:52:10.000000000 +0100
+++ segdec.c_macOS      2021-11-30 15:46:06.000000000 +0100
@@ -52,6 +52,25 @@
 //================================================================
 // Memory access functions
 //================================================================
+#if (defined(WIN32) && !defined(UNDER_CE) && (!defined(__MINGW32__) || defined(__MINGW64_TOOLCHAIN__))) || (defined(UNDER_CE) && defined(_ARM_))
+// WinCE ARM and Desktop x86
+#else
+// other platform
+#ifdef _BIG__ENDIAN_
+#define _byteswap_ulong2(x)  (x)
+#else // _BIG__ENDIAN_
+U32 _byteswap_ulong2(U32 bits)
+{
+    U32 r = (bits & 0xffu) << 24;
+    r |= (bits << 8) & 0xff0000u;
+    r |= ((bits >> 8) & 0xff00u);
+    r |= ((bits >> 24) & 0xffu);
+
+    return r;
+}
+#endif // _BIG__ENDIAN_
+#endif
+
 static U32 _FORCEINLINE _load4(void* pv)
 {
 #ifdef _BIG__ENDIAN_
@@ -61,9 +80,9 @@
     U32  v;
     v = ((U16 *) pv)[0];
     v |= ((U32)((U16 *) pv)[1]) << 16;
-    return _byteswap_ulong(v);
+    return _byteswap_ulong2(v);
 #else // _M_IA64
-    return _byteswap_ulong(*(U32*)pv);
+    return _byteswap_ulong2(*(U32*)pv);
 #endif // _M_IA64
 #endif // _BIG__ENDIAN_
 }
EOF

cd ../..

echo
echo "Setting up FFmpeg"
rm -rf ffmpeg.wasm
git clone https://github.com/ffmpegwasm/ffmpeg.wasm.git

if [ ! -d ffmpeg.wasm ]; then
  echo "ffmpeg.wasm directory is missing, aborting."
  exit
fi

cd ffmpeg.wasm
git checkout v12.15

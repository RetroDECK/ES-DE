#!/bin/sh
#  SPDX-License-Identifier: MIT
#
#  ES-DE Frontend
#  macOS_dependencies_setup.sh
#
#  Downloads and prepares the external dependencies for building in-tree.
#  If the directories already exist they will be removed and the source code will be downloaded again.
#
#  This script needs to run from the root of the repository.
#

if [ ! -f .clang-format ]; then
  echo "You need to run this script from the root of the repository."
  exit
fi

echo "Setting up dependencies in the ./external directory..."

cd external
rm -rf local_install
mkdir local_install

echo
echo "Setting up libiconv"
rm -rf libiconv*
curl -LO https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.17.tar.gz
tar xvzf libiconv-1.17.tar.gz

if [ ! -d libiconv-1.17 ]; then
  echo "libiconv directory is missing, aborting."
  exit
fi

mv libiconv-1.17 libiconv
rm libiconv-1.17.tar.gz

echo
echo "Setting up gettext"
rm -rf gettext*
curl -LO https://ftp.gnu.org/pub/gnu/gettext/gettext-0.22.5.tar.gz
tar xvzf gettext-0.22.5.tar.gz

if [ ! -d gettext-0.22.5 ]; then
  echo "gettext directory is missing, aborting."
  exit
fi

mv gettext-0.22.5 gettext
rm gettext-0.22.5.tar.gz

echo
echo "Setting up ICU"
rm -rf icu
git clone -n --filter=tree:0 https://github.com/unicode-org/icu.git
cd icu
git sparse-checkout set --no-cone icu4c
git checkout release-75-1
cp ../../es-app/assets/icu_filters.json icu4c/source/
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
git checkout v1.6.40
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
git checkout 9.0.0
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
git checkout VER-2-13-0
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
echo "Setting up libjpeg-turbo"
rm -rf libjpeg-turbo
git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git

if [ ! -d libjpeg-turbo ]; then
  echo "libjpeg-turbo directory is missing, aborting."
  exit
fi

cd libjpeg-turbo
git checkout 2.1.91
mkdir build
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
git checkout v4.5.1
mkdir build
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
echo "Setting up Poppler"
rm -rf poppler
git clone https://gitlab.freedesktop.org/poppler/poppler.git

if [ ! -d poppler ]; then
  echo "poppler directory is missing, aborting."
  exit
fi

cd poppler
git checkout poppler-23.06.0
mkdir build
cd ..

echo
echo "Setting up FreeImage"
rm -rf freeimage
mkdir freeimage
cd freeimage
curl -LO https://downloads.sourceforge.net/project/freeimage/Source%20Distribution/3.18.0/FreeImage3180.zip
unzip FreeImage3180.zip

if [ ! -d FreeImage ]; then
  echo "FreeImage directory is missing, aborting."
  exit
fi

cd FreeImage

# We need to set the LC_CTYPE variable to C or we won't be able to strip out multi-byte characters using "tr".
export LC_CTYPE=C

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

if [ $(uname -m) == "arm64" ]; then
cat << EOF | patch Makefile.osx -
--- Makefile.osx        2022-02-02 11:09:46.000000000 +0100
+++ Makefile.osx_ARM64  2022-02-02 11:08:42.000000000 +0100
@@ -15,9 +15,9 @@
 CPP_I386 = \$(shell xcrun -find clang++)
 CPP_X86_64 = \$(shell xcrun -find clang++)
 MACOSX_DEPLOY = -mmacosx-version-min=\$(MACOSX_DEPLOYMENT_TARGET)
-COMPILERFLAGS = -Os -fexceptions -fvisibility=hidden -DNO_LCMS -D__ANSI__
+COMPILERFLAGS = -O2 -DNDEBUG -fexceptions -fvisibility=hidden -DNO_LCMS -D__ANSI__ -DHAVE_UNISTD_H -DDISABLE_PERF_MEASUREMENT -DPNG_ARM_NEON_OPT=0
 COMPILERFLAGS_I386 = -arch i386
-COMPILERFLAGS_X86_64 = -arch x86_64
+COMPILERFLAGS_X86_64 = -arch arm64
 COMPILERPPFLAGS = -Wno-ctor-dtor-privacy -D__ANSI__ -std=c++11 -stdlib=libc++ -Wc++11-narrowing
 INCLUDE +=
 INCLUDE_I386 = -isysroot \$(MACOSX_SYSROOT)
EOF
cat Makefile.osx | sed s/"arch_only x86_64"/"arch_only arm64"/g > Makefile.osx_TEMP
mv Makefile.osx_TEMP Makefile.osx
else
cat << EOF | patch Makefile.osx -
--- Makefile.osx        2021-11-30 15:06:53.000000000 +0100
+++ Makefile.osx_X86  2021-11-30 15:07:23.000000000 +0100
@@ -15,7 +15,7 @@
 CPP_I386 = \$(shell xcrun -find clang++)
 CPP_X86_64 = \$(shell xcrun -find clang++)
 MACOSX_DEPLOY = -mmacosx-version-min=\$(MACOSX_DEPLOYMENT_TARGET)
-COMPILERFLAGS = -Os -fexceptions -fvisibility=hidden -DNO_LCMS -D__ANSI__
+COMPILERFLAGS = -O2 -DNDEBUG -fexceptions -fvisibility=hidden -DNO_LCMS -D__ANSI__ -DHAVE_UNISTD_H -DDISABLE_PERF_MEASUREMENT
 COMPILERFLAGS_I386 = -arch i386
 COMPILERFLAGS_X86_64 = -arch x86_64
 COMPILERPPFLAGS = -Wno-ctor-dtor-privacy -D__ANSI__ -std=c++11 -stdlib=libc++ -Wc++11-narrowing
EOF
fi
cd ../..

echo
echo "Setting up libgit2"
rm -rf libgit2
git clone https://github.com/libgit2/libgit2.git

if [ ! -d libgit2 ]; then
  echo "libgit2 directory is missing, aborting."
  exit
fi

cd libgit2
git checkout v1.7.1
mkdir build
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
git checkout v1.13
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
git checkout release-2.30.9
ln -s include SDL2
mkdir build
cd ..

echo
echo "Setting up libvpx"
rm -rf libvpx
git clone https://github.com/webmproject/libvpx.git

if [ ! -d libvpx ]; then
  echo "libvpx directory is missing, aborting."
  exit
fi

cd libvpx
git checkout v1.13.0
cd ..

echo
echo "Setting up Ogg"
rm -rf ogg
git clone https://github.com/xiph/ogg.git

if [ ! -d ogg ]; then
  echo "Ogg directory is missing, aborting."
  exit
fi

cd ogg
git checkout v1.3.5
cd ..

echo
echo "Setting up Vorbis"
rm -rf vorbis
git clone https://gitlab.xiph.org/xiph/vorbis.git

if [ ! -d vorbis ]; then
  echo "Vorbis directory is missing, aborting."
  exit
fi

cd vorbis
git checkout v1.3.7
cd ..

echo
echo "Setting up Opus"
rm -rf opus
git clone https://gitlab.xiph.org/xiph/opus.git

if [ ! -d opus ]; then
  echo "Opus directory is missing, aborting."
  exit
fi

cd opus
git checkout v1.3.1
cd ..

echo
echo "Setting up FFmpeg"
rm -rf FFmpeg
git clone https://github.com/FFmpeg/FFmpeg.git

if [ ! -d FFmpeg ]; then
  echo "FFmpeg directory is missing, aborting."
  exit
fi

cd FFmpeg
git checkout n6.0

echo
echo "Done setting up all dependencies."

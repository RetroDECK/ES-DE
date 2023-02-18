#!/bin/sh
#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
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

echo "Setting up dependencies in the ./external directory...\n"

cd external
rm -rf local_install
mkdir local_install

echo "Setting up libpng"
rm -rf libpng code
git clone https://git.code.sf.net/p/libpng/code.git

if [ ! -d code ]; then
  echo "libpng directory is missing, aborting."
  exit
fi

mv code libpng
cd libpng
git checkout v1.6.38
cd ..

echo "\nSetting up FreeType"
rm -rf freetype
git clone https://github.com/freetype/freetype.git

if [ ! -d freetype ]; then
  echo "FreeType directory is missing, aborting."
  exit
fi

cd freetype
git checkout VER-2-12-1
mkdir build
cd ..

echo "\nSetting up FreeImage"
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
+COMPILERFLAGS = -Os -fexceptions -fvisibility=hidden -DNO_LCMS -D__ANSI__ -DHAVE_UNISTD_H -DDISABLE_PERF_MEASUREMENT -DPNG_ARM_NEON_OPT=0
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
+COMPILERFLAGS = -Os -fexceptions -fvisibility=hidden -DNO_LCMS -D__ANSI__ -DHAVE_UNISTD_H -DDISABLE_PERF_MEASUREMENT
 COMPILERFLAGS_I386 = -arch i386
 COMPILERFLAGS_X86_64 = -arch x86_64
 COMPILERPPFLAGS = -Wno-ctor-dtor-privacy -D__ANSI__ -std=c++11 -stdlib=libc++ -Wc++11-narrowing
EOF
fi
cd ../..

echo "\nSetting up pugixml"
rm -rf pugixml
git clone https://github.com/zeux/pugixml.git

if [ ! -d pugixml ]; then
  echo "pugixml directory is missing, aborting."
  exit
fi

cd pugixml
git checkout v1.12.1
cd ..

echo "\nSetting up SDL"
rm -rf SDL
git clone https://github.com/libsdl-org/SDL.git

if [ ! -d SDL ]; then
  echo "SDL directory is missing, aborting."
  exit
fi

cd SDL
git checkout release-2.26.3
ln -s include SDL2
mkdir build
cd ..

echo "\nSetting up libvpx"
rm -rf libvpx
git clone https://github.com/webmproject/libvpx.git

if [ ! -d libvpx ]; then
  echo "libvpx directory is missing, aborting."
  exit
fi

cd libvpx
git checkout v1.12.0
cd ..

echo "\nSetting up Ogg"
rm -rf ogg
git clone https://github.com/xiph/ogg.git

if [ ! -d ogg ]; then
  echo "Ogg directory is missing, aborting."
  exit
fi

cd ogg
git checkout v1.3.5
cd ..

echo "\nSetting up Vorbis"
rm -rf vorbis
git clone https://gitlab.xiph.org/xiph/vorbis.git

if [ ! -d vorbis ]; then
  echo "Vorbis directory is missing, aborting."
  exit
fi

cd vorbis
git checkout v1.3.7
cd ..

echo "\nSetting up Opus"
rm -rf opus
git clone https://gitlab.xiph.org/xiph/opus.git

if [ ! -d opus ]; then
  echo "Opus directory is missing, aborting."
  exit
fi

cd opus
git checkout v1.3.1
cd ..

echo "\nSetting up FFmpeg"
rm -rf FFmpeg
git clone https://github.com/FFmpeg/FFmpeg.git

if [ ! -d FFmpeg ]; then
  echo "FFmpeg directory is missing, aborting."
  exit
fi

cd FFmpeg
git checkout n5.1.2

echo
echo "Done setting up all dependencies."

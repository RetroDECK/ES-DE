::  SPDX-License-Identifier: MIT
::
::  ES-DE Frontend
::  Windows_dependencies_setup.bat
::
::  Downloads and prepares the external dependencies for building in-tree using MSVC.
::  If the directories already exist they will be removed and the libraries will be downloaded again.
::
::  This script needs to run from the root of the repository and 7z.exe and curl.exe need to be
::  reachable via the Path environment variable.
::
@echo off

if not exist .clang-format (
  echo You need to run this script from the root of the repository.
  goto end
)

where /Q 7z.exe
if %ERRORLEVEL% neq 0 (
  echo Can't find 7z.exe and it's required by this script, aborting.
  goto end
)

where /Q curl.exe
if %ERRORLEVEL% neq 0 (
  echo Can't find curl.exe and it's required by this script, aborting.
  goto end
)

cd external

echo Setting up dependencies in the .\external directory...

echo:
echo Setting up gettext

if exist gettext\ (
  rmdir /S /Q gettext
)

mkdir gettext
cd gettext

curl -LO https://github.com/vslavik/gettext-tools-windows/releases/download/v0.23.1/gettext-tools-windows-0.23.1.zip
7z x gettext-tools-windows-0.23.1.zip

if not exist bin\msgfmt.exe (
  echo msgfmt.exe is missing, aborting.
  cd ..\..
  goto end
)

mkdir include
copy ..\..\es-app\assets\libintl_Windows.h include\libintl.h

cd bin

dumpbin /exports libintl-8.dll > exports.txt
echo LIBRARY libintl-8 > libintl-8.def
echo EXPORTS >> libintl-8.def
for /f "skip=90 tokens=4" %%A in (exports.txt) do echo %%A >> libintl-8.def
echo DllMain >> libintl-8.def
lib /def:libintl-8.def /out:libintl-8.lib /machine:x64

copy /Y libintl-8.dll ..\..\..
copy /Y libintl-8.lib ..\..\..
copy /Y libiconv-2.dll ..\..\..
cd ..\..

echo:
echo Setting up ICU

if exist icu\ (
  rmdir /S /Q icu
)

git clone -n --filter=tree:0 https://github.com/unicode-org/icu.git

if not exist icu\ (
  echo icu directory is missing, aborting.
  cd ..
  goto end
)

cd icu
git sparse-checkout set --no-cone icu4c
git checkout release-77-1
copy /Y ..\..\es-app\assets\icu_filters.json icu4c\source\
cd ..

echo:
echo Setting up curl

if exist curl-8.13.0_1-win64-mingw\ (
  rmdir /S /Q curl-8.13.0_1-win64-mingw
)

if exist curl\ (
  rmdir /S /Q curl
)

if exist curl-8.13.0_1-win64-mingw.zip (
  del curl-8.13.0_1-win64-mingw.zip
)

curl -O https://curl.se/windows/dl-8.13.0_1/curl-8.13.0_1-win64-mingw.zip
7z x curl-8.13.0_1-win64-mingw.zip

if not exist curl-8.13.0_1-win64-mingw\bin\ (
  echo curl directory is missing, aborting.
  cd ..
  goto end
)

rename curl-8.13.0_1-win64-mingw curl

cd curl\bin

dumpbin /exports libcurl-x64.dll > exports.txt
echo LIBRARY libcurl-x64 > libcurl-x64.def
echo EXPORTS >> libcurl-x64.def
for /f "skip=19 tokens=4" %%A in (exports.txt) do echo %%A >> libcurl-x64.def
lib /def:libcurl-x64.def /out:libcurl-x64.lib /machine:x64

copy /Y libcurl-x64.dll ..\..\..
copy /Y libcurl-x64.lib ..\..\..
cd ..\..

echo:
echo Setting up GLEW

if exist glew-2.2.0\ (
  rmdir /S /Q glew-2.2.0
)

if exist glew\ (
  rmdir /S /Q glew
)

if exist glew-2.2.0-win32.zip (
  del glew-2.2.0-win32.zip
)

curl -LO https://downloads.sourceforge.net/project/glew/glew/2.2.0/glew-2.2.0-win32.zip
7z x glew-2.2.0-win32.zip

if not exist glew-2.2.0\ (
  echo GLEW directory is missing, aborting.
  cd ..
  goto end
)

rename glew-2.2.0 glew

copy /Y glew\bin\Release\x64\glew32.dll ..
copy /Y glew\lib\Release\x64\glew32.lib ..

echo:
echo Setting up HarfBuzz

if exist harfbuzz\ (
  rmdir /S /Q harfbuzz
)

git clone https://github.com/harfbuzz/harfbuzz.git

if not exist harfbuzz\ (
  echo harfbuzz directory is missing, aborting.
  cd ..
  goto end
)

cd harfbuzz
git checkout 11.0.1
mkdir build
cd ..

echo:
echo Setting up FreeType

if exist freetype\ (
  rmdir /S /Q freetype
)

git clone https://github.com/freetype/freetype.git

if not exist freetype\ (
  echo FreeType directory is missing, aborting.
  cd ..
  goto end
)

cd freetype
git checkout VER-2-13-3
mkdir build
cd ..

echo:
echo Setting up FreeImage

if exist FreeImage\ (
  rmdir /S /Q FreeImage
)

if exist FreeImage3180Win32Win64.zip (
  del FreeImage3180Win32Win64.zip
)

curl -LO https://downloads.sourceforge.net/project/freeimage/Binary%%20Distribution/3.18.0/FreeImage3180Win32Win64.zip
7z x FreeImage3180Win32Win64.zip

if not exist FreeImage\ (
  echo FreeImage directory is missing, aborting.
  cd ..
  goto end
)

copy /Y FreeImage\Dist\x64\FreeImage.dll ..
copy /Y FreeImage\Dist\x64\FreeImage.lib ..

echo:
echo Setting up libgit2

if exist libgit2\ (
  rmdir /S /Q libgit2
)

git clone https://github.com/libgit2/libgit2.git

if not exist libgit2\ (
  echo libgit2 directory is missing, aborting.
  cd ..
  goto end
)

cd libgit2
git checkout v1.9.1
mkdir build
cd ..

echo:
echo Setting up Poppler

if exist poppler-24.08.0\ (
  rmdir /S /Q poppler-24.08.0
)

if exist poppler\ (
  rmdir /S /Q poppler
)

if exist Release-24.08.0-0.zip (
  del Release-24.08.0-0.zip
)

curl -LO https://github.com/oschwartz10612/poppler-windows/releases/download/v24.08.0-0/Release-24.08.0-0.zip
7z x Release-24.08.0-0.zip

if not exist poppler-24.08.0\Library\ (
  echo Poppler directory is missing, aborting.
  cd ..
  goto end
)

rename poppler-24.08.0 poppler

copy /Y poppler\Library\lib\poppler-cpp.lib ..\es-pdf-converter
copy /Y poppler\Library\bin\charset.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\deflate.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\freetype.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\iconv.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\jpeg8.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\lcms2.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\Lerc.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\libcrypto-3-x64.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\libcurl.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\liblzma.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\libpng16.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\libssh2.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\openjp2.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\poppler.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\poppler-cpp.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\tiff.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\zlib.dll ..\es-pdf-converter
copy /Y poppler\Library\bin\zstd.dll ..\es-pdf-converter

echo:
echo Setting up pugixml

if exist pugixml\ (
  rmdir /S /Q pugixml
)

git clone https://github.com/zeux/pugixml.git

if not exist pugixml\ (
  echo pugixml directory is missing, aborting.
  cd ..
  goto end
)

cd pugixml
git checkout v1.15
cd ..

echo:
echo Setting up SDL

if exist SDL2-2.32.10\ (
  rmdir /S /Q SDL2-2.32.10
)

if exist SDL2\ (
  rmdir /S /Q SDL2
)

if exist SDL2-devel-2.32.10-VC.zip (
  del SDL2-devel-2.32.10-VC.zip
)

curl -LO https://libsdl.org/release/SDL2-devel-2.32.10-VC.zip

7z x SDL2-devel-2.32.10-VC.zip

if not exist SDL2-2.32.10\ (
  echo SDL directory is missing, aborting.
  cd ..
  goto end
)

rename SDL2-2.32.10 SDL2

cd SDL2
rename include SDL2
cd ..
copy /Y SDL2\lib\x64\SDL2.dll ..
copy /Y SDL2\lib\x64\SDL2.lib ..
copy /Y SDL2\lib\x64\SDL2main.lib ..

echo:
echo Setting up FFmpeg

if exist ffmpeg-n6.0-22-g549430e14d-win64-gpl-shared-6.0\ (
  rmdir /S /Q ffmpeg-n6.0-22-g549430e14d-win64-gpl-shared-6.0
)

if exist ffmpeg\ (
  rmdir /S /Q ffmpeg
)

if exist ffmpeg-n7.1-214-g71889a8437-win64-gpl-shared-7.1.zip (
  del ffmpeg-n7.1-214-g71889a8437-win64-gpl-shared-7.1.zip
)

:: This package should be available for download for two years.
curl -LO https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2025-02-28-13-02/ffmpeg-n7.1-214-g71889a8437-win64-gpl-shared-7.1.zip

7z x ffmpeg-n7.1-214-g71889a8437-win64-gpl-shared-7.1.zip

if not exist ffmpeg-n7.1-214-g71889a8437-win64-gpl-shared-7.1\ (
  echo FFmpeg directory is missing, aborting.
  cd ..
  goto end
)

rename ffmpeg-n7.1-214-g71889a8437-win64-gpl-shared-7.1 ffmpeg

copy /Y ffmpeg\bin\avcodec-61.dll ..
copy /Y ffmpeg\bin\avfilter-10.dll ..
copy /Y ffmpeg\bin\avformat-61.dll ..
copy /Y ffmpeg\bin\avutil-59.dll ..
copy /Y ffmpeg\bin\postproc-58.dll ..
copy /Y ffmpeg\bin\swresample-5.dll ..
copy /Y ffmpeg\bin\swscale-8.dll ..
copy /Y ffmpeg\lib\avcodec.lib ..
copy /Y ffmpeg\lib\avfilter.lib ..
copy /Y ffmpeg\lib\avformat.lib ..
copy /Y ffmpeg\lib\avutil.lib ..
copy /Y ffmpeg\lib\swresample.lib ..
copy /Y ffmpeg\lib\swscale.lib ..

echo:
echo Setting up OpenSSL

if exist Win64OpenSSL_Light-3_4_0.exe (
  del Win64OpenSSL_Light-3_4_0.exe
)

if not exist "C:\Program Files\OpenSSL-Win64\libcrypto-3-x64.dll" (
  curl -O https://slproweb.com/download/Win64OpenSSL_Light-3_6_1.exe
  :: Run the installer.
  .\Win64OpenSSL_Light-3_6_1.exe
)

:: Return to the root of the repository.
cd ..

if exist "C:\Program Files\OpenSSL-Win64\libcrypto-3-x64.dll" (
  copy /Y "C:\Program Files\OpenSSL-Win64\libcrypto-3-x64.dll"
  copy /Y "C:\Program Files\OpenSSL-Win64\libssl-3-x64.dll"
)

echo:
echo Copying DLL files from Windows\System32

copy /Y C:\Windows\System32\vcomp140.dll

echo:
echo Done setting up all dependencies.

:end

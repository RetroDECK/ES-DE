::  SPDX-License-Identifier: MIT
::
::  EmulationStation Desktop Edition
::  Windows_dependencies_setup_MSVC.bat
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

echo Setting up dependencies in the .\external directory...
echo:

cd external

echo Setting up curl

if exist curl-7.86.0-win64-mingw\ (
  rmdir /S /Q curl-7.86.0-win64-mingw
)

curl -O https://curl.se/windows/dl-7.86.0/curl-7.86.0-win64-mingw.zip
7z x curl-7.86.0-win64-mingw.zip

if not exist curl-7.86.0-win64-mingw\bin\ (
  echo curl directory is missing, aborting.
  cd ..
  goto end
)

cd curl-7.86.0-win64-mingw\bin

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

if exist glew-2.1.0\ (
  rmdir /S /Q glew-2.1.0
)

curl -LO https://downloads.sourceforge.net/project/glew/glew/2.1.0/glew-2.1.0-win32.zip
7z x glew-2.1.0-win32.zip

if not exist glew-2.1.0\ (
  echo GLEW directory is missing, aborting.
  cd ..
  goto end
)

copy /Y glew-2.1.0\bin\Release\x64\glew32.dll ..
copy /Y glew-2.1.0\lib\Release\x64\glew32.lib ..

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
git checkout VER-2-12-1
mkdir build
cd ..

echo:
echo Setting up FreeImage

if exist FreeImage\ (
  rmdir /S /Q FreeImage
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
git checkout v1.12.1
cd ..

echo:
echo Setting up SDL

if exist SDL2-2.26.3\ (
  rmdir /S /Q SDL2-2.26.3
)

curl -LO https://libsdl.org/release/SDL2-devel-2.26.3-VC.zip

7z x SDL2-devel-2.26.3-VC.zip

if not exist SDL2-2.26.3\ (
  echo SDL directory is missing, aborting.
  cd ..
  goto end
)

cd SDL2-2.26.3
rename include SDL2
cd ..
copy /Y SDL2-2.26.3\lib\x64\SDL2.dll ..
copy /Y SDL2-2.26.3\lib\x64\SDL2.lib ..
copy /Y SDL2-2.26.3\lib\x64\SDL2main.lib ..

echo:
echo Setting up FFmpeg

if exist ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\ (
  rmdir /S /Q ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1
)

:: This package should be available for download for two years.
curl -LO https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2022-09-30-12-41/ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1.zip
7z x ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1.zip

if not exist ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\ (
  echo FFmpeg directory is missing, aborting.
  cd ..
  goto end
)

copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\bin\avcodec-59.dll ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\bin\avfilter-8.dll ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\bin\avformat-59.dll ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\bin\avutil-57.dll ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\bin\postproc-56.dll ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\bin\swresample-4.dll ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\bin\swscale-6.dll ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\lib\avcodec.lib ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\lib\avfilter.lib ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\lib\avformat.lib ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\lib\avutil.lib ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\lib\swresample.lib ..
copy /Y ffmpeg-n5.1.2-1-g05d6157aab-win64-gpl-shared-5.1\lib\swscale.lib ..

echo:
echo Setting up OpenSSL

if not exist "C:\Program Files\OpenSSL-Win64\libcrypto-1_1-x64.dll" (
  curl -O https://slproweb.com/download/Win64OpenSSL_Light-1_1_1m.exe
  :: Run the installer.
  .\Win64OpenSSL_Light-1_1_1m.exe
)

:: Return to the root of the repository.
cd ..

if exist "C:\Program Files\OpenSSL-Win64\libcrypto-1_1-x64.dll" (
  copy /Y "C:\Program Files\OpenSSL-Win64\libcrypto-1_1-x64.dll"
  copy /Y "C:\Program Files\OpenSSL-Win64\libssl-1_1-x64.dll"
)

echo:
echo Copying DLL files from Windows\System32

copy /Y C:\Windows\System32\MSVCP140.dll
copy /Y C:\Windows\System32\VCOMP140.DLL
copy /Y C:\Windows\System32\VCRUNTIME140.dll
copy /Y C:\Windows\System32\VCRUNTIME140_1.dll

echo:
echo Done setting up all dependencies.

:end

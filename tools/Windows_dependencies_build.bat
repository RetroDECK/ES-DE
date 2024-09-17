::  SPDX-License-Identifier: MIT
::
::  ES-DE Frontend
::  Windows_dependencies_build.bat
::
::  Builds the external dependencies in-tree using MSVC.
::  The Windows_dependencies_setup.bat script must have been executed prior to this.
::  All libraries will be recompiled from scratch every time.
::
::  This script needs to run from the root of the repository.
::
@echo off

if not exist .clang-format (
  echo You need to run this script from the root of the repository.
  goto end
)

if not exist external\pugixml\ (
  echo You need to first run tools\Windows_dependencies_setup.bat to download and configure the dependencies.
  goto end
)

cd external

echo Building all dependencies in the .\external directory...

echo:
echo Building ICU

if not exist icu/icu4c\ (
  echo icu/icu4c directory is missing, aborting.
  cd ..
  goto end
)

cd icu/icu4c
set ICU_DATA_FILTER_FILE=%cd%\source\icu_filters.json

if not exist %ICU_DATA_FILTER_FILE% (
  echo %ICU_DATA_FILTER_FILE% file is missing, aborting.
  cd ..
  goto end
)

msbuild source\allinone\allinone.sln /p:Configuration=Release /p:Platform=x64 /p:SkipUWP=true

copy /Y bin64\icudt75.dll ..\..\..\
copy /Y bin64\icuin75.dll ..\..\..\
copy /Y bin64\icuuc75.dll ..\..\..\
copy /Y lib64\icudt.lib ..\..\..\
copy /Y lib64\icuin.lib ..\..\..\
copy /Y lib64\icuuc.lib ..\..\..\
cd ..\..

echo:
echo Building HarfBuzz

if not exist harfbuzz\build\ (
  echo harfbuzz directory is missing, aborting.
  cd ..
  goto end
)

cd harfbuzz\build
if exist CMakeCache.txt (
  nmake clean
  del CMakeCache.txt
)

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DHB_BUILD_SUBSET=off ..
nmake
copy /Y harfbuzz.dll ..\..\..\
copy /Y harfbuzz.lib ..\..\..\
cd ..\..

echo:
echo Building FreeType

if not exist freetype\build\ (
  echo FreeType directory is missing, aborting.
  cd ..
  goto end
)

cd freetype\build
if exist CMakeCache.txt (
  nmake clean
  del CMakeCache.txt
)

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON ..
nmake
copy /Y freetype.dll ..\..\..\
copy /Y freetype.lib ..\..\..\
cd ..\..

echo:
echo Building libgit2

if not exist libgit2\build\ (
  echo libgit2 directory is missing, aborting.
  cd ..
  goto end
)

cd libgit2\build
if exist CMakeCache.txt (
  nmake clean
  del CMakeCache.txt
)

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF ..
nmake
copy /Y git2.dll ..\..\..\
copy /Y git2.lib ..\..\..\
cd ..\..

echo:
echo Building pugixml

if not exist pugixml\ (
  echo pugixml directory is missing, aborting.
  cd ..
  goto end
)

cd pugixml
if exist CMakeCache.txt (
  nmake clean
  del CMakeCache.txt
)

cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON .
nmake
copy /Y pugixml.dll ..\..
copy /Y pugixml.lib ..\..
cd..

:: Return to the root of the repository.
cd ..

echo:
echo Done building all dependencies.

:end

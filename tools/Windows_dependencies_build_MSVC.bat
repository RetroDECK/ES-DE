::  SPDX-License-Identifier: MIT
::
::  EmulationStation Desktop Edition
::  Windows_dependencies_build_MSVC.bat
::
::  Builds the external dependencies in-tree using MSVC.
::  The Windows_dependencies_setup_MSVC.bat script must have been executed prior to this.
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
  echo You need to first run tools\Windows_dependencies_setup_MSVC.bat to download and configure the dependencies.
  goto end
)

echo Building all dependencies in the .\external directory...
echo:

cd external

echo Building FreeType

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
echo Building pugixml

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

:end

#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  CMakeLists.txt (es-app)
#
#  CMake configuration for es-app.
#  Also contains the application packaging configuration.
#

project("emulationstation-de")

set(ES_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CollectionSystemsManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EmulationStation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileFilterIndex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileSorts.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gamelist.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MediaViewer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MetaData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MiximageGenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformId.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemScreensaver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VolumeControl.h

    # GUIs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiAlternativeEmulators.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiCollectionSystemsOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistFilter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiLaunchScreen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMediaViewerOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMenu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMetaDataEd.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiOfflineGenerator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperMenu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperMulti.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperSearch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScreensaverOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSettings.h

    # Scrapers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraperResources.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/Scraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/ScreenScraper.h

    # Views
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/BasicGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/DetailedGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/GridGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/IGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/ISimpleGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/VideoGameListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/SystemView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/UIModeController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/ViewController.h
)

set(ES_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CollectionSystemsManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileFilterIndex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileSorts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gamelist.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MediaViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MetaData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MiximageGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformId.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemScreensaver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VolumeControl.cpp

    # GUIs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiAlternativeEmulators.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiCollectionSystemsOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiLaunchScreen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMediaViewerOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMetaDataEd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiOfflineGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperMenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperMulti.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperSearch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScreensaverOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSettings.cpp

    # Scrapers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraperResources.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/Scraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/ScreenScraper.cpp

    # Views
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/BasicGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/DetailedGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/GridGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/IGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/ISimpleGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/VideoGameListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/SystemView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/UIModeController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/ViewController.cpp
)

if(WIN32)
    LIST(APPEND ES_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation.rc)
endif()

#---------------------------------------------------------------------------------------------------
# OS-specific installation and package generation setup.

# Define target.
if(APPLE OR WIN32)
    include_directories(${COMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_executable(EmulationStation ${ES_SOURCES} ${ES_HEADERS})
    target_link_libraries(EmulationStation ${COMMON_LIBRARIES} es-core)
    set_target_properties(EmulationStation PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
else()
    include_directories(${COMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_executable(emulationstation ${ES_SOURCES} ${ES_HEADERS})
    target_link_libraries(emulationstation ${COMMON_LIBRARIES} es-core)
    set_target_properties(emulationstation PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# Setup for installation and package generation.
if(WIN32)
    install(TARGETS EmulationStation RUNTIME DESTINATION .)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        install(FILES ../avcodec-58.dll
                      ../avfilter-7.dll
                      ../avformat-58.dll
                      ../avutil-56.dll
                      ../postproc-55.dll
                      ../swresample-3.dll
                      ../swscale-5.dll
                      ../FreeImage.dll
                      ../freetype.dll
                      ../glew32.dll
                      ../libcrypto-1_1-x64.dll
                      ../libcurl-x64.dll
                      ../libssl-1_1-x64.dll
                      ../MSVCP140.dll
                      ../pugixml.dll
                      ../SDL2.dll
                      ../VCOMP140.DLL
                      ../VCRUNTIME140.dll
                      ../VCRUNTIME140_1.dll
                      DESTINATION .)
        if(VLC_PLAYER)
            install(FILES ../libvlc.dll ../libvlccore.dll DESTINATION .)
        endif()
    else()
        install(FILES ../avcodec-58.dll
                      ../avfilter-7.dll
                      ../avformat-58.dll
                      ../avutil-56.dll
                      ../postproc-55.dll
                      ../swresample-3.dll
                      ../swscale-5.dll
                      ../FreeImage.dll
                      ../glew32.dll
                      ../libcrypto-1_1-x64.dll
                      ../libcurl-x64.dll
                      ../libfreetype.dll
                      ../libpugixml.dll
                      ../libssl-1_1-x64.dll
                      ../SDL2.dll
                      ../vcomp140.dll
                      DESTINATION .)
        if(VLC_PLAYER)
            install(FILES ../libvlc.dll ../libvlccore.dll DESTINATION .)
        endif()
    endif()
    if(VLC_PLAYER)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/plugins DESTINATION .)
    endif()
    install(FILES ../LICENSE DESTINATION .)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/licenses DESTINATION .)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/themes DESTINATION .)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION .)
elseif(APPLE)
    # For completely unknown reasons, when generating a Bundle using cpack, an extra 'Resources'
    # directory is added to the target path. Simply adding the two dots as a prefix fixes the
    # problem, but doing so would break 'make install' which was actually behaving correctly.
    # So an extra 'Resources' directory was added to the CMAKE_INSTALL_PREFIX variable as well
    # to compensate for this. It's a bad solution to the problem and there must surely be a
    # better way to fix this.
    install(TARGETS EmulationStation RUNTIME DESTINATION ../MacOS)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE.icns DESTINATION ../Resources)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE_Info.plist DESTINATION .. RENAME Info.plist)

    set(APPLE_DYLIB_PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE
                                GROUP_READ GROUP_EXECUTE
                                WORLD_READ WORLD_EXECUTE)

    install(FILES ${CMAKE_SOURCE_DIR}/libavcodec.58.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libavfilter.7.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libavformat.58.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libavutil.56.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libpostproc.55.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libswresample.3.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libswscale.5.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libfdk-aac.2.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libvorbis.0.4.9.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libvorbisenc.2.0.12.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libfreetype.6.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
    install(FILES ${CMAKE_SOURCE_DIR}/libSDL2-2.0.dylib
        PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)

    if(VLC_PLAYER)
        install(FILES ${CMAKE_SOURCE_DIR}/libvlc.dylib
            PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
        install(FILES ${CMAKE_SOURCE_DIR}/libvlccore.dylib
            PERMISSIONS ${APPLE_DYLIB_PERMISSIONS} DESTINATION ../MacOS)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/plugins
            DESTINATION ../MacOS)
    endif()

    install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION ../Resources)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION ../Resources)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/themes DESTINATION ../Resources)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/licenses DESTINATION ../Resources)
else()
    install(TARGETS emulationstation RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/emulationstation.6.gz
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man6)
    else()
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/emulationstation.6.gz
            DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man6)
    endif()
    install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/emulationstation)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/org.es_de.emulationstation-de.desktop
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/emulationstation.svg
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/org.es_de.emulationstation-de.appdata.xml
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/licenses
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/emulationstation)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/themes
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/emulationstation)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/emulationstation)
endif()

include(InstallRequiredSystemLibraries)

#---------------------------------------------------------------------------------------------------
# General CPack settings.

set(CPACK_PACKAGE_NAME "emulationstation-de")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Emulator frontend")
set(CPACK_PACKAGE_DESCRIPTION "EmulationStation Desktop Edition (ES-DE) is a frontend for browsing and launching games from your multi-platform game collection.

It's intended to be used in conjunction with emulators such as the RetroArch cores.")
if(APPLE)
    # Shorter line length license file to be able to fit inside the drag-and-drop installer window without introducing extra line breaks.
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE_LICENSE_macOS")
elseif(WIN32)
    # The installer window looks a bit different on Windows so a specific file for this OS is required.
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE_LICENSE_Windows")
else()
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()
set(CPACK_PACKAGE_VENDOR "Leon Styhre")

# Update this when there has been a new release.
set(CPACK_PACKAGE_VERSION "1.2.0-beta2-dev")

# Use the shorter x64 descriptor if on the x86_64/AMD64 architecture.
if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL AMD64)
    set(CPU_ARCHITECTURE "x64")
else()
    set(CPU_ARCHITECTURE "${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

# Settings per operating system and generator type.
if(APPLE)
    set(CPACK_GENERATOR "Bundle")
    if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 10.14)
        set(CPACK_PACKAGE_FILE_NAME "EmulationStation-DE-${CPACK_PACKAGE_VERSION}-${CPU_ARCHITECTURE}_legacy")
        set(CPACK_DMG_VOLUME_NAME "EmulationStation Desktop Edition ${CPACK_PACKAGE_VERSION}_legacy")
    else()
        set(CPACK_PACKAGE_FILE_NAME "EmulationStation-DE-${CPACK_PACKAGE_VERSION}-${CPU_ARCHITECTURE}")
        set(CPACK_DMG_VOLUME_NAME "EmulationStation Desktop Edition ${CPACK_PACKAGE_VERSION}")
    endif()
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE.icns")
    set(CPACK_DMG_DS_STORE "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE_DS_Store")
    set(CPACK_BUNDLE_NAME "EmulationStation Desktop Edition")
    set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE.icns")
    set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE_Info.plist")
    if(MACOS_CODESIGN_IDENTITY)
        set(CPACK_BUNDLE_APPLE_CERT_APP "Developer ID Application: ${MACOS_CODESIGN_IDENTITY}")
        if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_GREATER 10.13)
            set(CPACK_BUNDLE_APPLE_CODESIGN_PARAMETER "--deep --force --options runtime")
        endif()
    endif()
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_FILE_NAME "EmulationStation-DE-${CPACK_PACKAGE_VERSION}-${CPU_ARCHITECTURE}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "EmulationStation-DE")
    set(CPACK_PACKAGE_EXECUTABLES "EmulationStation" "EmulationStation")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation.ico")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_DISPLAY_NAME "EmulationStation Desktop Edition ${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_PACKAGE_NAME "EmulationStation Desktop Edition")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "EmulationStation.exe")
    set(CPACK_NSIS_WELCOME_TITLE "EmulationStation Desktop Edition Installer")
    set(CPACK_NSIS_FINISH_TITLE "EmulationStation Desktop Edition Installation Completed")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    set(CPACK_NSIS_MENU_LINKS "https://es-de.org" "ES-DE Website" "https://es-de.org/#Donations" "Please Donate")
else()
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "emulationstation_${CMAKE_PACKAGE_VERSION}")
    set(CPACK_PACKAGE_EXECUTABLES "emulationstation" "emulationstation")
    if(LINUX_CPACK_GENERATOR STREQUAL "DEB")
        set(CPACK_GENERATOR "DEB")
    endif()
    set(CPACK_DEBIAN_FILE_NAME "emulationstation-de-${CPACK_PACKAGE_VERSION}-${CPU_ARCHITECTURE}.deb")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Leon Styhre <leon@leonstyhre.com>")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://es-de.org")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    if(VLC_PLAYER)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "vlc")
    endif()
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    if(LINUX_CPACK_GENERATOR STREQUAL "RPM")
        set(CPACK_GENERATOR "RPM")
    endif()
    set(CPACK_RPM_FILE_NAME "emulationstation-de-${CPACK_PACKAGE_VERSION}-${CPU_ARCHITECTURE}.rpm")
    set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    if(VLC_PLAYER)
        set(CPACK_RPM_PACKAGE_REQUIRES "vlc")
    endif()
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CMAKE_INSTALL_PREFIX}")
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CMAKE_INSTALL_PREFIX}/bin")
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CMAKE_INSTALL_PREFIX}/share")
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CMAKE_INSTALL_PREFIX}/share/man")
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CMAKE_INSTALL_PREFIX}/share/man/man6")
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CMAKE_INSTALL_PREFIX}/share/metainfo")
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CMAKE_INSTALL_PREFIX}/share/pixmaps")
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CMAKE_INSTALL_PREFIX}/share/applications")
endif()

include(CPack)

project("emulationstation-de")

set(ES_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/EmulationStation.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FileData.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FileSorts.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MetaData.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformId.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemData.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VolumeControl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Gamelist.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FileFilterIndex.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemScreensaver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CollectionSystemManager.h

        # Guis
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMetaDataEd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameScraper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistOptions.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScreensaverOptions.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMenu.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSettings.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperMenu.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperMulti.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperSearch.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistFilter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiCollectionSystemsOptions.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInfoPopup.h

        # Scrapers
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/Scraper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraperResources.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/ScreenScraper.h

        # Views
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/BasicGameListView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/DetailedGameListView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/IGameListView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/ISimpleGameListView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/GridGameListView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/VideoGameListView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/SystemView.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/ViewController.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/UIModeController.h

        # Animations
        ${CMAKE_CURRENT_SOURCE_DIR}/src/animations/MoveCameraAnimation.h
)

set(ES_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FileData.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FileSorts.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MetaData.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PlatformId.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemData.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VolumeControl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Gamelist.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FileFilterIndex.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemScreensaver.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CollectionSystemManager.cpp

        # Guis
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMetaDataEd.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameScraper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistOptions.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScreensaverOptions.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMenu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSettings.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperMenu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperMulti.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScraperSearch.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistFilter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiCollectionSystemsOptions.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInfoPopup.cpp

        # Scrapers
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/Scraper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/GamesDBJSONScraperResources.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scrapers/ScreenScraper.cpp

        # Views
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/BasicGameListView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/DetailedGameListView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/IGameListView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/ISimpleGameListView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/GridGameListView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/gamelist/VideoGameListView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/SystemView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/ViewController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/views/UIModeController.cpp
)

#---------------------------------------------------------------------------------------------------
# Define OS specific sources and headers.
if(WIN32)
    LIST(APPEND ES_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation.rc)
endif()

# Define target.
if(APPLE OR WIN32)
    include_directories(${COMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_executable(EmulationStation ${ES_SOURCES} ${ES_HEADERS})
    target_link_libraries(EmulationStation ${COMMON_LIBRARIES} es-core)
else()
    include_directories(${COMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_executable(emulationstation ${ES_SOURCES} ${ES_HEADERS})
    target_link_libraries(emulationstation ${COMMON_LIBRARIES} es-core)
    set_target_properties(emulationstation PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# Setup for installation and package generation.
if(WIN32)
    install(TARGETS EmulationStation RUNTIME DESTINATION .)
    install(FILES ../FreeImage.dll ../glew32.dll ../libcrypto-1_1-x64.dll ../libcurl-x64.dll
            ../libfreetype.dll ../libgcc_s_seh-1.dll ../libpugixml.dll ../libssl-1_1-x64.dll
            ../libstdc++-6.dll ../libvlc.dll ../libvlccore.dll ../libwinpthread-1.dll
            ../SDL2.dll DESTINATION .)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/plugins DESTINATION .)
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
    install(TARGETS EmulationStation RUNTIME
            DESTINATION ../MacOS)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE.icns
            DESTINATION ../Resources)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE_Info.plist
            DESTINATION .. RENAME Info.plist)

    # Another hack/workaround. I have not been able to find any way whatsover to force the
    # linker to use rpaths for all shared libraries instead of absolute paths. So instead
    # we're running install_name_tool as a post-build command to manually change the absolute
    # paths to the rpaths for these libraries. The worst thing about this approach is that
    # the library version needs to be manually changed here depending on what's installed
    # on your system (e.g. if using libSDL2-2.1.0.dylib instead of libSDL2-2.0.0.dylib).
    # This problem definitely needs to be resolved properly at a later date.
    add_custom_command(TARGET EmulationStation POST_BUILD COMMAND ${CMAKE_INSTALL_NAME_TOOL}
            -change /usr/lib/libcurl.4.dylib @rpath/libcurl.4.dylib
            -change /usr/local/opt/freeimage/lib/libfreeimage.dylib @rpath/libfreeimage.dylib
            -change /usr/local/opt/freetype/lib/libfreetype.6.dylib @rpath/libfreetype.6.dylib
            -change /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib @rpath/libSDL2-2.0.0.dylib
            $<TARGET_FILE:EmulationStation>)

    if(NOT APPLE_SKIP_INSTALL_LIBS)
        install(FILES ${CMAKE_SOURCE_DIR}/libcurl.4.dylib
                DESTINATION ../MacOS)
        install(FILES ${CMAKE_SOURCE_DIR}/libfreeimage.dylib
                DESTINATION ../MacOS)
        install(FILES ${CMAKE_SOURCE_DIR}/libfreetype.6.dylib
                DESTINATION ../MacOS)
        install(FILES ${CMAKE_SOURCE_DIR}/libSDL2-2.0.0.dylib
                DESTINATION ../MacOS)
        install(FILES ${CMAKE_SOURCE_DIR}/libvlc.dylib
                DESTINATION ../MacOS)
        install(FILES ${CMAKE_SOURCE_DIR}/libvlccore.dylib
                DESTINATION ../MacOS)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/plugins
                DESTINATION ../MacOS)
    endif()
    install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
            DESTINATION ../Resources)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources
            DESTINATION ../Resources)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/themes
                   DESTINATION ../Resources)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/licenses
            DESTINATION ../Resources)
else()
    install(TARGETS emulationstation RUNTIME
            DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/emulationstation.6.gz
            DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man6)
    install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/emulationstation)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/emulationstation.desktop
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/emulationstation.svg
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps)
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
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An emulator front-end with controller navigation and theming support.")
set(CPACK_PACKAGE_DESCRIPTION "EmulationStation Desktop Edition is a fast and flexible front-end for browsing and launching games from your multi-platform retro game collection. It's intended to be used in conjunction with emulators such as the RetroArch cores.")
if(APPLE)
# Shorter line length license file to be able to fit inside the macOS drag and drop installer window without causing ugly line breaks.
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE_LICENSE")
else()
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()
set(CPACK_PACKAGE_VENDOR "Leon Styhre")

# Update this when there has been a new release.
set(CPACK_PACKAGE_VERSION "1.0.0-beta")

# Settings per operating system and generator type.
if(APPLE)
    set(CPACK_GENERATOR "Bundle")
    set(CPACK_PACKAGE_FILE_NAME "EmulationStation-DE-${CPACK_PACKAGE_VERSION}")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE.icns")
    set(CPACK_DMG_VOLUME_NAME "EmulationStation Desktop Edition ${CPACK_PACKAGE_VERSION}")
    set(CPACK_DMG_DS_STORE "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE_DS_Store")
    set(CPACK_BUNDLE_NAME "EmulationStation Desktop Edition")
    set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE.icns")
    set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation-DE_Info.plist")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_FILE_NAME "EmulationStation-DE-${CPACK_PACKAGE_VERSION}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "EmulationStation")
    set(CPACK_PACKAGE_EXECUTABLES "EmulationStation" "EmulationStation")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/EmulationStation.ico")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_DISPLAY_NAME "EmulationStation Desktop Edition")
    set(CPACK_NSIS_INSTALLED_ICON_NAME
            "${CMAKE_CURRENT_SOURCE_DIR}/es-app/assets/EmulationStation.ico")
    set(CPACK_NSIS_WELCOME_TITLE "EmulationStation Desktop Edition Installer")
    set(CPACK_NSIS_FINISH_TITLE "EmulationStation Desktop Edition Installation Completed")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
else()
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "emulationstation_${CMAKE_PACKAGE_VERSION}")
    set(CPACK_PACKAGE_EXECUTABLES "emulationstation" "emulationstation")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_FILE_NAME "emulationstation-de-${CPACK_PACKAGE_VERSION}.deb")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Leon Styhre <leon@leonstyhre.com>")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://gitlab.com/leonstyhre/emulationstation-de")
    set(CPACK_DEBIAN_PACKAGE_SECTION "misc")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    #set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libsdl2-2.0-0, libfreeimage3, libfreetype6, libvlc5, libcurl4, libpugixml1v5, libasound2")
    set(CPACK_DEBIAN_PACKAGE_BUILDS_DEPENDS "debhelper (>= 8.0.0), cmake, g++ (>= 4.8), libsdl2-dev, libfreeimage-dev, libfreetype6-dev, libcurl4-openssl-dev, libpugixml-dev, rapidjson-dev, libasound2-dev, libvlc-dev, libgl1-mesa-dev")

    # Uncomment the next line to generate .rpm packages.
    #set(CPACK_GENERATOR "RPM")
    set(CPACK_RPM_FILE_NAME "emulationstation-de-${CPACK_PACKAGE_VERSION}.rpm")
    set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
    set(CPACK_RPM_PACKAGE_REQUIRES "libc6, libsdl2-2.0-0, libfreeimage3, libfreetype6, libvlc5, libcurl4, libpugixml1v5, libasound2")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
endif()

include(CPack)

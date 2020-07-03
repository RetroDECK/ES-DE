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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemScreenSaver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CollectionSystemManager.h

    # Guis
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMetaDataEd.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameScraper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScreensaverOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGeneralScreensaverOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiVideoScreensaverOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSlideshowScreensaverOptions.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animations/LaunchAnimation.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SystemScreenSaver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CollectionSystemManager.cpp

    # Guis
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMetaDataEd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGameScraper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGamelistOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiScreensaverOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiGeneralScreensaverOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiVideoScreensaverOptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiSlideshowScreensaverOptions.cpp
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

#-------------------------------------------------------------------------------
# Define OS specific sources and headers.
if(WIN32)
    LIST(APPEND ES_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/EmulationStation.rc
    )
endif()

#-------------------------------------------------------------------------------
# Define target.
include_directories(${COMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_executable(emulationstation ${ES_SOURCES} ${ES_HEADERS})
target_link_libraries(emulationstation ${COMMON_LIBRARIES} es-core)

#-------------------------------------------------------------------------------
# Set up CPack install for `make install`.

install(TARGETS emulationstation
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/LICENSES
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/emulationstation)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/themes
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/emulationstation)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/emulationstation)

INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_NAME "emulationstation-de")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An emulator front-end with controller navigation and theming support.")
SET(CPACK_PACKAGE_DESCRIPTION "EmulationStation Desktop Edition is a fast, feature rich front-end for browsing and launching games from your multi-platform retro game collection. It's intended to be used in conjunction with emulators such as the RetroArch cores.")

SET(CPACK_RESOURCE_FILE LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
#SET(CPACK_RESOURCE_FILE README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Leon Styhre <leon@leonstyhre.com>")
SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://gitlab.com/leonstyhre/emulationstation-de")
SET(CPACK_DEBIAN_PACKAGE_SECTION "misc")
SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
#SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libsdl2-2.0-0, libfreeimage3, libfreetype6, libvlc5, libcurl4, libpugixml1v5, libasound2")
SET(CPACK_DEBIAN_PACKAGE_BUILDS_DEPENDS "debhelper (>= 8.0.0), cmake, g++ (>= 4.8), libsdl2-dev, libfreeimage-dev, libfreetype6-dev, libcurl4-openssl-dev, libpugixml-dev, rapidjson-dev, libasound2-dev, libvlc-dev, libgl1-mesa-dev")

SET(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
SET(CPACK_RPM_PACKAGE_REQUIRES "libc6, libsdl2-2.0-0, libfreeimage3, libfreetype6, libvlc5, libcurl4, libpugixml1v5, libasound2")
SET(CPACK_RPM_PACKAGE_LICENSE "MIT")

SET(CPACK_PACKAGE_VENDOR "https://gitlab.com/leonstyhre/emulationstation-de")
SET(CPACK_PACKAGE_VERSION "1.0.0")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "emulationstation_${CMAKE_PACKAGE_VERSION}")
SET(CPACK_PACKAGE_EXECUTABLES "emulationstation" "emulationstation")

SET(CPACK_RPM_FILE_NAME "emulationstation-de-${CPACK_PACKAGE_VERSION}.rpm")
SET(CPACK_DEBIAN_FILE_NAME "emulationstation-de-${CPACK_PACKAGE_VERSION}.deb")
#SET(CPACK_GENERATOR "RPM")
SET(CPACK_GENERATOR "DEB")

INCLUDE(CPack)

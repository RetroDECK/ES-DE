v1.0.0
======

### General

* Initial version, fork from RetroPie EmulationStation 2.10.0rp-dev (master)
* Reorganization and general overhaul of the menu system, hopefully making it more intuitive and easy to understand
* Many quality of life improvements and removal of GUI inconsistencies
* New game media file logic using a media directory with files matching the ROM names instead of pointing to the media files in gamelist.xml
* Updated scraper to support additional media files, detailed configuration of what to scrape, semi-automatic mode etc.
* For single-game scraping, any values updated by the scraper are now highlighted using a different font color in the metadata editor
* Gamelist sorting now working as expected and is persistent throughout the application session
* Full navigation sound support, configurable per theme
* New default theme rbsimple-DE bundled with the software, this theme is largely based on recalbox-multi by the Recalbox community
* Added extensive es_systems.cfg templates for Unix and Windows
* Updated the application to compile and work on Microsoft Windows, including full UTF-16 (Unicode) support
* Seamless (almost) launch of games without showing the desktop when starting and returning from RetroArch and other emulators
* Per-game launch command override, so that different cores or emulators can be used on a per-game basis (saved to gamelist.xml)
* Core location can be defined relative to the emulator binary using the %EMUPATH% varible in es_systems.cfg (mostly useful for Windows)
* Help system updated and expanded to the complete application (previously it was only partially implemented)
* GUI-configurable option to sort favorite games on the top of the game lists (favorites marked with stars)
* Added new component GuiComplexTextEditPopup to handle changes to configuration file entries and similar
* Speed improvements and optimizations, the application now starts faster and feels more responsive
* Moved all resources to a subdirectory structure and enabled the CMake install prefix variable to generate the resources search path
* Changed theme directory to the install prefix (e.g. /usr/local/share/emulationstation/themes) with themes in the home directory taking precedence
* No more attempts to open files directly under /etc, instead only the install prefix directory and the home directory are used
* Added proper error handling for missing resource files and improved overall logging
* Refactoring, cleanup and documentation of the source code, removal of deprecated files etc.
* All required fonts bundled with the application, no dependencies on the OS to provide them any longer
* Made pugixml an external dependency instead of bundling it
* Updated the CMake/CPack install and package build script to work as expected (can now generate .deb, .rpm and NSIS installation packages)
* Added support for Clang/LLVM, made the application build with no errors or warnings using this compiler (Unix only)
* License files included for all the libraries and resources that are bundled with the application
* Updated the MAME ROM index files to include ROMs up to MAME version 0.221 (and created scripts to easily generate these index files in the future)

### Bug fixes

* Metadata editor insisted that changes were made although nothing was updated
  Note: The editor will still ask for save confirmations after automatically rounding fractional game ratings to half-star values
* Game images were sometimes scaled incorrectly
* Non-transparent favorite icons were not rendered correctly
* Restart and power-off menu entries not working (at least not on my desktop OS)
* Toggling the screensaver didn't work as expected
* Lots and lots of small bugs and inconsistencies fixed

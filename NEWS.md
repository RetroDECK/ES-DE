# EmulationStation Desktop Edition v1.0.0

**Release date:** YYYY-MM-DD

### Release overview

First release, a major update to the application compared to the RetroPie version on which it is based. This includes new gamelist sorting logic, new game media handling and an updated Windows port as well as a macOS port (which both work about as well as the Unix version). The menu system has also been completely overhauled and the scraper has been expanded to support multiple media types (including videos) as well as providing detailed scraping configuration options.

Full navigation sound support has been implemented, and the metadata editor has seen a lot of updates including color coding of all changes done by the user and by the scraper. Favorite games can now also be sorted on top of the gamelists and game collections.

OpenGL GLSL shader support has been added (not for the OpenGL ES renderer though) and there are multiple effects implemented such as scanlines for videos, blurred background when opening menus etc.

A new default theme rbsimple-DE (based on Recalbox Multi) is bundled with the application and is part of the installation package/installer. However themes created for other EmulationStation ports should still work correctly.

Many bugs have been fixed, and numerous features that were only partially implemented or broken have been updated to a fully working state. The application runs much faster as well due to lots of optimizations.

### Detailed list of changes

* Initial version, fork from RetroPie EmulationStation 2.10.0rp-dev (master)
* Reorganization and general overhaul of the menu system, hopefully making it more intuitive and easy to understand
* Many quality of life improvements and removal of GUI inconsistencies
* Game systems are now sorted by full names which makes much more sense from a user perspective
* New game media file logic using a media directory with files matching the ROM names instead of pointing to the media files in gamelist.xml
* Updated scraper to support additional media files, detailed configuration of what to scrape, semi-automatic mode etc.
* In the metadata editor, any values updated by the single-game scraper or by the user are now highlighted using a different font color
* Files or folders can now be flagged for exclusion when scraping with the multi-scraper, and for folders it can be set to apply recursively
* Gamelist sorting is now working as expected and is persistent throughout the application session
* Overhaul of the game collection functionality including many bug fixes and optimizations
* Game counting is now done during sorting instead of every time a system is selected. This should make the UI more responsive in case of large game libraries
* Added a system view counter for favorite games in addition to the total number of games
* Added support for jumping to the start and end of gamelists and menus using the controller trigger buttons (or equivalent keyboard mappings)
* Full navigation sound support, configurable per theme with a fallback to the built-in sounds if the theme does not support it
* New default theme rbsimple-DE bundled with the software, this theme is largely based on recalbox-multi by the Recalbox community
* Added extensive es_systems.cfg templates for Unix and Windows
* Updated the application to compile and work on Microsoft Windows, including full UTF-16 (Unicode) support
* Updated the application to compile and work on Apple macOS
* Added support for OpenGL GLSL shaders (OpenGL 2.1 renderer only, no support for OpenGL ES 1.0 renderer)
* Added multiple animations and shader effects, such as when opening menus, playing videos in the gamelists and via the screensaver etc.
* Seamless (almost) launch of games without showing the desktop when starting and when returning from RetroArch and other emulators
* Per-game launch command override, so that different cores or emulators can be used on a per-game basis (saved to gamelist.xml)
* Core location can be defined relative to the emulator binary using the %EMUPATH% variable in es_systems.cfg (mostly useful for Windows)
* Properly implemented the option to show or hide hidden files and folders
* Properly implemented the option to show or hide games flagged as hidden in the metadata editor
* Custom event scripts can now be enabled or disabled with a menu option
* Help system updated and expanded to the complete application (previously it was only partially implemented)
* Improved input device configuration, and default keyboard mappings are now applied if the keyboard has not been configured by the user
* GUI-configurable option to sort favorite games above non-favorite games (favorites marked with stars)
* GUI-configurable option to sort folders on top of the gamelists
* Added a gamelist info text field displaying the game count, any applied filters as well as an icon if a folder has been entered
* Expanded the metadata for folders and made it possible to mark them as favorites
* Added new component GuiComplexTextEditPopup to handle changes to configuration file entries and similar
* Speed improvements and optimizations, the application now starts faster and feels more responsive
* Added metadata entry to mark games as broken/not working
* Added metadata entry to indicate whether the file should be counted as a game (for example useful to exclude setup files and similar for DOS games)
* Added metadata entry to hide the metadata values from the gamelist views (useful for general folders, DOS game configuration utilities etc.)
* Added a button to the metadata editor to delete the media files for a game or folder while retaining the game file and gamelist.xml entry
* Moved all resources to a subdirectory structure and enabled the CMake install prefix variable to generate the resources search path
* Changed theme directory to the install prefix (e.g. /usr/local/share/emulationstation/themes) with themes in the home directory taking precedence
* No more attempts to open files directly under /etc, instead only the install prefix directory and the home directory are used
* Added proper error handling for missing resource files and improved overall logging
* Refactoring, cleanup and documentation of the source code, removal of deprecated files etc.
* All required fonts bundled with the application, no dependencies on the OS to provide them any longer
* Made pugixml an external dependency instead of bundling it
* Decreased CPU usage dramatically by only rendering the currently visible view (previously all views were always rendered)
* Updated the CMake/CPack install and package build script to work as expected (it can now generate .deb, .rpm, .dmg and NSIS installation packages)
* Added support for Clang/LLVM, made the application build with no errors or warnings using this compiler (Unix and macOS only)
* License files included for all the libraries and resources that are bundled with the application
* Greatly expanded the application documentation (which is hosted with the source repository on GitLab)
* Updated the MAME ROM index files to include ROMs up to MAME version 0.221 and created scripts to easily generate these index files in the future

### Bug fixes

* Metadata editor insisted that changes were made although nothing was updated
  Note: The editor will still ask for save confirmations after automatically rounding fractional game ratings to half-star values, but any time such a rounding has taken place, the rating stars will be colored green in the metadata editor to nofity the user
* Game images were sometimes scaled incorrectly
* Non-transparent favorite icons were not rendered correctly
* Restart and power-off menu entries not working
* Unknown command line options were silently accepted instead of generating an error and notifying the user
* The scraper didn't handle error conditions correctly
* Fixed a massive memory leak related to SVG images
* Toggling the screensaver didn't work as expected
* The setting to enable or disable audio for the video screensaver only worked on Raspberry Pi
* The screensaver random function did not consider the previously selected game and could potentially show the same image or video over and over again
* The random system selection did not consider the currently selected system
* The random game selection did not consider the currently selected game
* The random game selection traversed folders, i.e. a game could be selected inside a subdirectory and vice versa
* Deleting a game from the metadata editor did not delete the game media files or the entry in the gamelist.xml file
* SystemView didn't properly loop the systems if only two systems were available
* When changing to the video view style from inside a gamelist, the view was not completely initialized
* Hidden files still showed up if they had a gamelist.xml entry
* Fixed an annoying gamelist issue that caused the game images and data to be updated and rendered up to six times every time the list was scrolled
* VRAM statistics overlay was somewhat broken and incorrectly displayed numbers in megabytes instead of mebibytes
* Not all input events were logged when running with debug logging activated
* Filters were not applied when leaving folders by using the back button
* Editing long text entries made the cursor jump outside the editing field
* Long game names would sometimes not scroll in the gamelist view
* On Unix, adding a hidden folder with a game in it crashed the application on startup
* If the user tried to enter a blank game name in the metadata editor, the application would crash upon saving
* The SliderComponent knob position was set incorrectly if the minimum value was not zero
* Lots and lots of additional small bugs and inconsistencies fixed

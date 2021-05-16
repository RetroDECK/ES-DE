# EmulationStation Desktop Edition (ES-DE) - Changelog

[[_TOC_]]

## Version 1.1.0 (in development)

**Release date:** TBD

### Release overview

### Detailed list of changes

* Added a fullscreen game media viewer
* Added a new video player based on FFmpeg
* Changed the button for jumping to a random system or game and added a setting for disabling the functionality altogether
* Moved the video screensaver audio setting to the sound settings menu
* Changed the setting description for the favorites game toggling button
* Changed the language standard from C++11 to C++14

### Bug fixes

* The "y" button help prompt wasn't displayed properly when using the Grid view style

## Version 1.0.1

**Release date:** 2021-05-01

### Release overview

v1.0 maintenance release.

### Detailed list of changes

* Added support for the new RetroArch v1.9.2 default core directory location on macOS

### Bug fixes

* Fixed high DPI display support on macOS

## Version 1.0.0

**Release date:** 2021-04-20

### Release overview

First release, a major update to the application compared to the RetroPie version on which it is based. This includes new gamelist sorting logic, new game media handling and an updated Windows port as well as a macOS port. The menu system has also been completely overhauled and the scraper has been expanded to support multiple media types as well as providing detailed scraping configuration options.

Full navigation sound support has been implemented, and the metadata editor has seen a lot of updates including color coding of all changes done by the user and by the scraper. Favorite games can now also be sorted on top of the gamelists and game collections.

OpenGL GLSL shader support has been added (not for the OpenGL ES renderer though) and there are multiple effects implemented such as scanlines for videos, blurred background when opening menus etc.

A new default theme rbsimple-DE (based on Recalbox Multi) is bundled with the application and is part of the installation package/installer. Theme sets created for the RetroPie EmulationStation fork will still work.

Many bugs have been fixed, and numerous features that were only partially implemented or broken have been updated to a fully working state. The application runs much faster as well due to lots of optimizations.

### Detailed list of changes

* Initial version, fork from RetroPie EmulationStation 2.10.0rp-dev (master)
* Added support for Windows, macOS, FreeBSD, NetBSD and OpenBSD
* New default theme rbsimple-DE bundled with the software (this theme is largely based on Recalbox Multi by the Recalbox community)
* Added extensive es_systems.cfg templates for Unix, macOS and Windows that are automatically installed on first application startup
* Added support for generating the ROM directory structure from within the application based on information in es_systems.cfg
* Added full navigation sound support, configurable per theme set with a fallback to the built-in sounds if there is no theme support
* Added multi-monitor support by giving the option to define on which display to run ES-DE
* Improved input device configuration and default keyboard mappings are now applied if the keyboard has not been configured by the user
* Reorganization and general overhaul of the menu system, hopefully making it more intuitive to navigate and easier to understand the menu entries
* New game media file logic using a media directory with files matching the ROM names instead of explicitly pointing to the media files from the gamelist.xml files
* GUI-configurable option to sort favorite games above non-favorite games
* GUI-configurable option to flag favorite games with star symbols
* GUI-configurable option to sort folders on top of the gamelists
* Added volume sliders for navigation sounds and game videos to the sound settings menu
* Added support for OpenGL GLSL shaders (OpenGL 2.1 renderer only, no support for OpenGL ES 1.0 renderer)
* Added multiple animations and shader effects, such as when opening menus, playing videos in the gamelists and via the screensaver etc.
* Updated the application to work properly on high resolution devices (such as 4K monitors)
* Seamless (almost) launch of games without showing the desktop when starting and returning from RetroArch and other emulators
* Updated scraper to support additional media files, detailed configuration of what to scrape, semi-automatic mode etc.
* Added user account support when scraping using ScreenScraper
* Added support for scraping game genres and game descriptions in multiple languages when using ScreenScraper
* Files or folders can now be flagged for exclusion when scraping with the multi-scraper, and for folders it can be set to apply recursively
* Overhaul of the game collection functionality including many bug fixes and optimizations
* Added ability to delete custom collections from the GUI menu
* Help system updated and expanded to the complete application (previously it was only partially implemented)
* Game systems are now sorted by full names which makes much more sense from a user perspective
* In the metadata editor, any values updated by the single-game scraper or by the user are now highlighted using different font colors
* Expanded the metadata for folders and made it possible to mark them as favorites
* Added metadata entry to mark games as broken/not working (e.g. useful for MAME games)
* Added metadata entry to indicate whether the file should be counted as a game (e.g. useful to exclude setup files and similar for DOS games)
* Added metadata entry to hide the metadata values from the gamelist views (useful for general folders, DOS game configuration utilities etc.)
* Added a 'clear' button to the metadata editor to delete the media files and gamelist.xml entry for a game or folder while still retaining the game file
* Added a system view counter for favorite games in addition to the total number of games
* Added a gamelist info text field displaying the game count, any applied filters as well as an icon if a folder has been entered (requires theme support)
* Properly implemented the option to show or hide hidden files and folders
* Properly implemented the option to show or hide games flagged as hidden in the metadata editor
* Added support for converting two-byte Unicode characters to uppercase and lowercase
* Added the ability to display pillarboxing and letterboxing for videos with non-standard aspect ratios
* Custom event scripts can now be enabled or disabled with a menu option
* Gamelist sorting is now working as expected and is persistent throughout the application session
* Expanded the gamelist filter functionality to include completed and broken games and added the ability to filter game names via a free text entry
* Added functionality to remember cursor positions inside folders and grouped custom collections
* Per-game launch command override, so that different cores or emulators can be used on a per-game basis (saved to gamelist.xml)
* The emulator core location can now be defined relative to the emulator binary using the %EMUPATH% variable in es_systems.cfg (used extensively on macOS and Windows)
* Core locations can be searched from a configurable list of directories if defined in the es_systems.cfg file using the %COREPATH% variable (mostly useful on Unix where there are no standardized core directories)
* Clear notifications and logging have been added for missing emulator binaries and cores when attempting to launch games
* Overhaul of the screensaver (the game info overlay now works correctly for instance)
* Added support for jumping to the start and end of gamelists and menus using the controller trigger buttons (or equivalent keyboard mappings)
* Many additional quality of life improvements and removal of GUI inconsistencies
* Replaced the main application font with Fontfabric Akrobat
* Replaced the on and off button icons with new graphics
* Replaced the checked checkmark icon with new graphics
* Changed the application icons and splash screen color theme from blue to red
* Improved the menu interface on 4:3 aspect ratio displays
* Made ScrollableContainer (used for the gamelist game descriptions) fade in as the text position is reset
* Made the ScrollableContainer scroll speed adaptive depending on the font size and width of the text container
* Moved all resources to a subdirectory structure and enabled the CMake install prefix variable to generate the resources search path
* Changed the theme set directory to the install prefix (e.g. /usr/share/emulationstation/themes) with themes in the home directory taking precedence
* No more attempts to open files directly under /etc, instead only the install prefix directory, the ES-DE executable directory and the home directory are used
* Added proper error handling for missing resource files and improved overall logging
* Refactoring, cleanup and documentation of the source code, removal of deprecated files etc.
* Speed improvements and optimizations, the application now starts faster and feels more responsive
* Added new component GuiComplexTextEditPopup to handle changes to configuration file entries and similar
* Added full UTF-16 (Unicode) support on Windows
* Removed the PowerSaver
* Game counting is now done during sorting instead of every time a system is selected. This should make the UI more responsive in case of large game libraries
* All required fonts bundled with the application, no dependencies on the OS to provide them any longer
* Made pugixml an external dependency instead of bundling it
* Replaced the custom math functions with standard C++ functions whenever possible
* Implemented proper random functions using Mersenne Twister pseudorandom number generators (it actually makes a practical difference)
* Modernized the audio code, for example using SDL_AudioStream instead of the older SDL_AudioCVT
* Overhaul of application settings, now the configuration file is only updated when there have been actual configuration changes
* Decreased CPU usage dramatically by only rendering the currently visible view (previously all views were always rendered)
* Updated the CMake/CPack install and package configuration files to work as expected (can now generate DEB, RPM, DMG and NSIS installation packages with correct dependencies)
* Added support for Clang/LLVM, made the application build with no errors or warnings using this compiler (Unix and macOS only)
* Added support for both MSVC and MinGW (GCC) on Windows
* License files are now included for all the libraries and resources that are bundled with the application
* Updated the MAME ROM index files to include ROMs up to MAME version 0.230 and created scripts to easily generate these index files in the future
* Greatly expanded the application documentation (which is hosted in the ES-DE repository on GitLab)

### Bug fixes

* On Unix, adding a hidden folder with a game in it crashed the application on startup
* If the user tried to enter a blank game name in the metadata editor, the application would crash upon saving
* Switching to the Grid view style with a placeholder shown in the gamelist crashed the application
* FileSystemUtil::getDirContent crashed when searching through directories recursively
* Large text sizes at higher resolutions (such as 4K) would crash the application as fixed-size texture buffers were used which weren't big enough to hold the larger font textures
* Fixed a massive memory leak related to SVG images
* Fixed an issue where SVG images would sometimes be cut off slightly on the right side (e.g. logos on the system view carousel)
* The audio volume control did not detect if there was a new default audio device or if the audio volume had been changed outside ES-DE
* The scraper didn't handle error conditions correctly
* The metadata editor insisted that changes had been made although nothing was updated
* Sorting by number of players did not work properly for games with ranges such as 1-2 or 1-8
* Restart and power-off menu entries not working on any of the tested operating systems
* Toggling the screensaver didn't work as expected
* The setting to enable or disable audio for the video screensaver only worked on Raspberry Pi
* The screensaver random function did not consider the previously selected game and could potentially show the same image or video over and over again
* The random system selection did not consider the currently selected system
* The random game selection did not consider the currently selected game
* The random game selection traversed folders, i.e. a game could be selected inside a subdirectory and vice versa
* The controller D-PAD could not be used for entering the UI mode change passkey
* Filters were not applied when leaving folders using the back button
* The cursor stack logic was not completely implemented for the Grid view style, making it largely broken
* Editing long text entries made the cursor jump outside the editing field
* Long words would sometimes render partly outside the designated text area instead of being abbreviated
* Fixed an annoying gamelist issue that caused the game images and data to be updated and rendered up to six times every time the list was scrolled
* Not all input events were logged when running with debug logging activated
* Unknown command line options were silently accepted instead of halting the application startup and logging an error
* Added a sanity check to the --resolution flag to keep the resolution within reason (and to avoid crashes when making a typo for this parameter)
* Deleting a game from the metadata editor did not delete the game media files or its entry in the gamelist.xml file
* Hidden files still showed up if they had a gamelist.xml entry
* Fixed multiple instances of misaligned GUI elements on high-resolution displays due to the use of fixed-pixel constants
* Fixed a rounding issue which caused single-pixel lines to sometimes be shown along the upper and left screen edges
* The VRAM statistics overlay was somewhat broken and incorrectly displayed numbers in megabytes instead of mebibytes
* Long game names would sometimes not scroll in the gamelist view
* Game media was not rendered when moving between gamelists using the slide transition style
* Wrapping around the first and last game systems generated strange camera movements when using the slide transition style
* Some bundled graphics (resource files) displayed excessive texture pop-in under some circumstances
* SystemView didn't properly loop the systems if only two systems were available
* When changing to the video view style from inside a gamelist, the view was not completely initialized
* Game images were sometimes scaled incorrectly
* The rating component would sometimes not render immediately if using SVG graphics
* Non-transparent favorite icons were not rendered correctly
* The SliderComponent knob position was set incorrectly if the minimum value was not zero
* The debug overlays didn't work for all image and text components
* Lots and lots of additional small bugs and inconsistencies fixed

## Known issues

**The issues below are relevant for ES-DE v1.0.1**

* The input configuration can be a bit glitchy on some devices, most notably the setup of trigger buttons for Xbox and PlayStation controllers. Once configured everything should work fine though. This configuration issue will hopefully be resolved in ES-DE v1.1 with the move to the SDL2 GameController API.

* Some screen tearing can be seen in the upper part of the screen when using the slide transitions with certain graphics drivers and resolutions. The issue is apparently more prevalent when running ES-DE at a lower resolution on 4K displays by using the --resolution command line option (which is only available on Unix). This problem will hopefully be resolved in ES-DE v1.2 when moving to the GLM library.

* The launching of games can freeze ES-DE on some Windows installations. It probably only occurs on Windows 8.1 but that's not confirmed. The setting 'Run in background (while game is launched)' can be enabled to get around this problem, but this causes some other issues so it should only be used as a last resort. It's unclear if this problem can or will be resolved. If it's confirmed to only affect older Windows versions, then it's probably not worthwhile fixing it.

* On Windows when using high DPI displays, if not running ES-DE on the primary monitor and the display where it runs does not have the same scaling percentage as the primary monitor, then the ES-DE resolution will not be properly set. The application will still work and if running in fullscreen mode it may not even be noticeable. This issue is caused by a bug in SDL where the primary display scaling is always used for calculating the display bounds and as such it needs to be fixed in that library. If using the same scaling percentage across all monitors, or if not using high DPI monitors at all, then this issue will not occur.

* Scraping using ScreenScraper could lead to the scraper halting if exceeding the allowed requests per minute. This will cause a popup window to be displayed in ES-DE with the option available to retry the scraping for the current game (you should first wait a minute or so though or the error will immediately reoccur). This is not really a fault in the application per-se but rather a restriction of ScreenScraper. A request per minute limiter is planned for ES-DE v1.4, which should help with avoiding this problem.
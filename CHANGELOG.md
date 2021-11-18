# EmulationStation Desktop Edition (ES-DE) - Changelog

[[_TOC_]]

## Version 1.2.0 (in development)

**Release date:** TBD

### Release overview

The 1.2 release introduces multiple new features and brings extensive bug fixing and lots of other small improvements. Support for alternative emulators has been added which can be selected system-wide or per game. These alternative emulators are added to the es_systems.xml file, making it easy to expand or customize the configuration. For this release, most of the available RetroArch cores have been preconfigured.

A virtual keyboard has been added (some code borrowed from Batocera.linux) which is fully integrated and can be used to input text via a game controller. By introducing this feature, a keyboard should now be completely optional for day-to-day use.

Another new feature is support for badges that display icons in the gamelist view indicating favorite games, completed games, game-specific controllers etc. Note that these badges require support from the theme set. And on the topic of theme sets, a new theme named modern-DE has been included with the installation as an alternative to the default rbsimple-DE theme.

The scraper has been improved and expanded, and in addition to the previously supported media it can now scrape box back covers, title screens and physical media images (cartridges, diskettes, tapes, CD-ROMs etc.). These physical media images are also included in the generated miximages, although that can be disabled using a menu option.

As for supported platforms, v1.2 brings official support for the Raspberry Pi 4/400, both for the 32-bit (armv7l) and 64-bit (aarch64) versions of Raspberry Pi OS.

Apart from all the above, a huge amount of work has gone into fixing bugs, refactoring the code and optimizing for performance. The language standard has been increased from C++14 to C++17 and the built-in vector and matrix data types and functions have been replaced with the GLM (OpenGL Mathematics) library equivalents.

### Detailed list of changes

* Added alternative emulators support where additional emulators can be defined in es_systems.xml and be selected system-wide or per game via the user interface
* Populated the bundled es_systems.xml files with alternative emulator entries for most RetroArch cores
* Added a virtual keyboard, partly based on code from batocera-emulationstation
* Added badges that indicate favorite/completed/broken games as well as games suitable for children and those with a selected alternative emulator
* Added game-specific controllers that are selectable via the metadata editor and displayed as a controller badge
* Added scraping of title screens, box back covers and physical media images
* Updated the media viewer to display title screens and box back cover images
* Added physical media images to the generated miximages
* Added an option to rotate horizontally oriented game boxes when generating miximages
* Added size options (small/medium/large) for the boxes/covers and physical media images when generating miximages
* Added support for the Raspberry Pi 4 (Raspberry Pi OS 32-bit/armv7l and 64-bit/aarch64)
* Bundled the new alternative theme "modern-DE" which supports all the latest features from this release
* Added the ability to make complementary game system customizations without having to replace the entire bundled es_systems.xml file
* Added support for an optional \<systemsortname\> tag for es_systems.xml that can be used to override the default \<fullname\> systems sorting
* Added menu scroll indicators showing if there are additional entries available below or above what's currently shown on screen
* Added scraping of controller metadata (only for ScreenScraper and only for arcade systems)
* Improved the layout of the scraper GUIs (single-game scraper and multi-scraper)
* Added horizontal scrolling of long game names to the scraper GUIs
* Removed the "Scrape" text prefix from the scraper content settings
* Added proper frame drop functionality to the FFmpeg video player to greatly reduce stuttering on slower machines
* Made multiple optimizations to the FFmpeg video player to reduce CPU usage and to increase framerates on slower machines
* Disabled the FFmpeg video player hardware decoding option (it can still be built using a CMake flag)
* Removed the copying of es_settings.cfg to es_settings.xml as it caused issues when migrating from other EmulationStation forks
* Improved the gamelist filter GUI to not allow filtering of values where there is no actual data to filter, e.g. Favorites for a system with no favorite games
* Grayed out all fields in the gamelist filter GUI where there is no data to filter, previously some fields were removed entirely and some could still be used
* Added filters for "Controller" and "Alternative emulator" and sorted the filters in the same order as the metadata editor fields
* Added the ability to filter on blank/unknown values for Genre, Player, Developer, Publisher, Controller and Alternative emulator
* Added a menu option to change the application exit key combination
* Added an option to preload the gamelists on startup which leads to smoother navigation when first entering each gamelist
* Lowered the minimum supported screen resolution from 640x480 to 224x224 to support arcade cabinet displays such as those running at 384x224 and 224x384
* Added support for the Commodore VIC-20, Epic Games Store, Google Android, Java 2 Micro Edition, Philips CD-i and Symbian systems
* Added support for a more advanced system view carousel logo placeholder (for unthemed systems) by allowing the combination of text and graphics
* Expanded the themeable options for "helpsystem" to support custom button graphics, dimmed text and dimmed icon colors, upper/lower/camel case and custom spacing
* Made the scrolling speed of ScrollableContainer more consistent across various screen resolutions and display aspect ratios
* Decreased the amount of text that ScrollableContainer renders above and below the starting position as content is scrolled
* Made the game name and description stop scrolling when running the media viewer, the screensaver or when running in the background while a game is launched
* Added notification popups when plugging in or removing controllers
* Made large optimizations to the SVG rendering which reduces application startup time dramatically when many systems are populated
* Changed to loading the default theme set rbsimple-DE instead of the first available theme if the currently configured theme set is missing
* Added support for displaying the left and right trigger buttons in the help prompts
* Removed the "Choose" entry from the help prompts in the gamelist view
* Replaced a number of help prompt hacks with proper solutions
* Changed the "Toggle screensaver" help entry in the system view to simply "Screensaver"
* Changed the font size for the custom collection deletion screen to use the same size as all other menus
* Added support for upscaling bitmap images using linear filtering
* Changed the marquee image upscale filtering from nearest neighbor to linear for the launch screen and the gamelist views
* Moved the Media viewer and Screensaver settings higher in the UI settings menu
* Moved the game media directory setting to the top of the Other settings menu, following the new Alternative emulators entry
* Lowered the default volumes slightly for videos and navigation sounds
* Added loading of the System view to the ViewController preload function to decrease theme extras texture pop-in
* Changed the filter description "Text filter (game name)" to simply "Game name"
* Removed a margin hack from TextComponent
* If abbreviated strings end with a space character, that space is now removed (TextComponent)
* Added support for multi-select total count and exclusive multi-select to OptionListComponent
* Added support for a maximum name length to OptionListComponent (non-multiselect only) with an abbreviation of the name if it exceeds this value
* Added support for key repeat to OptionListComponent, making it possible to cycle through the options by holding the left and right buttons
* Added key repeat for the "Jump to" and "Sort games by" selectors on the game options menu
* Added key repeat when editing the "Release date" entry in the metadata editor (DateTimeEditComponent)
* Added support for setting the Kidgame metadata flag for folders (which will only affect the badges)
* Added a blinking cursor to TextEditComponent
* Achieved a massive speed improvement for OptionListComponent by not resizing each added MenuComponent row (most notable in the filter GUI)
* Made multiple optimizations to the GUI components by removing lots of unnecessary function calls for sizing, placement, opacity changes etc.
* Simplified the logic for info popups and prepared the code for the future "multiple popups" feature
* Added support for a new type of "flat style" button to ButtonComponent
* Added support for correctly navigating arbitrarily sized ComponentGrid entries, i.e. those spanning multiple cells
* Bundled the bold font version of Fontfabric Akrobat
* Moved the resources/help directory to resources/graphics/help
* Removed the unused graphics files resources/graphics/fav_add.svg and resources/graphics/fav_remove.svg
* Added RapidJSON as a Git subtree
* Added the GLM (OpenGL Mathematics) library as a Git subtree
* Replaced all built-in matrix and vector data types and functions with GLM library equivalents
* Replaced some additional math functions and moved the remaining built-in functions to a math utility namespace
* Added a function to generate MD5 hashes
* Improved thread safety at multiple places throughout the codebase
* Made an optimization for SVG graphics to avoid a lot of unnecessary re-rasterizations
* Lots of other general code refactoring
* Increased the warning level for Clang/LLVM and GCC by adding -Wall, -Wpedantic and some additional flags
* Fixed a lot of compiler warnings introduced by the -Wall and -Wpedantic flags
* Changed the language standard from C++14 to C++17
* Increased the minimal required compiler version to 5.0.0 for Clang/LLVM and 7.1 for GCC
* Added CMake options to build with AddressSanitizer, ThreadSanitizer and UndefinedBehaviorSanitizer
* Changed two clang-format rules related to braced lists and reformatted the codebase
* Bundled the October 2021 release of the Mozilla TLS/SSL certificates
* Updated the MAME index files to include ROMs up to MAME version 0.237
* rbsimple-DE: Added some missing graphics for the xbox360 and residualvm systems
* rbsimple-DE: Improved existing graphics for the dos, pc, residualvm and scummvm systems

### Bug fixes

* Setting a really small font size in a theme would crash the application
* Setting an invalid UIMode value in the configuration file could crash the application
* Setting an invalid scraper service value in the configuration file could crash the application
* When scraping in interactive mode with "Auto-accept single game matches" enabled, the game name could not be refined if there were no games found
* When scraping in interactive mode, the game counter was not decreased when skipping games, making it impossible to skip the final games in the queue
* When scraping in interactive mode, "No games found" results could be accepted using the "A" button
* When scraping in interactive mode, any refining done using the "Y" button shortcut would not be shown when doing another refine using the "Refine search" button
* The multi-scraper did not update the filter index
* Fixed multiple minor rendering issues where graphics would be slightly cut off or incorrectly resized
* Under some circumstances ScrollableContainer (used for the game descriptions) would contain a partially rendered bottom line
* If the TextListComponent height was not evenly dividable by the font height + line spacing, a partial bottom row would get rendered
* The line spacing for TextListComponent was incorrectly calculated for some resolutions such as 2560x1440
* Fixed multiple issues with scaling of images which led to various inconsistencies and sometimes cut-off graphics
* Removing games from custom collections did not remove their filter index entries
* Input consisting of only whitespace characters would get accepted by TextEditComponent which led to various strange behaviors
* Leading and trailing whitespace characters would not get trimmed from the collection name when creating a new custom collection
* Leading and trailing whitespace characters would get included in scraper search refines and TheGamesDB searches
* Leading and trailing whitespace characters would get included in game name filters
* Fixed multiple data races throughout the codebase caused by insufficient thread safety
* Game name (text) filters were matching the system names for collection systems if the "Show system names in collections" setting was enabled
* Brackets such as () and [] were filtered from game names in collection systems if the "Show system names in collections" setting was enabled
* If a theme used the forceUppercase property for a TextListComponent, this value was always set to true even if the theme defined it as false
* Fixed multiple issues where ComponentGrid would display incorrect help prompts
* Help prompts were missing for the "Rating" and "Release date" fields in the metadata editor
* There was some strange behavior in DateTimeEditComponent when changing the date all the way down to 1970-01-01
* When navigating menus, the separator lines and menu components did not align properly and moved up and down slightly
* Under some circumstances and at some screen resolutions, the last menu separator line would not get rendered (still an issue at extreme resolutions like 320x240)
* When scrolling in menus, pressing other buttons than "Up" or "Down" did not stop the scrolling which caused all sorts of weird behavior
* With the menu scale-up effect enabled and entering a submenu before the parent menu was completely scaled up, the parent would get stuck at a semi-scaled size
* The custom collection deletion screen had incorrect row heights when running at lower resolutions such as 1280x720
* If there was an abbreviated full system name for the "Gamelist on startup" option, that abbreviation would also get displayed when opening the selector window
* Really long theme set names would not get abbreviated in the UI settings menu, leading to a garbled "Theme set" setting row
* Disabling a collection while its gamelist was displayed would lead to a slide transition from a black screen if a gamelist on startup had been set
* When marking a game to not be counted in the metadata editor and the game was part of a custom collection, no collection disabling notification was displayed
* When running really low on texture memory, the menu texture would not get rendered correctly
* At low screen resolutions, logos on the System view carousel would sometimes jump down a pixel when scaling down
* There was a tiny and randomly occuring gap between the system carousel and systemInfo bar during slide transitions between the System and Gamelist views
* SliderComponent had very inconsistent widths at different screen aspect ratios
* SliderComponent did not properly align the knob and bar vertically
* Buttons were not sized and padded consistently across different screen resolutions
* OptionListComponent arrows were not padded consistently across different screen resolutions
* Resizing in SwitchComponent did not reposition the image properly leading to a non-centered image
* Horizontal sizing of the TextComponent input field was not consistent across different screen resolutions
* The sizing of the metadata editor was strange, which was clearly visible when activating the Ctrl+G debug mode
* The "sortname" window header was incorrectly spelled when editing this type of entry in the metadata editor
* When the last row of a menu had its text color changed, this color was completely desaturated when navigating to a button below the list

## Version 1.1.0

**Release date:** 2021-08-10

### Release overview

The 1.1 release brings many large changes including a fullscreen media viewer, a game launch screen, a miximage generator, a new video player and a new controller API featuring automatic controller configuration and controller profiles.

A much better mechanism to find emulators and emulator cores has been implemented as well, which among other things removes the need to manually modify the Path variable on Windows to find RetroArch. It also eliminates the requirement for a separate Flatpak-specific es_systems.xml file on Linux.

There are also several changes under the hood, such as the addition of the CImg image processing library, automatic code formatting of the entire codebase using clang-format, change of language standard from C++11 to C++14 and lots of general code refactoring.

Apart from this, numerous small improvements and bug fixes are part of the release, as detailed below.

### Detailed list of changes

* Added a miximage generator that can be run automatically from the scraper and which includes comprehensive options, configurable from the menu
* Added an offline generator GUI for the miximage generator which can be used for bulk miximage generation without going via the scraper
* Added a fullscreen game media viewer
* Added a game launch screen that displays the marquee image, the game name and the system name
* Added a new video player based on FFmpeg
* Added a 60 FPS frame rate upscaler option to the video player which results in slightly smoother playback for low frame rate videos (e.g. 24 and 30 FPS)
* Implemented a new mechanism for locating emulators and cores, with configurable find rules (this eliminates some hacks such as the separate Flatpak es_systems.cfg file)
* Added a Windows-specific find rule that searches the Registry for the App Paths keys, which eliminates the need to modify the Path manually to find RetroArch
* Removed the deprecated %COREPATH% setting and corresponding menu entry
* The "Run in background (while game is launched)" option can now be enabled on all operating systems instead of only on Windows
* Added a workaround for a game launch issue on Windows when using AMD and Intel GPUs
* Moved to the SDL GameController API which gives numerous improvements to the controller handling
* Default controller configuration is now automatically applied, input configuration should rarely if ever be required any longer except for deliberate button customization
* Added support for selecting the controller type (Xbox, Xbox 360, PS4, PS5 and SNES), which changes the help icons, help text and the input configuration tool icons and text
* Added an option to limit the input in ES-DE to only the first controller (this does not affect the emulators)
* Switched the order of the "Back" and "Start" buttons (or equivalents) in the input configurator to align with the other button entries which go from left to right
* Added separate controller deadzone values for the triggers and thumbsticks
* Removed the startup notification regarding default keyboard mappings being in use, instead default mappings are now considered the recommended input configuration
* The controller input configuration is not automatically started any longer if there is no es_input.cfg file or if there are no applicable configuration entries in the file
* Increased the max allowed size for images when scraping, which should now only downscale files which really need it
* Changed the resampling algorithm for image downscaling for the scraper from bilinear to Lanczos which results in noticeably sharper images
* Added a configurable option to automatically retry scraping up to eight times in case of ScreenScraper TLS errors
* Changed the button for jumping to a random system or game and added a setting for enabling or disabling the functionality altogether
* The help text for the "A" button now shows "Enter" instead of "Launch" in the grouped custom collections view
* Added navigation sounds for some actions where it was missing, such as when attempting to add folders, placeholders or systems to custom collections
* Changed the custom collection "Jump to" navigation sound to the select sound instead of the scroll sound
* A notification is now displayed in the grouped custom collections view if a filter is applied to the selected collection
* Changed the default screensaver type from "dim" to "video" and made the fallback screensaver "dim" instead of "black"
* Moved the video screensaver audio setting to the sound settings menu
* Added support for the Nintendo Switch game system (using the Yuzu emulator)
* Added an option to use plain ASCII characters for the favorite, folder and tickmark symbols, which makes some themes with very pixelated fonts look coherent
* Created a new main menu entry for input device settings
* Moved the input device configuration tool to the input device settings menu
* Adjusted the size and position of the various menus to accomodate one additional entry on the screen
* The quit menu is now disabled by default, instead showing the "Quit EmulationStation" entry unless configured otherwise
* Removed the "Display game media from ROM directories" setting as it doesn't make sense to support this legacy functionality any longer
* Added support for using the %ESPATH% and %ROMPATH% variables in the slideshow screensaver custom image directory setting
* Improved scaling relative to the screen aspect ratio for various GUI components which enhances the layout on 4:3 displays and ultrawide monitors
* Removed the menu fade-in effect as it looked terrible
* Enabled the menu scale-up effect for the OpenGL ES renderer
* Renamed es_systems.cfg, es_settings.cfg and es_input.cfg to es_systems.xml, es_settings.xml and es_input.xml
* Changed the es_systems.xml logic so it loads from the program resources directory by default (a customized file can be placed in ~/.emulationstation/custom_systems)
* Added a %HIDEWINDOW% variable which can be used in the es_systems.xml file on Windows, primarily intended for hiding console windows when launching scripts
* Added support for using the %ESPATH% variable in the media directory setting
* Removed the marquee image from rbsimple-DE as it's now baked into the miximages
* Set the gamelist video scanline rendering option to disabled by default
* Changed the setting description for the favorites game toggling button
* Simplified and improved the setup of portable installations on Windows
* Converted all navigation sound files to stereo as previously it was a mix of mono and stereo files (done for rbsimple-DE and the fallback sounds)
* The themes and scripts directories are now automatically created during startup
* Cleaned up some application startup messages
* The application version is now saved to es_settings.xml, which can be used in the future to notify the user after upgrades to a newer release
* Added a DebugSkipInputLogging option which is intended primarily for development and needs to be manually set in es_settings.xml
* Added the CImg library as a Git subtree and created some utility functions for it (used by the miximage generator and the game launch screen)
* Added a function to ImageComponent to crop fully transparent areas around an image
* Added and clarified startup log warnings for missing or invalid es_systems.xml platform tags
* Added a CMake option to control whether the VLC video player should be built, and set this to off by default
* Made it possible to build on the Raspberry Pi 4 (tested on Raspberry Pi OS)
* Removed the deprecated VideoOmxComponent
* Removed the pointless APPLE_SKIP_INSTALL_LIBS CMake option
* Added a clang-format style configuration file to use for automatic code formatting
* Formatted the entire codebase using clang-format
* Integrated clang-tidy with CMake and made it possible to enable it via a flag
* Added the NanoSVG library as a proper Git subtree
* Changed the language standard from C++11 to C++14

### Bug fixes

* Marking all games as favorites for a system or folder or removing all favorite markings would sometimes crash the application
* Scraping new game media using the single-game scraper followed by a re-scrape that was aborted could crash the application
* The scraper search could be refined or skipped after the result was accepted which sometimes crashed the application
* Attempting to load a non-existent font file defined by the theme crashed the application instead of using the bundled font as fallback
* Refining a search before it was completed and then cancelling the dialog would lead to an empty scraper screen
* Game media would sometimes not get displayed after single-game scraping
* Games that were filtered out were included in the random game selection for the grouped custom collections view
* After switching theme sets with only a single system available, diagonal slide transitions would sometimes play when moving to the system view
* Ongoing slide transition animations would continue to play after switching theme sets
* When using the Video view style, the static image would not get rendered during the first Slide transition when coming from the System view
* Long game names that were horizontally scrolling in the gamelist view would sometimes flicker when returning to the start position
* On Windows, images with Unicode characters in the game name that were resized when scraping would not get saved with valid filenames
* The glitches when configuring trigger buttons in GuiInputConfig have been fixed
* GuiDetectDevice wouldn't detect controller input that was of the "axis" type (i.e. analog inputs)
* GuiInputConfig didn't correctly inform which buttons could be skipped for some rows
* The scraper would sometimes consider very small images to be invalid
* Scraper searches for Nintendo Family Computer (Famicom) games were not accurate
* The Quick System Select help prompt was shown even when there was only a single game system present
* The "Back (cancel)" help prompt was missing for the single-game scraper
* The "Y" button help prompt wasn't displayed correctly when using the Grid view style
* Fractional game rating values would always get rounded up
* Encountering a corrupt image file would lead to a continuous loop of attempts to load the image while filling the log file with error messages
* Cropping in ImageComponent didn't work correctly
* The debug logging for the analog controller inputs had some inconsistent signs

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
* Added a "Clear" button to the metadata editor to delete the media files and gamelist.xml entry for a game or folder while still retaining the game file
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
* Updated the MAME index files to include ROMs up to MAME version 0.230 and created scripts to easily generate these index files in the future
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

**The issues below are relevant for ES-DE v1.1.0**

* There is an issue with launching games on Windows when using AMD and Intel GPUs. This causes the emulator to just output a blank screen. There is a workaround available for this which is enabled by default and that can be disabled via the menu option "AMD and Intel GPU game launch workaround". The drawback of this workaround is that a white instead of a black screen will be displayed when launching games. If using an Nvidia GPU, it should be safe to disable this option for a slightly better user experience. An alternative workaround is to enable the option "Run in background (while game is launched)".

* On Windows when using high DPI displays, if not running ES-DE on the primary monitor and the display where it runs does not have the same scaling percentage as the primary monitor, then the ES-DE resolution will not be properly set. The application will still work and if running in fullscreen mode it may not even be noticeable. This issue is caused by a bug in SDL where the primary display scaling is always used for calculating the display bounds and as such it needs to be fixed in that library. If using the same scaling percentage across all monitors, or if not using high DPI monitors at all, then this issue will not occur.

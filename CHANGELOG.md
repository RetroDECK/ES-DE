# EmulationStation Desktop Edition (ES-DE) - Changelog

[[_TOC_]]

## Version 2.0.1 (in development)

**Release date:** TBD

### Release overview

v2.0 maintenance release.

### Detailed list of changes

### Bug fixes

* Fixed several potential container overflows in GuiTextEditKeyboardPopup that could lead to rare crashes when running on an ARM processor
* Fixed a potential container overflow in ViewController that could lead to rare crashes when running on an ARM processor
* (macOS) When opening the main menu an error message was logged about accessing a nonexistent ShowQuitMenu setting

## Version 2.0.0

**Release date:** 2023-03-11

### Release overview

The 2.0 release introduces multiple fundamental changes to the application, most notably a new theme engine which brings various new concepts. The traditional view styles (basic/detailed/video/grid) have been replaced with only system and gamelist views in addition to _variants_, which are essentially theme profiles. These are much more flexible than view styles and can be freely defined by the theme author. The new engine is also fully generalized and almost all theme elements can be used anywhere and in unlimited numbers. Many of these elements and properties are new additions as well.

Theme variants, color schemes, aspect ratios and transitions can now be defined by the theme author and are user-selectable from within the menu system. In addition to all this new theme functionality the application is still backward compatible with legacy theme sets.

The renderer has been modernized and rewritten and while still using OpenGL it's now fully shader-based instead of using a fixed function pipeline as was the case in previous releases. Tate mode (vertical screen orientation) support has been added including the ability to rotate screen contents within the application window. Rendering performance has been greatly improved in some areas such as post processing (used for video player scanline and blur shaders among other things). A new SVG rendering library named LunaSVG has been introduced which offers much better file compatibility than before, while doing it with better performance.

Overall application speed and performance has been greatly improved with faster startup times, less latency and lower CPU utilization. Many bugs have been fixed and multiple quality of life improvements have been made. Support for a lot more systems and standalone emulators has also been included with this release, see below for more details.

### Detailed list of changes

* Made fundamental changes to the application logic by removing most view styles and replacing them with a new theme variants concept
* Added theme support for defining and applying different layouts for various display aspect ratios such as 16:9 and 4:3
* Added theme support for defining and applying different color schemes
* Added theme support for defining and applying granular transition animation profiles
* Added a new grid component that is usable in both the system and gamelist views
* Made gamelist theming much more flexible by allowing any number of elements of any types to be defined
* Deprecated multiple older theming concepts like features, extras and hardcoded metadata attributes
* Renamed the default theme set from rbsimple-DE to Slate (slate-es-de)
* Ported modern-DE to the new theme engine and renamed it to Modern (modern-es-de)
* Added variant trigger (override) support to replicate the legacy theme engine automatic view style switching
* Events are now parsed during startup which avoids issues with the OS thinking the application has hung when loading
* Added an application updater that checks for new releases on startup (currently only provides notifications)
* Replaced the NanoSVG library with LunaSVG for greatly improved SVG rendering capabilities
* Added support for caching of SVG images
* Added support for sizing SVG images arbitrarily (overriding the image aspect ratio by stretching and squashing)
* Added support for 2x and 4x MSAA anti-aliasing (OpenGL renderer only)
* (Windows) Made game launching more seamless by making the application window one pixel wider instead of one pixel less in height
* gamelist.xml files are no longer loaded from the ROMs/system directories (although old behavior can be retained via an es_settings.xml option)
* Added support for always grouping custom collections under the custom collections system regardless of theme set support
* Added support for mixed-case custom collection names
* Added a startup progress bar to the splash screen
* Expanded the quick system select menu option from an on/off entry to a selection of different button combinations
* Expanded the random system or game button from an on/off entry to a selection between "Games only", "Games and systems" or "Disabled"
* Changed the order of the help system entries Y, X, B and A to instead be listed as A, B, X and Y
* Changed the start button for the screensaver from "Back" to "X"
* Changed the help system description for the "A" button in the gamelist view from "Launch" to "Select"
* Changed the menu header for the gamelist options menu from "Options" to "Gamelist options"
* Added support for the Emerson Arcadia 2001 (arcadia) game system
* Added support for the Arduboy Miniature Game System (arduboy)
* Added support for the Acorn Computers BBC Micro (bbcmicro) game system by adding the MAME standalone emulator
* Added support for the Capcom Play System I, II and III (cps1, cps2 and cps3) game systems
* Added support for the VTech CreatiVision (crvision) game system
* Added support for the Tiger Electronics Game.com (gamecom) game system
* Added support for the LCD Handheld Games (lcdgames) game system
* Added support for the Sega Mega Drive (megadrivejp) game system (Japanese region)
* Added support for the Casio PV-1000 (pv1000) game system
* Added support for the Quake (quake) game system
* Added support for the VTech V.Smile (vsmile) game system
* Added support for the WASM-4 Fantasy Console (wasm4) game system
* Added support for the Microsoft Windows (windows) game system
* Added support for the Nintendo Wii U (wiiu) game system on Linux and macOS by adding the Cemu standalone emulator
* Added support for the Infocom Z-machine (zmachine) game system by adding the Gargoyle standalone emulator
* Added support for the Ava release of Ryujinx for the switch system on Linux and Windows
* (macOS) Added support for the Nintendo Switch (switch) game system by adding the Ryujinx standalone emulator
* Added ares standalone as an alternative emulator for many systems
* Added MAME standalone as the default emulator for the macintosh system (Mac SE and Mac Plus diskette images)
* Added MAME standalone as the default emulator for the gameandwatch system
* Added Stella standalone as an alternative emulator for the atari2600 system
* Added Gopher2600 standalone as an alternative emulator for the atari2600 system on Unix and Windows
* Added PrBoom+ standalone as an alternative emulator for the doom system
* Added openMSX standalone as an alternative emulator for the colecovision, msx, msx1, msx2 and msxturbor systems
* Added SameBoy standalone as an alternative emulator for the gb and gbc systems
* Added Gearboy standalone as an alternative emulator for the gb and gbc systems on Unix and Windows
* Added puNES standalone as an alternative emulator for the famicom and nes systems on Unix and Windows
* Added Hatari standalone as an alternative emulator for the atarist system
* Added Fuse standalone as an alternative emulator for the zxspectrum system
* (Linux) Added support for the Sega Model 3 (model3) game system by adding the Supermodel standalone emulator
* (Linux) Added Supermodel standalone as an alternative emulator for the arcade and mame systems
* (Linux) Added support for the mGBA Qt and Snes9x GTK standalone emulators
* (Linux) Added support for the official xemu AppImage release
* (Linux) Added support for the AppImage release of Flycast
* (Linux) Added .esprefix file injections to the Dolphin, PrimeHack, Triforce, Yuzu and xemu standalone emulators
* Added support for the Sega Model 2 (model2) game system on Linux on macOS by adding the MAME - Current RetroArch core
* Added MAME standalone as an alternative emulator for the model2 system
* (Windows) Added the MAME - Current RetroArch core as an alternative emulator for the model2 system
* (Windows) Added a -force-feedback option and an %INJECT% variable to the Supermodel emulator for the arcade, mame and model3 systems
* Added a %GAMEDIR% variable to the -rompath option for all MAME standalone entries to allow launching games from subdirectories
* Added Triforce (Dolphin fork) standalone as an alternative emulator for the gc system on Linux and Windows
* Added simple64 standalone as an alternative emulator for the n64 system on Linux and Windows
* Added Rosalie's Mupen GUI standalone as an alternative emulator for the n64 and n64dd systems on Linux and Windows
* Added VICE standalone as an alternative emulator for the c64 (x64sc only) and vic20 systems
* (Windows) Added PrimeHack as an alternative emulator for the gc and wii systems
* (Windows) Added Project64 as an alternative emulator for the n64 system
* (Windows) Added SSF as an alternative emulator for the saturn and saturnjp systems
* (Windows) Changed the binary for emulator Citra from citra.exe to citra-qt.exe as the command line binary is broken on this OS
* Added support for the mugen system on Linux and macOS using the Ikemen GO game engine
* Added CPCemu standalone as an alternative emulator for the amstradcpc system
* Added MAME standalone as an alternative emulator for the gx4000 system
* Added MAME standalone as an alternative emulator for the atarijaguar system
* (Windows) Added BigPEmu standalone as an alternative emulator for the atarijaguar and atarijaguarcd systems
* Reverted the atarijaguarcd system to placeholder status on Unix and macOS as no emulators including Virtual Jaguar run these games properly
* Added the pcwindows platform to the ports system to improve scraping
* Added the .cdi and .cue file extensions to the atarijaguar and atarijaguarcd systems
* Added the . (dot) file extension to the xbox360 system on Windows to support extensionless XBLA games
* Added the .d64 file extension to the n64 and n64dd systems
* Added the .car and .rom file extensions to the a5200 system
* Added the .car file extension to the atari800 system
* Added the .bin file extension to the gx4000 system
* Added the .dsk file extension to the macintosh system
* Added the .m3u file extension to the pc98 and pcfx systems
* Added the .minipsf file extension to the psx system
* Removed the .7z and .zip file extensions from the 3do, neogeocd, neogeocdjp and switch systems
* Removed the .ccd and .cue file extensions from the fbneo system
* Removed the .ccd, .cue and .iso file extensions from the neogeo system
* Added the FinalBurn Neo RetroArch core as an alternative emulator for the neogeocd and neogeocdjp systems
* Added MAME standalone as an alternative emulator for the neogeo, neogeocd and neogeocdjp systems
* (Linux) Changed to find rule wildcard matching for the RetroArch, Basilisk II, DuckStation and SheepShaver AppImages
* (Linux) Added FinalBurn Neo standalone as an alternative emulator for the arcade, cps, fbneo, mame, neogeo, neogeocd and neogeocdjp systems
* (Windows) Added FinalBurn Neo standalone as an alternative emulator for the arcade, cps, fbneo, mame and neogeo system
* Set DOSBox-X and DOSBox Staging to start in the game directory so per-game dosbox.conf files can be used
* Changed the fullname for the 3do system from "3DO" to "3DO Interactive Multiplayer"
* Changed the fullname for the atomiswave system from "Atomiswave" to "Sammy Corporation Atomiswave"
* Changed the fullname for the bbcmicro system from "BBC Micro" to "Acorn Computers BBC Micro"
* Changed the fullname for the colecovision system from "ColecoVision" to "Coleco ColecoVision"
* Changed the fullname for the dragon32 system from "Dragon 32" to "Dragon Data Dragon 32"
* Changed the fullname for the samcoupe system from "SAM Coupé" to "MGT SAM Coupé"
* Changed the fullname for the uzebox system from "Uzebox" to "Uzebox Open Source Console"
* Changed the fullname for the vectrex system from "Vectrex" to "Smith Engineering Vectrex"
* (macOS) Added an additional find rule entry for DOSBox-X as the binary name has been changed
* (Linux) Added Flatpak support for Mednafen using the Mednaffe package
* (Linux) Added Flatpak support for Solarus using the Solarus Launcher package
* (Linux) Added a --fullscreen flag to the lightspark emulator for the flash game system
* Added support for folder links, used for launching game files inside folders without having to enter them
* Added a folder badge, including a folder link overlay in case a link has been configured
* Added a collection badge, shown when editing custom collections to indicate that a game is part of the collection
* Added the ability to color shift badge icons, badge controller icons and badge folder link icons
* Added the ability to center-align badges
* Adjusted the splash screen sizing to be more consistent across various screen aspect ratios
* Modernized the OpenGL renderer by replacing the fixed function pipeline with shaders
* Unified the desktop OpenGL and OpenGL ES renderers and upgraded to OpenGL 3.3 (4.6 on the Steam Deck) and OpenGL ES 3.0 respectively
* Changed the renderer pixel format from RGBA to BGRA
* Changed to premultiplied alpha for images as well as for GIF and Lottie animations, leading to a significant speed increase
* OpenGL: Added an OpenGLVersion setting for choosing between OpenGL 3.3, 4.2 and 4.6 (has to be manually set in es_settings.xml)
* OpenGL ES: Added an OpenGLVersion setting for choosing between OpenGL ES 3.0, 3.1 and 3.2 (has to be manually set in es_settings.xml)
* Greatly improved the performance of shader post-processing such as scanlines and blur rendering
* Greatly improved application startup speed by avoiding a lot of unnecessary SVG rasterizations
* Implemented dynamic texture allocation to the font code to reduce memory usage and avoid missing glyphs
* Added support for changing the saturation for font textures
* Large optimizations to the text wrapping code (generallly 300-400% faster)
* Added support for linear interpolation for font texture magnifications
* Added support for texture mipmapping with trilinear filtering
* Added on-demand texture loading to the carousel
* Improved the renderer scaling accuracy
* Added new tileSize, tileHorizontalAlignment and tileVerticalAlignment properties to the image element for better tiling support
* Added support for substituting the emulator binary in staticpath rules with an explicit command (useful for launching specific binaries in Flatpaks)
* The actual names for emulators with find rule entries are now displayed in the error popup window if they're not found during game launch
* Reorganized the UI Settings menu a bit and added entries to set the variant, color scheme, aspect ratio and transitions for newer theme sets
* Removed the "Preload gamelists on startup" setting
* Removed the "Play videos immediately (override theme)" setting
* Renamed the sound menu option "Play audio for videos in the gamelist view" to "Play audio for gamelist and system view videos"
* Added an "Ignore keyboard input" option to the input device settings menu
* (Unix) Set the "Disable desktop composition" option as disabled by default as it caused issues with some GPU drivers
* (macOS) Enabled startup animations which were previously disabled specifically for this operating system
* The media viewer now always loads all images upfront to avoid audio stutter when browsing the files
* Added support for defining which types of game media to use for all image elements (and also for the video component static image)
* Added a legacy (backward compatibility) mode for still supporting older RetroPie EmulationStation themes
* Added theme support for Lottie animations (vector graphics)
* Added theme support for GIF animations
* Added a GameSelectorComponent for displaying game media and metadata in the system view
* Added support to the system view for displaying videos, Lottie animations, GIF animations, date/time components and game ratings
* Replaced the forceUppercase theme property with a more versatile letterCase property (forceUppercase is retained for legacy theme compatibility)
* Added two letterCaseCollections and letterCaseGroupedCollections properties to control the letter case more specifically for the primary components
* Added support for overriding the default "unknown" values for the datetime and text elements (in case of missing game metadata)
* Renamed the textlist property selectorOffsetY to selectorVerticalOffset
* Added a selectorHorizontalOffset textlist property to control the selector's relative horizontal position
* Added a selectedSecondaryColor property to the textlist to highlight folder entries in the gamelist view with a different color than file entries
* Added theme support for setting the textlist indicators to symbols, ascii or none
* Added theme support for setting the textlist custom collection indicators to symbols or ascii
* Removed the "Use plain ASCII for special gamelist characters" menu option as it's now theme-controlled
* Removed the "Enable menu scroll indicators" menu option
* Removed the "Show system names in collections" menu option
* Added a menu option to retain extra MAME name information (region, version/revision, license, release date etc.) for unscraped game names
* Added theme support for appending system names to game names in collection systems
* Changed the badges default lines property value from 2 to 3
* Made it possible to set any text element as a scrollable container using either metadata values or literal strings
* Added support for defining the scrollable container speed, start delay and reset delay from the theme configuration
* Added support for arbitrary aspect ratios for RatingComponent images and also added an overlay property
* Added theme support for defining the opacity for most element types
* Added theme support for defining relative brightness for images, videos and animations
* Added theme support for defining color saturation for images, videos and animations
* Added theme support for defining the video fade-in time
* Added theme support for enabling and disabling video pillarboxes and scanline rendering
* Added theme support for defining the threshold for when pillarboxes should be applied to a video
* Added theme support for enabling or disabling audio playback for videos
* Added theme support for color shifting videos (and the static image)
* Added theme support for setting separate textColorDimmed and iconColorDimmed properties for the system and gamelist views
* Added a new cropSize property to the image and video elements to crop images and videos to the defined size
* Added two new flipHorizontal and flipVertical properties to the image element
* Added support for nesting theme variables
* Added support for defining multiple theme "variables" tags in the same XML file
* Added support for overriding/redefining variables
* When encountering missing theme include files defined by variables, a debug message is now printed instead of throwing an error
* When encountering missing theme files defined by element path properties, a debug message is now printed instead of a warning
* Added two DebugSkipMissingThemeFiles and DebugSkipMissingThemeFilesCustomCollections settings (which need to be manually set in es_settings.xml)
* Prevented loading of theme sets using the "resolution" tag introduced by RetroPie in 2020 as it's a very bad idea to use this logic
* Added support for vertical abbreviations of multiline text entries
* Disabled the pillarboxes and scanline rendering menu options when using a non-legacy theme set
* Improved theme element placement by replacing the "alignment" and "logoAlignment" properties with specific horizontal and vertical properties
* Made it possible to use almost all game metadata field when theming text elements
* Made it possible to set the image interpolation method (nearest neighbor or linear filtering) per image from the theme configuration
* Changed the helpsystem properties entrySpacing and iconTextSpacing from fixed pixel values to relative values
* Added support for using unsigned integers for theme properties
* Added a metadataElement theme property to the image, video, animation and text element types to control fading and hiding of arbitrary elements
* Added the rendering of a green rectangle around the CarouselComponent when pressing Ctrl+i while in debug mode
* Changed the color of the rectangle from blue to green for TextListComponent when pressing Ctrl+t while in debug mode
* Added scraper settings for defining a retry count and a retry timer in case of errors
* Added scraper support for displaying the returned platform if it does not match the game platform, or if multiple platforms are defined for a system
* Added scraping of fan art and updated the media viewer to display these images
* Added scraping of box back covers when using TheGamesDB
* If a wheel (marquee) image on ScreenScraper falls back to another region, then the wheel-hd image is now used instead if it matches the set region
* Removed scraping of arcade controller information using ScreenScraper as they have ruined this functionality
* Newline characters are now removed from game names when scraping with ScreenScraper (these occur in some very rare instances)
* Hex-encoded ampersand characters in game names are now converted correctly when scraping with ScreenScraper
* Added a ScreenScraper-specific option to remove dots from game name searches when using the multi-scraper in automatic mode
* Moved the option "Scrape actual folders" higher up in the scraper options menu
* Added the ability to set a manual sortname specifically for custom collections using the metadata editor
* When scraping in semi-automatic mode, horizontal scrolling of long game names are no longer reset when automatically selecting the result
* Added support for using the tilde (~) symbol in the es_systems.xml path entries to expand to the user home directory
* Reduced CPU usage significantly when a menu is open by not rendering the bottom of the stack
* Reduced CPU usage significantly by only rendering the necessary systems in SystemView
* Added support for dimming components (fade to black)
* Added support for rotating the application screen contents 0, 90, 180 or 270 degrees
* Added support for offsetting the screen contents within the application window
* Added support for running the application at a lower resolution in fullscreen padded mode
* Added logging of the display refresh rate on startup
* Made many improvements to GUI sizing and positioning when running in vertical screen resolutions
* The application startup can now be aborted via an OS signal or using the configured keyboard quit shortcut
* Improved the behavior and consistency for the virtual keyboard when using non-standard keyboard mappings
* Improved the theme loading error logging to make it consistent and easier to understand
* Added a log warning for unthemed systems during theme set loading
* Changed the warning log level for missing theme files to debug level if the paths are set using variables
* Added new theme system variables for differentiating between collections and non-collection systems
* Added shader support for performing BGRA to RGBA color conversion
* Added opacity support to the scanline shader
* Added the rlottie library as a Git subtree
* Updated to build correctly with FFmpeg 5.1
* Updated FFmpeg to 5.1.2, FreeType to 2.12.1 and pugixml to 1.12.1 on Windows and macOS
* Updated SDL to 2.26.3 on Windows, macOS and the Linux AppImage builds
* Updated curl to 7.86.0 on Windows
* Added a workaround for playing broken video files with invalid PTS values
* Refactored the rendering code from a shared namespace into proper classes
* Removed the deprecated OpenGL ES 1.0 renderer
* Increased the maximum VRAM limit from 1024 MiB to 2048 MiB
* Increased the minimum VRAM limit from 80 MiB to 128 MiB
* Increased the default VRAM limit from 256 MiB to 512 MiB
* Increased the default VRAM limit for the Raspberry Pi from 184 MiB to 192 MiB
* (Unix) Removed the RetroArch core directory for the NixOS and nixpkgs repository
* The .emulationstation/gamelists directory is now created on application startup instead of when scraping for the first time
* On Windows all dependencies were moved in-tree to the "external" directory to greatly simplify the build environment
* Updated the build scripts to support native M1/ARM builds on macOS
* Improved the in-tree build on macOS to not needing to install any libraries when compiling the "external" dependencies
* When building as an AppImage a current SDL library release is now built and bundled instead of including the OS-supplied version
* When building as an AppImage the "data" directory (e.g. /usr/share/emulationstation) is now excluded when looking for resources and themes
* Added libGLdispatch.so.0 to the AppImage to fix a crash introduced by a Mesa library update
* Updated the CImg library to commit 4d061dcd67c08e9a36a01875e78b53ee86501dd7
* Large refactoring to improve thread safety and singleton pattern usage
* Made the logging thread safe
* (Windows) Changed many log entries to use backslashes instead of forward slashes as directory separators
* Added the build date to to main menu for alpha, beta and dev builds
* Added a left trigger + right trigger help system icon and removed the deprecated hotkey icon
* Added PlayStation 1/2/3 and Switch Pro controller type support
* Renamed some SNES controller button assets to be more intuitive
* (slate-es-de) Recreated the Nintendo GameCube controller as the old graphics had some issues and was not accurate
* Added Nintendo GameCube, Sega Master System and Sega Dreamcast controller badge icons
* Added an arcade twin stick controller badge icon
* Moved all Platform functions to the utility namespace instead of using the global namespace
* Implemented proper XML attribute support in ThemeData that eliminates the risk of name collisions
* Added size restrictions to images and fonts so incorrect theme configuration would not lead to crashes or excessive memory utilization
* Made animations on the carousel better looking by using a new non-linear interpolation method
* Migrated the carousel code from SystemView to a separate CarouselComponent
* Changed the carousel properties to be more generic by renaming "logo" to "item", e.g. itemSize, maxItemCount etc.
* Added the properties "itemsBeforeCenter" and "itemsAfterCenter" to define entries for carousels of the wheel type
* Added support for horizontal wheel carousels
* Added two wheelHorizontalAlignment and wheelVerticalAlignment properties for aligning wheel carousels within the overall element area
* Added reflections support to the carousel
* Added a new itemAxisHorizontal property to the carousel to keep wheel items horizontal at all times
* Added a new imageFit property to the carousel to allow items to be stretched/squashed or cropped to the defined item size
* Added carousel theme support for setting the opacity, saturation and dimming for unfocused items
* Added carousel theme support for applying image color shifts
* Added carousel theme support for defining image brightness
* Added carousel theme support for defining image saturation
* Added carousel theme support for setting item transitions to "slide" or "instant"
* Added carousel theme support for enabling faster scrolling speed with a third scroll tier
* Added carousel theme support for controlling item stacking for overlapping items
* Added carousel theme support for defining margins around the currently selected item
* Added carousel theme support for rotating items around their own axis (not supported for wheel carousels)
* Added carousel theme support for color shifting the selected item
* Added carousel theme support for offsetting items to achieve a diagonal layout
* Added a fadeAbovePrimary property to control whether elements above the system view carousel and textlist should be rendered during fade transitions
* Removed support for the thumbnail game media type
* Changed all occurances of "GameList" to "Gamelist" throughout the codebase
* Removed a huge amount of unnecessary Window* function parameters throughout the codebase
* Removed a lot of unnecessary applyTheme() calls when updating help prompts
* Removed the last remnants of the PowerSaver
* Removed the RetroPie ES sizing bug replication from TextListComponent for non-legacy theme sets
* Added checks for nonexistent navigation sounds in the theme configuration with fallback to the default sounds
* Changed the opacity data type and functions from unsigned char to float throughout the codebase
* Refactored the six gamelist classes into two new classes; GamelistBase and GamelistView
* Rewrote the gamelist logic to handle an arbitrary amount of components per type and split the legacy code into a separate file
* Renamed Gamelist.cpp to GamelistFileParser.cpp and moved it to its own namespace instead of using the global namespace
* Renamed GuiGameScraper.cpp to GuiScraperSingle.cpp
* Renamed SystemScreensaver.cpp to Screensaver.cpp
* Refactored RatingComponent to improve rendering accuracy and performance
* Moved UIModeController.cpp from the es-app/views directory to es-app
* Set the clang-format option SpaceBeforeCpp11BracedList to true and reformatted the codebase
* Added the clang-format option AllowShortEnumsOnASingleLine and set it to false
* Removed some unnecessary typedefs and replaced the remaining ones with the more modern "using" keyword
* Greatly simplified the video controls code (play/stop/pause etc.)
* Removed the deprecated VideoVlcComponent
* Removed the deprecated tools/update_theme_formatversion.sh script
* Lots of general code cleanup and refactoring
* (Windows) Added error handling to StringUtil::stringToWideString() and StringUtil::wideStringToString() to perform an emergency shutdown if needed
* Updated the MAME index files to include ROMs up to MAME version 0.251
* Added a program release number in addition to the version number
* Changed tools/create_AppImage.sh to not include version information in the AppImage filename
* Updated and improved the theming documentation

### Bug fixes

* Multiple levels of symlinking in the ROMs directory tree could crash the application on startup
* Adding a dot (.) to the es_systems.xml extension tag (to setup extensionless entries) led to a crash if the system contained folders
* Enabling collections from the Game Collection Settings menu could crash the application in some rare cases
* There was a use after free issue in the multi-scraper which could in theory crash the application
* Running with the --force-input-config command line option and aborting the input configuration would lead to a black screen and eventually a crash
* Parsing of .desktop files on Unix did not properly handle escaping of % characters which made game launching fail for some RPCS3 games
* For the cps system, MAME standalone was configured with the wrong system directory for the -rompath option, pointing to "arcade" instead of "cps"
* Invalid ScreenScraper game entries were sometimes not filtered out from server responses
* During some menu operations that reloaded the gamelist view, the cached background could miss some components as they were not rendered in time
* Text wrapping did not work correctly for text that typically does not contain spaces, like Japanese
* Changing some values using the metadata editor could lead to an incorrect sort order if the changes were done from within a grouped custom collection
* Changing the setting "Group unthemed custom collections" could lead to incorrect custom collections sorting under some circumstances
* For gamelists which mixed files and folders, the folder sorting was sometimes incorrect
* Incorrect folder paths were displayed in the metadata editor if the setting "Only show ROMs from gamelist.xml files" was enabled
* Games located in subdirectories were not added back to custom collections when disabling the "Exclude from game counter" metadata option
* Enabling and then disabling the "Exclude from game counter" metadata option would remove a game from all currently disabled custom collections
* Navigation sounds for the trigger buttons would play when repeatedly pressed at the start or end of textlists
* Slide and fade transitions would sometimes stop working after changing theme sets
* Using fade transitions, when holding a direction button to scroll the system view carousel, the key repeat would cause an unwanted background rendering
* Textlist entries would sometimes scroll horizontally even though they fit inside the element width
* A star was included in the quick selector for gamelists that had a mix of files and folders but where only folders were marked as favorites
* Custom collections editing mode did not end when switching UI modes
* Editing a folder using the metadata editor added any new values to the filter index, even though it shouldn't be possible to filter folders directly
* There were multiple issues with filtering games inside folders (too many games filtered, inconsistent behavior etc.)
* The outermost logos would sometimes glitch out during carousel transitions
* Horizontal and vertical gradients were mixed up (showing the opposite gradient type if set in a theme)
* The VideoComponent static images were not fading out smoothly on gamelist fast-scrolling
* Rating icon outlines would not fade out correctly when fast-scrolling in a gamelist
* The rating icons would not fit into the designated space in the scraper GUI when running at some vertically oriented screen resolutions
* The game description would start to scroll too late when running the multi-scraper in semi-automatic mode
* If setting an origin other than 0.5 for a video with pillarboxes enabled, the video would not get centered on the black rectangle
* The video player output frame width was not set correctly which made some videos render as garbled when using FFmpeg 5.1 and later
* If a gamelist scroll fade-in animation was playing when opening a menu, it would continue to play after closing the menu
* The gamelist quick list scrolling overlay would not disappear as intended under some circumstances
* Using the trigger buttons to jump to the start or end of a gamelist did not reset any currently held navigation buttons
* When a legacy theme set had a video view style but did not have a valid md_video entry then the video player would still start (and play the audio)
* Clearing a game in the metadata editor would sometimes not remove all media files (if there were both a .jpg and a .png for a certain file type)
* Adding collections or changing collection settings could sometime lead to the gamelist not getting rendered until navigating away from the current system
* The tile property for the image element did not work correctly with SVG images
* Defining an itemScale (logoScale) property lower than 1.0 for the carousel did not work correctly
* Carousel text did not get scaled/multiplied correctly with the itemScale property (bug retained for legacy themes for maximum backward compatibility)
* Letters would sometimes get rendered with ugly edge artifacts, visible when scaling text on the carousel
* Text opacity did not work correctly in some places, such as for the help prompts
* ScrollableContainer faded semi-transparent text to fully opaque when resetting
* ScrollableContainer faded in the background text color in addition to the text color when resetting
* Text elements that had an opacity set to lower than FF via the color tag were faded in during gamelist scrolling
* The help system was offset by the entrySpacing property width when right-aligned using an X origin value of 1
* The help system was rendered on top of menus if placed at such a location on the screen
* The help system "A" button was incorrectly shown as "Apply" instead of "Newline" when using the virtual keyboard with multi-line input fields
* Theme sets were not always sorted correctly (as seen when mixing uppercase and lowercase letters in theme names)
* The SliderComponent knob was not consistently positioned
* The device text flickered in GuiDetectDevice when configuring a controller
* The selector bar was not aligned correctly during menu scale-up animations
* The bottom menu separator bar was not getting rendered when running at really low resolutions like 320x240
* Doing a manual reload using Ctrl+r in debug mode would sometimes not update modified image files
* Abbreviations of long words in multiline text entries sometimes exceeded the designated text area
* Navigation sounds would sometimes not play when browsing game media in the media viewer
* When defining the same sound file for multiple navigation sounds in the theme configuration an error was logged to es_log.txt on theme change
* The text debug overlay had the wrong size for scrollable containers
* The textlist debug overlay would sometimes get positioned incorrectly
* StringUtil::delimitedStringToVector could return empty elements
* (Windows) File paths would get escaped with quotation marks even if they did not contain any spaces
* (Windows) The emulator binary path would sometimes not get escaped correctly in es_log.txt on game launch
* The .chd and .rp9 file extensions were missing for the amigacd32 system on macOS and Windows

## Version 1.2.6

**Release date:** 2022-08-03

### Release overview

This release enables a lot of systems by adding new platforms altogether and by including emulator configuration for previous placeholder entries. A number of additional standalone emulators have been added as well. The newly released PCSX2 Qt standalone emulator has replaced the old PCSX2 release which has been designated a legacy emulator. Connection timeout support has also been added to the scraper which among other things makes it possible to continue scraping after resuming a computer that went to sleep when the scraper was running.

### Detailed list of changes

* Added experimental support for Vita3K for the psvita system on Unix and Windows
* Added support for the Fujitsu FM Towns (fmtowns) game system on Unix and Windows
* Added support for the Adobe Flash (flash) game system
* Added support for the EasyRPG game engine (easyrpg) game system
* Added support for the Nintendo Super Game Boy (sgb) game system
* Added support for the Creatronic Mega Duck (megaduck) game system
* Added support for the Watara Supervision (supervision) game system
* Added support for the M.U.G.E.N Game Engine (mugen) game system on Windows
* Added emulator configuration for the apple2 system
* Added emulator configuration for the apple2gs system
* Added emulator configuration for the macintosh system
* Added emulator configuration for the trs-80 system
* Added emulator configuration for the coco system
* Added emulator configuration for the dragon32 system
* Added emulator configuration for the tanodragon system
* Added emulator configuration for the gx4000 system
* Added emulator configuration for the solarus system
* Added emulator configuration for the tic80 system
* Added emulator configuration for the ags system
* Renamed the Nintendo 64DD system from 64dd to n64dd and removed the Mupen64Plus standalone emulator
* Replaced the invalid SimCoupé RetroArch core with SimCoupé standalone for the samcoupe system
* Added a find rule for the Flatpak release of MAME
* Added Mednafen standalone as an alternative emulator for many systems
* Changed the emulator PCSX2 standalone to PCSX2 Legacy standalone
* Changed the emulator PCSX2 Qt standalone to PCSX2 standalone
* (Windows) Removed the emulator PCSX2 wxWidgets standalone
* Renamed the ROM directory for the ColecoVision system from coleco to colecovision
* Added ScummVM standalone as an alternative emulator for the scummvm system
* Added Cxbx-Reloaded standalone as an alternative emulator for the xbox system on Windows
* Added Atari800 standalone as an alternative emulator for the a5200 system
* Added the Kronos RetroArch core as an alternative emulator for the arcade and mame systems
* Added KEmulator standalone as an alternative emulator for the j2me system on Windows
* Added Model 2 Emulator [Suspend ES-DE] as an alternative emulator for the arcade, mame and model2 systems on Windows
* Added "Shortcut or script" as an alternative emulator for the doom system
* Added the Boom 3 and Boom 3 xp RetroArch cores as alternative emulators for the doom system on Unix and Windows
* Added support for the repository-installed PPSSPP standalone (SDL and Qt) on Unix
* Added the .chd and .rp9 file extensions to the amiga, amiga1200, amiga600, amigacd32 and cdtv systems
* Added the .fds file extension to the famicom and nes systems
* Added the .32x file extension to the genesis and megadrive systems
* Added the .json file extension to the gc and wii systems
* Added the .ndd file extension to the n64 and n64dd systems
* Added find rule entries for Valve Steam to simplify the setup of the RetroArch Steam release
* Added scraper support for the dragon32 platform
* Added a %GAMEENTRYDIR% variable to be used with the %STARTDIR% variable (required by EasyRPG Player standalone)
* Added a %FILENAME% variable to extract the filename including the extension when used in es_systems.xml
* Added connection and transfer timeout settings to the scraper (not configurable via the GUI)
* Added an es_log.txt entry when the "Only show ROMs from gamelist.xml files" setting is enabled
* Passing the --ignore-gamelist command line option now immediately disables the ParseGamelistOnly setting
* (Windows) Added code signing to both the application binary and installer
* (macOS) Improved Apple compliance for the Info.plist file
* (rbsimple-DE) Added console graphics for the psvita system

### Bug fixes

* The IgnoreGamelist setting was saved to es_settings.xml although it shouldn't
* (modern-DE) The trs-80 system had graphics and logo for the wrong computer

## Version 1.2.5

**Release date:** 2022-06-22

### Release overview

v1.2 maintenance release. Shortcut support has been added on Unix and macOS which greatly simplifies setup of applications as well as games for platforms like ports, steam, lutris and ps3. A couple of new systems have been added, most notably PICO-8, and RetroPie EmulationStation theme compatibility has been improved. As well the default keyboard quit shortcut has been changed from F4 to the operating system defaults (Alt + F4 on Unix and Windows and Command + Q on macOS). Some bugs were also fixed and some missing graphic assets were added to the default rbsimple-DE theme set.

### Detailed list of changes

* Added an %ENABLESHORTCUTS% variable to support launching of .desktop files on Unix and apps and aliases on macOS
* Added support for launching .desktop files to the desktop, epic, kodi, lutris, ports and steam systems on Unix
* Added support for launching .app directories and alias files to the desktop, epic, kodi, ports and steam systems on macOS
* Setup two emulator entries for the desktop system, "Suspend ES-DE" and "Keep ES-DE running" to control the launch behavior
* Changed the ps3 system to use shortcuts by default and created an alternative emulator entry for the old directory approach
* Added the .lnk file extension to the epic system on Windows
* Added support for the PICO-8 (pico8) game system
* Added support for the Capcom Play System (cps) game system
* Added support for the Sega Model 2 and 3 (model2 and model3) game systems on Windows
* Added configuration for the Tangerine Computer Systems Oric (oric) system on Unix and Windows
* Added configuration for the Texas Instruments TI-99 (ti99) system on Unix and Windows
* Added configuration for the Google Android (android) system on Windows
* (Windows) Changed the emulator directory for Model 2 Emulator from Model2 to m2emulator
* (Windows) Changed the emulator find rule name for the Model 2 Emulator from MODEL2 to M2EMULATOR
* Added support for asterisks/wildcards for emulator name matching, usable in both es_find_rules.xml and es_systems.xml
* (Linux) Changed to find rule wildcard matching for the AppImages for Dolphin, mGBA, Play!, RPCS3, Xemu and Yuzu
* (Windows) Changed to find rule wildcard matching for the PCSX2-QT and PCSX2-WXWIDGETS entries to support the AVX2 releases of PCSX2
* Added a ScreenScraper option to fall back to additional regions to allow scraping of country-specific games and unofficial releases
* Changed the sorting in the Alternative emulators interface to use short system names instead of full system names
* Added scraper support for the ti99 platform
* Added TheGamesDB scraper support for the oric platform
* Added the pcwindows platform to the lutris system on Unix to improve scraping
* Renamed the "Exit button combo" menu option to "Keyboard quit shortcut"
* Changed the default quit shortcut from F4 to Alt + F4 on Windows and Unix
* Changed the default quit shortcut from F4 to Command + Q on macOS
* Added Ctrl + Q as a user-selectable quit shortcut
* Added support for placing a noload.txt file in the root of a system/ROM directory to keep it from getting loaded
* (Windows) Added support for creating a portable installation in the root of a device, such as E: or F:
* Increased the window width slightly for the Alternative emulators interface when displaying long system names
* Added size restrictions to the "size" and "maxSize" theme properties for the image and video elements
* Changed to a new API key for TheGamesDB
* (Linux) Changed the manually downloaded Redream emulator location from ~/Applications/redream to ~/Applications/redream/redream
* (rbsimple-DE) Added console and controller graphics for the wiiu system
* (rbsimple-DE) Added controller graphics for the saturn, saturnjp and xbox systems
* (modern-DE) Replaced the carousel image for the desktop system

### Bug fixes

* When running ES-DE in the background, quitting a game or application using Alt + F4 sometimes made ES-DE quit as well
* Thumbnails were not included in theme sets that used them for the Detailed view style (affected RetroPie ES compatibility)
* Game images were not included in theme sets that used them for the Video view style (affected RetroPie ES compatibility)
* Themes with large pixelated fonts sometimes displayed too many textlist rows (affected RetroPie ES compatibility)
* When entering the text edit field for the virtual keyboard, a navigation key repeat would sometimes continue to run
* The menu scroll indicators and title sometimes overlapped (e.g. seen in the Alternative emulators interface)
* There was a small rounding error that caused a minimal distortion when rendering menu titles
* (macOS) The platform tag was missing for the NEC TurboGrafx-CD (tg-cd) system, leading to inaccurate scraping

## Version 1.2.4

**Release date:** 2022-05-27

### Release overview

v1.2 maintenance release. Support has been added for multiple systems such as Daphne (arcade LaserDisc) and OpenBOR. A number of default emulators have been changed, some standalone emulators have been added and numerous bugs have been fixed. Some low-level functionality has also been implemented to enable more advanced emulator launch options and quite a number of missing theme assets have been created for the default rbsimple-DE theme.

### Detailed list of changes

* Added configuration for the Daphne arcade LaserDisc system using the Hypseus Singe emulator
* Added configuration for the OpenBOR game engine on Linux and Windows
* Added emulator configuration for the astrocde (Bally Astrocade) and cdtv systems
* Changed the default emulator for the famicom, fds and nes systems from Nestopia UE to Mesen
* Changed the default emulator for the gb and gbc systems from SameBoy to Gambatte
* Changed the default emulator for the gamegear and sg-1000 systems from Gearsystem to Genesis Plus GX
* Changed the default emulator for the dos and pc systems from DOSBox-Core to DOSBox-Pure
* Changed the default emulator for the mame system from MAME 2003-Plus to MAME - Current
* Removed the --escape-exit command line option for the PPSSPP standalone emulator as it caused issues for some users
* Added experimental support for PCSX2 Qt and PCSX2 wxWidgets for the ps2 system on Windows
* Added AetherSX2 standalone as an alternative emulator for the ps2 system on macOS
* Added DOSBox-X standalone as an alternative emulator for the dos and pc systems
* Added bsnes standalone as an alternative emulator for the satellaview, sfc, snes, snesna and sufami systems
* Added Snes9x standalone as an alternative emulator for the satellaview and sufami systems
* Added Atari800 standalone as an alternative emulator for the atarixe system
* Added the Flycast RetroArch core and Flycast standalone as alternative emulators for the arcade and mame systems
* Added the Gearcoleco RetroArch core as an alternative emulator for the colecovision system
* Added Nestopia UE standalone as an alternative emulator for the famicom, fds and nes systems on Unix
* Added DeSmuME standalone as an alternative emulator for the nds system on Unix
* Added Model 2 Emulator standalone as an alternative emulator for the arcade and mame systems on Windows
* Added Supermodel standalone as an alternative emulator for the arcade and mame systems on Windows
* Added the file extensions .ciso, .dump, .gz, .m3u, .mdf, .img and .nrg to the ps2 system
* Removed the .chd file extension from the naomi, naomigd and atomiswave systems
* Removed the .bin file extension from the pcengine, pcenginecd, tg16 and tg-cd systems
* Added the "citra" binary for Citra standalone on Linux (making it possible to use the repository or AUR release)
* Removed the -full-screen command line option for the xemu emulator
* Set %STARTDIR%=%EMUDIR% for the xenia (Xbox 360) emulator
* Added scraper support for the astrocde platform
* Added scraper support for the arcadia (Emerson Arcadia 2001) and crvision (VTech CreatiVision) platforms
* Changed the platform to arcade for the atomiswave, naomi and naomigd systems to improve scraping
* Added TheGamesDB scraper support for the atomiswave and naomi platforms
* Added a new pcwindows platform for scraping PC (Windows) games
* Changed the steam platform internally to use the ScreenScraper "PC Windows" platform ID
* Changed the platform to pcwindows for the epic system
* Enabled screensaver controls when running in Kid UI mode
* Added custom event script triggers on application startup, screensaver start and screensaver end
* Added a --create-system-dirs command line option to generate the game system directories
* Added an %INJECT% variable for injecting launch arguments from game config files (required by Hypseus Singe)
* Added a %GAMEDIR% variable that expands to the game directory (required by Hypseus Singe)
* Made the %GAMEDIR% variable usable with the %STARTDIR% variable (required by OpenBOR)
* (Windows) Set %RUNINBACKGROUND% when launching MAME standalone as this emulator may otherwise hang on exit
* (Windows) Added an %ESCAPESPECIALS% variable that escapes the special characters &()^=;,
* (Windows) Added %ESCAPESPECIALS% to the desktop, epic, kodi, ports and steam systems
* (rbsimple-DE) Added console and controller graphics for the ps2 and ps3 systems
* (rbsimple-DE) Added console graphics for the n3ds, saturn, saturnjp, switch and xbox360 systems
* Added an Xbox Kinect controller badge icon
* Swapped the colors of the Joy-Con controller badge icons
* (macOS) Categorized the application as a game so it shows up in the Launchpad games section
* Replaced the explicit shell commands in es_systems.xml with %EMULATOR_OS-SHELL% find rules
* Updated the StringUtil::replace function as the old function was dangerous and could run into an endless loop
* Added experimental support for folder flattening

### Bug fixes

* All games were included in the video and slideshow screeensavers when in Kid UI mode
* Under very rare circumstances, games and folders could get mixed up during gamelist.xml parsing
* The %BASENAME% variable didn't work correctly with the "Directories interpreted as files" functionality
* The scroll indicators would sometimes not work correctly in the Alternative Emulators screen
* Fixed a minor rounding issue which sometimes led to the menu scroll indicators not being positioned correctly
* The game-end event was triggered immediately on game launch if running ES-DE in the background
* The "quit" custom event script trigger was not executed when quitting the application using the quit key combo
* Chinese characters would sometimes not render correctly
* The "Jump to.." quick selector didn't work correctly with multi-byte Unicode characters
* (Windows) Fixed an issue where symlinking game media directories would crash the application
* (Windows) Scripts and links executed using cmd.exe could not contain the special characters &()^=;,
* (Windows) ROM directories could not be created in the root of a device such as D:\ or E:\
* (Linux) Flatpak directories were missing for user installations of the standalone emulators BlastEm, Play! and Snes9x
* (rbsimple-DE) The systeminfo text for the saturn and saturnjp systems had mixed up megabytes with megabits

## Version 1.2.3

**Release date:** 2022-05-04

### Release overview

v1.2 maintenance release. Support has been added for displaying multi-disc/multi-file games as single entries, underscores can now be filtered out when doing scraper searches and a number of additional emulators and emulator file extensions have been added. A few bugs have been fixed as well.

### Detailed list of changes

* Made it possible to directly launch files inside directories that are interpreted as files
* Added a scraper setting to convert underscores _ to spaces when searching
* If no ScreenScraper video is found when scraping, a fallback will now be done to the normalized (low quality) video
* Added support for using the manually downloaded emulators Redream and Ryujinx on Unix
* Added Play! standalone as an alternative emulator for the ps2 system
* Added Snes9x standalone as an alternative emulator for the sfc, snes and snesna systems
* Added Atari800 standalone as an alternative emulator for the atari800 system
* Added BlastEm standalone as an alternative emulator for the megadrive and genesis systems on Unix
* Added MAME standalone as an alternative emulator for the arcade and mame systems on macOS
* Added the SAME CDi and CDi 2015 RetroArch cores for the cdimono1 system
* Added the PUAE 2021 RetroArch core to the amiga, amiga600, amiga1200 and amigacd32 systems
* Replaced the RetroArch core 4DO with Opera for the 3do system
* Removed the RetroArch DuckStation core as it has been superseded by SwanStation
* Added the .ps3 file extension to the ps3 system
* Added the .rom file extension to the atari800 system
* Added the .svm file extension to the scummvm system and removed support for .7z and .zip extensions
* Added the .wua, .wud and .wux file extensions for the wiiu system on Windows
* Added --escape-exit command line option for the PPSSPP standalone emulator
* Added -batch command line option for the DuckStation standalone emulator
* (Windows) Added a separate find rules configuration file for use with portable installations
* (Windows) Added an "Emulators" directory to all emulators for portable installations
* Added missing scraper entries for samcoupe and zx81 for TheGamesDB and zmachine for ScreenScraper
* neogeocd is now scraped specifically as "Neo Geo CD" instead of the more generic "Neo Geo"
* (rbsimple-DE) Made the xbox console graphics slightly darker
* The %ROMPATH% variable can now be used inside the es_systems.xml command tag
* Added a %STARTDIR% variable to set the start directory when running an emulator (required by MAME standalone)
* Added an %EMUDIR% variable that expands to the emulator binary directory
* Added a CMake flag to build as Flatpak which prefixes "flatpak-spawn --host" to all launch commands
* Added some Flatpak-specific code to work around the sandbox restrictions of this package format
* A check is now done on game launch that emulator binaries are actually files or symlinks
* (Unix) Renamed the icon emulationstation.svg to org.es_de.emulationstation-de.svg

### Bug fixes

* The MAME standalone emulator couldn't be launched
* Using a custom image directory for the slideshow screensaver would hang the application if there was only a single image
* On Unix and macOS, staticpaths rules in es_find_rules.xml containing spaces would not work
* %ESPATH% variables could not be used in es_systems.xml
* Navigating the list of alternative emulators would sometimes lead to an incorrect row positioning
* On Windows, the find rule for the Mupen64Plus standalone emulator was not setup correctly

## Version 1.2.2

**Release date:** 2022-04-07

### Release overview

v1.2 maintenance release. The accuracy of the automatic (non-interactive) scraper has been greatly increased when using ScreenScraper. Some bugs were also fixed and a number of alternative emulators and file extensions have been added.

### Detailed list of changes

* ScreenScraper searches in automatic mode are now faster and much more accurate
* Added fallback to high resolution marquee/wheel images for ScreenScraper if no regular wheel image was found
* Set the interactive scraper as disabled by default
* Added support for the Nintendo SFC (Super Famicom) game system
* Added the SwanStation RetroArch core as an alternative emulator for the psx system
* Added mGBA, mGBA standalone, VBA-M and VBA-M standalone as alternative emulators for the gb and gbc systems
* Added the bsnes-hd RetroArch core as an alternative emulator to the satellaview, snes, snesna and sufami systems
* Added the FCEUmm and Mesen RetroArch cores as alternative emulators for the fds system
* Added the a5200 RetroArch core for the atari5200 system and set it as default, with atari800 as the alternative
* Added the .chd file extension to the ps2 system
* Added the .dosz file extension to the dos and pc systems
* Added the .lnk file extension to the desktop, kodi and ports systems on Windows
* Added the .url file extension to the steam system on Windows
* Emulator Flatpaks can now be installed to the home directory on Debian
* Increased the default VRAM limit from 256 MiB to 512 MiB on the Steam Deck

### Bug fixes

* A crash could occur under some circumstances due to an insufficient font texture size
* The systems were not always sorted correctly
* The help system was not properly updated after results were returned by the scraper

## Version 1.2.1

**Release date:** 2022-03-30

### Release overview

v1.2 maintenance release. Some minor bugs were fixed and some smaller adjustments were made but most importantly this release brings support for a lot more standalone emulators on all supported platforms.

### Detailed list of changes

* Added support for the standalone emulators Citra, Dolphin, DuckStation, MAME, melonDS, mGBA, Mupen64Plus, PPSSPP, Redream, Ryujinx and VBA-M
* Added support for the standalone emulators sixtyforce and xemu for macOS, Cemu for Windows and PrimeHack for Unix
* Added the .m3u, .rvz and .wia file extensions for the Dolphin emulator
* Set the option "Scrape actual folders" as enabled by default
* Set the option "Play audio for screensaver videos" as enabled by default

### Bug fixes

* When multi-scraping in semi-automatic mode and a long game name was scrolling, the start position was not reset when scraping the next game
* During multi-scraping the busy indicator was not displayed after a result was acquired but before the thumbnail was completely downloaded
* The ScummVM platform entry was missing for TheGamesDB which resulted in very inaccurate scraper searches
* Fixed an incorrect RetroArch core path for the emulator VBA-M
* modern-DE: Small adjustment to make the help system fit on screen at all times

## Version 1.2.0

**Release date:** 2021-12-28

### Release overview

The 1.2 release introduces multiple new features and brings extensive bug fixing and lots of other small improvements. Support for alternative emulators has been added which can be selected system-wide or per game. These alternative emulators are added to the es_systems.xml file, making it easy to expand or customize the configuration. For this release most of the available RetroArch cores have been preconfigured, and a couple of standalone emulators have been included as well.

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
* Bundled a new alternative theme "modern-DE" which supports all the latest features from this release
* Changed the Unix fullscreen mode and removed the --windowed, --fullscreen-normal and --fullscreen-borderless command line options
* Removed the Unix-specific menu option "Fullscreen mode (requires restart)"
* Changed the Windows fullscreen mode and removed the "AMD and Intel GPU game launch workaround" menu option
* Made game launching more seamless on Windows for all GPU types
* Added the ability to make complementary game system customizations without having to replace the entire bundled es_systems.xml file
* Added support for an optional \<systemsortname\> tag for es_systems.xml that can be used to override the default \<fullname\> systems sorting
* Added a "winregistryvalue" find rule for Windows which can be used to retrieve emulator installation locations from arbitrary Windows Registry keys
* Added a %RUNINBACKGROUND% es_systems.xml variable and removed the hardcoded run in background logic for the Valve Steam system
* Added support for prefixing the %EMULATOR_% variable in the es_systems.xml file with a command, for example to use Wine to launch Windows emulators on Linux
* Added proper support for interpreting directories as files (for use with emulators where directories rather than files are passed during game launch)
* Added menu scroll indicators showing if there are additional entries available below or above what's currently shown on screen
* Added scraping of controller metadata (only for ScreenScraper and only for arcade systems)
* Improved the layout of the scraper GUIs (single-game scraper and multi-scraper)
* Added horizontal scrolling of long game names to the scraper GUIs
* Removed the "Scrape" text prefix from the scraper content settings
* Setting a blank name for an arcade game in the metadata editor now sets the value to the MAME expanded name instead of the physical file name
* Added proper frame drop functionality to the FFmpeg video player to greatly reduce stuttering on slower machines
* Made multiple optimizations to the FFmpeg video player to reduce CPU usage and to increase framerates on slower machines
* Disabled the FFmpeg video player hardware decoding option (it can still be built using a CMake flag)
* Significantly reduced the CPU usage on macOS while running in the background
* Removed the copying of es_settings.cfg to es_settings.xml as it caused issues when migrating from other EmulationStation forks
* Improved the gamelist filter GUI to not allow filtering of values where there is no actual data to filter, e.g. Favorites for a system with no favorite games
* Grayed out all fields in the gamelist filter GUI where there is no data to filter, previously some fields were removed entirely and some could still be used
* Added filters for "Controller" and "Alternative emulator" and sorted the filters in the same order as the metadata editor fields
* Added the ability to filter on blank/unknown values for Genre, Player, Developer, Publisher, Controller and Alternative emulator
* Added a menu option to change the application exit key combination
* If there are no custom collections, the "Custom game collections" menu entry is now grayed out
* Added an option to preload the gamelists on startup which leads to smoother navigation when first entering each gamelist
* Increased the amount of arguments for the custom event scripts from two to four
* Added the system name and full system name as additional arguments to the game-start and game-end custom events
* Lowered the minimum supported screen resolution from 640x480 to 224x224 to support arcade cabinet displays such as those running at 384x224 and 224x384
* Removed the ResidualVM system as it has been merged with ScummVM
* Added support for the Commodore VIC-20, Epic Games Store, Google Android, Java 2 Micro Edition, Philips CD-i and Symbian systems
* Added emulator configurations for the Microsoft Xbox, Microsoft Xbox 360 and Sony PlayStation 3 systems
* Added support for a more advanced system view carousel logo placeholder (for unthemed systems) by allowing the combination of text and graphics
* Expanded the themeable options for "helpsystem" to support custom button graphics, dimmed text and dimmed icon colors, upper/lower/camel case and custom spacing
* Made the scrolling speed of ScrollableContainer more consistent across various screen resolutions and display aspect ratios
* Decreased the amount of text that ScrollableContainer renders above and below the starting position as content is scrolled
* Made the game name and description stop scrolling when running the media viewer, the screensaver or when running in the background while a game is launched
* Added notification popups when plugging in or removing controllers
* Made large optimizations to the SVG rendering which reduces application startup time dramatically when many systems are populated
* Changed to loading the default theme set rbsimple-DE instead of the first available theme if the currently configured theme set is missing
* Added support for displaying the left and right trigger buttons in the help prompts
* Removed the "Choose" entry from the gamelist view help prompts
* Replaced a number of help prompt hacks with proper solutions
* Changed the "Toggle screensaver" help entry in the system view to simply "Screensaver"
* Changed the font size for the custom collection deletion screen to use the same size as all other menus
* Added support for upscaling bitmap images using linear filtering
* Changed the marquee image upscale filtering from nearest neighbor to linear for the launch screen and gamelist views
* Made the window corners slightly more rounded
* Moved the Media viewer and Screensaver settings higher in the UI settings menu
* Moved the game media directory setting to the top of the Other settings menu, following the new Alternative emulators entry
* Moved the ScreenScraper account toggle to the bottom of the scraper account settings menu
* Lowered the default volumes slightly for videos and navigation sounds
* Added loading of the System view to the ViewController preload function to decrease theme extras texture pop-in
* Disabled the application startup animations on macOS as they were very choppy and looked bad after moving to SDL 2.0.18
* Changed the filter description "Text filter (game name)" to simply "Game name" and a keyboard icon
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
* Made all dependencies build in-tree on macOS instead of having to rely on Homebrew-supplied libraries
* Added a script to generate an AppImage on Linux
* Lots of other general code refactoring
* Increased the warning level for Clang/LLVM and GCC by adding -Wall, -Wpedantic and some additional flags
* Fixed a lot of compiler warnings introduced by the -Wall and -Wpedantic flags
* Changed the language standard from C++14 to C++17
* Increased the minimal required compiler version to 5.0.0 for Clang/LLVM and 7.1 for GCC
* Added CMake options to build with AddressSanitizer, ThreadSanitizer and UndefinedBehaviorSanitizer
* Changed two clang-format rules related to braced lists and reformatted the codebase
* Upgraded the bundled SDL version 2.0.14 to 2.0.18 on Windows and macOS
* Bundled the October 2021 release of the Mozilla TLS/SSL certificates
* Updated the MAME index files to include ROMs up to MAME version 0.237
* rbsimple-DE: Added some missing graphics for the xbox360 system
* rbsimple-DE: Improved existing graphics for the dos, pc and scummvm systems
* rbsimple-DE: Updated the info text for most systems

### Bug fixes

* Single-scraping a game, aborting and then re-scraping without leaving the metadata editor would sometimes lead to a crash
* Setting a really small font size in a theme would crash the application
* Text containing invalid (partial) Unicode characters could crash the application
* Deleting the last custom collection could crash the application if the grouped "collections" system was set as the startup gamelist
* Connecting a controller with buggy drivers could crash the application
* Setting an invalid UIMode value in the configuration file could crash the application
* Setting an invalid scraper service value in the configuration file could crash the application
* When scraping in interactive mode with "Auto-accept single game matches" enabled, the game name could not be refined if there were no games found
* When scraping in interactive mode, the game counter was not decreased when skipping games, making it impossible to skip the final games in the queue
* When scraping in interactive mode, "No games found" results could be accepted using the "A" button
* When scraping in interactive mode, any refining done using the "Y" button shortcut would not be shown when doing another refine using the "Refine search" button
* When scraping in interactive mode, the first result row would get focused after the search completed even if the cursor was moved to a button beneath the list
* The multi-scraper did not update the filter index
* Multi-scraping and aborting before any games were fully scraped but after some game media was downloaded did not trigger a gamelist reload
* (Windows) Launching a game that changed the screen resolution would offset the ES-DE application window when exiting
* (Windows) Enabling the option to hide the taskbar would sometimes not focus the application window on startup (possibly only an issue on Windows 8.1)
* If there were gamelist.xml entries for existing files whose extensions were not setup in es_systems.xml, these would still get loaded and displayed
* Fixed multiple minor rendering issues where graphics would be slightly cut off or incorrectly sized
* Under some circumstances ScrollableContainer (used for the game descriptions) would contain a partially rendered bottom line
* If the TextListComponent height was not evenly dividable by the font height + line spacing, a partial bottom row would get rendered
* The line spacing for TextListComponent was incorrectly calculated for some resolutions such as 2560x1440
* Fixed multiple issues with scaling of images which led to various inconsistencies and sometimes cut-off graphics
* The system time zone was not taken into consideration when using the Unix epoch which lead to various strange problems in the metadata editor
* Removing games from custom collections did not remove their filter index entries
* Enabling the All Games collection lead to a potentially large memory leak under some circumstances
* Input consisting of only whitespace characters would get accepted by TextEditComponent which led to various strange behaviors
* Leading and trailing whitespace characters would not get trimmed from the ROM directory when entering this during initial setup
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
* The launch screen text had ugly scaling artifacts if the menu opening effect was set to "Scale-up"
* The custom collection deletion screen had incorrect row heights when running at lower resolutions such as 1280x720
* If there was an abbreviated full system name for the "Gamelist on startup" option, that abbreviation would also get displayed when opening the selector window
* Really long theme set names would not get abbreviated in the UI settings menu, leading to a garbled "Theme set" setting row
* Disabling a collection while its gamelist was displayed would lead to a slide transition from a black screen if a gamelist on startup had been set
* When marking a game to not be counted in the metadata editor and the game was part of a custom collection, no collection disabling notification was displayed
* When running really low on texture memory, the menu texture would not get rendered correctly
* At low screen resolutions, logos on the System view carousel would sometimes jump down a pixel when scaling down
* There was a tiny and randomly occuring gap between the system carousel and systemInfo bar during slide transitions between the System and Gamelist views
* The "no games" dialog did not have correct line wrapping when running at 1280x1024
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

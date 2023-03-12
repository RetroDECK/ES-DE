# EmulationStation Desktop Edition (ES-DE) - Release roadmap

This roadmap is under constant review so expect it to change from time to time.

A more detailed breakdown can be found on the [Kanban](https://gitlab.com/es-de/emulationstation-de/-/boards) board, and for previous releases the [Changelog](CHANGELOG.md) contains all relevant details.

#### _v1.0_

* _New gamelist sorting and media handling logic_
* _Overhaul of the menu system_
* _Scraping of additional image types as well as videos_
* _OpenGL shader support for blurred backgrounds, scanline rendering etc._
* _Navigation sound support_
* _A comprehensive theme set bundled with the application_
* _Ports for Windows, macOS and BSD Unix_
* _Preconfigured systems configuration files for all supported operating systems_
* _A detailed user guide_

#### _v1.1_

* _Mix image generation based on screenshots, 3D boxes and marquee files (as in Skyscraper)_
* _Ability to display game media in full screen from the gamelist view_
* _Proper game launching screen_
* _New FFmpeg-based video player_
* _Better method to find installed emulators and cores_
* _Move to the SDL2 GameController API_
* _Different button graphics and names applied depending on controller type (Xbox, PlayStation or SNES style)_

#### _v1.2_

* _Support for pre-defined alternative emulators and cores (configured in es_systems.xml)_
* _Badges highlighting things like favorite games, completed games etc. (will require theme support)_
* _Virtual (on-screen) keyboard_
* _Support for the Raspberry Pi 4 (Raspberry Pi OS)_
* _Improve fullscreen support and make game launching more seamless, remove the temporary fullscreen hacks_
* _Add GLM library dependency for matrix and vector operations, decommission the built-in functions_
* _AppImage and AUR releases on Linux_

#### v2.0 (in development)

* _New theme engine with generalized views (only System and Gamelist) and theme variants support_
* _Multiple new components (carousel support for the Gamelist view, grid component etc.)_
* _Support for Tate mode, i.e. vertical screen orientation for arcade cabinets and similar_
* _Lottie animation (vector graphics) and GIF animation support_
* _OpenGL ES 3.0 renderer for use on the Raspberry Pi_
* _Replace the OpenGL fixed function pipeline renderer with a shader-based renderer_
* _Replace NanoSVG with a more capable SVG rendering library_
* _Improve text and font functions, e.g. dynamic texture allocation and faster and cleaner text wrapping_
* _Improve GLSL shader post-processing performance_

#### v2.1

* Theme downloader
* Theme engine composite element support for enabling advanced and finely controlled layouts
* Theme engine element animation support (storyboards)
* New texture/cache manager with improved memory management and performance
* Better and more accurate RAM and VRAM usage statistics
* Scraping of game manuals and maps plus a viewer for these (with PDF and raster image support)
* Support for additional scraper services
* Checksum support for the scraper for exact searches and for determining when to overwrite files
* RetroAchievements.org integration

#### v2.2

* Removal of legacy theme support
* Background music
* Proper audio mixer
* Controller button mappings from inside ES-DE (similar to pad2key in Batocera)
* Reorganize the menus, possibly adding basic/advanced modes
* Reduced amount of gamelist reloading to retain cached textures and improve overall performance
* Add "time played" counter per game, similar to how it works in Steam
* Replace the built-in Unicode functions and lookup tables with those of the ICU library
* Add text kerning support using the HarfBuzz library

#### v2.3

* Vulkan renderer for all supported operating systems
* Use of MoltenVK to get Metal support on macOS
* Localization/multi-language support
* Bulk metadata editor
* Scrollbar component for the gamelist view which can be used by themes
* Simple file browsing component
* Web proxy support for the scraper
* Improve multi-threading

#### v2.4

* Mouse/touch screen support
* Animated menu elements like switches and tick boxes
* Migration tools for importing game metadata and media from other frontend applications
* Audit tools to clean up orphaned gamelist entries, media files etc.
* Auto-import tools for Steam, Lutris etc.
* Replace the abandoned FreeImage library

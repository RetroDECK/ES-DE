# EmulationStation Desktop Edition (ES-DE) - Contributing

### Help needed

Contributions are very much appreciated as there are many things that need to be implemented and improved.

There are basically three areas where help is needed:

* Programming: If you are experienced with developing cross-platform client applications in C++ then ES-DE could be an interesting project! Although the application does not have a huge codebase, it is still fairly complex as it covers many different areas like OpenGL rendering, audio playback, video playback, controller input, network access, XML and JSON parsing etc. To work on the project you need to be able to test your code on Linux, macOS and Windows, but apart from that it should hopefully be fairly straightforward.

* Graphics: There are a few different areas where help is needed, but primarily it would be to create game systems vector graphics for the default rbsimple-DE theme as it's missing a lot of assets today. Check out [MISSING.md](themes/rbsimple-DE/MISSING.md) for more details about this.

* Emulator configuration and testing: As one of the main goals of ES-DE is easy setup, the application needs to ship with all game systems preconfigured. This means that the es_systems.xml files have to be populated with both primary and alternative emulators for all supported operating systems. There could also be a need to add additional platforms that ES-DE does not support today. Please refer to [USERGUIDE-DEV.md](USERGUIDE-DEV.md#supported-game-systems) for the current game systems status. You can also review the es_systems.xml files directly: [unix/es_systems.xml](resources/systems/unix/es_systems.xml), [macos/es_systems.xml](resources/systems/macos/es_systems.xml) and [windows/es_systems.xml](resources/systems/windows/es_systems.xml).

Another specific area where help is needed is to research and potentially develop a usable web version of ES-DE. A proof of concept compilation to WebAssembly (using Emscripten) has been done and the application actually runs somehow correctly in a browser. But it needs to be investigated whether games/emulators can actually be launched when running in this environment and there are many improvements to be completed before the WebAssembly build is usable.

The ES-DE development is tracked using a Kanban board which is publicly visible at the GitLab project site:

[https://gitlab.com/leonstyhre/emulationstation-de/-/boards](https://gitlab.com/leonstyhre/emulationstation-de/-/boards)

Development takes place in the `master` branch, and bug fixes/point releases are handled in the `stable` branch.

Only the latest stable version is maintained.

You can contact me (Leon) via email, either at info@es-de.org or alternatively using the address I use for my code commits.

### Release roadmap

The roadmap is under constant review so expect it to change from time to time. Still it should give some feeling for which direction to move and what to work on first. These are only the larger topics, there are of course many smaller changes and improvements in addition to these. The plans for previous releases are shown in italics.

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

#### v1.3 (in progress)

* New theme engine with generalized views (only System and Gamelist) and theme variants support
* Multiple new gamelist components (wheels, wall/grid etc.)
* Lottie animation (vector graphics) support
* Checksum support for the scraper for exact searches and for determining when to overwrite files

#### v1.4

* Animated menu elements like switches, tick boxes, smooth scrolling etc.
* Add scraping of game manuals and maps and create a viewer for these (with PDF, GIF, JPG and PNG support)
* Scrollbar component for the gamelist view which can be used by the themes
* Support for additional scraper services (if feasible?)
* Web proxy support for the scraper
* RetroAchievements.org integration
* Add "time played" counter per game, similar to how it works in Steam

#### v1.5

* Reorganize the menus, possibly adding basic/advanced modes
* Localization/multi-language support
* Simple file browsing component
* New texture/cache manager with support for SVG images and Lottie animations
* Support for portrait orientation, e.g. for Tate Mode arcade cabinets
* Improved text and font functions, e.g. faster and cleaner line wrapping and more exact sizing

#### v1.6

* Vulkan renderer for all supported operating systems
* Dependency on MoltenVK to get Metal support on macOS
* Decommission of the OpenGL 2.1 and OpenGL ES renderers (or keep as legacy mode/legacy build?)
* Better and more accurate GPU and memory usage statistics overlay
* Improve the performance of the GLSL shader code
* Bulk metadata editor
* Improve multi-threading

#### v1.7

* Migration tools for importing game metadata and media from other frontend applications
* Audit tools to clean up orphaned gamelist entries, media files etc.
* Auto-import tools for Steam, Lutris etc.
* Replacements for the abandoned NanoSVG and FreeImage libraries

To see which features have been implemented in previous versions, please refer to [CHANGELOG.md](CHANGELOG.md).

### Coding style

The visual appearance aspect of the coding style is automatically applied using clang-format, so to understand the exact formatting rules, refer to the .clang-format file in the root of the repository.

Due to this approach, all code changes must be auto-formatted before they are committed. You can read in [INSTALL.md](INSTALL.md#using-clang-format-for-automatic-code-formatting) how clang-format is installed and used.

But as clang-format won't change the actual code content or fix all code style choices, here are some additional key points:

* Always write comments in C++ style, i.e. `//` instead of `/* */`
* Comments should be proper sentences, starting with a capital letter and ending with a dot
* As a general rule, use C++ syntax instead of C syntax, for example `static_cast<int>(someFloatVariable)` instead of `(int)someFloatVariable`
* Always declare one variable per line, never combine multiple declarations of the same type
* Name member variables starting with an `m` such as `mMyMemberVariable` and name static variables starting with an `s` such as `sMyStaticVariable`
* Use braced initializations when possible, e.g. `float myFloat{1.5f}` as this is generally the safest way to do it
* Short function definitions can be placed in either the .h or .cpp file depending on the situation
* Avoid overoptimizations, especially if it sacrifices readability, makes the code hard to expand on or is error prone
* Try to be coherent with the existing codebase regarding names, structure etc., it should not be obvious what person wrote which parts
* For the rest, check the code and have fun :)

### Building and configuring

Please refer to [INSTALL.md](INSTALL.md) and [INSTALL-DEV.md](INSTALL-DEV.md) for details on everything related to building ES-DE.

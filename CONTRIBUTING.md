# EmulationStation Desktop Edition (ES-DE) - Contributing

### Help needed

Contributions are very much appreciated as there are many things that need to be implemented and improved.

Code commits is only one part of it, as work is also needed on the rbsimple-DE theme set as well as thorough application testing. It's impossible for me to test every game system as ES-DE has support for well over a 100 different platforms.

And a thorough review of [unix/es_systems.xml](resources/systems/unix/es_systems.xml), [macos/es_systems.xml](resources/systems/macos/es_systems.xml) and [windows/es_systems.xml](resources/systems/windows/es_systems.xml) would be great as these templates are not fully populated yet. For some of the game systems there are only placeholder entries, mostly for platforms that RetroArch does not support and where discrete emulators are required.

As for rbsimple-DE there are quite some missing graphic files and other customizations for a number of game systems. Check out [MISSING.md](themes/rbsimple-DE/MISSING.md) for more details on what needs to be added or updated. Note that although rbsimple-DE is based on Recalbox Multi, it's only possible to use assets from this theme set created before its change to a more restrictive license, which happened in 2018. As such it's unfortunately necessary to recreate all the system graphics from scratch that have since been released by the Recalbox community.

Another area where help is really needed is for creation of installation instruction videos and similar. There are some rudimentary videos available at the ES-DE [YouTube channel](https://www.youtube.com/channel/UCosLuC9yIMQPKFBJXgDpvVQ), but proper videos with voice-over and similar are needed to help especially new ES-DE users. However the videos don't need to be located at this YouTube channel. I have zero interest in creating videos or maintaining a channel of my own, there's simply a need to have video instructions available somewhere. So if you can create good videos and prefer to use your own YouTube channel, I will link to them from this repository and from https://es-de.org.

The ES-DE development is tracked using a Kanban board which is publicly visible at the GitLab project site:

[https://gitlab.com/leonstyhre/emulationstation-de/-/boards](https://gitlab.com/leonstyhre/emulationstation-de/-/boards)

Development takes place in the `master` branch, and bug fixes/point releases are handled in the `stable` branch.

Only the latest stable version is maintained.

You can contact me (Leon) via email, either at info@es-de.org or alternatively using the address I use for my code commits.

### High level release plan

This plan is under constant review so expect it to change from time to time. Still it should give some feeling for which direction to move and what to work on first. These are only the larger topics, there are of course many smaller changes and improvements in addition to these.

#### v1.1

* Mix image generation based on screenshots, 3D boxes and marquee files (as in Skyscraper)
* Ability to show game media in full screen from the gamelist view
* Proper game launching screen
* New FFmpeg-based video player
* Move to the SDL2 GameController API
* Different button graphics and names applied depending on controller type (Xbox, PlayStation and SNES style)
* Add to the Debian repository

#### v1.2

* Support for pre-defined alternative emulators and cores (configured in es_systems.xml)
* Badges highlighting things like favorite games, completed games etc. (will require theme support)
* Improved full-screen support, removing the temporary full-screen hacks
* On-screen keyboard
* Support for the Raspberry Pi 4 with OpenGL ES 2.0 and GLSL shaders (Raspberry Pi OS)
* Add GLM library dependency for matrix and vector operations, start to decommission the built-in functions
* Add to more Linux repositories, BSD ports collections etc.
* Flatpak and Snap releases on Linux

#### v1.3

* Localization/multi-language support
* Overhaul of the theme handling, adding capabilities and improving compatibility with Recalbox and Batocera themes
* Scrapping the Grid view style and adding a general grid/wall component instead
* Checksum support for the scraper for exact searches and for determining when to overwrite files
* Improved text and font functions, e.g. faster and cleaner line wrapping and more exact sizing

#### v1.4

* Authoring tools to clean up orphaned gamelist entries, media files etc.
* Scrollbar component for the gamelist view which can be used by the themes
* Web proxy support for the scraper
* Add "time played" counter per game, similar to how it works in Steam
* Preload all built-in resources and never clear them from the cache
* Improved multi-threading

#### v1.5

* Bulk metadata editor
* Simple file browsing component
* Improve the performance of the GLSL shader code
* Animated menu elements like switches, tick boxes, smooth scrolling etc.
* Support for additional scraper services (if feasible?)
* Support for portrait orientation, e.g. for Tate Mode arcade cabinets

#### v2.0

* Vulkan renderer for all supported operating systems
* Dependency on MoltenVK to get Metal support on macOS
* Decommission of the OpenGL 2.1 and OpenGL ES renderers (or keep as legacy mode/legacy build?)
* Better and more accurate GPU and memory usage statistics overlay

#### v2.1

* Migration tools for importing game metadata and media from other front-end applications
* Auto-import tools for Steam, Lutris etc.

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
* Short function definitions can be placed in either the .h or .cpp file depending on the situation
* Avoid overoptimizations, especially if it sacrifices readability, makes the code hard to expand on or is error prone
* Try to be coherent with the existing codebase regarding names, structure etc., it should not be obvious what person wrote which parts
* For the rest, check the code and have fun :)

### Building and configuring

Please refer to [INSTALL.md](INSTALL.md) for details on everything related to building ES-DE.

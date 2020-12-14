Contributing to EmulationStation Desktop Edition (ES-DE)
========================================================


### Help needed:

Contributions to ES are very much appreciated as there are many things that need to be implemented and improved. Code commits is only one part of it, as work is also needed on the rbsimple-DE theme as well as thorough application testing.

A review of the CMake configuration files would also be helpful. Although the building and package generation works more or less correctly, there are some hacks and similar in the CMakeLists.txt files that need improving by someone who really knows how this software package works.

Work is also needed to get ES-DE into the repositories of the various supported operating systems. For example Debian, Fedora, FreeBSD, NetBSD and OpenBSD. This is an area where I have no experience so help with this would be fantastic.

Regarding testing, it's impossible for me to test every game system as rbsimple-DE has support for more than a 100 different systems. There could be issues with the configuration template files, or within ES itself. So more testing is needed!

In general, a thorough review of [es_systems.cfg_linux](resources/templates/es_systems.cfg_linux), [es_systems.cfg_macos](resources/templates/es_systems.cfg_macos) and [es_systems.cfg_windows](resources/templates/es_systems.cfg_windows) would be great!

To a lesser extent, a review would also be needed of [es_systems.cfg_freebsd](resources/templates/es_systems.cfg_freebsd), [es_systems.cfg_netbsd](resources/templates/es_systems.cfg_netbsd) and [es_systems.cfg_openbsd](resources/templates/es_systems.cfg_openbsd) but these are really minority systems and mostly a copy of the Linux templates with modified paths to the RetroArch cores.

As for rbsimple-DE there are quite some missing graphic files and other customizations for a number of game systems. Check out [MISSING.md](themes/rbsimple-DE/MISSING.md) for more details of what needs to be added or updated.

### High level release plan

This plan is under constant review so expect it to change from time to time. Still it should give some feeling for which direction to move and what to work on first. These are only the larger topics, there are of course many smaller changes and improvements in addition to these.

#### v1.1

* Support for Raspberry Pi 4 (Ubuntu and Raspberry Pi OS)
* Mix image generation based on screenshots, 3D boxes and marquee files (as in Skyscraper)
* Improve full-screen support, remove the temporary full screen hacks
* Proper game launching screen
* Ability to show game media in full screen from the gamelist view
* Checksum support for the scraper to verify each file before accepting/saving it
* Move to the SDL2 GameController API

#### v1.2

* Decals highlighting things like favorite games, completed games etc. (will require theme support)
* On-screen keyboard
* Different button graphics and names applied depending on controller type (Xbox, PlayStation and SNES style)
* Web proxy support for the scraper
* Support for pre-defined alternative emulators and cores (configured in es_systems.cfg)
* Add GLM library dependency for matrix and vector operations, decommissioning the built-in functions
* Add to Linux repositories, BSD ports collections etc.

#### v1.3

* Internationalization/multi-language support
* Complete overhaul of the grid view style
* A nice and useful grid view implementation in rbsimple-DE
* Better shader support (more adaptive to screen resolution, higher performance, cleaner code)
* Improve text and font functions, e.g. faster and cleaner line wrapping code
* Flatpak and Snap support on Linux

#### v1.4

* Authoring tools to clean up orphaned gamelist entries, media files etc.
* Simple file browsing component
* Requests per minute limitation setting for the scraper
* Add 'time played' counter per game, similar to how it works in Steam
* Preload all built-in resources and never clear them from the cache
* Improved multi-threading

#### v1.5

* Overhaul of the GUI element scaling and placement logic to make ES-DE look more consistent across different resolutions
* Scrollbars for menus and gamelists
* Animated menu elements like switches, tick boxes, smooth scrolling etc.
* Possibly replace libVLC with FFmpeg
* Support for additional scraper services (if feasible?)

#### v2.0

* Vulkan renderer for all supported operating systems
* Dependency on MoltenVK to get Metal support on macOS
* Decommission of the OpenGL 2.1 and OpenGL ES renderers (or keep as legacy mode/legacy build?)
* Better and more accurate GPU and memory usage statistics overlay

#### v2.1

* Migration tools for importing game metadata and media from other front-end applications
* Auto-import tools for Steam, Lutris etc.

To see which features have been implemented in previous versions, refer to [NEWS.md](NEWS.md).

### Coding style:

The coding style for EmulationStation-DE is mostly a combination of the Linux Kernel style (although that's C it's close enough to C++ as far as I'm concerned) and Google's C++ guidelines.

Please refer to these documents here:

https://www.kernel.org/doc/html/v4.10/process/coding-style.html \
https://google.github.io/styleguide/cppguide.html

**Some key points:**

* Column width (line length) is 100 characters
* Indentation is 4 spaces, don't use tabs as they can be interpreted differently
* Line break is Unix-style (line feed only, no carriage return)
* Do not leave trailing whitespaces at the end of the lines (a good source code editor should have a setting to automatically trim these for you)
* When breaking up long lines into multiple lines, consider what could be useful data to grep for so you don't break in the middle of such a string
* Comments always in C++ style, i.e. // instead of /* */
* Comments should be proper sentences, starting with a capital letter and ending with a dot
* Use K&R placements of braces, read the Linux Kernel coding style document for clarifications
* Always use spaces between keywords and opening brackets, i.e. `if ()`, `for ()`, `while ()` etc.
* Indentation of switch/case statements is optional, but it's usually easier to read the code with indentations in place
* Use `std::string` or `std::vector<char>` instead of `char *` or `char []` unless there is a specific reason requiring the latter
* Actually, try to use C++ syntax in general instead of C syntax, another example would be `static_cast<int>(someFloatVariable)` instead of `(int)someFloatVariable`
* If the arguments (and initializer list) for a function or class exceeds 4 items, arrange them vertically to make the code easier to read
* Always declare one variable per line, never combine multiple declarations of the same type
* Name local variables with the first word in small letters and the proceeding words starting with capital letters, e.g. myExampleVariable
* Name member variables starting with a small 'm', e.g. mMyMemberVariable
* Use the same naming convention for functions as for local variables, e.g. someFunction()
* Inline functions makes perfect sense to use, but don't overdo it by using them for functions that won't be called very frequently
* Never put more than one statement on a single line (there are some exceptions though like lambda expressions and possibly switch statements)
* Avoid overoptimizations, especially if it sacrifices readability, makes the code hard to expand on or is error prone
* For the rest, check the code and have fun! :)

### Building and configuring:

Please refer to the [INSTALL.md](INSTALL.md) file for details on everything related to building EmulationStation Desktop Edition.

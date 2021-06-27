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
* Replace libVLC with FFmpeg
* Move to the SDL2 GameController API
* Different button graphics and names applied depending on controller type (Xbox, PlayStation and SNES style)
* Add to Linux repositories, BSD ports collections etc.

#### v1.2

* Support for Raspberry Pi 4 with OpenGL 2.1 (Ubuntu and Raspberry Pi OS)
* Improve full-screen support, remove the temporary full screen hacks
* Badges highlighting things like favorite games, completed games etc. (will require theme support)
* On-screen keyboard
* Web proxy support for the scraper
* Add GLM library dependency for matrix and vector operations, decommissioning the built-in functions

#### v1.3

* Localization/multi-language support
* Checksum support for the scraper for exact searches and for determining when to overwrite files
* Complete overhaul of the grid view style
* A nice and useful grid view implementation in rbsimple-DE
* Improved text and font functions, e.g. faster and cleaner line wrapping
* Flatpak and Snap support on Linux

#### v1.4

* Authoring tools to clean up orphaned gamelist entries, media files etc.
* Support for pre-defined alternative emulators and cores (configured in es_systems.xml)
* Simple file browsing component
* Add 'time played' counter per game, similar to how it works in Steam
* Preload all built-in resources and never clear them from the cache
* Improved multi-threading

#### v1.5

* Bulk metadata editor
* Overhaul of the GUI element scaling and placement logic to make ES-DE look more consistent across different resolutions
* Scrollbars for menus and gamelists
* Improve the performance of the GLSL shader code
* Animated menu elements like switches, tick boxes, smooth scrolling etc.
* Support for additional scraper services (if feasible?)

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

The coding style for ES-DE is mostly a combination of the Linux kernel style (although that's C it's close enough to C++ as far as I'm concerned) and Google's C++ guidelines.

Please refer to these documents here:

https://www.kernel.org/doc/html/v4.10/process/coding-style.html \
https://google.github.io/styleguide/cppguide.html

**Some key points:**

* Column width (line length) is 100 characters
* Indentation is 4 spaces, don't use tabs as they can be interpreted differently
* Line break is Unix-style (line feed only, no carriage return)
* Do not leave trailing whitespaces at the end of the lines (a good source code editor should have a setting to automatically trim these for you)
* When breaking up long lines into multiple lines, consider what could be useful data to grep for so you don't break in the middle of such a string
* Comments always in C++ style, i.e. `//` instead of `/* */`
* Comments should be proper sentences, starting with a capital letter and ending with a dot
* Use K&R placements of braces, read the Linux kernel coding style document for clarifications
* Always use spaces between keywords and opening brackets, i.e. `if ()`, `for ()`, `while ()` etc.
* Indentation of switch/case statements is optional, but it's usually easier to read the code with indentations in place
* Use `std::string` or `std::vector<char>` instead of `char *` or `char []` unless there is a specific reason requiring the latter
* Actually, try to use C++ syntax in general instead of C syntax, another example would be `static_cast<int>(someFloatVariable)` instead of `(int)someFloatVariable`
* If the arguments (and initializer list) for a function or class exceeds 4 items, arrange them vertically to make the code easier to read
* Always declare one variable per line, never combine multiple declarations of the same type
* Name local variables with the first word in small letters and the proceeding words starting with capital letters, e.g. `myExampleVariable`
* Name member variables starting with an `m` such as `mMyMemberVariable` and name static variables with an `s` such as `sMyStaticVariable`
* Use the same naming convention for functions as for local variables, e.g. `someFunction()`
* Single-line function definitions are fine to put in the header files, but if it's more than one line, place it in the corresponding .cpp file
* Inline functions makes perfect sense to use, but don't overdo it by using them for functions that won't be called very frequently
* Don't put more than one statement on a single line (there are some exceptions though like lambda expressions and possibly switch statements)
* Avoid overoptimizations, especially if it sacrifices readability, makes the code hard to expand on or is error prone
* For the rest, check the code and have fun! :)

### Building and configuring

Please refer to [INSTALL.md](INSTALL.md) for details on everything related to building ES-DE.

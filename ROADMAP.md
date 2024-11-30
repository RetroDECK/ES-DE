# ES-DE Frontend - Feature roadmap

ES-DE is developed using an agile methodology so which features to include per release is reviewed and adjusted continuously. As such this document is basically a list of the main features that are planned to be added eventually.

A more detailed breakdown can be found on the [Kanban](https://gitlab.com/es-de/emulationstation-de/-/boards) board, and for previous releases the [Changelog](CHANGELOG.md) contains all relevant details.

**General functionality**

* RetroAchievements.org integration
* "Time played" counter per game, similar to how it works in Steam
* Bulk metadata editor
* Background music
* Controller button mappings from inside ES-DE (similar to pad2key in Batocera)
* Auto-import tools for Android apps, Steam, Lutris etc.

**User interface**

* Simple file browsing component

**Theme engine**

* Composite element support for enabling advanced and finely controlled layouts
* Element animation support (storyboards)
* Scrollbar component for the system and gamelist views

**Scraper**

* Support for additional scraper services

**Infrastructure**

* New texture/cache manager with improved memory management and performance
* Better and more accurate RAM and VRAM usage statistics
* Vulkan renderer for all supported operating systems (via MoltenVK on macOS)
* Proper audio mixer
* Improved multi-threading
* Reduced amount of gamelist reloading to retain cached textures and improve overall performance
* Replacement for the FreeImage library

EmulationStation Desktop Edition
================================

EmulationStation Desktop Edition is a cross-platform graphical front-end for emulators with controller and keyboard navigation.

The difference to other versions of EmulationStation is that this fork is intended for use primarily on desktop computers where it's not the primary interface for the computer. As such, this version does not aim to provide full control over emulator settings or emulator button mappings, or include system administration functions and similar. Instead it's assumed that the emulators and the overall environment has been properly configured upfront.

The software comes preconfigured for use primarily with [RetroArch](https://www.retroarch.com), although this can certainly be changed as all emulator settings are fully configurable, even on a per-game basis.

**Help needed:**

Apart from code commits, help is especially needed for thorough testing of the software and for working on the RBSimple-DE theme.

It's impossible for me to test every game system (RBSimple-DE has support for almost a 100 different systems!) so it would be especially useful to hear about any issues with starting games using the default es_systems.cfg configuration file and also if there are any issues regarding scraping for certain systems.

In general, a review of the [es_systems.cfg](resources/templates/es_systems.cfg_unix) file including the supported file extensions would be great!

As for RBSimple-DE there are quite some missing graphic files and other customizations for a number of game systems. \
Check out [MISSING.md](themes/rbsimple-DE/MISSING.md) for more details of what needs to be added or updated.

Finally, if someone could make a proper port to macOS, that would be great as then all of the three major desktop operating systems would be supported. There is some code present specifically for macOS but I've been unable to test it. And according to the code comments, some things seem to be missing altogether for this operating system, so it may take some effort.


General information
===================

[NEWS.md](NEWS.md) contains information about new functionality, improvements and bug fixes. \
(An overview of all previous versions will be included here as well.)

[INSTALL.md](INSTALL.md) provides details on how to build and configure the application.

[DEVNOTES.md](DEVNOTES.md) is the place to go if you're interested in participating in the development of EmulationStation Desktop Edition.


Overview of the software
========================


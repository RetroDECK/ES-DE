# ES-DE Frontend – RetroDECK Custom Build with tiny light tweaks

**Official ES-DE website:** https://es-de.org

It's officially supported on Android, Linux, macOS, Windows and Haiku. There is also an unofficial ES-DE package in the FreeBSD ports collection.

---

## Overview

RetroDECK uses a custom ES-DE build configured for integration with the RetroDECK Flatpak environment.

The build uses the upstream **ES-DE** codebase and the existing `RETRODECK` build flag to enable RetroDECK-specific adjustments. 

Upstream ES-DE development is performed by the ES-DE team. RetroDECK regularly updates its build against upstream changes to remain aligned with the current ES-DE codebase.

---

## RetroDECK-Specific Changes

| Area | RetroDECK Custom Build |
|---|---|
| **Flatpak compatibility** | Adjusts filesystem paths, permissions and runtime dependencies for the Flatpak sandbox. |
| **Menu structure** | Adjusts menu entries to expose RetroDECK-specific features, such as the *RetroDECK Configurator*. |
| **Application updater** | The build system disables the application updater when `RETRODECK` is enabled. |

---

## About upstream ES-DE

The goal of the upstream project is to create a high quality frontend that is easy to use, requires minimal setup and configuration, looks nice, and is available across a wide range of operating systems.

It comes preconfigured for use with a large selection of emulators, game engines, game managers and gaming services and it can also run locally installed games and applications. It's fully customizable, so you can easily expand it with support for additional systems and applications.

You can find the complete list of supported game systems in the [User guide](USERGUIDE.md#supported-game-systems) and in the [Android](ANDROID.md#supported-game-systems), [Linux on AArch64](LINUX-AARCH64.md#supported-game-systems) and [Haiku](HAIKU.md#supported-game-systems) documentation.

**Note:** these are ES-DE's own community channels for the upstream project. They are **not** a support channel for RetroDECK — see [Bug Reports](#bug-reports) below for where to get help with the RetroDECK build.

Reddit:\
https://www.reddit.com/r/ESDE_Frontend

X/Twitter:\
https://x.com/ES_DE_Frontend

Bluesky:\
https://bsky.app/profile/es-de.org

---

## Suggestions and Improvements

For suggestions or improvements to upstream **ES-DE**, use the official GitLab issue tracker:

https://gitlab.com/es-de/emulationstation-de/-/issues

Before opening an issue, review existing reports to avoid duplicates and provide all relevant technical information.

---

## Additional information

[FAQ-ANDROID.md](FAQ-ANDROID.md) -  Frequently Asked Questions specifically for Android

[USERGUIDE.md](USERGUIDE.md) / [USERGUIDE-DEV.md](USERGUIDE-DEV.md) - Comprehensive guide and reference for all application settings

[ANDROID.md](ANDROID.md) / [ANDROID-DEV.md](ANDROID-DEV.md) - Documentation specifically for Android

[LINUX-AARCH64.md](LINUX-AARCH64.md) / [LINUX-AARCH64-DEV.md](LINUX-AARCH64-DEV.md) - Documentation specifically for Linux on AArch64/ARM64

[HAIKU.md](HAIKU.md) - Documentation specifically for Haiku

[INSTALL.md](INSTALL.md) / [INSTALL-DEV.md](INSTALL-DEV.md) - Building from source code and advanced configuration topics

---

## Bug Reports

For issues specific to **RetroDECK**, use the official GitHub issue tracker:

https://github.com/RetroDECK/RetroDECK/issues

---

## Acknowledgements

RetroDECK maintains a positive and cooperative relationship with the upstream ES-DE project and is grateful to the ES-DE team for its continued collaboration and support for so many years, including providing and maintaining the `RETRODECK` custom build option from the very beginning. The ongoing development of ES-DE provides the foundation of many features used by RetroDECK as the main front-end.

---


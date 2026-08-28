# ES-DE Frontend – RetroDECK Custom Build

**Official ES-DE website:** https://es-de.org

---

## Overview

RetroDECK uses a custom ES-DE build configured for integration with the RetroDECK Flatpak environment.

The build uses the upstream **ES-DE** codebase and the existing `RETRODECK` build flag to enable RetroDECK-specific adjustments. 

Upstream ES-DE development is performed by the ES-DE team. RetroDECK regularly updates its build against upstream changes to remain aligned with the current ES-DE codebase.

---

## RetroDECK-Specific Build Changes

| Area | RetroDECK Custom Build |
|---|---|
| **Flatpak compatibility** | Adjusts filesystem paths, permissions and runtime dependencies for the Flatpak sandbox. |
| **Menu structure** | Adjusts menu entries to expose RetroDECK-specific features, such as the *RetroDECK Configurator*. |
| **Application updater** | The upstream build system disables the application updater when `RETRODECK` or `FLATPAK_BUILD` is enabled. |

---

## Suggestions and Improvements

For suggestions or improvements to upstream **ES-DE**, use the official GitLab issue tracker:

https://gitlab.com/es-de/emulationstation-de/-/issues

Before opening an issue, review existing reports to avoid duplicates and provide all relevant technical information.

---

## Bug Reports

For issues specific to **RetroDECK**, use the official GitHub issue tracker:

https://github.com/RetroDECK/RetroDECK/issues

---

## Acknowledgements

RetroDECK maintains a positive and cooperative relationship with the upstream ES-DE project and is grateful to the ES-DE team for its continued collaboration and support for so many years, including providing and maintaining the `RETRODECK` custom build option from the very beginning. The ongoing development of ES-DE provides the foundation of many features used by RetroDECK and as main front-end.

---


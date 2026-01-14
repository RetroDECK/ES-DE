# ES‑DE Frontend – RetroDECK Light Fork

**ES‑DE (EmulationStation Desktop Edition)** is a modern, cross‑platform frontend that enables users to browse and launch games from diverse emulation libraries. It provides a polished UI, extensive controller support, and a flexible configuration system.

## Overview of the RetroDECK Light Fork of ES-DE

Development of core ES‑DE functionality is performed by the upstream ES-DE team. The Light Fork contributes only the necessary patches for Flatpak integration and UI refinements. The RetroDECK team have good cooperation with the ES-DE team.

The **RetroDECK Light Fork** builds upon the upstream ES‑DE codebase, adapting it for seamless operation within a Flatpak sandbox. Key adaptations include:

| Aspect | RetroDECK Light Fork Modification |
|--------|------------------------------------|
| **Flatpak Compatibility** | Adjusted paths, permissions, and runtime dependencies to conform to Flatpak’s confined environment. |
| **Menu Structure** | Reorganized entries to expose RetroDECK‑specific tools such as the *RetroDECK Configurator*. |
| **Feature Set** | Disabled features that conflict with Flatpak restrictions or that are not available in RetroDECK. |
| **Upstream Integration** | All new features and bug fixes originate from the upstream ES‑DE project; the fork merges these changes regularly. |

## Resources

- **Official ES‑DE Website:** <https://es-de.org>


# ES-DE Frontend – RetroDECK Light Fork

**Official ES-DE Website:** https://es-de.org

**ES-DE (EmulationStation Desktop Edition)** 

Is a modern, cross-platform frontend that enables users to browse and launch games from diverse game libraries. It provides a polished user interface, extensive controller support, and a flexible configuration system.

---

## Overview of the RetroDECK Light Fork

Core ES-DE development is performed by the upstream ES-DE team. The RetroDECK Light Fork applies only the minimal, targeted modifications required for Flatpak integration and RetroDECK-specific refinements. The RetroDECK and ES-DE teams maintain a positive and cooperative relationship.

The **RetroDECK Light Fork** builds directly upon the upstream ES-DE codebase and adapts it for seamless operation within a Flatpak sandbox environment.

### Key Alterations from Upstream

All minor source code modifications are conditionally compiled using the `RETRODECK` build flag.

This flag is has been built-in by upstream in ES-DE over many years and allows RetroDECK-specific adjustments to be included only when building the RetroDECK Light Fork.

All new features and bug fixes originate from upstream ES-DE not from RetroDECK. The fork regularly merges upstream changes to remain aligned. 

| Aspect | RetroDECK Light Fork Modification |
|--------|------------------------------------|
| **Flatpak Compatibility** | Adjusted filesystem paths, permissions, and runtime dependencies to comply with Flatpak sandbox constraints. |
| **Menu Structure** | Reorganized menu entries to expose RetroDECK-specific tools such as the *RetroDECK Configurator*. |
| **Feature Set** | Disabled features that conflict with Flatpak restrictions or are not applicable within RetroDECK. |
| **es_systems.xml** / **es_find_rules.xml** | These configuration files contain RetroDECK-specific additions and omissions. |

---

## Suggestions and Improvements

For suggestions or improvements related to upstream **ES-DE**, please use the official GitLab issue tracker:

https://gitlab.com/es-de/emulationstation-de/-/issues

Before opening a new issue, please review existing reports to avoid duplicates and include all relevant technical details.

---

## Bug Reports

For issues specific to **RetroDECK**, please use the official GitHub issue tracker:

https://github.com/RetroDECK/RetroDECK/issues

---

## Thanks

The RetroDECK Team expresses its sincere gratitude to the ES-DE team for their continued collaboration, support, and assistance throughout the years. Their ongoing efforts and upstream development make the RetroDECK Light Fork possible.

---

#!/bin/bash

### Emulator Entry Update Rules for es_systems.xml.
# If the emulator entry in es_systems.xml has changed upstream, follow the update rules outlined below.
#
# If the emulator is:
#
# 1. Already present as a RetroDECK component (active entry):
#    - Verify whether its existing entry needs to be updated.
#    - If upstream has changed the invocation (arguments/flags), update the entry to match upstream's implementation.
#    - Keep the entry active (uncommented).
#
# 2. Already present as a RetroDECK component, but commented out:
#    - Update the entry if necessary to match upstream's implementation, but keep it commented out.
#    - This preserves RetroDECK's deliberate disabling (e.g., standalone emulators incompatible with Flatpak).
#
# 3. Not present as a RetroDECK component:
#    - If it's a standalone emulator: add its entry in a commented-out state.
#    - If it's a RetroArch core: add its entry in an active (uncommented) state, treating it as "already present" since RetroDECK includes RetroArch.
#
# An entry update means verifying how the emulator is invoked and updating the command invocation, arguments, or any other relevant parameters as necessary to match the current emulator implementation.
#
# Special case - MAME configuration paths: RetroDECK's MAME entries include `-inipath /var/config/mame/ini` to point to RetroDECK's Flatpak-sandboxed config paths.
# When merging upstream changes: if upstream removes this path or changes MAME invocation, verify that Flatpak sandbox compatibility is maintained before accepting the change.
# If the change is only to arguments/flags but sandbox paths are intact, update to upstream's arguments while preserving sandbox-specific paths.
#
# Ordering convention: Within each system's <system> block, list ES-DE/upstream entries first, followed by RetroDECK-specific customizations (e.g., alternative cores, RetroDECK-only emulators). This maintains a clear separation between upstream content and RetroDECK additions.


git fetch https://gitlab.com/es-de/emulationstation-de/ "stable-3.5"  # Fetch the latest changes from the remote stable-3.4 branch
git merge FETCH_HEAD  # Merge the fetched changes into your current branch
echo -e "PLEASE CHECK IF ANYTHING IS CHANGED IN:\n-resources/systems/linux/es_find_rules.xml\n-resources/systems/linux/es_systems.xml"
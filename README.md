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

All the changes are done under `#if defined(RETRODECK)` definition in the C++ files, so compiling the application without that flag makes the build identical to the original, with the exception of course of `es_systems.xml` and `es_find_rules.xml` that are hardly customized for RetroDECK only.

## Resources

- **Official ES‑DE Website:** <https://es-de.org>




# ES-DE External Command Interface

This document describes the external command interface (IPC) that allows external applications to trigger actions in ES-DE. This feature is **RetroDECK-only** and requires the `RETRODECK` compile flag.

## Overview

ES-DE exposes a named pipe (FIFO) that accepts plain text commands. This allows external tools like RetroDECK to trigger actions such as rescanning the ROM directory.

## Usage

```bash
echo "RESCAN" > ~/.var/app/net.retrodeck.retrodeck/config/ES-DE/es-de-command.fifo
```


## Available Commands

| Command | Description | Coalescing |
|---------|-------------|------------|
| `RESCAN` | Triggers a complete rescan of the ROM directory | Yes - duplicate commands are merged |
| `rescan_rom_directory` | Alias for RESCAN | Yes |

## Technical Details

### FIFO Location

The FIFO is created at:
- **RetroDECK Flatpak**: `$HOME/.var/app/net.retrodeck.retrodeck/config/ES-DE/es-de-command.fifo`
- Falls back to `/tmp/ES-DE/es-de-command.fifo` if config directory not set

### Protocol

- **Transport**: Named pipe (FIFO)
- **Message format**: Plain text, newline-terminated
- **Command format**: `<COMMAND>\n`
- **Response**: None (fire-and-forget)

## Implementation Details

### Files Added/Modified

1. **es-app/src/CommandServer.h** - Header for the command server (RETRODECK-only)
2. **es-app/src/CommandServer.cpp** - Implementation of the FIFO-based command server (RETRODECK-only)
3. **es-app/src/main.cpp** - Integrated CommandServer event handling (RETRODECK-only)
4. **es-app/src/guis/GuiMenu.cpp** - Safety checks after external process execution (RETRODECK-only)

### Server Implementation Details

- **Threading**: Dedicated server thread reads from FIFO, main thread executes commands
- **Synchronization**: Thread-safe command queue with mutex protection
- **Event handling**: SDL_USEREVENT notifies main thread of pending commands
- **Coalescing**: Duplicate RESCAN commands are automatically merged
- **Error handling**: Robust handling of FIFO reopening, EINTR, and EOF conditions

## Platform Support

| Platform | Support | Notes |
|----------|---------|-------|
| Linux (Flatpak) | ✅ Full | Primary target for RetroDECK |
| Linux (native) | ✅ Full | Requires `-DRETRODECK=ON` |
| macOS | ✅ Should work | Not tested |
| Windows | ❌ None | Unix domain sockets not supported |
| Android | ❌ Disabled | Compile-time exclusion |
| iOS | ❌ Disabled | Compile-time exclusion |

## Compile Instructions

Add the `RETRODECK` flag to your CMake configuration:

```bash
cmake .. -DRETRODECK=ON -DAPPLICATION_UPDATER=OFF
make -j$(nproc)
```

## Security Considerations

- The FIFO is created in the user's configuration directory
- The FIFO is removed when ES-DE exits normally
- No authentication is currently implemented (relies on filesystem permissions)

## Troubleshooting

### Command not executing

- Verify ES-DE was compiled with `-DRETRODECK=ON`
- Check that the FIFO path matches between client and server
- Ensure ES-DE has finished loading (FIFO is created after startup completes)
- Check ES-DE logs for "CommandServer" messages

### Rescan doesn't start immediately

- ES-DE may be in a state where it can't perform a rescan (e.g., during another operation)
- The command is queued and will execute when ES-DE is ready
- If ES-DE was in the background, the command executes when it regains focus
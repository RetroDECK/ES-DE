# ES-DE External Command Interface

This document describes the external command interface (IPC) that allows external applications to trigger actions in ES-DE. This feature is **RetroDECK-only** and requires the `RETRODECK` compile flag.

## Overview

ES-DE exposes a Unix domain socket that accepts JSON-formatted commands. This allows external tools like RetroDECK to trigger actions such as rescanning the ROM directory.

## Usage

### Using socat (recommanded / netcat should work the same)

```bash
printf '{"command":"rescan_rom_directory"}\n' | socat - UNIX-CONNECT:~/.var/app/net.retrodeck.retrodeck/config/ES-DE/es-de-command.sock
```

### Using Python (works everywhere)

```python
import socket
import json

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect('/home/user/.var/app/net.retrodeck.retrodeck/config/ES-DE/es-de-command.sock')
sock.send(b'{"command":"rescan_rom_directory"}')
print(sock.recv(1024).decode())
sock.close()
```

## Available Commands

| Command | Description | Response |
|---------|-------------|----------|
| `rescan_rom_directory` | Triggers a complete rescan of the ROM directory | `{"status":"success","message":"Rescan command queued"}` |

## Technical Details

### Socket Location

The socket is created at:
- **RetroDECK Flatpak**: `$HOME/.var/app/net.retrodeck.retrodeck/config/ES-DE/es-de-command.sock`
- Falls back to `$HOME/.emulationstation/ES-DE/es-de-command.sock` if config directory not set

### Protocol

- **Transport**: Unix domain socket (stream)
- **Message format**: JSON (raw, not HTTP)
- **Request format**: `{"command":"<command_name>"}`
- **Response format**: `{"status":"<success|error>","message":"<description>"}`

## Implementation Details

### Files Added/Modified

1. **es-app/src/CommandServer.h** - Header for the command server (RETRODECK-only)
2. **es-app/src/CommandServer.cpp** - Implementation of the Unix socket server (RETRODECK-only)
3. **es-app/src/views/ViewController.h** - Added `RescanResult` struct and `performRescan()` method (RETRODECK-only)
4. **es-app/src/views/ViewController.cpp** - Refactored rescan logic into `performRescan()` (RETRODECK-only)
5. **es-app/src/main.cpp** - Integrated CommandServer startup/shutdown and event handling (RETRODECK-only)
6. **es-app/CMakeLists.txt** - Added new source files to build


### Server Implementation Details

- **Connection handling**: Uses `select()` with 200ms timeout for efficient polling
- **CPU usage**: Should be Zero when idle, immediate response when connection arrives
- **Event code**: Uses SDL_USEREVENT code 100 (reserved for external commands)
- **Memory safety**: Proper cleanup on event queue failure

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

- The socket is created in the user's configuration directory
- The socket is removed when ES-DE exits normally
- No authentication is currently implemented (relies on filesystem permissions)
- Socket permissions are inherited from the parent directory (typically 0755)

## Troubleshooting

### "Invalid JSON" error with curl

Curl sends HTTP headers which the server doesn't understand. Use `nc` or `socat` instead:

```bash
# Wrong - sends HTTP headers
curl --unix-socket ...

# Correct - sends raw JSON
printf '{"command":"rescan_rom_directory"}\n' | nc -U ...
```

### Command not executing

- Verify ES-DE was compiled with `-DRETRODECK=ON`
- Check that the socket path matches between client and server
- Ensure ES-DE has finished loading (socket is created after startup completes)

### Rescan doesn't start
- ES-DE may be in a state where it can't perform a rescan (e.g., during another operation)
- The command is queued and will execute when ES-DE is ready
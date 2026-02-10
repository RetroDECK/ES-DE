//
//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  CommandServer.h
//
//  Provides a named pipe (FIFO) interface for external commands.
//

#pragma once

#if defined(RETRODECK)

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <SDL_stdinc.h>

class CommandServer {
public:
    static CommandServer* getInstance();

    CommandServer();
    ~CommandServer();

    // Start the command server on a named pipe (FIFO)
    bool start();

    // Stop the command server
    void stop();

    // Check if the server is running
    bool isRunning() const { return m_running; }

    // Get the FIFO path
    std::string getFifoPath() const;

    // Get the SDL user event type registered by this server
    static Uint32 getSDLUserEventType() { return s_sdlEventType; }

    // Execute any pending commands (call from main thread only)
    void executePendingCommands();

    // Set a path override for the next game launch (called by command handlers)
    void setPathOverride(const std::string& path);

    // Get and consume the path override (atomic read-and-clear)
    // Returns std::nullopt if no override is set
    std::optional<std::string> consumePathOverride();

private:
    // Command handler with optional payload support
    using CommandHandler = std::function<void(const std::string& payload)>;
    
    struct CommandInfo {
        CommandHandler handler;
        bool coalesce;
    };

    // Payload separator - " ::" is chosen because:
    // - Space prevents collision with paths starting with colons
    // - Double colon is distinct and unlikely to appear in normal text
    // - Safe for echo and shell usage
    static constexpr const char* PAYLOAD_SEPARATOR = " ::";

    void serverThreadFunc();
    void processCommand(const std::string& command);
    void executeCommand(const std::string& command);
    void initializeCommandRegistry();
    void registerCommand(const std::string& name, CommandHandler handler, bool coalesce = false);
    std::string trimWhitespace(const std::string& str) const;
    std::pair<std::string, std::string> parseCommandWithPayload(const std::string& rawCommand) const;

    std::thread m_serverThread;
    std::atomic<bool> m_running;
    mutable std::mutex m_queueMutex;
    std::vector<std::string> m_commandQueue;
    std::unordered_map<std::string, CommandInfo> m_commandRegistry;
    int m_fifoFd;
    static Uint32 s_sdlEventType;
    static std::once_flag s_sdlInitFlag;

    // Path override for MODIFYROMPATH command
    mutable std::mutex m_pathOverrideMutex;
    std::optional<std::string> m_pendingPathOverride;

    static constexpr const char* FIFO_NAME = "es-de-command.fifo";
};

#endif // defined(RETRODECK)
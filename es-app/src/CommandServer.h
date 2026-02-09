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
#include <SDL_stdinc.h>
#include <mutex>

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

private:
    using CommandHandler = std::function<void()>;
    
    struct CommandInfo {
        CommandHandler handler;
        bool coalesce;
    };

    void serverThreadFunc();
    void processCommand(const std::string& command);
    void executeCommand(const std::string& command);
    void initializeCommandRegistry();
    void registerCommand(const std::string& name, CommandHandler handler, bool coalesce = false);
    std::string trimWhitespace(const std::string& str) const;

    std::thread m_serverThread;
    std::atomic<bool> m_running;
    mutable std::mutex m_queueMutex;
    std::vector<std::string> m_commandQueue;
    std::unordered_map<std::string, CommandInfo> m_commandRegistry;
    int m_fifoFd;
    static Uint32 s_sdlEventType;
    static std::once_flag s_sdlInitFlag;

    static constexpr const char* FIFO_NAME = "es-de-command.fifo";
};

#endif // defined(RETRODECK)
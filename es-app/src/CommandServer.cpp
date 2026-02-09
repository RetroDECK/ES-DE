//
//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  CommandServer.cpp
//
//  Provides a named pipe (FIFO) interface for external commands.
//

#include "CommandServer.h"

#if defined(RETRODECK)

#include "views/ViewController.h"
#include "Log.h"
#include "Settings.h"
#include "Window.h"

#include <SDL_events.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <thread>

// Constants
static constexpr size_t BUFFER_SIZE = 256;
static constexpr int SELECT_TIMEOUT_MS = 200;
static constexpr mode_t FIFO_PERMISSIONS = 0644;

// Static member definition
std::once_flag CommandServer::s_sdlInitFlag;
Uint32 CommandServer::s_sdlEventType = 0;

CommandServer* CommandServer::getInstance()
{
    static CommandServer instance;
    return &instance;
}

CommandServer::CommandServer()
    : m_running(false)
    , m_fifoFd(-1)
{
    initializeCommandRegistry();
}

CommandServer::~CommandServer()
{
    stop();
}

bool CommandServer::start()
{
    if (m_running) {
        LOG(LogInfo) << "CommandServer: Already running";
        return true;
    }

    // Thread-safe SDL event registration
    std::call_once(s_sdlInitFlag, []() {
        s_sdlEventType = SDL_RegisterEvents(1);
        if (s_sdlEventType == (Uint32)-1) {
            LOG(LogError) << "CommandServer: Failed to register SDL event type";
        } else {
            LOG(LogDebug) << "CommandServer: Registered SDL event type " << s_sdlEventType;
        }
    });

    if (s_sdlEventType == (Uint32)-1 || s_sdlEventType == 0) {
        LOG(LogError) << "CommandServer: SDL event type not available";
        return false;
    }

    std::string fifoPath = getFifoPath();

    // Remove existing FIFO if it exists
    unlink(fifoPath.c_str());

    // Create the FIFO with secure permissions
    if (mkfifo(fifoPath.c_str(), FIFO_PERMISSIONS) < 0) {
        LOG(LogError) << "CommandServer: Failed to create FIFO at " << fifoPath
                      << " (errno: " << errno << ")";
        return false;
    }

    // Open FIFO in non-blocking read mode
    m_fifoFd = open(fifoPath.c_str(), O_RDONLY | O_NONBLOCK);
    if (m_fifoFd < 0) {
        LOG(LogError) << "CommandServer: Failed to open FIFO (errno: " << errno << ")";
        unlink(fifoPath.c_str());
        return false;
    }

    m_running = true;
    m_serverThread = std::thread(&CommandServer::serverThreadFunc, this);
    
    LOG(LogInfo) << "CommandServer: Started on FIFO " << fifoPath;
    return true;
}

void CommandServer::stop()
{
    if (!m_running) {
        return;
    }

    m_running = false;
    
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }

    if (m_fifoFd >= 0) {
        close(m_fifoFd);
        m_fifoFd = -1;
    }

    // Remove FIFO file
    std::string fifoPath = getFifoPath();
    unlink(fifoPath.c_str());
    
    LOG(LogInfo) << "CommandServer: Stopped";
}

std::string CommandServer::getFifoPath() const
{
    // Try to get config directory from Settings
    std::string configDir = Settings::getInstance()->getString("ConfigDirectory");
    
    // If not set, construct the RetroDECK flatpak path
    if (configDir.empty()) {
        const char* home = getenv("HOME");
        if (home) {
            configDir = std::string(home) + "/.var/app/net.retrodeck.retrodeck/config";
        } else {
            configDir = "/tmp";
        }
    }
    
    // Append ES-DE subdirectory
    std::string esdeDir = configDir + "/ES-DE";
    
    // Create the ES-DE subdirectory if it doesn't exist
    struct stat st;
    if (stat(esdeDir.c_str(), &st) != 0) {
        // Try to create the directory recursively
        size_t pos = 0;
        while ((pos = esdeDir.find('/', pos + 1)) != std::string::npos) {
            std::string subdir = esdeDir.substr(0, pos);
            if (mkdir(subdir.c_str(), 0755) < 0 && errno != EEXIST) {
                LOG(LogWarning) << "CommandServer: Failed to create directory " << subdir
                                << " (errno: " << errno << ")";
            }
        }
        if (mkdir(esdeDir.c_str(), 0755) < 0 && errno != EEXIST) {
            LOG(LogError) << "CommandServer: Failed to create ES-DE directory " << esdeDir
                          << " (errno: " << errno << ")";
        }
    }
    
    return esdeDir + "/" + FIFO_NAME;
}

void CommandServer::serverThreadFunc()
{
    LOG(LogInfo) << "CommandServer: Server thread started";

    char buffer[BUFFER_SIZE];
    std::string commandBuffer;

    while (m_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(m_fifoFd, &readfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = SELECT_TIMEOUT_MS * 1000; // Convert to microseconds

        int ret = select(m_fifoFd + 1, &readfds, nullptr, nullptr, &tv);

        if (ret < 0) {
            if (errno == EINTR) {
                // Interrupted by signal, retry
                continue;
            }
            LOG(LogError) << "CommandServer: select() failed (errno: " << errno << ")";
            std::this_thread::sleep_for(std::chrono::milliseconds(SELECT_TIMEOUT_MS));
            continue;
        }

        if (ret > 0 && FD_ISSET(m_fifoFd, &readfds)) {
            ssize_t bytesRead = read(m_fifoFd, buffer, sizeof(buffer) - 1);

            if (bytesRead < 0) {
                if (errno == EINTR) {
                    continue;
                }
                LOG(LogError) << "CommandServer: read() failed (errno: " << errno << ")";
                continue;
            }

            if (bytesRead == 0) {
                // EOF - FIFO closed, reopen it
                LOG(LogInfo) << "CommandServer: FIFO closed by writer, reopening...";
                close(m_fifoFd);
                std::string fifoPath = getFifoPath();
                m_fifoFd = open(fifoPath.c_str(), O_RDONLY | O_NONBLOCK);
                if (m_fifoFd < 0) {
                    LOG(LogError) << "CommandServer: Failed to reopen FIFO";
                    m_running = false;
                    return;
                }
                continue;
            }

            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                commandBuffer.append(buffer);

                // Process complete lines from the buffer
                size_t pos;
                while ((pos = commandBuffer.find('\n')) != std::string::npos) {
                    std::string command = commandBuffer.substr(0, pos);
                    commandBuffer.erase(0, pos + 1);

                    // Trim whitespace using utility function
                    command = trimWhitespace(command);

                    if (!command.empty()) {
                        processCommand(command);
                    }
                }
            }
        }
    }
}

void CommandServer::processCommand(const std::string& command)
{
    LOG(LogDebug) << "CommandServer: Received command: " << command;

    // Check if command is registered
    auto it = m_commandRegistry.find(command);
    if (it == m_commandRegistry.end()) {
        LOG(LogWarning) << "CommandServer: Unknown command: " << command;
        return;
    }

    const auto& [handler, shouldCoalesce] = it->second;

    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    // Coalesce if configured
    if (shouldCoalesce) {
        auto dupIt = std::find(m_commandQueue.begin(), m_commandQueue.end(), command);
        if (dupIt != m_commandQueue.end()) {
            LOG(LogDebug) << "CommandServer: " << command << " already pending, skipping duplicate";
            return;
        }
    }

    m_commandQueue.push_back(command);
    LOG(LogInfo) << "CommandServer: Command queued: " << command;

    // Notify main thread via SDL event
    SDL_Event event;
    event.type = s_sdlEventType;
    event.user.code = 0;
    event.user.data1 = nullptr;
    event.user.data2 = nullptr;
    
    if (SDL_PushEvent(&event) != 1) {
        LOG(LogError) << "CommandServer: Failed to push SDL event";
    }
}

void CommandServer::executePendingCommands()
{
    std::vector<std::string> commands;

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        // Early exit optimization - avoid swap if queue is empty
        if (m_commandQueue.empty())
            return;
        commands.swap(m_commandQueue);
    }

    for (const auto& command : commands) {
        LOG(LogInfo) << "CommandServer: Executing command: " << command;
        executeCommand(command);
    }
}

void CommandServer::executeCommand(const std::string& command)
{
    auto it = m_commandRegistry.find(command);
    if (it != m_commandRegistry.end()) {
        it->second.handler();
    } else {
        LOG(LogWarning) << "CommandServer: Unknown command during execution: " << command;
    }
}

// Command registry for extensibility
void CommandServer::initializeCommandRegistry()
{
    // Register RESCAN command (coalesces duplicates)
    registerCommand("RESCAN", [this]() {
        ViewController::getInstance()->rescanROMDirectory();
    }, true);
    
    // Alias for rescan_rom_directory
    registerCommand("rescan_rom_directory", [this]() {
        ViewController::getInstance()->rescanROMDirectory();
    }, true);
}

void CommandServer::registerCommand(const std::string& name, 
                                      CommandHandler handler, 
                                      bool coalesce)
{
    m_commandRegistry[name] = {handler, coalesce};
}

std::string CommandServer::trimWhitespace(const std::string& str) const
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

#endif // defined(RETRODECK)
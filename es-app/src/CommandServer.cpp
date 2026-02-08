//
//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  CommandServer.cpp
//
//  Provides a Unix domain socket interface for external commands.
//

#include "CommandServer.h"

#if defined(RETRODECK)

#include "views/ViewController.h"
#include "Log.h"
#include "Settings.h"
#include "Window.h"

#include <SDL_events.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <nlohmann/json.hpp>

CommandServer* CommandServer::getInstance()
{
    static CommandServer instance;
    return &instance;
}

CommandServer::CommandServer()
    : m_running(false)
    , m_serverSocket(-1)
{
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

    // Create socket
    m_serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_serverSocket < 0) {
        LOG(LogError) << "CommandServer: Failed to create socket";
        return false;
    }

    // Set socket to non-blocking
    int flags = fcntl(m_serverSocket, F_GETFL, 0);
    fcntl(m_serverSocket, F_SETFL, flags | O_NONBLOCK);

    // Bind to socket path
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    std::string socketPath = getSocketPath();
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
    
    // Remove existing socket file if it exists
    unlink(socketPath.c_str());
    
    if (bind(m_serverSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG(LogError) << "CommandServer: Failed to bind socket to " << socketPath;
        close(m_serverSocket);
        m_serverSocket = -1;
        return false;
    }

    // Listen for connections
    if (listen(m_serverSocket, 5) < 0) {
        LOG(LogError) << "CommandServer: Failed to listen on socket";
        close(m_serverSocket);
        m_serverSocket = -1;
        unlink(socketPath.c_str());
        return false;
    }

    m_running = true;
    m_serverThread = std::thread(&CommandServer::serverThreadFunc, this);
    
    LOG(LogInfo) << "CommandServer: Started on " << socketPath;
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

    if (m_serverSocket >= 0) {
        close(m_serverSocket);
        m_serverSocket = -1;
    }

    // Remove socket file
    std::string socketPath = getSocketPath();
    unlink(socketPath.c_str());
    
    LOG(LogInfo) << "CommandServer: Stopped";
}

std::string CommandServer::getSocketPath() const
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
        // Try to create the directory recursively using mkdir syscall
        size_t pos = 0;
        while ((pos = esdeDir.find('/', pos + 1)) != std::string::npos) {
            std::string subdir = esdeDir.substr(0, pos);
            mkdir(subdir.c_str(), 0755);
        }
        if (mkdir(esdeDir.c_str(), 0755) != 0 && errno != EEXIST) {
            LOG(LogError) << "CommandServer: Failed to create ES-DE directory " << esdeDir;
        }
    }
    
    return esdeDir + "/" + SOCKET_NAME;
}

void CommandServer::serverThreadFunc()
{
    LOG(LogInfo) << "CommandServer: Server thread started";
    
    while (m_running) {
        // Accept client connection (non-blocking due to O_NONBLOCK)
        struct sockaddr_un clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(m_serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket >= 0) {
            LOG(LogDebug) << "CommandServer: Client connected";
            handleClient(clientSocket);
        }
        
        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void CommandServer::handleClient(int clientSocket)
{
    char buffer[4096];
    std::string request;
    
    // Read data from client
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        close(clientSocket);
        return;
    }
    
    buffer[bytesRead] = '\0';
    request = buffer;
    
    LOG(LogDebug) << "CommandServer: Received request: " << request;
    
    // Parse JSON request
    nlohmann::json response;
    response["status"] = "error";
    response["message"] = "Unknown command";
    
    try {
        auto jsonRequest = nlohmann::json::parse(request);
        
        if (jsonRequest.contains("command")) {
            std::string command = jsonRequest["command"];
            
            if (command == "rescan_rom_directory") {
                // Queue the rescan command to be executed on the main thread
                SDL_Event event;
                event.type = SDL_USEREVENT;
                event.user.code = 100; // Command event code (avoid collision with ES-DE codes)
                auto* commandStr = new std::string("rescan_rom_directory");
                event.user.data1 = commandStr;
                event.user.data2 = nullptr;
                
                if (SDL_PushEvent(&event) == 0) {
                    response["status"] = "success";
                    response["message"] = "Rescan command queued";
                    LOG(LogInfo) << "CommandServer: Rescan command queued";
                } else {
                    delete commandStr; // Clean up on failure
                    response["message"] = "Failed to queue rescan command";
                    LOG(LogError) << "CommandServer: Failed to queue rescan command";
                }
            } else {
                response["message"] = "Unknown command: " + command;
                LOG(LogWarning) << "CommandServer: Unknown command: " << command;
            }
        } else {
            response["message"] = "No command specified";
        }
    } catch (const nlohmann::json::parse_error& e) {
        response["message"] = "Invalid JSON: " + std::string(e.what());
        LOG(LogError) << "CommandServer: Invalid JSON: " << e.what();
    }
    
    // Send response
    std::string responseStr = response.dump();
    ssize_t sent = send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
    if (sent < 0) {
        LOG(LogError) << "CommandServer: Failed to send response";
    }
    
    close(clientSocket);
    
    LOG(LogDebug) << "CommandServer: Sent response: " << responseStr;
}

#endif // defined(RETRODECK)
//
//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  CommandServer.h
//
//  Provides a Unix domain socket interface for external commands.
//

#pragma once

#if defined(RETRODECK)

#include <string>
#include <thread>
#include <atomic>

class CommandServer {
public:
    static CommandServer* getInstance();

    CommandServer();
    ~CommandServer();

    // Start the command server on a Unix domain socket
    bool start();
    
    // Stop the command server
    void stop();
    
    // Check if the server is running
    bool isRunning() const { return m_running; }
    
    // Get the socket path
    std::string getSocketPath() const;

private:
    void serverThreadFunc();
    void handleClient(int clientSocket);
    
    std::thread m_serverThread;
    std::atomic<bool> m_running;
    int m_serverSocket;
    
    static constexpr const char* SOCKET_NAME = "es-de-command.sock";
};

#endif // defined(RETRODECK)
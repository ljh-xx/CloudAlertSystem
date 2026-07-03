#pragma once
// TcpServer.h - Winsock2 TCP server, listens on SERVER_PORT

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include "AlertDB.h"
#include "Notifier.h"
#include "AlertEngine.h"

class TcpServer {
public:
    TcpServer(AlertDB* db, Notifier* notifier, AlertEngine* engine);
    ~TcpServer();

    // Initialize Winsock, create listening socket, bind, listen
    bool Init(int port);

    // Accept loop - blocks until server is shut down
    void Run();

    // Request shutdown
    void Stop();

private:
    AlertDB*      m_db;
    Notifier*     m_notifier;
    AlertEngine*  m_engine;
    SOCKET        m_listenSock;
    bool          m_running;
};

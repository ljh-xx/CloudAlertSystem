#pragma once
// ClientSession.h - Handles one TCP client connection in its own thread

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <string>
#include "AlertDB.h"
#include "Notifier.h"
#include "AlertEngine.h"

class ClientSession {
public:
    ClientSession(SOCKET sock, AlertDB* db, Notifier* notifier, AlertEngine* engine);
    ~ClientSession();

    // Thread entry point - called by TcpServer via CreateThread
    static DWORD WINAPI ThreadFunc(LPVOID pParam);

private:
    SOCKET        m_sock;
    AlertDB*      m_db;
    Notifier*     m_notifier;
    AlertEngine*  m_engine;
    int           m_userId;    // -1 until LOGIN succeeds
    std::string   m_username;

    // Main session loop: read lines and dispatch
    void Run();

    // Receive one newline-terminated line (may block across multiple recv calls)
    bool RecvLine(std::string& line);

    // Send a response string (appends \n if not present)
    void Send(const std::string& resp);
    void SendLine(const std::string& resp);

    // Command handlers
    void HandleRegister(const std::string& line);
    void HandleLogin(const std::string& line);
    void HandleAddPrice(const std::string& line);
    void HandleAddTime(const std::string& line);
    void HandleDelete(const std::string& line);
    void HandleModifyPrice(const std::string& line);
    void HandleModifyTime(const std::string& line);
    void HandleQuery(const std::string& line);
    void HandleSetEmail(const std::string& line);
    void HandleGetEmail(const std::string& line);
    void HandleGetPrices(const std::string& line);

    // Dispatch a received line to the appropriate handler
    void Dispatch(const std::string& line);
};

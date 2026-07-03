#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <string>
#pragma comment(lib, "ws2_32.lib")

class TcpClient {
public:
    TcpClient();
    ~TcpClient();

    bool Connect(const char* host, int port);
    bool SendLine(const std::string& line);
    bool RecvLine(std::string& line);
    void Close();
    bool IsConnected() const { return m_sock != INVALID_SOCKET; }

private:
    SOCKET m_sock;
};

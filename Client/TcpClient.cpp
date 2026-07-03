#include "TcpClient.h"
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>

TcpClient::TcpClient() : m_sock(INVALID_SOCKET) {
    WSADATA wd;
    WSAStartup(MAKEWORD(2, 2), &wd);
}

TcpClient::~TcpClient() {
    Close();
    WSACleanup();
}

bool TcpClient::Connect(const char* host, int port) {
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((u_short)port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(m_sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }
    return true;
}

bool TcpClient::SendLine(const std::string& line) {
    std::string buf = line;
    if (buf.empty() || buf.back() != '\n') buf += '\n';
    int sent = send(m_sock, buf.c_str(), (int)buf.size(), 0);
    return sent == (int)buf.size();
}

bool TcpClient::RecvLine(std::string& line) {
    line.clear();
    char c;
    while (true) {
        int r = recv(m_sock, &c, 1, 0);
        if (r <= 0) return false;
        if (c == '\n') break;
        if (c != '\r') line += c;
    }
    return true;
}

void TcpClient::Close() {
    if (m_sock != INVALID_SOCKET) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
}

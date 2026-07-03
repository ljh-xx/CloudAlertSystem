// TcpServer.cpp - Winsock2 TCP server implementation

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#include "TcpServer.h"
#include "ClientSession.h"
#include "Protocol.h"
#include <stdio.h>

TcpServer::TcpServer(AlertDB* db, Notifier* notifier, AlertEngine* engine)
    : m_db(db), m_notifier(notifier), m_engine(engine),
      m_listenSock(INVALID_SOCKET), m_running(false) {}

TcpServer::~TcpServer() {
    Stop();
}

bool TcpServer::Init(int port) {
    WSADATA wsa;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (rc != 0) {
        fprintf(stderr, "[TcpServer] WSAStartup failed: %d\n", rc);
        return false;
    }

    m_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSock == INVALID_SOCKET) {
        fprintf(stderr, "[TcpServer] socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    // Allow address reuse
    int opt = 1;
    setsockopt(m_listenSock, SOL_SOCKET, SO_REUSEADDR,
               (char*)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((u_short)port);

    if (bind(m_listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[TcpServer] bind() failed: %d\n", WSAGetLastError());
        closesocket(m_listenSock);
        WSACleanup();
        return false;
    }

    if (listen(m_listenSock, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "[TcpServer] listen() failed: %d\n", WSAGetLastError());
        closesocket(m_listenSock);
        WSACleanup();
        return false;
    }

    printf("[TcpServer] Listening on port %d\n", port);
    return true;
}

void TcpServer::Run() {
    m_running = true;
    while (m_running) {
        sockaddr_in clientAddr = {};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = accept(m_listenSock,
                                   (sockaddr*)&clientAddr, &addrLen);
        if (clientSock == INVALID_SOCKET) {
            if (m_running)
                fprintf(stderr, "[TcpServer] accept() failed: %d\n",
                        WSAGetLastError());
            break;
        }

        char ipBuf[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuf, sizeof(ipBuf));
        printf("[TcpServer] New client from %s:%d\n",
               ipBuf, ntohs(clientAddr.sin_port));

        // Each connection gets its own ClientSession object and thread.
        // The thread deletes the session object when it exits (see ThreadFunc).
        ClientSession* session =
            new ClientSession(clientSock, m_db, m_notifier, m_engine);

        HANDLE hThread = CreateThread(
            nullptr, 0,
            ClientSession::ThreadFunc,
            session,
            0, nullptr);

        if (hThread)
            CloseHandle(hThread); // detach
        else {
            fprintf(stderr, "[TcpServer] CreateThread failed\n");
            delete session;
        }
    }
    printf("[TcpServer] Accept loop exited\n");
}

void TcpServer::Stop() {
    m_running = false;
    if (m_listenSock != INVALID_SOCKET) {
        closesocket(m_listenSock);
        m_listenSock = INVALID_SOCKET;
    }
}

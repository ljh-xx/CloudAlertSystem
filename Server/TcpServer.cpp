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

static const int TCP_WORKER_COUNT = 32;
static const size_t TCP_PENDING_QUEUE_LIMIT = 256;

TcpServer::TcpServer(AlertDB* db, Notifier* notifier, AlertEngine* engine)
    : m_db(db), m_notifier(notifier), m_engine(engine),
      m_listenSock(INVALID_SOCKET), m_running(false),
      m_queueSemaphore(NULL) {
    InitializeCriticalSection(&m_queueCs);
}

TcpServer::~TcpServer() {
    Stop();
    DeleteCriticalSection(&m_queueCs);
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
    if (!StartWorkerPool()) {
        closesocket(m_listenSock);
        m_listenSock = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
    return true;
}

bool TcpServer::StartWorkerPool() {
    m_queueSemaphore = CreateSemaphore(nullptr, 0, 0x7fffffff, nullptr);
    if (!m_queueSemaphore) {
        fprintf(stderr, "[TcpServer] CreateSemaphore failed: %u\n", GetLastError());
        return false;
    }

    for (int i = 0; i < TCP_WORKER_COUNT; ++i) {
        HANDLE hThread = CreateThread(
            nullptr, 0,
            WorkerThreadProc,
            this,
            0, nullptr);
        if (!hThread) {
            fprintf(stderr, "[TcpServer] Failed to create worker thread %d (err=%u)\n",
                    i, GetLastError());
            StopWorkerPool();
            return false;
        }
        m_workerThreads.push_back(hThread);
    }

    printf("[TcpServer] Worker pool started (%d thread(s), queue limit %llu)\n",
           TCP_WORKER_COUNT, (unsigned long long)TCP_PENDING_QUEUE_LIMIT);
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

        if (!EnqueueClient(clientSock)) {
            fprintf(stderr, "[TcpServer] Client queue full, rejecting connection\n");
            closesocket(clientSock);
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
    StopWorkerPool();
    WSACleanup();
}

bool TcpServer::EnqueueClient(SOCKET clientSock) {
    bool queued = false;
    EnterCriticalSection(&m_queueCs);
    if (m_clientQueue.size() < TCP_PENDING_QUEUE_LIMIT) {
        m_clientQueue.push(clientSock);
        queued = true;
    }
    LeaveCriticalSection(&m_queueCs);

    if (queued && m_queueSemaphore) {
        ReleaseSemaphore(m_queueSemaphore, 1, nullptr);
    }
    return queued;
}

bool TcpServer::DequeueClient(SOCKET& clientSock) {
    clientSock = INVALID_SOCKET;
    EnterCriticalSection(&m_queueCs);
    if (!m_clientQueue.empty()) {
        clientSock = m_clientQueue.front();
        m_clientQueue.pop();
    }
    LeaveCriticalSection(&m_queueCs);
    return clientSock != INVALID_SOCKET;
}

void TcpServer::StopWorkerPool() {
    if (m_queueSemaphore && !m_workerThreads.empty()) {
        ReleaseSemaphore(m_queueSemaphore, (LONG)m_workerThreads.size(), nullptr);
    }

    for (HANDLE hThread : m_workerThreads) {
        if (hThread) {
            WaitForSingleObject(hThread, 3000);
            CloseHandle(hThread);
        }
    }
    m_workerThreads.clear();

    EnterCriticalSection(&m_queueCs);
    while (!m_clientQueue.empty()) {
        SOCKET sock = m_clientQueue.front();
        m_clientQueue.pop();
        closesocket(sock);
    }
    LeaveCriticalSection(&m_queueCs);

    if (m_queueSemaphore) {
        CloseHandle(m_queueSemaphore);
        m_queueSemaphore = NULL;
    }
}

DWORD WINAPI TcpServer::WorkerThreadProc(LPVOID pParam) {
    TcpServer* self = reinterpret_cast<TcpServer*>(pParam);
    printf("[TcpServer] Worker thread started\n");

    while (true) {
        DWORD waitRc = WaitForSingleObject(self->m_queueSemaphore, INFINITE);
        if (waitRc != WAIT_OBJECT_0) break;

        SOCKET clientSock = INVALID_SOCKET;
        if (!self->DequeueClient(clientSock)) {
            if (!self->m_running) break;
            continue;
        }

        ClientSession* session =
            new ClientSession(clientSock, self->m_db, self->m_notifier, self->m_engine);
        ClientSession::ThreadFunc(session);
    }

    printf("[TcpServer] Worker thread stopped\n");
    return 0;
}

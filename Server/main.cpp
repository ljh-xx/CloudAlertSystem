// main.cpp - Cloud Alert System Server entry point
//
// Startup sequence:
//   1. Init AlertDB (SQLite)
//   2. Init Notifier
//   3. Start TcpServer on a new thread (accepts client connections)
//   4. Start AlertEngine timer thread (checks time alerts every second)
//   5. Start CTPMdHandler (connects to market data front, drives SPI)
//   6. Sleep forever (main thread idle; SPI thread drives CTP)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>

#include "Protocol.h"
#include "AlertDB.h"
#include "Notifier.h"
#include "TcpServer.h"
#include "AlertEngine.h"
#include "CTPMdHandler.h"
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// TCP server thread wrapper
// ---------------------------------------------------------------------------

struct TcpServerThreadParam {
    TcpServer* server;
    int        port;
};

static DWORD WINAPI TcpServerThreadFunc(LPVOID pParam) {
    TcpServerThreadParam* p =
        reinterpret_cast<TcpServerThreadParam*>(pParam);
    if (!p->server->Init(p->port)) {
        fprintf(stderr, "[main] TcpServer::Init failed\n");
        return 1;
    }
    p->server->Run();
    return 0;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    printf("=== Cloud Alert System Server starting ===\n");

    // ---- 1. AlertDB ----
    AlertDB db;
    if (!db.Init(DB_FILE_NAME)) {
        fprintf(stderr, "[main] Failed to initialize database\n");
        return 1;
    }
    printf("[main] Database initialized: %s\n", DB_FILE_NAME);

    // ---- 2. Notifier ----
    Notifier notifier;
    printf("[main] Notifier initialized\n");

    // ---- 3. AlertEngine ----
    AlertEngine engine(&db, &notifier);
    if (!engine.StartTimerThread()) {
        fprintf(stderr, "[main] Failed to start AlertEngine timer thread\n");
        return 1;
    }
    printf("[main] AlertEngine timer thread started\n");

    // ---- 4. TcpServer (new thread) ----
    TcpServer tcpServer(&db, &notifier, &engine);
    TcpServerThreadParam tcpParam = { &tcpServer, SERVER_PORT };

    HANDLE hTcpThread = CreateThread(
        nullptr, 0,
        TcpServerThreadFunc,
        &tcpParam,
        0, nullptr);
    if (!hTcpThread) {
        fprintf(stderr, "[main] Failed to create TcpServer thread\n");
        return 1;
    }
    printf("[main] TcpServer thread started (port %d)\n", SERVER_PORT);

    // ---- 5. CTP market data handler ----
    CTPMdHandler ctpHandler(&engine);
    engine.SetCtpHandler(&ctpHandler);

    if (!ctpHandler.Connect(CTP_FRONT_ADDR)) {
        fprintf(stderr, "[main] CTPMdHandler::Connect failed\n");
        // Non-fatal: server still accepts clients; CTP reconnects automatically
    } else {
        printf("[main] CTP connecting to %s\n", CTP_FRONT_ADDR);
    }

    // ---- 6. Idle main thread ----
    printf("[main] Server running. Press Ctrl+C to stop.\n");
    Sleep(INFINITE);

    // Cleanup (normally not reached unless process killed gracefully)
    engine.StopTimerThread();
    tcpServer.Stop();
    WaitForSingleObject(hTcpThread, 5000);
    CloseHandle(hTcpThread);

    return 0;
}

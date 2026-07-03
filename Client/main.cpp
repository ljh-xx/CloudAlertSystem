// Client/main.cpp - Cloud Alert System Console Client

#include "TcpClient.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 8888

static TcpClient g_client;
static char g_username[64] = "";

// ---------------------------------------------------------------------------
// Receive queue — RecvThread is the ONLY reader of the socket.
// ---------------------------------------------------------------------------

static std::queue<std::string> g_recvQ;
static std::mutex              g_recvMtx;
static std::condition_variable g_recvCV;

static DWORD WINAPI RecvThread(LPVOID) {
    std::string line;
    while (g_client.IsConnected() && g_client.RecvLine(line)) {
        {
            std::lock_guard<std::mutex> lk(g_recvMtx);
            g_recvQ.push(line);
        }
        g_recvCV.notify_one();
    }
    return 0;
}

// Block until a line arrives or timeout_ms elapses. Returns "" on timeout.
static std::string QueueRead(int timeout_ms = 6000) {
    std::unique_lock<std::mutex> lk(g_recvMtx);
    bool ok = g_recvCV.wait_for(lk,
                  std::chrono::milliseconds(timeout_ms),
                  [] { return !g_recvQ.empty(); });
    if (!ok) return "";
    std::string line = g_recvQ.front();
    g_recvQ.pop();
    return line;
}

// Print any TRIGGERED alerts that arrived while user was typing.
static void DrainAlerts() {
    std::lock_guard<std::mutex> lk(g_recvMtx);
    while (!g_recvQ.empty()) {
        std::string line = g_recvQ.front();
        g_recvQ.pop();
        if (line.size() >= 9 && line.substr(0, 9) == "TRIGGERED")
            printf("\n[*** ALERT TRIGGERED ***] %s\n\n", line.c_str());
    }
}

// ---------------------------------------------------------------------------
// Command helpers
// ---------------------------------------------------------------------------

// Send command, read all response lines until terminal.
static void DoCmd(const std::string& cmd) {
    if (!g_client.SendLine(cmd)) { printf("[ERR] Send failed\n"); return; }
    while (true) {
        std::string line = QueueRead(6000);
        if (line.empty()) { printf("[ERR] Response timeout (server may be down)\n"); break; }
        if (line.size() >= 9 && line.substr(0, 9) == "TRIGGERED") {
            printf("\n[*** ALERT TRIGGERED ***] %s\n", line.c_str());
            continue;
        }
        printf("  <- %s\n", line.c_str());
        if (line == "END") break;
        if (line.substr(0, 2) == "OK" || line.substr(0, 3) == "ERR") break;
    }
}

// Send command, return the first non-TRIGGERED response line ("" on timeout).
static std::string DoCmdOneLine(const std::string& cmd) {
    if (!g_client.SendLine(cmd)) { printf("[ERR] Send failed\n"); return ""; }
    while (true) {
        std::string line = QueueRead(6000);
        if (line.empty()) { printf("[ERR] Response timeout (server may be down)\n"); return ""; }
        if (line.size() >= 9 && line.substr(0, 9) == "TRIGGERED") {
            printf("\n[*** ALERT TRIGGERED ***] %s\n", line.c_str());
            continue;
        }
        return line;
    }
}

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------

static void PrintMenu() {
    printf("\n=== Cloud Alert Client ===\n");
    if (g_username[0])
        printf("  Logged in as: %s\n", g_username);
    printf("  1. Register (new user)\n");
    printf("  2. Login\n");
    printf("  3. Set email\n");
    printf("  4. Add price alert\n");
    printf("  5. Add time alert\n");
    printf("  6. Delete alert\n");
    printf("  7. Modify price alert\n");
    printf("  8. Modify time alert\n");
    printf("  9. Query alerts\n");
    printf("  0. Exit\n");
    printf("Choice: ");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("Connecting to %s:%d ...\n", SERVER_HOST, SERVER_PORT);
    if (!g_client.Connect(SERVER_HOST, SERVER_PORT)) {
        printf("[ERR] Cannot connect to server. Is Server.exe running?\n");
        return 1;
    }
    printf("Connected!\n");

    CreateThread(nullptr, 0, RecvThread, nullptr, 0, nullptr);

    char input[256];
    while (true) {
        DrainAlerts();
        PrintMenu();
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        bool isNumeric = (input[0] >= '0' && input[0] <= '9');
        if (!isNumeric || input[0] == '\0') { printf("[ERR] Invalid choice\n"); continue; }
        int choice = atoi(input);
        if (choice == 0) break;

        // ---- 1. Register ----
        if (choice == 1) {
            char user[64], pass[64], email[128];
            printf("Username: ");  fgets(user,  sizeof(user),  stdin); user[strcspn(user, "\n")] = 0;
            printf("Password: ");  fgets(pass,  sizeof(pass),  stdin); pass[strcspn(pass, "\n")] = 0;
            printf("Email:    ");  fgets(email, sizeof(email), stdin); email[strcspn(email, "\n")] = 0;
            char cmd[320];
            _snprintf_s(cmd, sizeof(cmd), "REGISTER %s %s %s", user, pass, email);
            std::string resp = DoCmdOneLine(cmd);
            printf("  <- %s\n", resp.empty() ? "(no response)" : resp.c_str());
            if (!resp.empty() && resp.substr(0, 2) == "OK")
                printf("  [INFO] Registration successful. Now use Choice 2 to login.\n");
        }
        // ---- 2. Login ----
        else if (choice == 2) {
            char user[64], pass[64];
            printf("Username: "); fgets(user, sizeof(user), stdin); user[strcspn(user, "\n")] = 0;
            printf("Password: "); fgets(pass, sizeof(pass), stdin); pass[strcspn(pass, "\n")] = 0;
            char cmd[256];
            _snprintf_s(cmd, sizeof(cmd), "LOGIN %s %s", user, pass);
            std::string resp = DoCmdOneLine(cmd);
            printf("  <- %s\n", resp.empty() ? "(no response)" : resp.c_str());
            if (!resp.empty() && resp.substr(0, 2) == "OK")
                strncpy_s(g_username, user, sizeof(g_username) - 1);
        }
        // ---- 3. Set email ----
        else if (choice == 3) {
            if (g_username[0] == 0) { printf("[ERR] Please login first\n"); continue; }
            char pass[64], email[128];
            printf("Current password: "); fgets(pass, sizeof(pass), stdin); pass[strcspn(pass, "\n")] = 0;
            printf("New email: "); fgets(email, sizeof(email), stdin); email[strcspn(email, "\n")] = 0;
            char cmd[256];
            _snprintf_s(cmd, sizeof(cmd), "SET_EMAIL %s %s %s", g_username, pass, email);
            DoCmd(cmd);
        }
        // ---- 4. Add price alert ----
        else if (choice == 4) {
            if (g_username[0] == 0) { printf("[ERR] Please login first\n"); continue; }
            char contract[32]; double price; int cond;
            printf("Contract (e.g. rb2609): "); fgets(contract, sizeof(contract), stdin);
            contract[strcspn(contract, "\n")] = 0;
            printf("Condition:\n");
            printf("  0 = alert when price RISES to >= target\n");
            printf("  1 = alert when price DROPS to <= target\n");
            printf("Enter 0 or 1: "); scanf_s("%d", &cond); getchar();
            printf("Trigger price: "); scanf_s("%lf", &price); getchar();
            char cmd[256];
            _snprintf_s(cmd, sizeof(cmd), "ADD_PRICE %s %s %.4f %d", g_username, contract, price, cond);
            DoCmd(cmd);
        }
        // ---- 5. Add time alert ----
        else if (choice == 5) {
            if (g_username[0] == 0) { printf("[ERR] Please login first\n"); continue; }
            char contract[32], timestr[32];
            printf("Contract (e.g. rb2609): "); fgets(contract, sizeof(contract), stdin);
            contract[strcspn(contract, "\n")] = 0;
            printf("Trigger time (HH:MM:SS): "); fgets(timestr, sizeof(timestr), stdin);
            timestr[strcspn(timestr, "\n")] = 0;
            char cmd[256];
            _snprintf_s(cmd, sizeof(cmd), "ADD_TIME %s %s %s", g_username, contract, timestr);
            DoCmd(cmd);
        }
        // ---- 6. Delete alert ----
        else if (choice == 6) {
            if (g_username[0] == 0) { printf("[ERR] Please login first\n"); continue; }
            int id;
            printf("Alert ID to delete: "); scanf_s("%d", &id); getchar();
            char cmd[64];
            _snprintf_s(cmd, sizeof(cmd), "DELETE %d", id);
            DoCmd(cmd);
        }
        // ---- 7. Modify price alert ----
        else if (choice == 7) {
            if (g_username[0] == 0) { printf("[ERR] Please login first\n"); continue; }
            int id, cond; double price; char contract[32];
            printf("Alert ID: ");     scanf_s("%d", &id); getchar();
            printf("New contract: "); fgets(contract, sizeof(contract), stdin);
            contract[strcspn(contract, "\n")] = 0;
            printf("Condition  0=price rises to >=  1=price drops to <=: ");
            scanf_s("%d", &cond); getchar();
            printf("New price: "); scanf_s("%lf", &price); getchar();
            char cmd[256];
            _snprintf_s(cmd, sizeof(cmd), "MODIFY_PRICE %d %s %.4f %d", id, contract, price, cond);
            DoCmd(cmd);
        }
        // ---- 8. Modify time alert ----
        else if (choice == 8) {
            if (g_username[0] == 0) { printf("[ERR] Please login first\n"); continue; }
            int id; char contract[32], timestr[32];
            printf("Alert ID: ");         scanf_s("%d", &id); getchar();
            printf("New contract: ");     fgets(contract, sizeof(contract), stdin);
            contract[strcspn(contract, "\n")] = 0;
            printf("New time (HH:MM:SS): "); fgets(timestr, sizeof(timestr), stdin);
            timestr[strcspn(timestr, "\n")] = 0;
            char cmd[256];
            _snprintf_s(cmd, sizeof(cmd), "MODIFY_TIME %d %s %s", id, contract, timestr);
            DoCmd(cmd);
        }
        // ---- 9. Query alerts ----
        else if (choice == 9) {
            if (g_username[0] == 0) { printf("[ERR] Please login first\n"); continue; }
            int status;
            printf("Status  -1=all  0=pending  1=triggered  2=deleted: ");
            scanf_s("%d", &status); getchar();
            char cmd[128];
            _snprintf_s(cmd, sizeof(cmd), "QUERY %s %d", g_username, status);
            DoCmd(cmd);
        }
        else {
            printf("[ERR] Unknown choice\n");
        }
    }

    g_client.Close();
    printf("Disconnected.\n");
    return 0;
}

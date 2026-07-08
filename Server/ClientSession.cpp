// ClientSession.cpp - TCP client session: parse commands, call DB, send responses

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>

#include "ClientSession.h"
#include "Protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<std::string> SplitLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ClientSession::ClientSession(SOCKET sock, AlertDB* db, Notifier* notifier, AlertEngine* engine)
    : m_sock(sock), m_db(db), m_notifier(notifier), m_engine(engine), m_userId(-1) {}

ClientSession::~ClientSession() {
    if (m_userId != -1)
        m_notifier->UnregisterUser(m_userId);
    closesocket(m_sock);
}

// ---------------------------------------------------------------------------
// Thread entry
// ---------------------------------------------------------------------------

DWORD WINAPI ClientSession::ThreadFunc(LPVOID pParam) {
    ClientSession* session = reinterpret_cast<ClientSession*>(pParam);
    session->Run();
    delete session;
    return 0;
}

// ---------------------------------------------------------------------------
// Session loop
// ---------------------------------------------------------------------------

void ClientSession::Run() {
    printf("[Session] New connection socket %llu\n",
           (unsigned long long)m_sock);
    std::string line;
    while (RecvLine(line)) {
        if (!line.empty())
            Dispatch(line);
    }
    printf("[Session] Connection closed socket %llu user=%s\n",
           (unsigned long long)m_sock,
           m_username.empty() ? "(none)" : m_username.c_str());
}

// ---------------------------------------------------------------------------
// Recv one line (terminated by \n, strips \r\n)
// ---------------------------------------------------------------------------

bool ClientSession::RecvLine(std::string& line) {
    line.clear();
    char ch = 0;
    while (true) {
        int n = recv(m_sock, &ch, 1, 0);
        if (n <= 0) return false;   // connection closed or error
        if (ch == '\r') continue;
        if (ch == '\n') return true;
        line += ch;
        if (line.size() > RECV_BUF_SIZE) {
            line.clear(); // oversized; discard and continue
        }
    }
}

// ---------------------------------------------------------------------------
// Send helpers
// ---------------------------------------------------------------------------

void ClientSession::SendLine(const std::string& resp) {
    std::string msg = resp + "\n";
    send(m_sock, msg.c_str(), (int)msg.size(), 0);
}

void ClientSession::Send(const std::string& resp) {
    SendLine(resp);
}

// ---------------------------------------------------------------------------
// Dispatcher
// ---------------------------------------------------------------------------

void ClientSession::Dispatch(const std::string& line) {
    // First token is the command
    size_t sp = line.find(' ');
    std::string cmd = (sp == std::string::npos) ? line : line.substr(0, sp);

    if      (cmd == CMD_REGISTER)     HandleRegister(line);
    else if (cmd == CMD_LOGIN)        HandleLogin(line);
    else if (cmd == CMD_ADD_PRICE)    HandleAddPrice(line);
    else if (cmd == CMD_ADD_TIME)     HandleAddTime(line);
    else if (cmd == CMD_DELETE)       HandleDelete(line);
    else if (cmd == CMD_MODIFY_PRICE) HandleModifyPrice(line);
    else if (cmd == CMD_MODIFY_TIME)  HandleModifyTime(line);
    else if (cmd == CMD_QUERY)        HandleQuery(line);
    else if (cmd == CMD_SET_EMAIL)    HandleSetEmail(line);
    else if (cmd == CMD_GET_EMAIL)    HandleGetEmail(line);
    else if (cmd == CMD_GET_PRICE)    HandleGetPrices(line);
    else if (cmd == CMD_GET_PRICES)   HandleGetPrices(line);
    else {
        SendLine("ERR Unknown command");
    }
}

// ---------------------------------------------------------------------------
// REGISTER <username> <password> <email>
// ---------------------------------------------------------------------------

void ClientSession::HandleRegister(const std::string& line) {
    auto tokens = SplitLine(line);
    if (tokens.size() < 4) {
        SendLine("ERR REGISTER requires username password email");
        return;
    }
    const std::string& username = tokens[1];
    const std::string& password = tokens[2];
    const std::string& email    = tokens[3];

    if (username.empty() || password.empty() || email.empty()) {
        SendLine("ERR username, password and email cannot be empty");
        return;
    }
    int uid = m_db->RegisterUser(username, password, email);
    if (uid < 0) {
        SendLine("ERR username already taken");
        return;
    }
    SendLine("OK");
    printf("[Session] Registered user '%s' (id=%d)\n", username.c_str(), uid);
}

// ---------------------------------------------------------------------------
// LOGIN <username> <password>
// ---------------------------------------------------------------------------

void ClientSession::HandleLogin(const std::string& line) {
    auto tokens = SplitLine(line);
    if (tokens.size() < 3) {
        SendLine("ERR LOGIN requires username and password");
        return;
    }
    const std::string& username = tokens[1];
    const std::string& password = tokens[2];

    int uid = m_db->VerifyLogin(username, password);
    if (uid < 0) {
        SendLine("ERR wrong username or password");
        return;
    }

    m_userId   = uid;
    m_username = username;
    m_notifier->RegisterUser(uid, m_sock);
    SendLine("OK");
    printf("[Session] User '%s' logged in (id=%d)\n", username.c_str(), uid);
}

// ---------------------------------------------------------------------------
// ADD_PRICE <username> <contract> <price> <condition>
// ---------------------------------------------------------------------------

void ClientSession::HandleAddPrice(const std::string& line) {
    if (m_userId < 0) { SendLine("ERR Not logged in"); return; }
    auto tokens = SplitLine(line);
    if (tokens.size() < 5) {
        SendLine("ERR ADD_PRICE requires username contract price condition");
        return;
    }
    const std::string& contract = tokens[2];
    double price = atof(tokens[3].c_str());
    int cond = atoi(tokens[4].c_str());
    if (cond != COND_GE && cond != COND_LE) {
        SendLine("ERR condition must be 0 (>=) or 1 (<=)");
        return;
    }
    int alertId = m_db->AddPriceAlert(m_userId, contract, price, cond);
    if (alertId < 0) {
        SendLine("ERR DB error adding price alert");
        return;
    }
    // Subscribe contract via CTP so ticks arrive immediately
    m_engine->SubscribeContract(contract);
    char buf[64];
    _snprintf_s(buf, sizeof(buf), "OK %d", alertId);
    SendLine(std::string(buf));
}

// ---------------------------------------------------------------------------
// ADD_TIME <username> <contract> <HH:MM:SS>
// ---------------------------------------------------------------------------

void ClientSession::HandleAddTime(const std::string& line) {
    if (m_userId < 0) { SendLine("ERR Not logged in"); return; }
    auto tokens = SplitLine(line);
    if (tokens.size() < 4) {
        SendLine("ERR ADD_TIME requires username contract HH:MM:SS");
        return;
    }
    const std::string& contract  = tokens[2];
    const std::string& timeStr   = tokens[3];
    int alertId = m_db->AddTimeAlert(m_userId, contract, timeStr);
    if (alertId < 0) {
        SendLine("ERR DB error adding time alert");
        return;
    }
    // Subscribe contract so market data flows for this instrument
    m_engine->SubscribeContract(contract);
    char buf[64];
    _snprintf_s(buf, sizeof(buf), "OK %d", alertId);
    SendLine(std::string(buf));
}

// ---------------------------------------------------------------------------
// DELETE <alert_id>
// ---------------------------------------------------------------------------

void ClientSession::HandleDelete(const std::string& line) {
    if (m_userId < 0) { SendLine("ERR Not logged in"); return; }
    auto tokens = SplitLine(line);
    if (tokens.size() < 2) {
        SendLine("ERR DELETE requires alert_id");
        return;
    }
    int alertId = atoi(tokens[1].c_str());
    if (m_db->DeleteAlert(alertId, m_userId))
        SendLine("OK");
    else
        SendLine("ERR Delete failed (not found or already deleted)");
}

// ---------------------------------------------------------------------------
// MODIFY_PRICE <alert_id> <contract> <price> <condition>
// ---------------------------------------------------------------------------

void ClientSession::HandleModifyPrice(const std::string& line) {
    if (m_userId < 0) { SendLine("ERR Not logged in"); return; }
    auto tokens = SplitLine(line);
    if (tokens.size() < 5) {
        SendLine("ERR MODIFY_PRICE requires alert_id contract price condition");
        return;
    }
    int    alertId  = atoi(tokens[1].c_str());
    const std::string& contract = tokens[2];
    double price    = atof(tokens[3].c_str());
    int cond = atoi(tokens[4].c_str());
    if (cond != COND_GE && cond != COND_LE) {
        SendLine("ERR condition must be 0 (>=) or 1 (<=)");
        return;
    }
    if (m_db->ModifyPriceAlert(alertId, m_userId, contract, price, cond))
        SendLine("OK");
    else
        SendLine("ERR Modify failed");
}

// ---------------------------------------------------------------------------
// MODIFY_TIME <alert_id> <contract> <HH:MM:SS>
// ---------------------------------------------------------------------------

void ClientSession::HandleModifyTime(const std::string& line) {
    if (m_userId < 0) { SendLine("ERR Not logged in"); return; }
    auto tokens = SplitLine(line);
    if (tokens.size() < 4) {
        SendLine("ERR MODIFY_TIME requires alert_id contract HH:MM:SS");
        return;
    }
    int alertId = atoi(tokens[1].c_str());
    const std::string& contract = tokens[2];
    const std::string& timeStr  = tokens[3];
    if (m_db->ModifyTimeAlert(alertId, m_userId, contract, timeStr))
        SendLine("OK");
    else
        SendLine("ERR Modify failed");
}

// ---------------------------------------------------------------------------
// QUERY <username> <status>
// Response: multiple ALERT lines followed by END
// ALERT <id> <type> <contract> <price> <cond> <time> <status> <created>
// ---------------------------------------------------------------------------

void ClientSession::HandleQuery(const std::string& line) {
    if (m_userId < 0) { SendLine("ERR Not logged in"); return; }
    auto tokens = SplitLine(line);
    if (tokens.size() < 3) {
        SendLine("ERR QUERY requires username status");
        return;
    }
    int statusFilter = atoi(tokens[2].c_str());
    auto records = m_db->QueryAlerts(m_userId, statusFilter);

    for (const auto& r : records) {
        char buf[512];
        _snprintf_s(buf, sizeof(buf),
            "ALERT %d %d %s %.2f %d %s %d %s",
            r.id, r.alert_type, r.contract.c_str(),
            r.trigger_price, r.condition,
            r.trigger_time.empty() ? "-" : r.trigger_time.c_str(),
            r.status, r.created_at.c_str());
        SendLine(std::string(buf));
    }
    SendLine("END");
}

// ---------------------------------------------------------------------------
// SET_EMAIL <username> <password> <email>
// ---------------------------------------------------------------------------

void ClientSession::HandleSetEmail(const std::string& line) {
    if (m_userId < 0) { SendLine("ERR Not logged in"); return; }
    auto tokens = SplitLine(line);
    if (tokens.size() < 4) {
        SendLine("ERR SET_EMAIL requires username password email");
        return;
    }
    const std::string& username = tokens[1];
    const std::string& password = tokens[2];
    const std::string& email = tokens[3];
    if (username != m_username || m_db->VerifyLogin(username, password) != m_userId) {
        SendLine("ERR wrong username or password");
        return;
    }
    if (m_db->SetEmail(m_userId, email))
        SendLine("OK");
    else
        SendLine("ERR Failed to set email");
}

// ---------------------------------------------------------------------------
// GET_EMAIL <username>
// ---------------------------------------------------------------------------

void ClientSession::HandleGetEmail(const std::string& line) {
    if (m_userId < 0) { SendLine("ERR Not logged in"); return; }
    auto tokens = SplitLine(line);
    if (tokens.size() < 2) {
        SendLine("ERR GET_EMAIL requires username");
        return;
    }
    const std::string& username = tokens[1];
    if (username != m_username) {
        SendLine("ERR wrong username");
        return;
    }
    SendLine("OK " + m_db->GetEmail(m_userId));
}

// ---------------------------------------------------------------------------
// GET_PRICE <contract>
// GET_PRICES <contract1> <contract2> ...
// Response: PRICE <contract> <last_price|-> lines followed by END
// ---------------------------------------------------------------------------

void ClientSession::HandleGetPrices(const std::string& line) {
    if (m_userId < 0) { SendLine("ERR Not logged in"); return; }
    auto tokens = SplitLine(line);
    if (tokens.size() < 2) {
        SendLine("ERR GET_PRICES requires at least one contract");
        return;
    }

    for (size_t i = 1; i < tokens.size(); ++i) {
        const std::string& contract = tokens[i];
        if (contract.empty()) continue;
        m_engine->SubscribeContract(contract);

        double lastPrice = 0.0;
        char buf[256];
        if (m_engine->GetLastPrice(contract, lastPrice)) {
            _snprintf_s(buf, sizeof(buf), "PRICE %s %.2f", contract.c_str(), lastPrice);
        } else {
            _snprintf_s(buf, sizeof(buf), "PRICE %s -", contract.c_str());
        }
        SendLine(std::string(buf));
    }
    SendLine("END");
}

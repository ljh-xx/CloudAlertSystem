// Notifier.cpp - Notification dispatch (online TCP push / offline email via EmailNotify.dll)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>

#include "Notifier.h"
#include "EmailSendUnitInterFace.h"
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>

// ---------------------------------------------------------------------------
// DLL function pointer types (must match exported signature exactly)
// ---------------------------------------------------------------------------

typedef CEmailSendUnitInterFace* (*FN_CreateEmailUnit)(
    char*, char*, char*, char*, bool, EN_CallBackNotify, uint32_t);
typedef void (*FN_FreeEmailUnit)(CEmailSendUnitInterFace*);

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

Notifier::Notifier()
    : m_hEmailDll(NULL), m_pEmailUnit(nullptr),
      m_emailAccount("jinan20"),
      m_emailPassword("KGCSHVRQMZEETIKX"),
      m_emailSmtp("smtp.126.com"),
      m_emailFrom("jinan20@126.com"),
      m_emailSsl(true)
{
    InitializeCriticalSection(&m_cs);
    LoadConfig("server.conf");
    InitEmail();
}

Notifier::~Notifier() {
    if (m_pEmailUnit && m_hEmailDll) {
        FN_FreeEmailUnit fnFree =
            (FN_FreeEmailUnit)GetProcAddress(m_hEmailDll, "ENDLL_FreeEmailNotifyUnit");
        if (fnFree) fnFree(m_pEmailUnit);
        m_pEmailUnit = nullptr;
    }
    if (m_hEmailDll) {
        FreeLibrary(m_hEmailDll);
        m_hEmailDll = NULL;
    }
    DeleteCriticalSection(&m_cs);
}

// ---------------------------------------------------------------------------
// Load config from server.conf (key=value format, # comments)
// ---------------------------------------------------------------------------

void Notifier::LoadConfig(const char* confPath) {
    std::ifstream f(confPath);
    if (!f.is_open()) {
        printf("[Notifier] server.conf not found, using default email credentials\n");
        return;
    }
    std::string line;
    while (std::getline(f, line)) {
        // Strip leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        if (line[0] == '#') continue;          // comment
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        // Trim trailing whitespace/CR
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!val.empty() && (val.back() == '\r' || val.back() == '\n' ||
                                val.back() == ' '  || val.back() == '\t')) val.pop_back();

        if (key == "email_account")  m_emailAccount  = val;
        if (key == "email_password") m_emailPassword = val;
        if (key == "email_smtp")     m_emailSmtp     = val;
        if (key == "email_from")     m_emailFrom     = val;
        if (key == "email_ssl")      m_emailSsl      = (val == "1" || val == "true");
    }
    printf("[Notifier] Loaded config: from=%s smtp=%s ssl=%s\n",
           m_emailFrom.c_str(), m_emailSmtp.c_str(), m_emailSsl ? "yes" : "no");
}

// ---------------------------------------------------------------------------
// Email DLL init (called once from constructor)
// ---------------------------------------------------------------------------

void Notifier::InitEmail() {
    m_hEmailDll = LoadLibraryA("EmailNotify.dll");
    if (!m_hEmailDll) {
        fprintf(stderr, "[Notifier] LoadLibrary(EmailNotify.dll) failed (err=%u) - email disabled\n",
                GetLastError());
        return;
    }

    FN_CreateEmailUnit fnCreate =
        (FN_CreateEmailUnit)GetProcAddress(m_hEmailDll, "ENDLL_CreateEmailNotifyUnit");
    if (!fnCreate) {
        fprintf(stderr, "[Notifier] ENDLL_CreateEmailNotifyUnit not found - email disabled\n");
        FreeLibrary(m_hEmailDll);
        m_hEmailDll = NULL;
        return;
    }

    EN_CallBackNotify nullCb;
    m_pEmailUnit = fnCreate(
        const_cast<char*>(m_emailAccount.c_str()),
        const_cast<char*>(m_emailPassword.c_str()),
        const_cast<char*>(m_emailSmtp.c_str()),
        const_cast<char*>(m_emailFrom.c_str()),
        m_emailSsl,   // true = SSL/TLS port 465
        nullCb,
        1
    );

    if (!m_pEmailUnit) {
        fprintf(stderr, "[Notifier] ENDLL_CreateEmailNotifyUnit returned null - email disabled\n");
        FreeLibrary(m_hEmailDll);
        m_hEmailDll = NULL;
        return;
    }

    printf("[Notifier] Email initialized: %s via %s (ssl=%s)\n",
           m_emailFrom.c_str(), m_emailSmtp.c_str(), m_emailSsl ? "yes" : "no");
}

// ---------------------------------------------------------------------------
// User registration
// ---------------------------------------------------------------------------

void Notifier::RegisterUser(int userId, SOCKET sock) {
    EnterCriticalSection(&m_cs);
    m_userSockets[userId] = sock;
    LeaveCriticalSection(&m_cs);
    printf("[Notifier] User %d online (socket %llu)\n",
           userId, (unsigned long long)sock);
}

void Notifier::UnregisterUser(int userId) {
    EnterCriticalSection(&m_cs);
    m_userSockets.erase(userId);
    LeaveCriticalSection(&m_cs);
    printf("[Notifier] User %d offline\n", userId);
}

bool Notifier::SendToUser(int userId, const std::string& data) {
    EnterCriticalSection(&m_cs);
    auto it = m_userSockets.find(userId);
    bool found = (it != m_userSockets.end());
    SOCKET sock = found ? it->second : INVALID_SOCKET;
    LeaveCriticalSection(&m_cs);

    if (!found || sock == INVALID_SOCKET)
        return false;

    int sent = send(sock, data.c_str(), (int)data.size(), 0);
    return (sent == (int)data.size());
}

// ---------------------------------------------------------------------------
// Notify: online -> TCP push; offline -> email
// ---------------------------------------------------------------------------

void Notifier::Notify(int userId,
                      const std::string& msg,
                      const std::string& emailAddress,
                      const std::string& subject,
                      const std::string& body) {
    EnterCriticalSection(&m_cs);
    auto it = m_userSockets.find(userId);
    bool online = (it != m_userSockets.end()) &&
                  (it->second != INVALID_SOCKET);
    SOCKET sock = online ? it->second : INVALID_SOCKET;
    LeaveCriticalSection(&m_cs);

    if (online) {
        int ret = send(sock, msg.c_str(), (int)msg.size(), 0);
        if (ret == (int)msg.size()) {
            printf("[Notifier] Online push to user %d: %s", userId, msg.c_str());
            return;
        }
        printf("[Notifier] Online push failed for user %d, falling back to email\n", userId);
    }

    if (!emailAddress.empty()) {
        SendEmail(emailAddress, subject, body);
    } else {
        printf("[Notifier] User %d offline, no email address configured\n", userId);
    }
}

// ---------------------------------------------------------------------------
// SendEmail: deliver via EmailNotify.dll
// ---------------------------------------------------------------------------

void Notifier::SendEmail(const std::string& toAddress,
                         const std::string& subject,
                         const std::string& body) {
    if (!m_pEmailUnit) {
        printf("[Notifier] Email unit not available, cannot send to %s\n", toAddress.c_str());
        fflush(stdout);
        return;
    }

    EMSend ems;
    ems.dRecvEmails = toAddress.c_str();
    ems.dSubject    = subject.c_str();
    ems.dInfo       = body.c_str();

    m_pEmailUnit->SendEmail(ems);
    printf("[Notifier] Email queued -> %s | %s\n", toAddress.c_str(), subject.c_str());
    fflush(stdout);
}

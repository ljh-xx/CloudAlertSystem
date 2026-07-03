#pragma once
// Notifier.h - Online push and offline email notification

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <map>
#include <string>

class CEmailSendUnitInterFace;

// Notifier manages which users are online (socket registered),
// and dispatches triggered-alert messages accordingly.
class Notifier {
public:
    Notifier();
    ~Notifier();

    // Register/unregister a user's socket (called by ClientSession)
    void RegisterUser(int userId, SOCKET sock);
    void UnregisterUser(int userId);

    // Send TRIGGERED message to online user; if offline, send email
    // msg should be the full protocol line including trailing '\n'
    void Notify(int userId, const std::string& msg,
                const std::string& emailAddress,
                const std::string& subject,
                const std::string& body);

    // Send raw data to a registered socket (used internally)
    bool SendToUser(int userId, const std::string& data);

private:
    CRITICAL_SECTION m_cs;
    std::map<int, SOCKET> m_userSockets; // user_id -> socket

    HMODULE                  m_hEmailDll;
    CEmailSendUnitInterFace* m_pEmailUnit;

    // Email credentials loaded from server.conf
    std::string m_emailAccount;
    std::string m_emailPassword;
    std::string m_emailSmtp;
    std::string m_emailFrom;
    bool        m_emailSsl;

    void LoadConfig(const char* confPath);
    void InitEmail();
    void SendEmail(const std::string& toAddress,
                   const std::string& subject,
                   const std::string& body);
};

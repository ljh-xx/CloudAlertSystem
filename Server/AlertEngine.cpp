// AlertEngine.cpp - Price and time alert evaluation

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "AlertEngine.h"
#include "CTPMdHandler.h"
#include "Protocol.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

static std::string GetCurrentTimeStr() {
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_s(&tm_info, &now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    return std::string(buf);
}

static std::string GetCurrentHMS() {
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_s(&tm_info, &now);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &tm_info);
    return std::string(buf);
}

// ---------------------------------------------------------------------------

AlertEngine::AlertEngine(AlertDB* db, Notifier* notifier)
    : m_db(db), m_notifier(notifier), m_ctpHandler(nullptr),
      m_timerThread(nullptr), m_timerRunning(false) {}

AlertEngine::~AlertEngine() {
    StopTimerThread();
}

void AlertEngine::SetCtpHandler(CTPMdHandler* ctpHandler) {
    m_ctpHandler = ctpHandler;
}

// ---------------------------------------------------------------------------
// Price alert check (called on every market data tick)
// ---------------------------------------------------------------------------

void AlertEngine::CheckPriceAlert(const std::string& contract,
                                  double lastPrice) {
    std::vector<AlertRecord> pending =
        m_db->GetPendingPriceAlerts(contract);

    for (const auto& rec : pending) {
        bool fired = false;
        if (rec.condition == COND_GE && lastPrice >= rec.trigger_price)
            fired = true;
        else if (rec.condition == COND_LE && lastPrice <= rec.trigger_price)
            fired = true;

        if (fired) {
            std::string ts = GetCurrentTimeStr();
            if (m_db->TriggerAlert(rec.id, ts)) {
                FireAlert(rec, lastPrice, ts);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Time alert check (called once per second)
// ---------------------------------------------------------------------------

void AlertEngine::CheckTimeAlerts() {
    std::string hms = GetCurrentHMS();
    std::vector<AlertRecord> pending =
        m_db->GetPendingTimeAlerts(hms);

    for (const auto& rec : pending) {
        std::string ts = GetCurrentTimeStr();
        if (m_db->TriggerAlert(rec.id, ts)) {
            FireAlert(rec, 0.0, ts);
        }
    }
}

// ---------------------------------------------------------------------------
// Fire: update DB, build protocol message, dispatch via Notifier
// ---------------------------------------------------------------------------

void AlertEngine::FireAlert(const AlertRecord& rec,
                            double lastPrice,
                            const std::string& timestamp) {
    // Build TRIGGERED protocol line
    char msgBuf[256];
    _snprintf_s(msgBuf, sizeof(msgBuf),
        "TRIGGERED %d %s %.4f %s\n",
        rec.id,
        rec.contract.c_str(),
        lastPrice,
        timestamp.c_str());

    std::string msg(msgBuf);

    // Get user email for offline fallback
    std::string email = m_db->GetEmail(rec.user_id);

    // Build email subject/body
    char subjectBuf[256];
    char bodyBuf[512];
    _snprintf_s(subjectBuf, sizeof(subjectBuf),
        "Alert Triggered: %s", rec.contract.c_str());
    if (rec.alert_type == ALERT_TYPE_PRICE) {
        _snprintf_s(bodyBuf, sizeof(bodyBuf),
            "Alert ID %d: %s reached %.4f at %s",
            rec.id, rec.contract.c_str(), lastPrice, timestamp.c_str());
    } else {
        _snprintf_s(bodyBuf, sizeof(bodyBuf),
            "Alert ID %d: %s time alert triggered at %s",
            rec.id, rec.contract.c_str(), timestamp.c_str());
    }

    printf("[AlertEngine] Firing alert id=%d user=%d contract=%s ts=%s\n",
           rec.id, rec.user_id, rec.contract.c_str(), timestamp.c_str());

    m_notifier->Notify(rec.user_id, msg, email,
                       std::string(subjectBuf),
                       std::string(bodyBuf));
}

// ---------------------------------------------------------------------------
// Pending contracts (for CTP subscription)
// ---------------------------------------------------------------------------

std::vector<std::string> AlertEngine::GetPendingContracts() {
    return m_db->GetPendingContracts();
}

void AlertEngine::SubscribeContract(const std::string& contract) {
    if (m_ctpHandler)
        m_ctpHandler->Subscribe({ contract });
}


// ---------------------------------------------------------------------------
// Timer thread
// ---------------------------------------------------------------------------

DWORD WINAPI AlertEngine::TimerThreadFunc(LPVOID pParam) {
    AlertEngine* engine = reinterpret_cast<AlertEngine*>(pParam);
    printf("[AlertEngine] Timer thread started\n");
    while (engine->m_timerRunning) {
        engine->CheckTimeAlerts();
        Sleep(1000);
    }
    printf("[AlertEngine] Timer thread stopped\n");
    return 0;
}

bool AlertEngine::StartTimerThread() {
    m_timerRunning = true;
    m_timerThread = CreateThread(
        nullptr, 0,
        TimerThreadFunc,
        this,
        0, nullptr);
    if (!m_timerThread) {
        fprintf(stderr, "[AlertEngine] Failed to create timer thread\n");
        m_timerRunning = false;
        return false;
    }
    return true;
}

void AlertEngine::StopTimerThread() {
    m_timerRunning = false;
    if (m_timerThread) {
        WaitForSingleObject(m_timerThread, 3000);
        CloseHandle(m_timerThread);
        m_timerThread = nullptr;
    }
}

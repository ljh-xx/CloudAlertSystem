// AlertEngine.cpp - Price and time alert evaluation

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "AlertEngine.h"
#include "CTPMdHandler.h"
#include "Protocol.h"
#include <algorithm>
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
      m_timerThread(nullptr), m_timerRunning(false) {
    RefreshPriceAlertCache();
}

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
    if (lastPrice > 0.0 && lastPrice < 1.0e100) {
        std::lock_guard<std::mutex> lk(m_priceMtx);
        m_lastPrices[contract] = lastPrice;
    }

    std::vector<AlertRecord> pending;
    {
        std::lock_guard<std::mutex> lk(m_alertCacheMtx);
        auto it = m_priceAlertCache.find(contract);
        if (it != m_priceAlertCache.end()) {
            pending = it->second;
        }
    }

    for (const auto& rec : pending) {
        bool fired = false;
        if (rec.condition == COND_GE && lastPrice >= rec.trigger_price)
            fired = true;
        else if (rec.condition == COND_LE && lastPrice <= rec.trigger_price)
            fired = true;

        if (fired) {
            std::string ts = GetCurrentTimeStr();
            if (m_db->TriggerAlert(rec.id, ts)) {
                RemoveCachedPriceAlert(rec.id);
                FireAlert(rec, lastPrice, ts);
            }
        }
    }
}

bool AlertEngine::GetLastPrice(const std::string& contract, double& lastPrice) const {
    std::lock_guard<std::mutex> lk(m_priceMtx);
    auto it = m_lastPrices.find(contract);
    if (it == m_lastPrices.end()) return false;
    lastPrice = it->second;
    return true;
}

void AlertEngine::RefreshPriceAlertCache() {
    std::map<std::string, std::vector<AlertRecord>> nextCache;
    std::vector<std::string> contracts = m_db->GetPendingContracts();

    for (const auto& contract : contracts) {
        std::vector<AlertRecord> records = m_db->GetPendingPriceAlerts(contract);
        if (!records.empty()) {
            nextCache[contract] = records;
        }
    }

    size_t alertCount = 0;
    for (const auto& item : nextCache) {
        alertCount += item.second.size();
    }

    size_t contractCount = 0;
    {
        std::lock_guard<std::mutex> lk(m_alertCacheMtx);
        m_priceAlertCache.swap(nextCache);
        contractCount = m_priceAlertCache.size();
    }
    printf("[AlertEngine] Price alert cache refreshed: %llu contract(s), %llu alert(s)\n",
           (unsigned long long)contractCount,
           (unsigned long long)alertCount);
}

void AlertEngine::RemoveCachedPriceAlert(int alertId) {
    std::lock_guard<std::mutex> lk(m_alertCacheMtx);
    for (auto it = m_priceAlertCache.begin(); it != m_priceAlertCache.end(); ) {
        auto& records = it->second;
        records.erase(
            std::remove_if(records.begin(), records.end(),
                [alertId](const AlertRecord& rec) { return rec.id == alertId; }),
            records.end());
        if (records.empty()) {
            it = m_priceAlertCache.erase(it);
        } else {
            ++it;
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
        "TRIGGERED %d %s %.2f %s\n",
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
            "Alert ID %d: %s reached %.2f at %s",
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

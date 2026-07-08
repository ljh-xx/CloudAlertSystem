#pragma once
// AlertEngine.h - Alert evaluation: price checks and time checks

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "AlertDB.h"
#include "Notifier.h"

// Forward declaration to avoid circular includes
class CTPMdHandler;

class AlertEngine {
public:
    AlertEngine(AlertDB* db, Notifier* notifier);
    ~AlertEngine();

    // Set the CTP handler pointer (used to subscribe new contracts on demand)
    void SetCtpHandler(CTPMdHandler* ctpHandler);

    // Called by CTPMdHandler when a market data tick arrives
    void CheckPriceAlert(const std::string& contract, double lastPrice);

    // Read the latest cached market price for a contract.
    bool GetLastPrice(const std::string& contract, double& lastPrice) const;

    // Called once per second by the timer thread
    void CheckTimeAlerts();

    // Get all distinct contracts from pending alerts (for CTP subscription)
    std::vector<std::string> GetPendingContracts();

    // Subscribe a single contract via CTP immediately (called when a new alert is added)
    void SubscribeContract(const std::string& contract);


    // Timer thread entry point
    static DWORD WINAPI TimerThreadFunc(LPVOID pParam);

    // Start the once-per-second timer thread
    bool StartTimerThread();

    // Stop the timer thread
    void StopTimerThread();

private:
    AlertDB*      m_db;
    Notifier*     m_notifier;
    CTPMdHandler* m_ctpHandler;

    HANDLE  m_timerThread;
    bool    m_timerRunning;
    mutable std::mutex m_priceMtx;
    std::map<std::string, double> m_lastPrices;

    // Build and dispatch a TRIGGERED protocol line
    void FireAlert(const AlertRecord& rec,
                   double lastPrice,
                   const std::string& timestamp);
};

#pragma once
// AlertDB.h - Database access layer for Cloud Alert System

#include <string>
#include <vector>
#include "sqlite3.h"

struct AlertRecord {
    int         id;
    int         user_id;
    int         alert_type;   // 0=price, 1=time
    int         condition;    // 0=>=, 1=<=
    int         status;       // 0=pending, 1=triggered, 2=deleted
    std::string contract;
    std::string trigger_time;
    std::string created_at;
    std::string triggered_at;
    double      trigger_price;
};

class AlertDB {
public:
    AlertDB();
    ~AlertDB();

    // Open (or create) database at dbPath, create tables if needed
    bool Init(const std::string& dbPath);

    // Users
    // Register a new user; returns uid on success, -1 if username already taken with a password
    int  RegisterUser(const std::string& username,
                      const std::string& password,
                      const std::string& email);
    // Verify login credentials; returns uid on success, -1 on failure
    int  VerifyLogin(const std::string& username, const std::string& password);
    int  GetUserId(const std::string& username);
    bool SetEmail(int userId, const std::string& email);
    std::string GetEmail(int userId);

    // Alerts
    int  AddPriceAlert(int userId, const std::string& contract,
                       double price, int condition);
    int  AddTimeAlert(int userId, const std::string& contract,
                      const std::string& time);
    bool DeleteAlert(int alertId, int userId);
    bool ModifyPriceAlert(int alertId, int userId,
                          const std::string& contract,
                          double price, int cond);
    bool ModifyTimeAlert(int alertId, int userId,
                         const std::string& contract,
                         const std::string& time);

    // Query alerts by user and status (-1 = all)
    std::vector<AlertRecord> QueryAlerts(int userId, int status);

    // Get all pending price alerts for a given contract (for engine check)
    std::vector<AlertRecord> GetPendingPriceAlerts(const std::string& contract);

    // Get all pending time alerts matching the given time string "HH:MM:SS"
    std::vector<AlertRecord> GetPendingTimeAlerts(const std::string& timeStr);

    // Mark alert as triggered
    bool TriggerAlert(int alertId, const std::string& triggeredAt);

    // Get distinct contracts from pending alerts (for CTP subscription)
    std::vector<std::string> GetPendingContracts();

private:
    sqlite3* m_db;

    bool ExecSQL(const char* sql);
    bool CreateTables();
    bool MigrateSchema();   // add password column if missing
    AlertRecord RowToRecord(sqlite3_stmt* stmt);
};

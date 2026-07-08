// AlertDB.cpp - SQLite implementation of alert database

#include "AlertDB.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

class DbLock {
public:
    explicit DbLock(CRITICAL_SECTION& cs) : m_cs(cs) {
        EnterCriticalSection(&m_cs);
    }
    ~DbLock() {
        LeaveCriticalSection(&m_cs);
    }
private:
    CRITICAL_SECTION& m_cs;
};

static std::string CurrentTimestamp() {
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_s(&tm_info, &now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    return std::string(buf);
}

AlertDB::AlertDB() : m_db(nullptr) {
    InitializeCriticalSection(&m_cs);
}

AlertDB::~AlertDB() {
    EnterCriticalSection(&m_cs);
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
    LeaveCriticalSection(&m_cs);
    DeleteCriticalSection(&m_cs);
}

bool AlertDB::Init(const std::string& dbPath) {
    DbLock lock(m_cs);
    int rc = sqlite3_open(dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AlertDB] Cannot open database: %s\n",
                sqlite3_errmsg(m_db));
        return false;
    }
    ExecSQL("PRAGMA journal_mode=WAL;");
    ExecSQL("PRAGMA foreign_keys=ON;");
    if (!CreateTables()) return false;
    MigrateSchema();   // add password column to existing DB if missing
    return true;
}

bool AlertDB::MigrateSchema() {
    DbLock lock(m_cs);
    // ALTER TABLE ADD COLUMN fails silently on "duplicate column name" — that's fine
    char* errMsg = nullptr;
    sqlite3_exec(m_db,
        "ALTER TABLE users ADD COLUMN password TEXT NOT NULL DEFAULT '';",
        nullptr, nullptr, &errMsg);
    if (errMsg) {
        std::string err(errMsg);
        sqlite3_free(errMsg);
        // Only warn if it's not the expected duplicate-column error
        if (err.find("duplicate column name") == std::string::npos)
            fprintf(stderr, "[AlertDB] Migration warning: %s\n", err.c_str());
    }
    return true;
}

bool AlertDB::ExecSQL(const char* sql) {
    DbLock lock(m_cs);
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AlertDB] SQL error: %s\n", errMsg ? errMsg : "unknown");
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool AlertDB::CreateTables() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT NOT NULL UNIQUE,"
        "  email    TEXT NOT NULL DEFAULT ''"
        ");"
        "CREATE TABLE IF NOT EXISTS alerts ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  user_id       INTEGER NOT NULL,"
        "  alert_type    INTEGER NOT NULL,"
        "  contract      TEXT NOT NULL,"
        "  trigger_price REAL    DEFAULT 0,"
        "  condition     INTEGER DEFAULT 0,"
        "  trigger_time  TEXT    DEFAULT '',"
        "  status        INTEGER DEFAULT 0,"
        "  created_at    TEXT NOT NULL,"
        "  triggered_at  TEXT    DEFAULT ''"
        ");";
    return ExecSQL(sql);
}

// ---------------------------------------------------------------------------
// Users
// ---------------------------------------------------------------------------

// Register new user.
// Returns uid on success.
// Returns -1 if username already exists with a non-empty password (taken).
// If username exists with empty password (legacy / incomplete), update password+email.
int AlertDB::RegisterUser(const std::string& username,
                          const std::string& password,
                          const std::string& email) {
    DbLock lock(m_cs);
    // Check if user already exists
    const char* checkSql = "SELECT id, password FROM users WHERE username=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, checkSql, -1, &stmt, nullptr) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    int existId = -1;
    std::string existPwd;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        existId = sqlite3_column_int(stmt, 0);
        const char* p = (const char*)sqlite3_column_text(stmt, 1);
        existPwd = p ? p : "";
    }
    sqlite3_finalize(stmt);

    if (existId >= 0) {
        if (!existPwd.empty()) {
            // Username taken with a real password
            return -1;
        }
        // Legacy user with no password: set password and email
        const char* upd = "UPDATE users SET password=?, email=? WHERE id=?;";
        if (sqlite3_prepare_v2(m_db, upd, -1, &stmt, nullptr) != SQLITE_OK)
            return -1;
        sqlite3_bind_text(stmt, 1, password.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, email.c_str(),    -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt,  3, existId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return existId;
    }

    // New user: insert
    const char* sql =
        "INSERT INTO users (username, password, email) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, email.c_str(),    -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return -1;
    return (int)sqlite3_last_insert_rowid(m_db);
}

// Verify login credentials. Returns uid on success, -1 on failure.
int AlertDB::VerifyLogin(const std::string& username,
                         const std::string& password) {
    DbLock lock(m_cs);
    const char* sql = "SELECT id, password FROM users WHERE username=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    int uid = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* p = (const char*)sqlite3_column_text(stmt, 1);
        std::string storedPwd = p ? p : "";
        if (storedPwd == password)
            uid = id;
    }
    sqlite3_finalize(stmt);
    return uid;
}

int AlertDB::GetUserId(const std::string& username) {
    DbLock lock(m_cs);
    const char* sql = "SELECT id FROM users WHERE username=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return id;
}

bool AlertDB::SetEmail(int userId, const std::string& email) {
    DbLock lock(m_cs);
    const char* sql = "UPDATE users SET email=? WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  2, userId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::string AlertDB::GetEmail(int userId) {
    DbLock lock(m_cs);
    const char* sql = "SELECT email FROM users WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return "";
    sqlite3_bind_int(stmt, 1, userId);
    std::string email;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* p = (const char*)sqlite3_column_text(stmt, 0);
        if (p) email = p;
    }
    sqlite3_finalize(stmt);
    return email;
}

// ---------------------------------------------------------------------------
// Alerts
// ---------------------------------------------------------------------------

int AlertDB::AddPriceAlert(int userId, const std::string& contract,
                           double price, int condition) {
    DbLock lock(m_cs);
    std::string ts = CurrentTimestamp();
    const char* sql =
        "INSERT INTO alerts (user_id, alert_type, contract, trigger_price, "
        "condition, trigger_time, status, created_at) "
        "VALUES (?, 0, ?, ?, ?, '', 0, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return -1;
    sqlite3_bind_int(stmt,    1, userId);
    sqlite3_bind_text(stmt,   2, contract.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, price);
    sqlite3_bind_int(stmt,    4, condition);
    sqlite3_bind_text(stmt,   5, ts.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return -1;
    return (int)sqlite3_last_insert_rowid(m_db);
}

int AlertDB::AddTimeAlert(int userId, const std::string& contract,
                          const std::string& time) {
    DbLock lock(m_cs);
    std::string ts = CurrentTimestamp();
    const char* sql =
        "INSERT INTO alerts (user_id, alert_type, contract, trigger_price, "
        "condition, trigger_time, status, created_at) "
        "VALUES (?, 1, ?, 0, 0, ?, 0, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return -1;
    sqlite3_bind_int(stmt,  1, userId);
    sqlite3_bind_text(stmt, 2, contract.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, time.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, ts.c_str(),       -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return -1;
    return (int)sqlite3_last_insert_rowid(m_db);
}

bool AlertDB::DeleteAlert(int alertId, int userId) {
    DbLock lock(m_cs);
    // Soft delete: set status=2
    const char* sql =
        "UPDATE alerts SET status=2 WHERE id=? AND user_id=? AND status!=2;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(stmt, 1, alertId);
    sqlite3_bind_int(stmt, 2, userId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE)
              && (sqlite3_changes(m_db) > 0);
    sqlite3_finalize(stmt);
    return ok;
}

bool AlertDB::ModifyPriceAlert(int alertId, int userId,
                               const std::string& contract,
                               double price, int cond) {
    DbLock lock(m_cs);
    const char* sql =
        "UPDATE alerts SET contract=?, trigger_price=?, condition=?, "
        "alert_type=0, status=0 "
        "WHERE id=? AND user_id=? AND status!=2;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt,   1, contract.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, price);
    sqlite3_bind_int(stmt,    3, cond);
    sqlite3_bind_int(stmt,    4, alertId);
    sqlite3_bind_int(stmt,    5, userId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE)
              && (sqlite3_changes(m_db) > 0);
    sqlite3_finalize(stmt);
    return ok;
}

bool AlertDB::ModifyTimeAlert(int alertId, int userId,
                              const std::string& contract,
                              const std::string& time) {
    DbLock lock(m_cs);
    const char* sql =
        "UPDATE alerts SET contract=?, trigger_time=?, alert_type=1, status=0 "
        "WHERE id=? AND user_id=? AND status!=2;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, contract.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, time.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  3, alertId);
    sqlite3_bind_int(stmt,  4, userId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE)
              && (sqlite3_changes(m_db) > 0);
    sqlite3_finalize(stmt);
    return ok;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

AlertRecord AlertDB::RowToRecord(sqlite3_stmt* stmt) {
    AlertRecord r;
    r.id            = sqlite3_column_int(stmt, 0);
    r.user_id       = sqlite3_column_int(stmt, 1);
    r.alert_type    = sqlite3_column_int(stmt, 2);
    const char* c   = (const char*)sqlite3_column_text(stmt, 3);
    r.contract      = c ? c : "";
    r.trigger_price = sqlite3_column_double(stmt, 4);
    r.condition     = sqlite3_column_int(stmt, 5);
    const char* t   = (const char*)sqlite3_column_text(stmt, 6);
    r.trigger_time  = t ? t : "";
    r.status        = sqlite3_column_int(stmt, 7);
    const char* ca  = (const char*)sqlite3_column_text(stmt, 8);
    r.created_at    = ca ? ca : "";
    const char* ta  = (const char*)sqlite3_column_text(stmt, 9);
    r.triggered_at  = ta ? ta : "";
    return r;
}

std::vector<AlertRecord> AlertDB::QueryAlerts(int userId, int status) {
    DbLock lock(m_cs);
    std::vector<AlertRecord> result;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = nullptr;

    if (status == -1) {
        sql = "SELECT id,user_id,alert_type,contract,trigger_price,condition,"
              "trigger_time,status,created_at,triggered_at "
              "FROM alerts WHERE user_id=? ORDER BY id;";
    } else {
        sql = "SELECT id,user_id,alert_type,contract,trigger_price,condition,"
              "trigger_time,status,created_at,triggered_at "
              "FROM alerts WHERE user_id=? AND status=? ORDER BY id;";
    }

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return result;

    sqlite3_bind_int(stmt, 1, userId);
    if (status != -1)
        sqlite3_bind_int(stmt, 2, status);

    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(RowToRecord(stmt));

    sqlite3_finalize(stmt);
    return result;
}

std::vector<AlertRecord> AlertDB::GetPendingPriceAlerts(
    const std::string& contract) {
    DbLock lock(m_cs);
    std::vector<AlertRecord> result;
    const char* sql =
        "SELECT id,user_id,alert_type,contract,trigger_price,condition,"
        "trigger_time,status,created_at,triggered_at "
        "FROM alerts WHERE status=0 AND alert_type=0 AND contract=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return result;
    sqlite3_bind_text(stmt, 1, contract.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(RowToRecord(stmt));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<AlertRecord> AlertDB::GetPendingTimeAlerts(
    const std::string& timeStr) {
    DbLock lock(m_cs);
    std::vector<AlertRecord> result;
    const char* sql =
        "SELECT id,user_id,alert_type,contract,trigger_price,condition,"
        "trigger_time,status,created_at,triggered_at "
        "FROM alerts WHERE status=0 AND alert_type=1 AND trigger_time=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return result;
    sqlite3_bind_text(stmt, 1, timeStr.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.push_back(RowToRecord(stmt));
    sqlite3_finalize(stmt);
    return result;
}

bool AlertDB::TriggerAlert(int alertId, const std::string& triggeredAt) {
    DbLock lock(m_cs);
    const char* sql =
        "UPDATE alerts SET status=1, triggered_at=? WHERE id=? AND status=0;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, triggeredAt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  2, alertId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE)
              && (sqlite3_changes(m_db) > 0);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<std::string> AlertDB::GetPendingContracts() {
    DbLock lock(m_cs);
    std::vector<std::string> result;
    const char* sql =
        "SELECT DISTINCT contract FROM alerts WHERE status=0;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* p = (const char*)sqlite3_column_text(stmt, 0);
        if (p) result.push_back(std::string(p));
    }
    sqlite3_finalize(stmt);
    return result;
}

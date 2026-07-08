#pragma once
// Protocol.h - Text-line protocol constants for Cloud Alert System

// ---- Client -> Server commands ----
#define CMD_REGISTER        "REGISTER"
#define CMD_LOGIN           "LOGIN"
#define CMD_ADD_PRICE       "ADD_PRICE"
#define CMD_ADD_TIME        "ADD_TIME"
#define CMD_DELETE          "DELETE"
#define CMD_MODIFY_PRICE    "MODIFY_PRICE"
#define CMD_MODIFY_TIME     "MODIFY_TIME"
#define CMD_QUERY           "QUERY"
#define CMD_SET_EMAIL       "SET_EMAIL"       // SET_EMAIL <username> <password> <email>
#define CMD_GET_EMAIL       "GET_EMAIL"
#define CMD_GET_PRICE       "GET_PRICE"
#define CMD_GET_PRICES      "GET_PRICES"

// ---- Server -> Client responses ----
#define RESP_OK             "OK"
#define RESP_ERR            "ERR"
#define RESP_TRIGGERED      "TRIGGERED"
#define RESP_ALERT          "ALERT"
#define RESP_PRICE          "PRICE"
#define RESP_END            "END"

// ---- Alert types ----
#define ALERT_TYPE_PRICE    0
#define ALERT_TYPE_TIME     1

// ---- Condition codes ----
#define COND_GE             0   // last_price >= trigger_price
#define COND_LE             1   // last_price <= trigger_price

// ---- Status codes ----
#define STATUS_PENDING      0
#define STATUS_TRIGGERED    1
#define STATUS_DELETED      2

// ---- Query all status ----
#define QUERY_ALL          -1

// ---- Network ----
#define SERVER_PORT         8888
#define RECV_BUF_SIZE       4096

// ---- CTP ----
#define CTP_FRONT_ADDR      "tcp://182.254.243.31:30011"
#define CTP_BROKER_ID       ""
#define CTP_USER_ID         ""
#define CTP_PASSWORD        ""

// ---- Database ----
#define DB_FILE_NAME        "alerts.db"

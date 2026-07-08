// CTPMdHandler.cpp - CTP market data implementation

#include "CTPMdHandler.h"
#include "Protocol.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

CTPMdHandler::CTPMdHandler(AlertEngine* engine)
    : m_pApi(nullptr), m_engine(engine), m_requestId(0) {}

CTPMdHandler::~CTPMdHandler() {
    if (m_pApi) {
        m_pApi->RegisterSpi(nullptr);
        m_pApi->Release();
        m_pApi = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Connect
// ---------------------------------------------------------------------------

bool CTPMdHandler::Connect(const char* frontAddr) {
    if (sizeof(TThostFtdcPriceType) < sizeof(double)) {
        fprintf(stderr,
            "[CTP] Real market data is unavailable: bundled CTP headers are stubs. "
            "Replace Server/ctp headers and link the official thostmduserapi_se import library.\n");
        return false;
    }

    // CTP creates a flow directory; use current directory
    m_pApi = CThostFtdcMdApi::CreateFtdcMdApi("./ctpflow/", false, false);
    if (!m_pApi) {
        fprintf(stderr, "[CTP] CreateFtdcMdApi failed\n");
        return false;
    }
    m_pApi->RegisterSpi(this);
    m_pApi->RegisterFront(const_cast<char*>(frontAddr));
    m_pApi->Init();
    printf("[CTP] Connecting to %s ...\n", frontAddr);
    return true;
}

// ---------------------------------------------------------------------------
// Subscribe / Unsubscribe
// ---------------------------------------------------------------------------

void CTPMdHandler::Subscribe(const std::vector<std::string>& contracts) {
    if (contracts.empty()) return;
    if (!m_pApi) return;
    int n = (int)contracts.size();
    char** ppIds = new char*[n];
    for (int i = 0; i < n; ++i)
        ppIds[i] = const_cast<char*>(contracts[i].c_str());
    m_pApi->SubscribeMarketData(ppIds, n);
    printf("[CTP] Subscribing %d contract(s)\n", n);
    delete[] ppIds;
}

void CTPMdHandler::Unsubscribe(const std::vector<std::string>& contracts) {
    if (contracts.empty()) return;
    if (!m_pApi) return;
    int n = (int)contracts.size();
    char** ppIds = new char*[n];
    for (int i = 0; i < n; ++i)
        ppIds[i] = const_cast<char*>(contracts[i].c_str());
    m_pApi->UnSubscribeMarketData(ppIds, n);
    printf("[CTP] Unsubscribing %d contract(s)\n", n);
    delete[] ppIds;
}

// ---------------------------------------------------------------------------
// SPI Callbacks
// ---------------------------------------------------------------------------

void CTPMdHandler::OnFrontConnected() {
    printf("[CTP] Front connected. Logging in...\n");
    CThostFtdcReqUserLoginField req = {};
    // For public market data feed, broker/user/password can be empty
    strncpy_s(req.BrokerID,  sizeof(req.BrokerID),  CTP_BROKER_ID, _TRUNCATE);
    strncpy_s(req.UserID,    sizeof(req.UserID),     CTP_USER_ID,   _TRUNCATE);
    strncpy_s(req.Password,  sizeof(req.Password),   CTP_PASSWORD,  _TRUNCATE);
    int rc = m_pApi->ReqUserLogin(&req, NextRequestId());
    if (rc != 0)
        fprintf(stderr, "[CTP] ReqUserLogin returned %d\n", rc);
}

void CTPMdHandler::OnFrontDisconnected(int nReason) {
    printf("[CTP] Front disconnected, reason=%d. Will reconnect automatically.\n",
           nReason);
}

void CTPMdHandler::OnRspUserLogin(
    CThostFtdcRspUserLoginField*  pRspUserLogin,
    CThostFtdcRspInfoField*       pRspInfo,
    int /*nRequestID*/, bool /*bIsLast*/)
{
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        fprintf(stderr, "[CTP] Login error %d: %s\n",
                pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        return;
    }
    printf("[CTP] Login OK. TradingDay=%s\n",
           pRspUserLogin ? pRspUserLogin->TradingDay : "?");

    // Fetch all pending contracts from the AlertEngine and subscribe
    std::vector<std::string> contracts = m_engine->GetPendingContracts();
    if (contracts.empty()) {
        printf("[CTP] No pending contracts to subscribe at startup.\n");
    } else {
        Subscribe(contracts);
    }
}

void CTPMdHandler::OnRspError(
    CThostFtdcRspInfoField* pRspInfo,
    int /*nRequestID*/, bool /*bIsLast*/)
{
    if (pRspInfo)
        fprintf(stderr, "[CTP] Error %d: %s\n",
                pRspInfo->ErrorID, pRspInfo->ErrorMsg);
}

void CTPMdHandler::OnRspSubMarketData(
    CThostFtdcSpecificInstrumentField* pSpecificInstrument,
    CThostFtdcRspInfoField*            pRspInfo,
    int /*nRequestID*/, bool /*bIsLast*/)
{
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        fprintf(stderr, "[CTP] Subscribe error %d: %s\n",
                pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        return;
    }
    if (pSpecificInstrument)
        printf("[CTP] Subscribed: %s\n", pSpecificInstrument->InstrumentID);
}

void CTPMdHandler::OnRtnDepthMarketData(
    CThostFtdcDepthMarketDataField* pDepthMarketData)
{
    if (!pDepthMarketData) return;

    const char* contract = pDepthMarketData->InstrumentID;
    double lastPrice     = pDepthMarketData->LastPrice;

    // Pass to alert engine for evaluation
    m_engine->CheckPriceAlert(std::string(contract), lastPrice);
}

void CTPMdHandler::OnHeartBeatWarning(int nTimeLapse) {
    printf("[CTP] HeartBeat warning, time lapse=%d\n", nTimeLapse);
}

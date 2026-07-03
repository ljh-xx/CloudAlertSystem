#pragma once
// CTPMdHandler.h - CTP market data SPI implementation
//
// IMPORTANT: Before compiling, copy the CTP header files to a subdirectory
// named "ctp/" under the Server/ directory:
//   Server/ctp/ThostFtdcMdApi.h
//   Server/ctp/ThostFtdcUserApiDataType.h
//   Server/ctp/ThostFtdcUserApiStruct.h
// Also copy the CTP import library (thostmduserapi_se.lib) and DLL
// (thostmduserapi_se.dll) to the Server/ directory.

#include "ctp/ThostFtdcMdApi.h"
#include <string>
#include <vector>
#include "AlertEngine.h"

class CTPMdHandler : public CThostFtdcMdSpi {
public:
    CTPMdHandler(AlertEngine* engine);
    ~CTPMdHandler();

    // Connect to CTP front and start the SPI event loop (non-blocking after init)
    bool Connect(const char* frontAddr);

    // Subscribe a list of instrument IDs
    void Subscribe(const std::vector<std::string>& contracts);

    // Unsubscribe a list of instrument IDs
    void Unsubscribe(const std::vector<std::string>& contracts);

    // --- CThostFtdcMdSpi callbacks ---
    virtual void OnFrontConnected() override;
    virtual void OnFrontDisconnected(int nReason) override;
    virtual void OnRspUserLogin(
        CThostFtdcRspUserLoginField*   pRspUserLogin,
        CThostFtdcRspInfoField*        pRspInfo,
        int nRequestID, bool bIsLast) override;
    virtual void OnRspError(
        CThostFtdcRspInfoField*        pRspInfo,
        int nRequestID, bool bIsLast) override;
    virtual void OnRspSubMarketData(
        CThostFtdcSpecificInstrumentField* pSpecificInstrument,
        CThostFtdcRspInfoField*            pRspInfo,
        int nRequestID, bool bIsLast) override;
    virtual void OnRtnDepthMarketData(
        CThostFtdcDepthMarketDataField* pDepthMarketData) override;
    virtual void OnHeartBeatWarning(int nTimeLapse) override;

private:
    CThostFtdcMdApi* m_pApi;
    AlertEngine*     m_engine;
    int              m_requestId;

    int NextRequestId() { return ++m_requestId; }
};

#pragma once
#include "ThostFtdcUserApiDataType.h"
#include "ThostFtdcUserApiStruct.h"

struct CThostFtdcReqUserLoginField {
    TThostFtdcBrokerIDType BrokerID;
    TThostFtdcUserIDType UserID;
    TThostFtdcPasswordType Password;
};

struct CThostFtdcUserLogoutField {
    TThostFtdcBrokerIDType BrokerID;
    TThostFtdcUserIDType UserID;
};

struct CThostFtdcQryMulticastInstrumentField {
    TThostFtdcExchangeIDType ExchangeID;
};

class CThostFtdcMdSpi {
public:
    virtual ~CThostFtdcMdSpi() {}
    virtual void OnFrontConnected() {}
    virtual void OnFrontDisconnected(int nReason) {}
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
    virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {}
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) {}
    virtual void OnHeartBeatWarning(int nTimeLapse) {}
};

class CThostFtdcMdApi {
public:
    static CThostFtdcMdApi* CreateFtdcMdApi(const char* pszFlowPath = "", const bool bIsUsingUdp = false, const bool bIsMulticast = false) {
        return new CThostFtdcMdApi();
    }
    
    virtual ~CThostFtdcMdApi() {}
    virtual void RegisterFront(char* pszFrontAddress) {}
    virtual void RegisterSpi(CThostFtdcMdSpi* pSpi) {}
    virtual void Init() {}
    virtual int Join() { return 0; }
    virtual void Exit() {}
    virtual void Release() {}
    virtual int ReqUserLogin(CThostFtdcReqUserLoginField* pReqUserLoginField, int nRequestID) { return 0; }
    virtual int ReqUserLogout(CThostFtdcUserLogoutField* pUserLogout, int nRequestID) { return 0; }
    virtual int SubscribeMarketData(char* ppInstrumentID[], int nCount) { return 0; }
    virtual int UnSubscribeMarketData(char* ppInstrumentID[], int nCount) { return 0; }
    virtual int ReqQueryMulticastInstrument(CThostFtdcQryMulticastInstrumentField* pQryMulticastInstrument, int nRequestID) { return 0; }
};

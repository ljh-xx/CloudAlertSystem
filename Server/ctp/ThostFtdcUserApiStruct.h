#pragma once
#include "ThostFtdcUserApiDataType.h"

struct CThostFtdcRspInfoField {
    TThostFtdcErrorIDType ErrorID;
    TThostFtdcErrorMsgType ErrorMsg;
};

struct CThostFtdcRspUserLoginField {
    TThostFtdcDateType TradingDay;
    TThostFtdcTimeType LoginTime;
    TThostFtdcBrokerIDType BrokerID;
    TThostFtdcUserIDType UserID;
};

struct CThostFtdcSpecificInstrumentField {
    TThostFtdcInstrumentIDType InstrumentID;
};

struct CThostFtdcDepthMarketDataField {
    TThostFtdcInstrumentIDType InstrumentID;
    TThostFtdcExchangeIDType ExchangeID;
    TThostFtdcPriceType LastPrice;
    TThostFtdcPriceType PreSettlementPrice;
    TThostFtdcPriceType PreClosePrice;
    TThostFtdcPriceType PreOpenInterest;
    TThostFtdcPriceType OpenPrice;
    TThostFtdcPriceType HighestPrice;
    TThostFtdcPriceType LowestPrice;
    TThostFtdcPriceType Volume;
    TThostFtdcPriceType Turnover;
    TThostFtdcPriceType OpenInterest;
    TThostFtdcPriceType ClosePrice;
    TThostFtdcPriceType SettlementPrice;
    TThostFtdcPriceType UpperLimitPrice;
    TThostFtdcPriceType LowerLimitPrice;
    TThostFtdcPriceType PreDelta;
    TThostFtdcPriceType CurrDelta;
    TThostFtdcPriceType BidPrice1;
    TThostFtdcVolumeType BidVolume1;
    TThostFtdcPriceType AskPrice1;
    TThostFtdcVolumeType AskVolume1;
    TThostFtdcDateType TradingDay;
    TThostFtdcTimeType UpdateTime;
    TThostFtdcIntType UpdateMillisec;
};

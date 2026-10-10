#ifndef ITCH_MSG_TYPE
#define ITCH_MSG_TYPE

namespace itch
{
    enum class MsgType: char{
        SystemEvent = 'S',
        StockDirectory = 'R',
        StockTradingAction = 'H',
        RegSHOIndicator = 'Y',
        MarketParticipantPosition = 'L',
        MWCBDeclineLevel = 'V',
        MWCBStatus = 'W',
        IPOQuotingPeriodUpdate = 'K',
        LULDAuctionCollar = 'J',
        OperationalHalt = 'h',
        AddOrder = 'A',
        AddOrderMPID = 'F',
        OrderExecuted = 'E',
        OrderExecutedWithPrice   = 'C',
        OrderCancel = 'X',
        OrderDelete = 'D',
        OrderReplace = 'U',
        Trade = 'P',
        CrossTrade = 'Q',
        BrokenTrade = 'B',
        NetOrderImbalanceIndicator = 'I',
        RetailPriceImprovementIndicator = 'N',
        DirectListingCapitalRaisePrice = 'O'
    };
}

#endif //ITCH_MSG_TYPE
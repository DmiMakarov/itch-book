#ifndef ITCH_MSG_TYPE
#define ITCH_MSG_TYPE

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

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

    // Message type descriptor: length excludes the 2-byte length prefix from the file
    struct MsgInfo
    {
        MsgType type;
        std::uint8_t length;
        std::string_view name;
    };

    // Single source of truth: docs/itch50_messages.md, section 4 of the ITCH 5.0 spec
    inline constexpr std::array<MsgInfo, 23> msg_infos{{
        {MsgType::SystemEvent, 12, "System Event"},
        {MsgType::StockDirectory, 39, "Stock Directory"},
        {MsgType::StockTradingAction, 25, "Stock Trading Action"},
        {MsgType::RegSHOIndicator, 20, "Reg SHO Indicator"},
        {MsgType::MarketParticipantPosition, 26, "Market Participant Position"},
        {MsgType::MWCBDeclineLevel, 35, "MWCB Decline Level"},
        {MsgType::MWCBStatus, 12, "MWCB Status"},
        {MsgType::IPOQuotingPeriodUpdate, 28, "IPO Quoting Period Update"},
        {MsgType::LULDAuctionCollar, 35, "LULD Auction Collar"},
        {MsgType::OperationalHalt, 21, "Operational Halt"},
        {MsgType::AddOrder, 36, "Add Order"},
        {MsgType::AddOrderMPID, 40, "Add Order MPID"},
        {MsgType::OrderExecuted, 31, "Order Executed"},
        {MsgType::OrderExecutedWithPrice, 36, "Order Executed With Price"},
        {MsgType::OrderCancel, 23, "Order Cancel"},
        {MsgType::OrderDelete, 19, "Order Delete"},
        {MsgType::OrderReplace, 35, "Order Replace"},
        {MsgType::Trade, 44, "Trade"},
        {MsgType::CrossTrade, 40, "Cross Trade"},
        {MsgType::BrokenTrade, 19, "Broken Trade"},
        {MsgType::NetOrderImbalanceIndicator, 50, "NOII"},
        {MsgType::RetailPriceImprovementIndicator, 20, "RPII"},
        {MsgType::DirectListingCapitalRaisePrice, 48, "DLCR Price Discovery"},
    }};

    // Type byte -> index 0..255 (via unsigned so bytes > 127 don't become negative)
    constexpr std::size_t type_index(MsgType type) noexcept
    {
        return static_cast<std::uint8_t>(type);
    }

    constexpr std::array<std::uint8_t, 256> make_length_table() noexcept
    {
        std::array<std::uint8_t, 256> table{}; // 0 = unknown type
        for (const auto& info : msg_infos)
        {
            table[type_index(info.type)] = info.length;
        }
        return table;
    }

    // Lookup table for the parse loop: msg_lengths[type byte] -> expected length, 0 if the type is unknown
    inline constexpr std::array<std::uint8_t, 256> msg_lengths = make_length_table();
}

#endif //ITCH_MSG_TYPE
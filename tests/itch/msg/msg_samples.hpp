#ifndef ITCH_TESTS_MSG_SAMPLES_HPP
#define ITCH_TESTS_MSG_SAMPLES_HPP

// Образцовые сообщения каждого типа и то, во что они должны разобраться.
// Числовые поля — с разными байтами (0x11.., 0x21.., ...), чтобы перепутанные поля или сдвиг смещения
// давали другое значение. Смещения — из docs/itch50_messages.md.

#include <array>
#include <cstddef>

#include "itch/msg/dispatcher.hpp"
#include "msg_test_utils.hpp"

namespace itch::test::samples
{
    using namespace itch::msg;

    constexpr std::uint64_t u64_1 = 0x1112131415161718;
    constexpr std::uint64_t u64_2 = 0x2122232425262728;
    constexpr std::uint64_t u64_3 = 0x3132333435363738;
    constexpr std::uint64_t u64_4 = 0x4142434445464748;
    constexpr std::uint32_t u32_1 = 0x11121314;
    constexpr std::uint32_t u32_2 = 0x21222324;
    constexpr std::uint32_t u32_3 = 0x31323334;
    constexpr std::uint32_t u32_4 = 0x41424344;
    constexpr std::uint32_t u32_5 = 0x51525354;
    constexpr std::uint32_t u32_6 = 0x61626364;

    // S — System Event (12)
    constexpr std::array<std::byte, 12> sys_event_bytes()
    {
        return MsgBuilder<12>{'S'}.put_char(11, 'Q').bytes();
    }
    constexpr SysEventMsg sys_event_expected()
    {
        return {.header = test_header, .code = EventCode::StartMarketClock};
    }

    // R — Stock Directory (39)
    constexpr std::array<std::byte, 39> stock_directory_bytes()
    {
        return MsgBuilder<39>{'R'}
            .put_str(11, 8, "AAPL")
            .put_char(19, 'Q')
            .put_char(20, 'N')
            .put_uint(21, 4, u32_1)
            .put_char(25, 'Y')
            .put_char(26, 'C')
            .put_str(27, 2, "Z")
            .put_char(29, 'P')
            .put_char(30, 'N')
            .put_char(31, ' ')
            .put_char(32, '1')
            .put_char(33, 'Y')
            .put_uint(34, 4, u32_2)
            .put_char(38, 'N')
            .bytes();
    }
    constexpr StockDirectoryMsg stock_directory_expected()
    {
        return {
            .header = test_header,
            .stock = Alpha<8>{"AAPL"},
            .market_category = MarketCategory::NasdaqGlobalSelect,
            .financial_status = FinancialStatus::Normal,
            .round_lot_size = u32_1,
            .round_lots_only = YesNo::Yes,
            .issue_classification = IssueClassification::CommonStock,
            .issue_sub_type = Alpha<2>{"Z"},
            .authenticity = Authenticity::Production,
            .short_sale_threshold = YesNo::No,
            .ipo_flag = YesNo::NotAvailable,
            .luld_reference_price_tier = LuldReferencePriceTier::Tier1,
            .etp_flag = YesNo::Yes,
            .etp_leverage_factor = u32_2,
            .inverse_indicator = YesNo::No,
        };
    }

    // H — Stock Trading Action (25)
    constexpr std::array<std::byte, 25> stock_trading_action_bytes()
    {
        return MsgBuilder<25>{'H'}.put_str(11, 8, "MSFT").put_char(19, 'P').put_char(20, ' ').put_str(21, 4, "LUDP").bytes();
    }
    constexpr StockTradingActionMsg stock_trading_action_expected()
    {
        return {.header = test_header, .stock = Alpha<8>{"MSFT"}, .trading_state = TradingState::Paused,
                .reason = Alpha<4>{"LUDP"}};
    }

    // Y — Reg SHO (20)
    constexpr std::array<std::byte, 20> reg_sho_bytes()
    {
        return MsgBuilder<20>{'Y'}.put_str(11, 8, "GOOG").put_char(19, '1').bytes();
    }
    constexpr RegShoMsg reg_sho_expected()
    {
        return {.header = test_header, .stock = Alpha<8>{"GOOG"}, .action = RegShoAction::RestrictionIntradayDrop};
    }

    // L — Market Participant Position (26)
    constexpr std::array<std::byte, 26> market_participant_position_bytes()
    {
        return MsgBuilder<26>{'L'}
            .put_str(11, 4, "GSCO")
            .put_str(15, 8, "IBM")
            .put_char(23, 'Y')
            .put_char(24, 'S')
            .put_char(25, 'E')
            .bytes();
    }
    constexpr MarketParticipantPositionMsg market_participant_position_expected()
    {
        return {.header = test_header, .mpid = Alpha<4>{"GSCO"}, .stock = Alpha<8>{"IBM"},
                .primary_market_maker = YesNo::Yes, .market_maker_mode = MarketMakerMode::Syndicate,
                .market_participant_state = MarketParticipantState::ExcusedWithdrawn};
    }

    // V — MWCB Decline Level (35)
    constexpr std::array<std::byte, 35> mwcb_decline_level_bytes()
    {
        return MsgBuilder<35>{'V'}.put_uint(11, 8, u64_1).put_uint(19, 8, u64_2).put_uint(27, 8, u64_3).bytes();
    }
    constexpr MwcbDeclineLevelMsg mwcb_decline_level_expected()
    {
        return {.header = test_header, .level1 = Price<8>{u64_1}, .level2 = Price<8>{u64_2}, .level3 = Price<8>{u64_3}};
    }

    // W — MWCB Status (12)
    constexpr std::array<std::byte, 12> mwcb_status_bytes()
    {
        return MsgBuilder<12>{'W'}.put_char(11, '2').bytes();
    }
    constexpr MwcbStatusMsg mwcb_status_expected()
    {
        return {.header = test_header, .breached_level = BreachedLevel::Level2};
    }

    // K — IPO Quoting Period Update (28)
    constexpr std::array<std::byte, 28> ipo_quoting_period_update_bytes()
    {
        return MsgBuilder<28>{'K'}.put_str(11, 8, "UBER").put_uint(19, 4, u32_1).put_char(23, 'A').put_uint(24, 4, u32_2).bytes();
    }
    constexpr IpoQuotingPeriodUpdateMsg ipo_quoting_period_update_expected()
    {
        return {.header = test_header, .stock = Alpha<8>{"UBER"}, .release_time_seconds = u32_1,
                .release_qualifier = IpoReleaseQualifier::Anticipated, .ipo_price = Price<4>{u32_2}};
    }

    // J — LULD Auction Collar (35)
    constexpr std::array<std::byte, 35> luld_auction_collar_bytes()
    {
        return MsgBuilder<35>{'J'}
            .put_str(11, 8, "TSLA")
            .put_uint(19, 4, u32_1)
            .put_uint(23, 4, u32_2)
            .put_uint(27, 4, u32_3)
            .put_uint(31, 4, u32_4)
            .bytes();
    }
    constexpr LuldAuctionCollarMsg luld_auction_collar_expected()
    {
        return {.header = test_header, .stock = Alpha<8>{"TSLA"}, .reference_price = Price<4>{u32_1},
                .upper_collar_price = Price<4>{u32_2}, .lower_collar_price = Price<4>{u32_3}, .extension = u32_4};
    }

    // h — Operational Halt (21)
    constexpr std::array<std::byte, 21> operational_halt_bytes()
    {
        return MsgBuilder<21>{'h'}.put_str(11, 8, "AMZN").put_char(19, 'B').put_char(20, 'H').bytes();
    }
    constexpr OperationalHaltMsg operational_halt_expected()
    {
        return {.header = test_header, .stock = Alpha<8>{"AMZN"}, .market_code = MarketCode::Bx,
                .action = OperationalHaltAction::Halted};
    }

    // A — Add Order (36)
    constexpr std::array<std::byte, 36> add_order_bytes()
    {
        return MsgBuilder<36>{'A'}
            .put_uint(11, 8, u64_1)
            .put_char(19, 'S')
            .put_uint(20, 4, u32_2)
            .put_str(24, 8, "AAPL")
            .put_uint(32, 4, u32_3)
            .bytes();
    }
    constexpr AddOrderMsg add_order_expected()
    {
        return {.header = test_header, .order_ref = u64_1, .side = BuySellIndicator::Sell, .shares = u32_2,
                .stock = Alpha<8>{"AAPL"}, .price = Price<4>{u32_3}};
    }

    // F — Add Order with MPID (40)
    constexpr std::array<std::byte, 40> add_order_mpid_bytes()
    {
        return MsgBuilder<40>{'F'}
            .put_uint(11, 8, u64_1)
            .put_char(19, 'B')
            .put_uint(20, 4, u32_2)
            .put_str(24, 8, "AAPL")
            .put_uint(32, 4, u32_3)
            .put_str(36, 4, "GSCO")
            .bytes();
    }
    constexpr AddOrderMpidMsg add_order_mpid_expected()
    {
        return {.header = test_header, .order_ref = u64_1, .side = BuySellIndicator::Buy, .shares = u32_2,
                .stock = Alpha<8>{"AAPL"}, .price = Price<4>{u32_3}, .attribution = Alpha<4>{"GSCO"}};
    }

    // E — Order Executed (31)
    constexpr std::array<std::byte, 31> order_executed_bytes()
    {
        return MsgBuilder<31>{'E'}.put_uint(11, 8, u64_1).put_uint(19, 4, u32_2).put_uint(23, 8, u64_3).bytes();
    }
    constexpr OrderExecutedMsg order_executed_expected()
    {
        return {.header = test_header, .order_ref = u64_1, .executed_shares = u32_2, .match_number = u64_3};
    }

    // C — Order Executed With Price (36)
    constexpr std::array<std::byte, 36> order_executed_with_price_bytes()
    {
        return MsgBuilder<36>{'C'}
            .put_uint(11, 8, u64_1)
            .put_uint(19, 4, u32_2)
            .put_uint(23, 8, u64_3)
            .put_char(31, 'N')
            .put_uint(32, 4, u32_4)
            .bytes();
    }
    constexpr OrderExecutedWithPriceMsg order_executed_with_price_expected()
    {
        return {.header = test_header, .order_ref = u64_1, .executed_shares = u32_2, .match_number = u64_3,
                .printable = YesNo::No, .execution_price = Price<4>{u32_4}};
    }

    // X — Order Cancel (23)
    constexpr std::array<std::byte, 23> order_cancel_bytes()
    {
        return MsgBuilder<23>{'X'}.put_uint(11, 8, u64_1).put_uint(19, 4, u32_2).bytes();
    }
    constexpr OrderCancelMsg order_cancel_expected()
    {
        return {.header = test_header, .order_ref = u64_1, .cancelled_shares = u32_2};
    }

    // D — Order Delete (19)
    constexpr std::array<std::byte, 19> order_delete_bytes()
    {
        return MsgBuilder<19>{'D'}.put_uint(11, 8, u64_1).bytes();
    }
    constexpr OrderDeleteMsg order_delete_expected()
    {
        return {.header = test_header, .order_ref = u64_1};
    }

    // U — Order Replace (35)
    constexpr std::array<std::byte, 35> order_replace_bytes()
    {
        return MsgBuilder<35>{'U'}.put_uint(11, 8, u64_1).put_uint(19, 8, u64_2).put_uint(27, 4, u32_3).put_uint(31, 4, u32_4).bytes();
    }
    constexpr OrderReplaceMsg order_replace_expected()
    {
        return {.header = test_header, .original_order_ref = u64_1, .new_order_ref = u64_2, .shares = u32_3,
                .price = Price<4>{u32_4}};
    }

    // P — Trade (44)
    constexpr std::array<std::byte, 44> trade_bytes()
    {
        return MsgBuilder<44>{'P'}
            .put_uint(11, 8, u64_1)
            .put_char(19, 'B')
            .put_uint(20, 4, u32_2)
            .put_str(24, 8, "QQQ")
            .put_uint(32, 4, u32_3)
            .put_uint(36, 8, u64_4)
            .bytes();
    }
    constexpr TradeMsg trade_expected()
    {
        return {.header = test_header, .order_ref = u64_1, .side = BuySellIndicator::Buy, .shares = u32_2,
                .stock = Alpha<8>{"QQQ"}, .price = Price<4>{u32_3}, .match_number = u64_4};
    }

    // Q — Cross Trade (40)
    constexpr std::array<std::byte, 40> cross_trade_bytes()
    {
        return MsgBuilder<40>{'Q'}
            .put_uint(11, 8, u64_1)
            .put_str(19, 8, "SPY")
            .put_uint(27, 4, u32_2)
            .put_uint(31, 8, u64_3)
            .put_char(39, 'H')
            .bytes();
    }
    constexpr CrossTradeMsg cross_trade_expected()
    {
        return {.header = test_header, .shares = u64_1, .stock = Alpha<8>{"SPY"}, .cross_price = Price<4>{u32_2},
                .match_number = u64_3, .cross_type = CrossType::IpoOrHalted};
    }

    // B — Broken Trade (19)
    constexpr std::array<std::byte, 19> broken_trade_bytes()
    {
        return MsgBuilder<19>{'B'}.put_uint(11, 8, u64_1).bytes();
    }
    constexpr BrokenTradeMsg broken_trade_expected()
    {
        return {.header = test_header, .match_number = u64_1};
    }

    // I — NOII (50)
    constexpr std::array<std::byte, 50> noii_bytes()
    {
        return MsgBuilder<50>{'I'}
            .put_uint(11, 8, u64_1)
            .put_uint(19, 8, u64_2)
            .put_char(27, 'S')
            .put_str(28, 8, "NFLX")
            .put_uint(36, 4, u32_3)
            .put_uint(40, 4, u32_4)
            .put_uint(44, 4, u32_5)
            .put_char(48, 'C')
            .put_char(49, 'L')
            .bytes();
    }
    constexpr NoiiMsg noii_expected()
    {
        return {.header = test_header, .paired_shares = u64_1, .imbalance_shares = u64_2,
                .imbalance_direction = ImbalanceDirection::Sell, .stock = Alpha<8>{"NFLX"},
                .far_price = Price<4>{u32_3}, .near_price = Price<4>{u32_4},
                .current_reference_price = Price<4>{u32_5}, .cross_type = CrossType::Closing,
                .price_variation = PriceVariation::LessThan1Pct};
    }

    // N — RPII (20)
    constexpr std::array<std::byte, 20> rpii_bytes()
    {
        return MsgBuilder<20>{'N'}.put_str(11, 8, "AAPL").put_char(19, 'A').bytes();
    }
    constexpr RpiiMsg rpii_expected()
    {
        return {.header = test_header, .stock = Alpha<8>{"AAPL"}, .interest_flag = InterestFlag::Both};
    }

    // O — Direct Listing with Capital Raise (48)
    constexpr std::array<std::byte, 48> direct_listing_bytes()
    {
        return MsgBuilder<48>{'O'}
            .put_str(11, 8, "DLCR")
            .put_char(19, 'Y')
            .put_uint(20, 4, u32_1)
            .put_uint(24, 4, u32_2)
            .put_uint(28, 4, u32_3)
            .put_uint(32, 8, u64_4)
            .put_uint(40, 4, u32_5)
            .put_uint(44, 4, u32_6)
            .bytes();
    }
    constexpr DirectListingMsg direct_listing_expected()
    {
        return {.header = test_header, .stock = Alpha<8>{"DLCR"}, .open_eligible = YesNo::Yes,
                .min_allowable_price = Price<4>{u32_1}, .max_allowable_price = Price<4>{u32_2},
                .near_execution_price = Price<4>{u32_3}, .near_execution_time = u64_4,
                .lower_price_range_collar = Price<4>{u32_5}, .upper_price_range_collar = Price<4>{u32_6}};
    }
}

#endif // ITCH_TESTS_MSG_SAMPLES_HPP

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "itch/error_handling/format_error.hpp"
#include "itch/itch_reader.hpp"
#include "itch/msg/dispatcher.hpp"
#include "msg_samples.hpp"
#include "msg_test_utils.hpp"

using itch::Message;
using itch::MsgType;
using itch::msg::AddOrderMsg;
using itch::msg::BuySellIndicator;
using itch::msg::EventCode;
using itch::msg::SysEventMsg;
using itch::test::make_bytes;

namespace
{
    constexpr std::array<std::byte, 12> sys_event_bytes(std::uint8_t code)
    {
        return make_bytes({0x53, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, code});
    }

    constexpr std::array<std::byte, 36> add_order_bytes(std::uint8_t side)
    {
        return make_bytes({
            0x41, 0x00, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC8,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, // order ref 42
            side,
            0x00, 0x00, 0x00, 0x64,                         // 100 shares
            'A', 'A', 'P', 'L', ' ', ' ', ' ', ' ',
            0x00, 0x00, 0x27, 0x10,                         // 1.0000
        });
    }

    // A Message as the reader would produce it: type, message bytes, record offset, timestamp
    Message make_message(MsgType type, std::span<const std::byte> bytes, std::size_t offset)
    {
        return Message{.type = type, .bytes = bytes, .offset = offset, .timestamp = 0};
    }

    // Handles only S: the dispatcher must not even parse other types
    struct SysEventOnly
    {
        std::vector<SysEventMsg> received;
        void on(const SysEventMsg& m) { received.push_back(m); }
    };

    // Handles only A
    struct AddOrderOnly
    {
        std::vector<AddOrderMsg> received;
        void on(const AddOrderMsg& m) { received.push_back(m); }
    };

    // Accepts everything via a single template overload and records the header timestamp
    struct Everything
    {
        std::vector<std::uint64_t> timestamps;
        void on(const auto& m) { timestamps.push_back(m.header.timestamp); }
    };

    // Takes an rvalue reference: the dispatcher's check must allow this
    struct ByRvalue
    {
        int calls = 0;
        void on(SysEventMsg&& /*m*/) { ++calls; }
    };

    // No on overloads at all
    struct Nothing
    {
    };
}

TEST(MsgDispatcherTest, DeliversSysEventToMatchingHandler)
{
    const auto bytes = sys_event_bytes('Q');
    SysEventOnly handler;

    itch::dispatch(make_message(MsgType::SystemEvent, bytes, 0), handler);

    ASSERT_EQ(handler.received.size(), 1u);
    EXPECT_EQ(handler.received[0].code, EventCode::StartMarketClock);
    EXPECT_EQ(handler.received[0].header.timestamp, 100u);
}

TEST(MsgDispatcherTest, DeliversAddOrderToMatchingHandler)
{
    const auto bytes = add_order_bytes('S');
    AddOrderOnly handler;

    itch::dispatch(make_message(MsgType::AddOrder, bytes, 0), handler);

    ASSERT_EQ(handler.received.size(), 1u);
    const AddOrderMsg& m = handler.received[0];
    EXPECT_EQ(m.header.stock_locate, 7u);
    EXPECT_EQ(m.order_ref, 42u);
    EXPECT_EQ(m.side, BuySellIndicator::Sell);
    EXPECT_EQ(m.shares, 100u);
    EXPECT_EQ(m.stock.view(), "AAPL");
    EXPECT_EQ(m.price.raw(), 10'000u);
}

TEST(MsgDispatcherTest, SkipsTypesHandlerDoesNotAccept)
{
    const auto s = sys_event_bytes('O');
    const auto a = add_order_bytes('B');

    SysEventOnly sys_handler;
    itch::dispatch(make_message(MsgType::AddOrder, a, 0), sys_handler);
    EXPECT_TRUE(sys_handler.received.empty());

    AddOrderOnly add_handler;
    itch::dispatch(make_message(MsgType::SystemEvent, s, 0), add_handler);
    EXPECT_TRUE(add_handler.received.empty());
}

TEST(MsgDispatcherTest, SkippedTypeIsNotEvenParsed)
{
    // Side 'X' is invalid: parsing would throw FormatError.
    // The handler doesn't need A, so there must be no parsing and no exception.
    const auto a = add_order_bytes('X');
    SysEventOnly handler;

    EXPECT_NO_THROW(itch::dispatch(make_message(MsgType::AddOrder, a, 0), handler));
    EXPECT_TRUE(handler.received.empty());
}

TEST(MsgDispatcherTest, GenericHandlerReceivesAllImplementedTypes)
{
    const auto s = sys_event_bytes('O');
    const auto a = add_order_bytes('B');
    Everything handler;

    itch::dispatch(make_message(MsgType::SystemEvent, s, 0), handler);
    itch::dispatch(make_message(MsgType::AddOrder, a, 14), handler);

    EXPECT_EQ(handler.timestamps, (std::vector<std::uint64_t>{100, 200}));
}

TEST(MsgDispatcherTest, AcceptsRvalueOverload)
{
    const auto bytes = sys_event_bytes('C');
    ByRvalue handler;

    itch::dispatch(make_message(MsgType::SystemEvent, bytes, 0), handler);

    EXPECT_EQ(handler.calls, 1);
}

TEST(MsgDispatcherTest, HandlerWithoutOverloadsCompilesAndIgnoresAll)
{
    const auto s = sys_event_bytes('O');
    Nothing handler;
    EXPECT_NO_THROW(itch::dispatch(make_message(MsgType::SystemEvent, s, 0), handler));
}

namespace
{
    namespace s = itch::test::samples;

    // Each type: a sample and the expected struct. The handler checks that it received exactly what was parsed from the sample
    struct Collector
    {
        std::array<int, 256> calls{};
        int mismatches = 0;

        template <typename M>
        void on(const M& m)
        {
            ++calls[static_cast<std::uint8_t>(M::type)];
            if (!(m == expected<M>()))
            {
                ++mismatches;
            }
        }

        template <typename M>
        static constexpr M expected()
        {
            if constexpr (std::is_same_v<M, itch::msg::SysEventMsg>) return s::sys_event_expected();
            else if constexpr (std::is_same_v<M, itch::msg::StockDirectoryMsg>) return s::stock_directory_expected();
            else if constexpr (std::is_same_v<M, itch::msg::StockTradingActionMsg>) return s::stock_trading_action_expected();
            else if constexpr (std::is_same_v<M, itch::msg::RegShoMsg>) return s::reg_sho_expected();
            else if constexpr (std::is_same_v<M, itch::msg::MarketParticipantPositionMsg>) return s::market_participant_position_expected();
            else if constexpr (std::is_same_v<M, itch::msg::MwcbDeclineLevelMsg>) return s::mwcb_decline_level_expected();
            else if constexpr (std::is_same_v<M, itch::msg::MwcbStatusMsg>) return s::mwcb_status_expected();
            else if constexpr (std::is_same_v<M, itch::msg::IpoQuotingPeriodUpdateMsg>) return s::ipo_quoting_period_update_expected();
            else if constexpr (std::is_same_v<M, itch::msg::LuldAuctionCollarMsg>) return s::luld_auction_collar_expected();
            else if constexpr (std::is_same_v<M, itch::msg::OperationalHaltMsg>) return s::operational_halt_expected();
            else if constexpr (std::is_same_v<M, itch::msg::AddOrderMsg>) return s::add_order_expected();
            else if constexpr (std::is_same_v<M, itch::msg::AddOrderMpidMsg>) return s::add_order_mpid_expected();
            else if constexpr (std::is_same_v<M, itch::msg::OrderExecutedMsg>) return s::order_executed_expected();
            else if constexpr (std::is_same_v<M, itch::msg::OrderExecutedWithPriceMsg>) return s::order_executed_with_price_expected();
            else if constexpr (std::is_same_v<M, itch::msg::OrderCancelMsg>) return s::order_cancel_expected();
            else if constexpr (std::is_same_v<M, itch::msg::OrderDeleteMsg>) return s::order_delete_expected();
            else if constexpr (std::is_same_v<M, itch::msg::OrderReplaceMsg>) return s::order_replace_expected();
            else if constexpr (std::is_same_v<M, itch::msg::TradeMsg>) return s::trade_expected();
            else if constexpr (std::is_same_v<M, itch::msg::CrossTradeMsg>) return s::cross_trade_expected();
            else if constexpr (std::is_same_v<M, itch::msg::BrokenTradeMsg>) return s::broken_trade_expected();
            else if constexpr (std::is_same_v<M, itch::msg::NoiiMsg>) return s::noii_expected();
            else if constexpr (std::is_same_v<M, itch::msg::RpiiMsg>) return s::rpii_expected();
            else if constexpr (std::is_same_v<M, itch::msg::DirectListingMsg>) return s::direct_listing_expected();
            else static_assert(sizeof(M) == 0, "no sample for this message type");
        }
    };
}

TEST(MsgDispatcherTest, EveryTypeReachesItsHandler)
{
    namespace s = itch::test::samples;

    const auto S = s::sys_event_bytes();
    const auto R = s::stock_directory_bytes();
    const auto H = s::stock_trading_action_bytes();
    const auto Y = s::reg_sho_bytes();
    const auto L = s::market_participant_position_bytes();
    const auto V = s::mwcb_decline_level_bytes();
    const auto W = s::mwcb_status_bytes();
    const auto K = s::ipo_quoting_period_update_bytes();
    const auto J = s::luld_auction_collar_bytes();
    const auto h = s::operational_halt_bytes();
    const auto A = s::add_order_bytes();
    const auto F = s::add_order_mpid_bytes();
    const auto E = s::order_executed_bytes();
    const auto C = s::order_executed_with_price_bytes();
    const auto X = s::order_cancel_bytes();
    const auto D = s::order_delete_bytes();
    const auto U = s::order_replace_bytes();
    const auto P = s::trade_bytes();
    const auto Q = s::cross_trade_bytes();
    const auto B = s::broken_trade_bytes();
    const auto I = s::noii_bytes();
    const auto N = s::rpii_bytes();
    const auto O = s::direct_listing_bytes();

    const std::vector<std::span<const std::byte>> all{S, R, H, Y, L, V, W, K, J, h, A, F, E, C, X, D, U, P, Q, B, I, N, O};
    ASSERT_EQ(all.size(), itch::msg_infos.size());

    Collector handler;
    for (const auto bytes : all)
    {
        const auto type = static_cast<MsgType>(std::to_integer<char>(bytes[0]));
        ASSERT_EQ(bytes.size(), itch::msg_lengths[itch::type_index(type)]);
        itch::dispatch(make_message(type, bytes, 0), handler);
    }

    EXPECT_EQ(handler.mismatches, 0);
    for (const auto& info : itch::msg_infos)
    {
        EXPECT_EQ(handler.calls[itch::type_index(info.type)], 1) << "type '" << static_cast<char>(info.type) << '\'';
    }
}

TEST(MsgDispatcherTest, FormatErrorCarriesRecordOffset)
{
    // Parsing throws FormatError with the byte position within the message;
    // the dispatcher replaces it with the record offset in the buffer, as the reader does
    const auto s = sys_event_bytes('Z');
    const auto a = add_order_bytes('X');

    try
    {
        SysEventOnly handler;
        itch::dispatch(make_message(MsgType::SystemEvent, s, 1000), handler);
        ADD_FAILURE() << "expected itch::FormatError for S";
    }
    catch (const itch::FormatError& e)
    {
        EXPECT_EQ(e.offset(), 1000u) << e.what();
    }

    try
    {
        AddOrderOnly handler;
        itch::dispatch(make_message(MsgType::AddOrder, a, 123'456), handler);
        ADD_FAILURE() << "expected itch::FormatError for A";
    }
    catch (const itch::FormatError& e)
    {
        EXPECT_EQ(e.offset(), 123'456u) << e.what();
    }
}

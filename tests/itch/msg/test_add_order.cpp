#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "itch/error_handling/format_error.hpp"
#include "itch/msg/add_order.hpp"
#include "msg_test_utils.hpp"

using itch::msg::AddOrderMsg;
using itch::msg::Alpha;
using itch::msg::BuySellIndicator;
using itch::msg::Header;
using itch::msg::Price;
using itch::msg::read_add_order_msg;
using itch::test::make_bytes;

namespace
{
    // Add Order: all numeric-field bytes are distinct, so swapped fields or a shifted offset show up immediately
    constexpr std::array<std::byte, 36> add_order_bytes(std::uint8_t side)
    {
        return make_bytes({
            0x41,                                           // 0  type 'A'
            0x01, 0x02,                                     // 1  stock locate
            0x03, 0x04,                                     // 3  tracking number
            0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,             // 5  timestamp
            0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, // 11 order reference number
            side,                                           // 19 buy/sell
            0x21, 0x22, 0x23, 0x24,                         // 20 shares
            'A', 'A', 'P', 'L', ' ', ' ', ' ', ' ',         // 24 stock
            0x31, 0x32, 0x33, 0x34,                         // 32 price
        });
    }

    constexpr AddOrderMsg expected_add_order(BuySellIndicator side)
    {
        return AddOrderMsg{
            .header = Header{.stock_locate = 0x0102, .tracking_number = 0x0304, .timestamp = 0x05060708090A},
            .order_ref = 0x1112131415161718,
            .side = side,
            .shares = 0x21222324,
            .stock = Alpha<8>{"AAPL"},
            .price = Price<4>{0x31323334},
        };
    }

    static_assert(AddOrderMsg::length == 36);
    static_assert(read_add_order_msg(add_order_bytes('B')) == expected_add_order(BuySellIndicator::Buy));
}

TEST(MsgAddOrderTest, DecodesBuyOrder)
{
    EXPECT_EQ(read_add_order_msg(add_order_bytes('B')), expected_add_order(BuySellIndicator::Buy));
}

TEST(MsgAddOrderTest, DecodesSellOrder)
{
    EXPECT_EQ(read_add_order_msg(add_order_bytes('S')), expected_add_order(BuySellIndicator::Sell));
}

TEST(MsgAddOrderTest, DecodesRealisticOrder)
{
    // Buy 100 shares of MSFT at 157.71: price 1,577,100 = 0x0018108C
    constexpr auto bytes = make_bytes({
        0x41,
        0x00, 0x2A,
        0x00, 0x00,
        0x1F, 0x1A, 0xCE, 0xD9, 0xF0, 0x00,             // 09:30:00
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x39, // order ref 12345
        'B',
        0x00, 0x00, 0x00, 0x64,                         // 100 shares
        'M', 'S', 'F', 'T', ' ', ' ', ' ', ' ',
        0x00, 0x18, 0x10, 0x8C,
    });

    const AddOrderMsg msg = read_add_order_msg(bytes);
    EXPECT_EQ(msg.header.stock_locate, 42u);
    EXPECT_EQ(msg.header.timestamp, 34'200'000'000'000u);
    EXPECT_EQ(msg.order_ref, 12345u);
    EXPECT_EQ(msg.side, BuySellIndicator::Buy);
    EXPECT_EQ(msg.shares, 100u);
    EXPECT_EQ(msg.stock.view(), "MSFT");
    EXPECT_EQ(msg.price.raw(), 1'577'100u);
    EXPECT_DOUBLE_EQ(msg.price.to_double(), 157.71);
}

TEST(MsgAddOrderTest, DecodesMaxValues)
{
    auto bytes = add_order_bytes('S');
    for (std::size_t i = 1; i < 19; ++i) // header and order ref
    {
        bytes[i] = std::byte{0xFF};
    }
    for (std::size_t i = 20; i < 24; ++i) // shares
    {
        bytes[i] = std::byte{0xFF};
    }
    for (std::size_t i = 32; i < 36; ++i) // price
    {
        bytes[i] = std::byte{0xFF};
    }

    const AddOrderMsg msg = read_add_order_msg(bytes);
    EXPECT_EQ(msg.header.stock_locate, 0xFFFFu);
    EXPECT_EQ(msg.header.timestamp, 0xFFFF'FFFF'FFFFu);
    EXPECT_EQ(msg.order_ref, 0xFFFF'FFFF'FFFF'FFFFu);
    EXPECT_EQ(msg.shares, 0xFFFF'FFFFu);
    EXPECT_EQ(msg.price.raw(), 0xFFFF'FFFFu);
}

TEST(MsgAddOrderTest, SideValuesMatchSpec)
{
    EXPECT_EQ(static_cast<char>(BuySellIndicator::Buy), 'B');
    EXPECT_EQ(static_cast<char>(BuySellIndicator::Sell), 'S');
    EXPECT_EQ(itch::msg::buy_sell_indicators.size(), 2u);
}

TEST(MsgAddOrderTest, RejectsInvalidSide)
{
    // 'b' tests case, 'A' is a message type, ' ' and 0x00 are empty values, 0xFF is index 255
    constexpr std::array<std::uint8_t, 5> invalid{'b', 'A', ' ', 0x00, 0xFF};

    for (const auto side : invalid)
    {
        SCOPED_TRACE(testing::Message() << "byte 0x" << std::hex << static_cast<unsigned>(side));
        try
        {
            [[maybe_unused]] auto msg = read_add_order_msg(add_order_bytes(side));
            ADD_FAILURE() << "expected itch::FormatError";
        }
        catch (const itch::FormatError& e)
        {
            // Byte position within the message; the dispatcher substitutes the record offset
            EXPECT_EQ(e.offset(), 19u) << e.what();
        }
    }
}

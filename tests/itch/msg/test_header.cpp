#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <span>

#include "itch/msg/header.hpp"
#include "msg_test_utils.hpp"

using itch::msg::Header;
using itch::msg::read_header;
using itch::test::make_bytes;

namespace
{
    // [type][locate 2][tracking 2][timestamp 6]: all bytes distinct,
    // so swapped fields or a shifted offset immediately yield a different number.
    constexpr auto distinct = make_bytes({
        0x53,                               // type 'S': read_header doesn't read it
        0x01, 0x02,                         // stock locate
        0x03, 0x04,                         // tracking number
        0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, // timestamp
    });

    constexpr Header distinct_expected{
        .stock_locate = 0x0102,
        .tracking_number = 0x0304,
        .timestamp = 0x05060708090A,
    };

    // An offset bug breaks the build, not just the test run
    static_assert(read_header(distinct) == distinct_expected);
}

TEST(MsgHeaderTest, DecodesDistinctFields)
{
    EXPECT_EQ(read_header(distinct), distinct_expected);
}

TEST(MsgHeaderTest, DecodesMaxValues)
{
    // 0xFF in every byte: catches sign extension and lost high bytes
    constexpr auto bytes = make_bytes({
        0x53,
        0xFF, 0xFF,
        0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    });

    const Header expected{
        .stock_locate = 0xFFFF,
        .tracking_number = 0xFFFF,
        .timestamp = 0xFFFF'FFFF'FFFF,
    };
    EXPECT_EQ(read_header(bytes), expected);
}

TEST(MsgHeaderTest, DecodesRealisticTimestampAbove32Bits)
{
    // 09:30:00 = 34,200 s since midnight = 34,200,000,000,000 ns = 0x1F1ACED9F000
    constexpr auto bytes = make_bytes({
        0x41,
        0x00, 0x07,
        0x00, 0x00,
        0x1F, 0x1A, 0xCE, 0xD9, 0xF0, 0x00,
    });

    const Header expected{
        .stock_locate = 7,
        .tracking_number = 0,
        .timestamp = 34'200'000'000'000,
    };
    EXPECT_EQ(read_header(bytes), expected);
}

TEST(MsgHeaderTest, IgnoresTypeByteAndTail)
{
    // A 36-byte message (like 'A'): same header, garbage tail
    std::array<std::byte, 36> bytes{};
    bytes.fill(std::byte{0xEE});
    std::copy(distinct.begin(), distinct.end(), bytes.begin());
    bytes[0] = std::byte{'A'};

    EXPECT_EQ(read_header(bytes), distinct_expected);
}

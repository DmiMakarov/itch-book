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
    // [тип][locate 2][tracking 2][timestamp 6] — все байты разные,
    // так что перепутанные поля или сдвинутое смещение сразу дают другое число.
    constexpr auto distinct = make_bytes({
        0x53,                               // тип 'S' — read_header его не читает
        0x01, 0x02,                         // stock locate
        0x03, 0x04,                         // tracking number
        0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, // timestamp
    });

    constexpr Header distinct_expected{
        .stock_locate = 0x0102,
        .tracking_number = 0x0304,
        .timestamp = 0x05060708090A,
    };

    // Ошибка в смещениях ломает сборку, а не только прогон тестов
    static_assert(read_header(distinct) == distinct_expected);
}

TEST(MsgHeaderTest, DecodesDistinctFields)
{
    EXPECT_EQ(read_header(distinct), distinct_expected);
}

TEST(MsgHeaderTest, DecodesMaxValues)
{
    // 0xFF во всех байтах: ловит расширение знака и потерю старших байтов
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
    // 09:30:00 = 34 200 с от полуночи = 34 200 000 000 000 нс = 0x1F1ACED9F000
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
    // Сообщение длиной 36 байт (как 'A'): заголовок тот же, хвост — мусор
    std::array<std::byte, 36> bytes{};
    bytes.fill(std::byte{0xEE});
    std::copy(distinct.begin(), distinct.end(), bytes.begin());
    bytes[0] = std::byte{'A'};

    EXPECT_EQ(read_header(bytes), distinct_expected);
}

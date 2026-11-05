#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

#include "itch/msg/price.hpp"
// PrintTo для Price: gtest находит его через ADL, поэтому clangd не видит использования
#include "msg_test_utils.hpp" // IWYU pragma: keep

using itch::msg::Price;

namespace
{
    static_assert(std::is_same_v<Price<4>::Rep, std::uint32_t>);
    static_assert(std::is_same_v<Price<8>::Rep, std::uint64_t>);
    static_assert(Price<4>{1'234'500}.raw() == 1'234'500);
    static_assert(Price<4>{} == Price<4>{0});
}

TEST(MsgPriceTest, DefaultIsZero)
{
    EXPECT_EQ(Price<4>{}.raw(), 0u);
    EXPECT_EQ(Price<8>{}.raw(), 0u);
}

TEST(MsgPriceTest, Price4HasFourDecimals)
{
    EXPECT_DOUBLE_EQ(Price<4>{1'234'500}.to_double(), 123.45);
    EXPECT_DOUBLE_EQ(Price<4>{1}.to_double(), 0.0001);
}

TEST(MsgPriceTest, Price8HasEightDecimals)
{
    EXPECT_DOUBLE_EQ(Price<8>{123'450'000'000}.to_double(), 1234.5);
    EXPECT_DOUBLE_EQ(Price<8>{1}.to_double(), 0.00000001);
}

TEST(MsgPriceTest, MaxPrice4DoesNotOverflow)
{
    // Максимум Price(4) по спецификации — 200 000.0000 = 2 000 000 000
    const Price<4> p{2'000'000'000};
    EXPECT_EQ(p.raw(), 2'000'000'000u);
    EXPECT_DOUBLE_EQ(p.to_double(), 200'000.0);
}

TEST(MsgPriceTest, OrdersByRawValue)
{
    EXPECT_LT(Price<4>{100}, Price<4>{101});
    EXPECT_EQ(Price<4>{100}, Price<4>{100});
    EXPECT_GT(Price<8>{2}, Price<8>{1});
}

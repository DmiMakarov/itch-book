// Сделки и аукционы: P, Q, B, I, N, O.
#include <gtest/gtest.h>

#include "msg_samples.hpp"
#include "msg_test_utils.hpp"

using namespace itch::msg;
using namespace itch::test::samples;
using itch::test::expect_flag_domain;
using itch::test::MsgBuilder;

namespace
{
    static_assert(read_trade_msg(trade_bytes()) == trade_expected());
    static_assert(read_cross_trade_msg(cross_trade_bytes()) == cross_trade_expected());
    static_assert(read_broken_trade_msg(broken_trade_bytes()) == broken_trade_expected());
    static_assert(read_noii_msg(noii_bytes()) == noii_expected());
    static_assert(read_rpii_msg(rpii_bytes()) == rpii_expected());
    static_assert(read_direct_listing_msg(direct_listing_bytes()) == direct_listing_expected());
}

// ---------- P — Trade ----------

TEST(MsgTradeTest, DecodesAllFields)
{
    EXPECT_EQ(read_trade_msg(trade_bytes()), trade_expected());
}

TEST(MsgTradeTest, FlagDomainsMatchSpec)
{
    // На практике с 2014 года всегда 'B', но спецификация допускает 'B' и 'S'
    expect_flag_domain(read_trade_msg, trade_bytes(), 19, "BS", [](const auto& m) { return m.side; });
}

// ---------- Q — Cross Trade ----------

TEST(MsgCrossTradeTest, DecodesAllFields)
{
    EXPECT_EQ(read_cross_trade_msg(cross_trade_bytes()), cross_trade_expected());
}

TEST(MsgCrossTradeTest, FlagDomainsMatchSpec)
{
    // Без 'A': Extended Trading Close бывает только в NOII
    expect_flag_domain(read_cross_trade_msg, cross_trade_bytes(), 39, "OCH", [](const auto& m) { return m.cross_type; });
}

TEST(MsgCrossTradeTest, SharesAreEightBytes)
{
    auto bytes = cross_trade_bytes();
    for (std::size_t i = 11; i < 19; ++i)
    {
        bytes[i] = std::byte{0xFF};
    }
    EXPECT_EQ(read_cross_trade_msg(bytes).shares, 0xFFFF'FFFF'FFFF'FFFFu);
}

// ---------- B — Broken Trade ----------

TEST(MsgBrokenTradeTest, DecodesAllFields)
{
    EXPECT_EQ(read_broken_trade_msg(broken_trade_bytes()), broken_trade_expected());
}

// ---------- I — NOII ----------

TEST(MsgNoiiTest, DecodesAllFields)
{
    EXPECT_EQ(read_noii_msg(noii_bytes()), noii_expected());
}

TEST(MsgNoiiTest, FlagDomainsMatchSpec)
{
    const auto bytes = noii_bytes();
    expect_flag_domain(read_noii_msg, bytes, 27, "BSNOP", [](const auto& m) { return m.imbalance_direction; });
    expect_flag_domain(read_noii_msg, bytes, 48, "OCHA", [](const auto& m) { return m.cross_type; });
    expect_flag_domain(read_noii_msg, bytes, 49, "L123456789ABC ", [](const auto& m) { return m.price_variation; });
}

TEST(MsgNoiiTest, StockIsAtOffset28)
{
    // Stock стоит после Imbalance Direction, а не сразу после заголовка
    const auto bytes = MsgBuilder<50>{'I'}
                           .put_uint(11, 8, 0)
                           .put_uint(19, 8, 0)
                           .put_char(27, 'N')
                           .put_str(28, 8, "ZVZZT")
                           .put_char(48, 'O')
                           .put_char(49, ' ')
                           .bytes();
    EXPECT_EQ(read_noii_msg(bytes).stock.view(), "ZVZZT");
}

// ---------- N — RPII ----------

TEST(MsgRpiiTest, DecodesAllFields)
{
    EXPECT_EQ(read_rpii_msg(rpii_bytes()), rpii_expected());
}

TEST(MsgRpiiTest, FlagDomainsMatchSpec)
{
    expect_flag_domain(read_rpii_msg, rpii_bytes(), 19, "BSAN", [](const auto& m) { return m.interest_flag; });
}

// ---------- O — Direct Listing with Capital Raise ----------

TEST(MsgDirectListingTest, DecodesAllFields)
{
    EXPECT_EQ(read_direct_listing_msg(direct_listing_bytes()), direct_listing_expected());
}

TEST(MsgDirectListingTest, FlagDomainsMatchSpec)
{
    expect_flag_domain(read_direct_listing_msg, direct_listing_bytes(), 19, "NY",
                       [](const auto& m) { return m.open_eligible; });
}

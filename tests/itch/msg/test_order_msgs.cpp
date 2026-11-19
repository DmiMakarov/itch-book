// Order messages: F, E, C, X, D, U ('A' is in test_add_order.cpp).
#include <gtest/gtest.h>

#include "msg_samples.hpp"
#include "msg_test_utils.hpp"

using namespace itch::msg;
using namespace itch::test::samples;
using itch::test::expect_flag_domain;
using itch::test::MsgBuilder;

namespace
{
    static_assert(read_add_order_mpid_msg(add_order_mpid_bytes()) == add_order_mpid_expected());
    static_assert(read_order_executed_msg(order_executed_bytes()) == order_executed_expected());
    static_assert(read_order_executed_with_price_msg(order_executed_with_price_bytes()) == order_executed_with_price_expected());
    static_assert(read_order_cancel_msg(order_cancel_bytes()) == order_cancel_expected());
    static_assert(read_order_delete_msg(order_delete_bytes()) == order_delete_expected());
    static_assert(read_order_replace_msg(order_replace_bytes()) == order_replace_expected());
}

// ---------- F — Add Order with MPID ----------

TEST(MsgAddOrderMpidTest, DecodesAllFields)
{
    EXPECT_EQ(read_add_order_mpid_msg(add_order_mpid_bytes()), add_order_mpid_expected());
}

TEST(MsgAddOrderMpidTest, FlagDomainsMatchSpec)
{
    expect_flag_domain(read_add_order_mpid_msg, add_order_mpid_bytes(), 19, "BS",
                       [](const auto& m) { return m.side; });
}

TEST(MsgAddOrderMpidTest, SharesFirst36BytesWithAddOrder)
{
    // The same 36 bytes with type 'A' yield the same fields
    auto f = add_order_mpid_bytes();
    std::array<std::byte, 36> a{};
    std::copy_n(f.begin(), 36, a.begin());
    a[0] = std::byte{'A'};

    const auto mpid = read_add_order_mpid_msg(f);
    const auto plain = read_add_order_msg(a);
    EXPECT_EQ(mpid.header, plain.header);
    EXPECT_EQ(mpid.order_ref, plain.order_ref);
    EXPECT_EQ(mpid.side, plain.side);
    EXPECT_EQ(mpid.shares, plain.shares);
    EXPECT_EQ(mpid.stock, plain.stock);
    EXPECT_EQ(mpid.price, plain.price);
    EXPECT_EQ(mpid.attribution.view(), "GSCO");
}

// ---------- E — Order Executed ----------

TEST(MsgOrderExecutedTest, DecodesAllFields)
{
    EXPECT_EQ(read_order_executed_msg(order_executed_bytes()), order_executed_expected());
}

// ---------- C — Order Executed With Price ----------

TEST(MsgOrderExecutedWithPriceTest, DecodesAllFields)
{
    EXPECT_EQ(read_order_executed_with_price_msg(order_executed_with_price_bytes()), order_executed_with_price_expected());
}

TEST(MsgOrderExecutedWithPriceTest, FlagDomainsMatchSpec)
{
    expect_flag_domain(read_order_executed_with_price_msg, order_executed_with_price_bytes(), 31, "YN",
                       [](const auto& m) { return m.printable; });
}

// ---------- X — Order Cancel ----------

TEST(MsgOrderCancelTest, DecodesAllFields)
{
    EXPECT_EQ(read_order_cancel_msg(order_cancel_bytes()), order_cancel_expected());
}

// ---------- D — Order Delete ----------

TEST(MsgOrderDeleteTest, DecodesAllFields)
{
    EXPECT_EQ(read_order_delete_msg(order_delete_bytes()), order_delete_expected());
}

// ---------- U — Order Replace ----------

TEST(MsgOrderReplaceTest, DecodesAllFields)
{
    EXPECT_EQ(read_order_replace_msg(order_replace_bytes()), order_replace_expected());
}

TEST(MsgOrderReplaceTest, RealisticReplace)
{
    // Order 1000 replaced by 1001: 300 shares at 157.70
    const auto bytes = MsgBuilder<35>{'U'}.put_uint(11, 8, 1000).put_uint(19, 8, 1001).put_uint(27, 4, 300).put_uint(31, 4, 1'577'000).bytes();
    const auto m = read_order_replace_msg(bytes);
    EXPECT_EQ(m.original_order_ref, 1000u);
    EXPECT_EQ(m.new_order_ref, 1001u);
    EXPECT_EQ(m.shares, 300u);
    EXPECT_DOUBLE_EQ(m.price.to_double(), 157.70);
}

#include <gtest/gtest.h>

#include <stdexcept>
#include <string_view>

#include "itch/msg/alpha.hpp"
#include "msg_test_utils.hpp"

using itch::msg::Alpha;
using itch::test::make_bytes;

namespace
{
    // Everything below is checked at compile time: Alpha is fully constexpr
    static_assert(Alpha<8>{}.empty());
    static_assert(Alpha<8>{}.view().empty());
    static_assert(Alpha<8>{"AAPL"}.view() == "AAPL");
    static_assert(Alpha<8>{"AAPL"} == Alpha<8>{"AAPL    "});
    static_assert(Alpha<8>{"ABCDEFGH"}.view() == "ABCDEFGH");
}

TEST(MsgAlphaTest, DefaultIsAllSpacesAndEmpty)
{
    const Alpha<8> a{};
    EXPECT_TRUE(a.empty());
    EXPECT_TRUE(a.view().empty());
    EXPECT_EQ(a, Alpha<8>{"        "});
}

TEST(MsgAlphaTest, FromStringPadsWithSpaces)
{
    const Alpha<8> a{"AAPL"};
    EXPECT_EQ(a.view(), "AAPL");
    EXPECT_FALSE(a.empty());
}

TEST(MsgAlphaTest, FullLengthHasNothingToTrim)
{
    EXPECT_EQ(Alpha<8>{"ABCDEFGH"}.view(), "ABCDEFGH");
    EXPECT_EQ(Alpha<4>{"NSDQ"}.view(), "NSDQ");
}

TEST(MsgAlphaTest, FromBytesMatchesFromString)
{
    // This is how a stock symbol appears in a message: left-justified, space-padded
    constexpr auto bytes = make_bytes({'M', 'S', 'F', 'T', ' ', ' ', ' ', ' '});
    const Alpha<8> from_bytes{std::span<const std::byte, 8>{bytes}};

    EXPECT_EQ(from_bytes, Alpha<8>{"MSFT"});
    EXPECT_EQ(from_bytes.view(), "MSFT");
}

TEST(MsgAlphaTest, TrimsOnlyTrailingSpaces)
{
    // Inner spaces are part of the value (symbols like "BRK A")
    EXPECT_EQ(Alpha<8>{"BRK A"}.view(), "BRK A");
}

TEST(MsgAlphaTest, LeadingSpaceIsNotEmpty)
{
    // Violates the spec, but empty() and view() must agree
    const Alpha<8> a{" X"};
    EXPECT_FALSE(a.empty());
    EXPECT_EQ(a.view(), " X");
}

TEST(MsgAlphaTest, ComparesRawBytes)
{
    EXPECT_NE(Alpha<8>{"AAPL"}, Alpha<8>{"AAP"});
    EXPECT_LT(Alpha<8>{"AAPL"}, Alpha<8>{"MSFT"});
}

TEST(MsgAlphaTest, TooLongStringThrows)
{
    EXPECT_THROW({ [[maybe_unused]] Alpha<8> a{"ABCDEFGHI"}; }, std::invalid_argument);
    EXPECT_THROW({ [[maybe_unused]] Alpha<4> a{"AAPL "}; }, std::invalid_argument);
}

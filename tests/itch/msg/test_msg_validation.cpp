#include <gtest/gtest.h>

#include <array>
#include <cstddef>

#include "itch/error_handling/format_error.hpp"
#include "itch/msg/alpha_validation.hpp"
#include "itch/msg/cross_type.hpp"
#include "itch/msg/yes_no.hpp"
#include "msg_test_utils.hpp"

using itch::msg::Alpha;
using itch::msg::read_alpha_code;
using itch::test::make_bytes;

namespace
{
    constexpr std::array allowed_codes{Alpha<2>{"AB"}, Alpha<2>{"C"}, Alpha<2>{}};

    // [мусор][код 2 байта][мусор]
    constexpr std::array<std::byte, 4> code_bytes(char first, char second)
    {
        return make_bytes({0xEE, static_cast<std::uint8_t>(first), static_cast<std::uint8_t>(second), 0xEE});
    }

    constexpr Alpha<2> read_code(const std::array<std::byte, 4>& bytes)
    {
        return read_alpha_code<2, 1>(std::span<const std::byte, 4>{bytes}, allowed_codes);
    }

    static_assert(read_code(code_bytes('A', 'B')) == Alpha<2>{"AB"});
}

TEST(MsgAlphaCodeTest, AcceptsListedCodes)
{
    EXPECT_EQ(read_code(code_bytes('A', 'B')).view(), "AB");
    EXPECT_EQ(read_code(code_bytes('C', ' ')).view(), "C");
    EXPECT_TRUE(read_code(code_bytes(' ', ' ')).empty()); // пустой код допустим, только если он в списке
}

TEST(MsgAlphaCodeTest, RejectsUnlistedCodesWithFieldOffset)
{
    itch::test::expect_format_error_at(read_code, code_bytes('A', 'C'), 1);
    itch::test::expect_format_error_at(read_code, code_bytes('a', 'b'), 1); // регистр
    itch::test::expect_format_error_at(read_code, code_bytes(' ', 'C'), 1); // ведущий пробел — другой код
    itch::test::expect_format_error_at(read_code, code_bytes('C', 'C'), 1);
}

TEST(MsgSharedEnumsTest, YesNoTablesMatchSpec)
{
    itch::test::expect_table_domain(itch::msg::valid_yes_no, "YN");
    itch::test::expect_table_domain(itch::msg::valid_yes_no_or_na, "YN ");
}

TEST(MsgSharedEnumsTest, CrossTypeTablesMatchSpec)
{
    // Cross Trade: O, C, H. NOII: ещё A (Extended Trading Close)
    itch::test::expect_table_domain(itch::msg::valid_cross_trade_cross_type, "OCH");
    itch::test::expect_table_domain(itch::msg::valid_noii_cross_type, "OCHA");
}

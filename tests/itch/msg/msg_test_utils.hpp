#ifndef ITCH_TESTS_MSG_TEST_UTILS_HPP
#define ITCH_TESTS_MSG_TEST_UTILS_HPP

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "itch/error_handling/format_error.hpp"
#include "itch/msg/dispatcher.hpp" // IWYU pragma: keep

namespace itch::test
{
    // Message from hand-written bytes: make_bytes({0x53, 0x01, ...}).
    // The size is deduced from the list, so the result also fits a fixed-extent span.
    // The C-array parameter is intentional: it's the only way to deduce N from a {...} list
    template <std::size_t N>
    constexpr std::array<std::byte, N> make_bytes(const std::uint8_t (&values)[N]) noexcept // NOLINT(modernize-avoid-c-arrays)
    {
        std::array<std::byte, N> out{};
        for (std::size_t i = 0; i < N; ++i)
        {
            out[i] = static_cast<std::byte>(values[i]);
        }
        return out;
    }

    // Builds a message using offsets from the spec. Offsets in tests are literals from
    // docs/itch50_messages.md, not constants from the parser, so a parser bug won't be replicated in the test.
    template <std::size_t N>
    class MsgBuilder
    {
    public:
        // Default header: all bytes distinct (locate 0x0102, tracking 0x0304, timestamp 0x05060708090A)
        constexpr explicit MsgBuilder(char type)
        {
            put_char(0, type);
            put_uint(1, 2, 0x0102);
            put_uint(3, 2, 0x0304);
            put_uint(5, 6, 0x05060708090A);
        }

        constexpr MsgBuilder& put_uint(std::size_t offset, std::size_t len, std::uint64_t value)
        {
            check(offset, len);
            for (std::size_t i = 0; i < len; ++i)
            {
                bytes_[offset + i] = static_cast<std::byte>(value >> (8 * (len - 1 - i)));
            }
            return *this;
        }

        constexpr MsgBuilder& put_char(std::size_t offset, char c)
        {
            check(offset, 1);
            bytes_[offset] = static_cast<std::byte>(c);
            return *this;
        }

        // Alpha: left-justified, space-padded
        constexpr MsgBuilder& put_str(std::size_t offset, std::size_t len, std::string_view s)
        {
            check(offset, len);
            if (s.size() > len)
            {
                throw std::invalid_argument("string longer than field");
            }
            for (std::size_t i = 0; i < len; ++i)
            {
                bytes_[offset + i] = static_cast<std::byte>(i < s.size() ? s[i] : ' ');
            }
            return *this;
        }

        [[nodiscard]] constexpr std::array<std::byte, N> bytes() const noexcept { return bytes_; }

    private:
        static constexpr void check(std::size_t offset, std::size_t len)
        {
            if (offset + len > N)
            {
                throw std::out_of_range("field outside message");
            }
        }

        std::array<std::byte, N> bytes_{};
    };

    // The header MsgBuilder writes
    inline constexpr itch::msg::Header test_header{
        .stock_locate = 0x0102,
        .tracking_number = 0x0304,
        .timestamp = 0x05060708090A,
    };

    // Expects FormatError at position offset within the message
    template <typename Parse, std::size_t N>
    void expect_format_error_at(Parse parse, const std::array<std::byte, N>& bytes, std::size_t offset)
    {
        try
        {
            [[maybe_unused]] auto msg = parse(bytes);
            ADD_FAILURE() << "expected itch::FormatError at offset " << offset;
        }
        catch (const itch::FormatError& e)
        {
            EXPECT_EQ(e.offset(), offset) << e.what();
        }
    }

    // Exhaustive check of a one-byte flag: tries all 256 byte values at offset.
    // Values in allowed (written in the test from the spec) must parse into a field with the same character;
    // all others must throw FormatError at that offset.
    template <typename Parse, std::size_t N, typename Get>
    void expect_flag_domain(Parse parse, const std::array<std::byte, N>& bytes, std::size_t offset,
                            std::string_view allowed, Get get)
    {
        for (unsigned b = 0; b < 256; ++b)
        {
            SCOPED_TRACE(testing::Message() << "offset " << offset << ", byte 0x" << std::hex << b);

            auto copy = bytes;
            copy[offset] = static_cast<std::byte>(b);

            if (allowed.find(static_cast<char>(b)) != std::string_view::npos)
            {
                const auto msg = parse(copy);
                EXPECT_EQ(static_cast<unsigned char>(get(msg)), b);
            }
            else
            {
                expect_format_error_at(parse, copy, offset);
            }
        }
    }

    // The valid-value table accepts exactly the characters in allowed
    inline void expect_table_domain(const std::array<bool, 256>& table, std::string_view allowed)
    {
        for (unsigned b = 0; b < 256; ++b)
        {
            const bool expected = allowed.find(static_cast<char>(b)) != std::string_view::npos;
            EXPECT_EQ(table[b], expected) << "byte 0x" << std::hex << b;
        }
    }
}

// Readable printing of structs in gtest messages instead of a byte dump.
// Must live in the structs' namespace so ADL finds them.
namespace itch::msg
{
    inline void PrintTo(const Header& h, std::ostream* os)
    {
        *os << "{locate=" << h.stock_locate << ", tracking=" << h.tracking_number << ", ts=" << h.timestamp << '}';
    }

    // All flag enums store the ASCII character from the message
    template <Enum E>
    void PrintTo(const E& e, std::ostream* os)
    {
        *os << '\'' << static_cast<char>(e) << '\'';
    }

    template <std::size_t N>
    void PrintTo(const Alpha<N>& a, std::ostream* os)
    {
        *os << '"' << a.view() << '"';
    }

    template <int Decimals>
    void PrintTo(const Price<Decimals>& p, std::ostream* os)
    {
        *os << p.raw() << " (" << p.to_double() << ')';
    }
}

namespace itch::test::detail
{
    template <typename T>
    void print_value(std::ostream& os, const T& v)
    {
        if constexpr (std::is_same_v<T, itch::msg::Header> || std::is_enum_v<T> ||
                      requires { v.view(); } || requires { v.raw(); })
        {
            PrintTo(v, &os);
        }
        else
        {
            os << v;
        }
    }

    inline void print_fields(std::ostream& /*os*/) {}

    template <typename T, typename... Rest>
    void print_fields(std::ostream& os, std::string_view name, const T& value, const Rest&... rest)
    {
        os << name << '=';
        print_value(os, value);
        if constexpr (sizeof...(Rest) > 0)
        {
            os << ", ";
            print_fields(os, rest...);
        }
    }

    template <typename... Fields>
    void print_msg(std::ostream* os, const Fields&... fields)
    {
        *os << '{';
        print_fields(*os, fields...);
        *os << '}';
    }
}

namespace itch::msg
{
    using itch::test::detail::print_msg;

    inline void PrintTo(const SysEventMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "code", m.code);
    }

    inline void PrintTo(const StockDirectoryMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "stock", m.stock, "market_category", m.market_category,
                  "financial_status", m.financial_status, "round_lot_size", m.round_lot_size,
                  "round_lots_only", m.round_lots_only, "issue_classification", m.issue_classification,
                  "issue_sub_type", m.issue_sub_type, "authenticity", m.authenticity,
                  "short_sale_threshold", m.short_sale_threshold, "ipo_flag", m.ipo_flag,
                  "luld_tier", m.luld_reference_price_tier, "etp_flag", m.etp_flag,
                  "etp_leverage_factor", m.etp_leverage_factor, "inverse_indicator", m.inverse_indicator);
    }

    inline void PrintTo(const StockTradingActionMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "stock", m.stock, "trading_state", m.trading_state, "reason", m.reason);
    }

    inline void PrintTo(const RegShoMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "stock", m.stock, "action", m.action);
    }

    inline void PrintTo(const MarketParticipantPositionMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "mpid", m.mpid, "stock", m.stock, "primary_mm", m.primary_market_maker,
                  "mm_mode", m.market_maker_mode, "state", m.market_participant_state);
    }

    inline void PrintTo(const MwcbDeclineLevelMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "level1", m.level1, "level2", m.level2, "level3", m.level3);
    }

    inline void PrintTo(const MwcbStatusMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "breached_level", m.breached_level);
    }

    inline void PrintTo(const IpoQuotingPeriodUpdateMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "stock", m.stock, "release_time_seconds", m.release_time_seconds,
                  "release_qualifier", m.release_qualifier, "ipo_price", m.ipo_price);
    }

    inline void PrintTo(const LuldAuctionCollarMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "stock", m.stock, "reference_price", m.reference_price,
                  "upper", m.upper_collar_price, "lower", m.lower_collar_price, "extension", m.extension);
    }

    inline void PrintTo(const OperationalHaltMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "stock", m.stock, "market_code", m.market_code, "action", m.action);
    }

    inline void PrintTo(const AddOrderMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "order_ref", m.order_ref, "side", m.side, "shares", m.shares,
                  "stock", m.stock, "price", m.price);
    }

    inline void PrintTo(const AddOrderMpidMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "order_ref", m.order_ref, "side", m.side, "shares", m.shares,
                  "stock", m.stock, "price", m.price, "attribution", m.attribution);
    }

    inline void PrintTo(const OrderExecutedMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "order_ref", m.order_ref, "executed_shares", m.executed_shares,
                  "match_number", m.match_number);
    }

    inline void PrintTo(const OrderExecutedWithPriceMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "order_ref", m.order_ref, "executed_shares", m.executed_shares,
                  "match_number", m.match_number, "printable", m.printable, "execution_price", m.execution_price);
    }

    inline void PrintTo(const OrderCancelMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "order_ref", m.order_ref, "cancelled_shares", m.cancelled_shares);
    }

    inline void PrintTo(const OrderDeleteMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "order_ref", m.order_ref);
    }

    inline void PrintTo(const OrderReplaceMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "original_order_ref", m.original_order_ref,
                  "new_order_ref", m.new_order_ref, "shares", m.shares, "price", m.price);
    }

    inline void PrintTo(const TradeMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "order_ref", m.order_ref, "side", m.side, "shares", m.shares,
                  "stock", m.stock, "price", m.price, "match_number", m.match_number);
    }

    inline void PrintTo(const CrossTradeMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "shares", m.shares, "stock", m.stock, "cross_price", m.cross_price,
                  "match_number", m.match_number, "cross_type", m.cross_type);
    }

    inline void PrintTo(const BrokenTradeMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "match_number", m.match_number);
    }

    inline void PrintTo(const NoiiMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "paired_shares", m.paired_shares, "imbalance_shares", m.imbalance_shares,
                  "direction", m.imbalance_direction, "stock", m.stock, "far", m.far_price, "near", m.near_price,
                  "reference", m.current_reference_price, "cross_type", m.cross_type,
                  "price_variation", m.price_variation);
    }

    inline void PrintTo(const RpiiMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "stock", m.stock, "interest_flag", m.interest_flag);
    }

    inline void PrintTo(const DirectListingMsg& m, std::ostream* os)
    {
        print_msg(os, "header", m.header, "stock", m.stock, "open_eligible", m.open_eligible,
                  "min", m.min_allowable_price, "max", m.max_allowable_price, "near", m.near_execution_price,
                  "near_time", m.near_execution_time, "lower", m.lower_price_range_collar,
                  "upper", m.upper_price_range_collar);
    }
}

#endif // ITCH_TESTS_MSG_TEST_UTILS_HPP

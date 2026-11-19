#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "itch/error_handling/format_error.hpp"
#include "itch/msg/sys_event.hpp"
#include "msg_test_utils.hpp"

using itch::msg::EventCode;
using itch::msg::Header;
using itch::msg::read_sys_event_msg;
using itch::msg::SysEventMsg;
using itch::test::make_bytes;

namespace
{
    // System Event: header with distinct bytes + Event Code at offset 11
    constexpr std::array<std::byte, 12> sys_event_bytes(std::uint8_t code)
    {
        return make_bytes({
            0x53,
            0x00, 0x00,                         // locate = 0 for system messages
            0x01, 0x02,                         // tracking number
            0x03, 0x04, 0x05, 0x06, 0x07, 0x08, // timestamp
            code,
        });
    }

    constexpr Header expected_header{
        .stock_locate = 0,
        .tracking_number = 0x0102,
        .timestamp = 0x030405060708,
    };

    static_assert(read_sys_event_msg(sys_event_bytes('O')) ==
                  SysEventMsg{.header = expected_header, .code = EventCode::StartMsg});
}

TEST(MsgSysEventTest, DecodesFullMessage)
{
    const SysEventMsg expected{.header = expected_header, .code = EventCode::StartMsg};
    EXPECT_EQ(read_sys_event_msg(sys_event_bytes('O')), expected);
}

TEST(MsgSysEventTest, DecodesEveryValidCode)
{
    for (const auto code : itch::msg::event_codes)
    {
        const auto byte = static_cast<std::uint8_t>(code);
        SCOPED_TRACE(testing::Message() << "code '" << static_cast<char>(byte) << '\'');

        EXPECT_EQ(read_sys_event_msg(sys_event_bytes(byte)).code, code);
    }
}

TEST(MsgSysEventTest, SpecCodesMatchEnum)
{
    // Check the enum against the spec: values are written independently of the source
    EXPECT_EQ(static_cast<char>(EventCode::StartMsg), 'O');
    EXPECT_EQ(static_cast<char>(EventCode::StartSystemClock), 'S');
    EXPECT_EQ(static_cast<char>(EventCode::StartMarketClock), 'Q');
    EXPECT_EQ(static_cast<char>(EventCode::EndMarketClock), 'M');
    EXPECT_EQ(static_cast<char>(EventCode::EndSystemClock), 'E');
    EXPECT_EQ(static_cast<char>(EventCode::EndMsg), 'C');
    EXPECT_EQ(itch::msg::event_codes.size(), 6u);
}

TEST(MsgSysEventTest, RejectsInvalidCodes)
{
    // 'Z' is a foreign letter, 'o' tests case, 'A' is a message type rather than an event code,
    // 0x00 and space are empty values, 0xFF is index 255 (unsignedness and table size)
    constexpr std::array<std::uint8_t, 6> invalid{'Z', 'o', 'A', 0x00, ' ', 0xFF};

    for (const auto code : invalid)
    {
        SCOPED_TRACE(testing::Message() << "byte 0x" << std::hex << static_cast<unsigned>(code));
        try
        {
            [[maybe_unused]] auto msg = read_sys_event_msg(sys_event_bytes(code));
            ADD_FAILURE() << "expected itch::FormatError";
        }
        catch (const itch::FormatError& e)
        {
            // Called directly, read_*_msg reports the byte position within the message.
            // itch::dispatch replaces it with the record offset in the file (see test_dispatcher.cpp).
            EXPECT_EQ(e.offset(), 11u) << e.what();
        }
    }
}

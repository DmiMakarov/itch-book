#ifndef ITCH_MSG_SYS_EVENT_HPP 
#define ITCH_MSG_SYS_EVENT_HPP

#include <cstdint>
#include <cstddef>
#include <span>
#include <array>
#include "itch/msg_type.hpp"
#include "itch/msg/header.hpp"
#include "itch/endian.hpp"
#include "itch/msg/enum_validation.hpp"

namespace itch::msg
{
    enum class EventCode: char
    {
        StartMsg = 'O',
        StartSystemClock = 'S',
        EndSystemClock = 'E',
        StartMarketClock = 'Q',
        EndMarketClock = 'M',
        EndMsg = 'C'
    };

    inline constexpr std::array<EventCode, 6> event_codes{
        EventCode::StartMsg,
        EventCode::StartSystemClock,
        EventCode::EndSystemClock,
        EventCode::StartMarketClock,
        EventCode::EndMarketClock,
        EventCode::EndMsg
    };

    inline constexpr std::array<bool, 256> valid_event_code = make_valid_table(event_codes);

    struct SysEventMsg
    {
        Header header{};
        EventCode code{};

        bool operator==(const SysEventMsg& msg) const = default;

        static constexpr MsgType type = MsgType::SystemEvent;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr SysEventMsg read_sys_event_msg(const std::span<const std::byte, SysEventMsg::length> bytes)
    {
        Header header = read_header(bytes);
        
        auto code = read_enum<EventCode, 11>(bytes, valid_event_code);

        SysEventMsg msg{.header=header, .code=code};

        return msg; 
    }

} // namespace itch::msg

#endif //ITCH_MSG_SYS_EVENT_HPP
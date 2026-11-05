#ifndef ITCH_MSG_MWCB_STATUS_HPP
#define ITCH_MSG_MWCB_STATUS_HPP

#include <array>
#include <cstddef>
#include <span>

#include "itch/msg_type.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"

namespace itch::msg
{
    enum class BreachedLevel: char
    {
        Level1 = '1',
        Level2 = '2',
        Level3 = '3'
    };

    inline constexpr std::array breached_levels{
        BreachedLevel::Level1,
        BreachedLevel::Level2,
        BreachedLevel::Level3
    };

    inline constexpr std::array<bool, 256> valid_breached_level = make_valid_table(breached_levels);

    struct MwcbStatusMsg
    {
        Header header;
        BreachedLevel breached_level;

        bool operator==(const MwcbStatusMsg& msg) const = default;

        static constexpr MsgType type = MsgType::MWCBStatus;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr MwcbStatusMsg read_mwcb_status_msg(const std::span<const std::byte, MwcbStatusMsg::length> bytes)
    {
        return MwcbStatusMsg{
            .header = read_header(bytes),
            .breached_level = read_enum<BreachedLevel, 11>(bytes, valid_breached_level),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_MWCB_STATUS_HPP

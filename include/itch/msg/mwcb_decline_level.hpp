#ifndef ITCH_MSG_MWCB_DECLINE_LEVEL_HPP
#define ITCH_MSG_MWCB_DECLINE_LEVEL_HPP

#include <cstddef>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/price.hpp"

namespace itch::msg
{
    struct MwcbDeclineLevelMsg
    {
        Header header;
        Price<8> level1;
        Price<8> level2;
        Price<8> level3;

        bool operator==(const MwcbDeclineLevelMsg& msg) const = default;

        static constexpr MsgType type = MsgType::MWCBDeclineLevel;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr MwcbDeclineLevelMsg read_mwcb_decline_level_msg(const std::span<const std::byte, MwcbDeclineLevelMsg::length> bytes)
    {
        return MwcbDeclineLevelMsg{
            .header = read_header(bytes),
            .level1 = Price<8>{itch::endian::be64(bytes.subspan<11, 8>())},
            .level2 = Price<8>{itch::endian::be64(bytes.subspan<19, 8>())},
            .level3 = Price<8>{itch::endian::be64(bytes.subspan<27, 8>())},
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_MWCB_DECLINE_LEVEL_HPP

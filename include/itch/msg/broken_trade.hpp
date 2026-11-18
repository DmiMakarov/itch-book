#ifndef ITCH_MSG_BROKEN_TRADE_HPP
#define ITCH_MSG_BROKEN_TRADE_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/header.hpp"

namespace itch::msg
{
    // Отмена ранее опубликованной сделки (E, C или P) с этим match_number. Стакан не меняет
    struct BrokenTradeMsg
    {
        Header header{};
        std::uint64_t match_number{};

        bool operator==(const BrokenTradeMsg& msg) const = default;

        static constexpr MsgType type = MsgType::BrokenTrade;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr BrokenTradeMsg read_broken_trade_msg(const std::span<const std::byte, BrokenTradeMsg::length> bytes)
    {
        return BrokenTradeMsg{
            .header = read_header(bytes),
            .match_number = itch::endian::be64(bytes.subspan<11, 8>()),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_BROKEN_TRADE_HPP

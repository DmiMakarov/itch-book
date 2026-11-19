#ifndef ITCH_MSG_ORDER_EXECUTED_HPP
#define ITCH_MSG_ORDER_EXECUTED_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/header.hpp"

namespace itch::msg
{
    // No price here: executed at the order's price from 'A'/'F'
    struct OrderExecutedMsg
    {
        Header header{};
        std::uint64_t order_ref{};
        std::uint32_t executed_shares{};
        std::uint64_t match_number{};

        bool operator==(const OrderExecutedMsg& msg) const = default;

        static constexpr MsgType type = MsgType::OrderExecuted;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr OrderExecutedMsg read_order_executed_msg(const std::span<const std::byte, OrderExecutedMsg::length> bytes)
    {
        return OrderExecutedMsg{
            .header = read_header(bytes),
            .order_ref = itch::endian::be64(bytes.subspan<11, 8>()),
            .executed_shares = itch::endian::be32(bytes.subspan<19, 4>()),
            .match_number = itch::endian::be64(bytes.subspan<23, 8>()),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_ORDER_EXECUTED_HPP

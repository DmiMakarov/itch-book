#ifndef ITCH_MSG_ORDER_CANCEL_HPP
#define ITCH_MSG_ORDER_CANCEL_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/header.hpp"

namespace itch::msg
{
    // Частичная отмена: заявка уменьшается на cancelled_shares, но остаётся в стакане
    struct OrderCancelMsg
    {
        Header header;
        std::uint64_t order_ref;
        std::uint32_t cancelled_shares;

        bool operator==(const OrderCancelMsg& msg) const = default;

        static constexpr MsgType type = MsgType::OrderCancel;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr OrderCancelMsg read_order_cancel_msg(const std::span<const std::byte, OrderCancelMsg::length> bytes)
    {
        return OrderCancelMsg{
            .header = read_header(bytes),
            .order_ref = itch::endian::be64(bytes.subspan<11, 8>()),
            .cancelled_shares = itch::endian::be32(bytes.subspan<19, 4>()),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_ORDER_CANCEL_HPP

#ifndef ITCH_MSG_ORDER_REPLACE_HPP
#define ITCH_MSG_ORDER_REPLACE_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/price.hpp"

namespace itch::msg
{
    // The old order is removed and a new one is added with the new price and shares.
    // Side, stock and MPID are not in the message; they are taken from the original order.
    struct OrderReplaceMsg
    {
        Header header{};
        std::uint64_t original_order_ref{};
        std::uint64_t new_order_ref{};
        std::uint32_t shares{};
        Price<4> price;

        bool operator==(const OrderReplaceMsg& msg) const = default;

        static constexpr MsgType type = MsgType::OrderReplace;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr OrderReplaceMsg read_order_replace_msg(const std::span<const std::byte, OrderReplaceMsg::length> bytes)
    {
        return OrderReplaceMsg{
            .header = read_header(bytes),
            .original_order_ref = itch::endian::be64(bytes.subspan<11, 8>()),
            .new_order_ref = itch::endian::be64(bytes.subspan<19, 8>()),
            .shares = itch::endian::be32(bytes.subspan<27, 4>()),
            .price = Price<4>{itch::endian::be32(bytes.subspan<31, 4>())},
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_ORDER_REPLACE_HPP

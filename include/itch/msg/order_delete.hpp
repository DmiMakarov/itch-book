#ifndef ITCH_MSG_ORDER_DELETE_HPP
#define ITCH_MSG_ORDER_DELETE_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/header.hpp"

namespace itch::msg
{
    // Заявка удаляется из стакана целиком
    struct OrderDeleteMsg
    {
        Header header{};
        std::uint64_t order_ref{};

        bool operator==(const OrderDeleteMsg& msg) const = default;

        static constexpr MsgType type = MsgType::OrderDelete;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr OrderDeleteMsg read_order_delete_msg(const std::span<const std::byte, OrderDeleteMsg::length> bytes)
    {
        return OrderDeleteMsg{
            .header = read_header(bytes),
            .order_ref = itch::endian::be64(bytes.subspan<11, 8>()),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_ORDER_DELETE_HPP

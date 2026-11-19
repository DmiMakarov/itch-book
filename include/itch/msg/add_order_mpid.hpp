#ifndef ITCH_MSG_ADD_ORDER_MPID_HPP
#define ITCH_MSG_ADD_ORDER_MPID_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/msg_type.hpp"
#include "itch/msg/add_order.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/buy_sell_indicator.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/price.hpp"

namespace itch::msg
{
    // Fields are flat rather than wrapping an AddOrderMsg: this way 'F' has msg.header like every other type,
    // and a handler with on(const auto& m) { m.header... } works without special cases
    struct AddOrderMpidMsg
    {
        Header header{};
        std::uint64_t order_ref{};
        BuySellIndicator side{};
        std::uint32_t shares{};
        Alpha<8> stock;
        Price<4> price;
        Alpha<4> attribution;

        bool operator==(const AddOrderMpidMsg& msg) const = default;

        static constexpr MsgType type = MsgType::AddOrderMPID;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    static_assert(AddOrderMpidMsg::length == AddOrderMsg::length + 4);

    [[nodiscard]] constexpr AddOrderMpidMsg read_add_order_mpid_msg(const std::span<const std::byte, AddOrderMpidMsg::length> bytes)
    {
        // The first 36 bytes match 'A' and are parsed by the same code
        const AddOrderMsg base = read_add_order_msg(bytes.first<AddOrderMsg::length>());

        return AddOrderMpidMsg{
            .header = base.header,
            .order_ref = base.order_ref,
            .side = base.side,
            .shares = base.shares,
            .stock = base.stock,
            .price = base.price,
            .attribution = Alpha<4>{bytes.subspan<36, 4>()},
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_ADD_ORDER_MPID_HPP

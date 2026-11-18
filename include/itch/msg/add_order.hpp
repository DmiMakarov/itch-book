#ifndef ITCH_MSG_ADD_ORDER_HPP
#define ITCH_MSG_ADD_ORDER_HPP

#include <cstdint>
#include <cstddef>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/price.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/buy_sell_indicator.hpp"
#include "itch/msg/enum_validation.hpp"

namespace itch::msg
{
    struct AddOrderMsg
    {
        Header header{};
        std::uint64_t order_ref{};
        BuySellIndicator side{};
        std::uint32_t shares{};
        Alpha<8> stock;
        Price<4> price;       
        
        bool operator==(const AddOrderMsg& msg) const = default;

        static constexpr MsgType type = MsgType::AddOrder;
        static constexpr std::size_t length = msg_lengths[type_index(type)]; 
    };

    [[nodiscard]] constexpr AddOrderMsg read_add_order_msg(const std::span<const std::byte, AddOrderMsg::length> bytes)
    {
        Header header = read_header(bytes);
        std::uint64_t order_ref = itch::endian::be64(bytes.subspan<11, 8>());
        auto side = read_enum<BuySellIndicator, 19>(bytes, valid_buy_sell_indicators);
        std::uint32_t shares = itch::endian::be32(bytes.subspan<20, 4>());
        Alpha<8> stock{bytes.subspan<24, 8>()};
        Price<4> price{itch::endian::be32(bytes.subspan<32, 4>())};

        return AddOrderMsg{.header=header,
                           .order_ref=order_ref,
                           .side=side,
                           .shares=shares,
                           .stock=stock, 
                           .price=price};
    }
} // namespace itch::msg

#endif //ITCH_MSG_ADD_ORDER_HPP
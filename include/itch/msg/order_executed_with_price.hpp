#ifndef ITCH_MSG_ORDER_EXECUTED_WITH_PRICE_HPP
#define ITCH_MSG_ORDER_EXECUTED_WITH_PRICE_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/price.hpp"
#include "itch/msg/yes_no.hpp"

namespace itch::msg
{
    struct OrderExecutedWithPriceMsg
    {
        Header header{};
        std::uint64_t order_ref{};
        std::uint32_t executed_shares{};
        std::uint64_t match_number{};
        YesNo printable{};            // 'N' — не учитывать в объёме и ленте сделок
        Price<4> execution_price;   // может отличаться от цены заявки

        bool operator==(const OrderExecutedWithPriceMsg& msg) const = default;

        static constexpr MsgType type = MsgType::OrderExecutedWithPrice;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr OrderExecutedWithPriceMsg read_order_executed_with_price_msg(const std::span<const std::byte, OrderExecutedWithPriceMsg::length> bytes)
    {
        return OrderExecutedWithPriceMsg{
            .header = read_header(bytes),
            .order_ref = itch::endian::be64(bytes.subspan<11, 8>()),
            .executed_shares = itch::endian::be32(bytes.subspan<19, 4>()),
            .match_number = itch::endian::be64(bytes.subspan<23, 8>()),
            .printable = read_enum<YesNo, 31>(bytes, valid_yes_no),
            .execution_price = Price<4>{itch::endian::be32(bytes.subspan<32, 4>())},
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_ORDER_EXECUTED_WITH_PRICE_HPP

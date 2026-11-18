#ifndef ITCH_MSG_TRADE_HPP
#define ITCH_MSG_TRADE_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/buy_sell_indicator.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/price.hpp"

namespace itch::msg
{
    // Сделка со скрытой заявкой: стакан не меняет.
    // order_ref с 2010 года всегда 0, side с 2014 года всегда 'B' — но проверяем по спецификации ('B'/'S')
    struct TradeMsg
    {
        Header header{};
        std::uint64_t order_ref{};
        BuySellIndicator side{};
        std::uint32_t shares{};
        Alpha<8> stock;
        Price<4> price;
        std::uint64_t match_number{};

        bool operator==(const TradeMsg& msg) const = default;

        static constexpr MsgType type = MsgType::Trade;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr TradeMsg read_trade_msg(const std::span<const std::byte, TradeMsg::length> bytes)
    {
        return TradeMsg{
            .header = read_header(bytes),
            .order_ref = itch::endian::be64(bytes.subspan<11, 8>()),
            .side = read_enum<BuySellIndicator, 19>(bytes, valid_buy_sell_indicators),
            .shares = itch::endian::be32(bytes.subspan<20, 4>()),
            .stock = Alpha<8>{bytes.subspan<24, 8>()},
            .price = Price<4>{itch::endian::be32(bytes.subspan<32, 4>())},
            .match_number = itch::endian::be64(bytes.subspan<36, 8>()),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_TRADE_HPP

#ifndef ITCH_MSG_CROSS_TRADE_HPP
#define ITCH_MSG_CROSS_TRADE_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/cross_type.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/price.hpp"

namespace itch::msg
{
    struct CrossTradeMsg
    {
        Header header;
        std::uint64_t shares;   // 8 байт, не 4
        Alpha<8> stock;
        Price<4> cross_price;
        std::uint64_t match_number;
        CrossType cross_type;   // без ExtendedTradingClose — это значение только у NOII

        bool operator==(const CrossTradeMsg& msg) const = default;

        static constexpr MsgType type = MsgType::CrossTrade;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr CrossTradeMsg read_cross_trade_msg(const std::span<const std::byte, CrossTradeMsg::length> bytes)
    {
        return CrossTradeMsg{
            .header = read_header(bytes),
            .shares = itch::endian::be64(bytes.subspan<11, 8>()),
            .stock = Alpha<8>{bytes.subspan<19, 8>()},
            .cross_price = Price<4>{itch::endian::be32(bytes.subspan<27, 4>())},
            .match_number = itch::endian::be64(bytes.subspan<31, 8>()),
            .cross_type = read_enum<CrossType, 39>(bytes, valid_cross_trade_cross_type),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_CROSS_TRADE_HPP

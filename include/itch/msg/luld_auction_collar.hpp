#ifndef ITCH_MSG_LULD_AUCTION_COLLAR_HPP
#define ITCH_MSG_LULD_AUCTION_COLLAR_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/price.hpp"

namespace itch::msg
{
    struct LuldAuctionCollarMsg
    {
        Header header;
        Alpha<8> stock;
        Price<4> reference_price;
        Price<4> upper_collar_price;
        Price<4> lower_collar_price;
        std::uint32_t extension;

        bool operator==(const LuldAuctionCollarMsg& msg) const = default;

        static constexpr MsgType type = MsgType::LULDAuctionCollar;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr LuldAuctionCollarMsg read_luld_auction_collar_msg(const std::span<const std::byte, LuldAuctionCollarMsg::length> bytes)
    {
        return LuldAuctionCollarMsg{
            .header = read_header(bytes),
            .stock = Alpha<8>{bytes.subspan<11, 8>()},
            .reference_price = Price<4>{itch::endian::be32(bytes.subspan<19, 4>())},
            .upper_collar_price = Price<4>{itch::endian::be32(bytes.subspan<23, 4>())},
            .lower_collar_price = Price<4>{itch::endian::be32(bytes.subspan<27, 4>())},
            .extension = itch::endian::be32(bytes.subspan<31, 4>()),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_LULD_AUCTION_COLLAR_HPP

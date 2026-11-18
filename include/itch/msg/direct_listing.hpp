#ifndef ITCH_MSG_DIRECT_LISTING_HPP
#define ITCH_MSG_DIRECT_LISTING_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/price.hpp"
#include "itch/msg/yes_no.hpp"

namespace itch::msg
{
    // Direct Listing with Capital Raise Price Discovery. Добавлен в 2023 году, в файле 2019 года не встречается
    struct DirectListingMsg
    {
        Header header{};
        Alpha<8> stock;
        YesNo open_eligible{};
        Price<4> min_allowable_price;
        Price<4> max_allowable_price;
        Price<4> near_execution_price;
        std::uint64_t near_execution_time{};   // наносекунды от полуночи, 8 байт
        Price<4> lower_price_range_collar;
        Price<4> upper_price_range_collar;

        bool operator==(const DirectListingMsg& msg) const = default;

        static constexpr MsgType type = MsgType::DirectListingCapitalRaisePrice;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr DirectListingMsg read_direct_listing_msg(const std::span<const std::byte, DirectListingMsg::length> bytes)
    {
        return DirectListingMsg{
            .header = read_header(bytes),
            .stock = Alpha<8>{bytes.subspan<11, 8>()},
            .open_eligible = read_enum<YesNo, 19>(bytes, valid_yes_no),
            .min_allowable_price = Price<4>{itch::endian::be32(bytes.subspan<20, 4>())},
            .max_allowable_price = Price<4>{itch::endian::be32(bytes.subspan<24, 4>())},
            .near_execution_price = Price<4>{itch::endian::be32(bytes.subspan<28, 4>())},
            .near_execution_time = itch::endian::be64(bytes.subspan<32, 8>()),
            .lower_price_range_collar = Price<4>{itch::endian::be32(bytes.subspan<40, 4>())},
            .upper_price_range_collar = Price<4>{itch::endian::be32(bytes.subspan<44, 4>())},
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_DIRECT_LISTING_HPP

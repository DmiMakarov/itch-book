#ifndef ITCH_MSG_NOII_HPP
#define ITCH_MSG_NOII_HPP

#include <array>
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
    enum class ImbalanceDirection: char
    {
        Buy = 'B',
        Sell = 'S',
        NoImbalance = 'N',
        InsufficientOrders = 'O',
        Paused = 'P'   // since 2023
    };

    inline constexpr std::array imbalance_directions{
        ImbalanceDirection::Buy,
        ImbalanceDirection::Sell,
        ImbalanceDirection::NoImbalance,
        ImbalanceDirection::InsufficientOrders,
        ImbalanceDirection::Paused
    };

    inline constexpr std::array<bool, 256> valid_imbalance_direction = make_valid_table(imbalance_directions);

    // Absolute deviation of Near Price from Current Reference Price
    enum class PriceVariation: char
    {
        LessThan1Pct = 'L',
        From1To2Pct = '1',
        From2To3Pct = '2',
        From3To4Pct = '3',
        From4To5Pct = '4',
        From5To6Pct = '5',
        From6To7Pct = '6',
        From7To8Pct = '7',
        From8To9Pct = '8',
        From9To10Pct = '9',
        From10To20Pct = 'A',
        From20To30Pct = 'B',
        From30Pct = 'C',
        NotAvailable = ' '
    };

    inline constexpr std::array price_variations{
        PriceVariation::LessThan1Pct,
        PriceVariation::From1To2Pct,
        PriceVariation::From2To3Pct,
        PriceVariation::From3To4Pct,
        PriceVariation::From4To5Pct,
        PriceVariation::From5To6Pct,
        PriceVariation::From6To7Pct,
        PriceVariation::From7To8Pct,
        PriceVariation::From8To9Pct,
        PriceVariation::From9To10Pct,
        PriceVariation::From10To20Pct,
        PriceVariation::From20To30Pct,
        PriceVariation::From30Pct,
        PriceVariation::NotAvailable
    };

    inline constexpr std::array<bool, 256> valid_price_variation = make_valid_table(price_variations);

    struct NoiiMsg
    {
        Header header{};
        std::uint64_t paired_shares{};
        std::uint64_t imbalance_shares{};
        ImbalanceDirection imbalance_direction{};
        Alpha<8> stock;
        Price<4> far_price;
        Price<4> near_price;
        Price<4> current_reference_price;
        CrossType cross_type{};
        PriceVariation price_variation{};

        bool operator==(const NoiiMsg& msg) const = default;

        static constexpr MsgType type = MsgType::NetOrderImbalanceIndicator;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr NoiiMsg read_noii_msg(const std::span<const std::byte, NoiiMsg::length> bytes)
    {
        return NoiiMsg{
            .header = read_header(bytes),
            .paired_shares = itch::endian::be64(bytes.subspan<11, 8>()),
            .imbalance_shares = itch::endian::be64(bytes.subspan<19, 8>()),
            .imbalance_direction = read_enum<ImbalanceDirection, 27>(bytes, valid_imbalance_direction),
            .stock = Alpha<8>{bytes.subspan<28, 8>()},
            .far_price = Price<4>{itch::endian::be32(bytes.subspan<36, 4>())},
            .near_price = Price<4>{itch::endian::be32(bytes.subspan<40, 4>())},
            .current_reference_price = Price<4>{itch::endian::be32(bytes.subspan<44, 4>())},
            .cross_type = read_enum<CrossType, 48>(bytes, valid_noii_cross_type),
            .price_variation = read_enum<PriceVariation, 49>(bytes, valid_price_variation),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_NOII_HPP

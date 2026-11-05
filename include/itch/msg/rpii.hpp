#ifndef ITCH_MSG_RPII_HPP
#define ITCH_MSG_RPII_HPP

#include <array>
#include <cstddef>
#include <span>

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"

namespace itch::msg
{
    enum class InterestFlag: char
    {
        Buy = 'B',
        Sell = 'S',
        Both = 'A',
        None = 'N'
    };

    inline constexpr std::array interest_flags{
        InterestFlag::Buy,
        InterestFlag::Sell,
        InterestFlag::Both,
        InterestFlag::None
    };

    inline constexpr std::array<bool, 256> valid_interest_flag = make_valid_table(interest_flags);

    struct RpiiMsg
    {
        Header header;
        Alpha<8> stock;
        InterestFlag interest_flag;

        bool operator==(const RpiiMsg& msg) const = default;

        static constexpr MsgType type = MsgType::RetailPriceImprovementIndicator;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr RpiiMsg read_rpii_msg(const std::span<const std::byte, RpiiMsg::length> bytes)
    {
        return RpiiMsg{
            .header = read_header(bytes),
            .stock = Alpha<8>{bytes.subspan<11, 8>()},
            .interest_flag = read_enum<InterestFlag, 19>(bytes, valid_interest_flag),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_RPII_HPP

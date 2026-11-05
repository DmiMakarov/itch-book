#ifndef ITCH_MSG_REG_SHO_HPP
#define ITCH_MSG_REG_SHO_HPP

#include <array>
#include <cstddef>
#include <span>

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"

namespace itch::msg
{
    enum class RegShoAction: char
    {
        NoPriceTest = '0',
        RestrictionIntradayDrop = '1',
        RestrictionRemains = '2'
    };

    inline constexpr std::array reg_sho_actions{
        RegShoAction::NoPriceTest,
        RegShoAction::RestrictionIntradayDrop,
        RegShoAction::RestrictionRemains
    };

    inline constexpr std::array<bool, 256> valid_reg_sho_action = make_valid_table(reg_sho_actions);

    struct RegShoMsg
    {
        Header header;
        Alpha<8> stock;
        RegShoAction action;

        bool operator==(const RegShoMsg& msg) const = default;

        static constexpr MsgType type = MsgType::RegSHOIndicator;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr RegShoMsg read_reg_sho_msg(const std::span<const std::byte, RegShoMsg::length> bytes)
    {
        return RegShoMsg{
            .header = read_header(bytes),
            .stock = Alpha<8>{bytes.subspan<11, 8>()},
            .action = read_enum<RegShoAction, 19>(bytes, valid_reg_sho_action),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_REG_SHO_HPP

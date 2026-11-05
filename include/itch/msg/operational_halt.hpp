#ifndef ITCH_MSG_OPERATIONAL_HALT_HPP
#define ITCH_MSG_OPERATIONAL_HALT_HPP

#include <array>
#include <cstddef>
#include <span>

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"

namespace itch::msg
{
    enum class MarketCode: char
    {
        Nasdaq = 'Q',
        Bx = 'B',
        Psx = 'X'
    };

    inline constexpr std::array market_codes{
        MarketCode::Nasdaq,
        MarketCode::Bx,
        MarketCode::Psx
    };

    inline constexpr std::array<bool, 256> valid_market_code = make_valid_table(market_codes);

    enum class OperationalHaltAction: char
    {
        Halted = 'H',
        Resumed = 'T'
    };

    inline constexpr std::array operational_halt_actions{
        OperationalHaltAction::Halted,
        OperationalHaltAction::Resumed
    };

    inline constexpr std::array<bool, 256> valid_operational_halt_action = make_valid_table(operational_halt_actions);

    struct OperationalHaltMsg
    {
        Header header;
        Alpha<8> stock;
        MarketCode market_code;
        OperationalHaltAction action;

        bool operator==(const OperationalHaltMsg& msg) const = default;

        static constexpr MsgType type = MsgType::OperationalHalt;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr OperationalHaltMsg read_operational_halt_msg(const std::span<const std::byte, OperationalHaltMsg::length> bytes)
    {
        return OperationalHaltMsg{
            .header = read_header(bytes),
            .stock = Alpha<8>{bytes.subspan<11, 8>()},
            .market_code = read_enum<MarketCode, 19>(bytes, valid_market_code),
            .action = read_enum<OperationalHaltAction, 20>(bytes, valid_operational_halt_action),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_OPERATIONAL_HALT_HPP

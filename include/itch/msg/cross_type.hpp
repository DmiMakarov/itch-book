#ifndef ITCH_MSG_CROSS_TYPE_HPP
#define ITCH_MSG_CROSS_TYPE_HPP

#include <array>

#include "itch/msg/enum_validation.hpp"

namespace itch::msg
{
    // Тип кросса (аукциона). Общий для Cross Trade ('Q') и NOII ('I'), но наборы допустимых значений разные.
    enum class CrossType: char
    {
        Opening = 'O',
        Closing = 'C',
        IpoOrHalted = 'H',
        ExtendedTradingClose = 'A'   // только NOII, с 2022 года
    };

    inline constexpr std::array cross_trade_cross_types{
        CrossType::Opening,
        CrossType::Closing,
        CrossType::IpoOrHalted
    };

    inline constexpr std::array noii_cross_types{
        CrossType::Opening,
        CrossType::Closing,
        CrossType::IpoOrHalted,
        CrossType::ExtendedTradingClose
    };

    inline constexpr std::array<bool, 256> valid_cross_trade_cross_type = make_valid_table(cross_trade_cross_types);
    inline constexpr std::array<bool, 256> valid_noii_cross_type = make_valid_table(noii_cross_types);

} // namespace itch::msg

#endif //ITCH_MSG_CROSS_TYPE_HPP

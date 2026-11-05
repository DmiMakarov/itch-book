#ifndef ITCH_MSG_BUY_SELL_INDICATOR_HPP
#define ITCH_MSG_BUY_SELL_INDICATOR_HPP

#include <array> 
#include <cstdint>

#include "itch/msg/enum_validation.hpp"

namespace itch::msg
{
    enum class BuySellIndicator: char
    {
        Buy = 'B',
        Sell = 'S'
    };

    inline constexpr std::array<BuySellIndicator, 2>  buy_sell_indicators{
        BuySellIndicator::Buy,
        BuySellIndicator::Sell
    };

    inline constexpr std::array<bool, 256> valid_buy_sell_indicators = make_valid_table(buy_sell_indicators);

} // namespace itch::msg

#endif //ITCH_MSG_BUY_SELL_INDICATOR_HPP
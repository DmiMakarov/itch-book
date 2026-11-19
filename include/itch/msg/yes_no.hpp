#ifndef ITCH_MSG_YES_NO_HPP
#define ITCH_MSG_YES_NO_HPP

#include <array>

#include "itch/msg/enum_validation.hpp"

namespace itch::msg
{
    // Yes/no flags used by several message types.
    // A space is not valid everywhere: each field has its own table, with or without it.
    enum class YesNo: char
    {
        Yes = 'Y',
        No = 'N',
        NotAvailable = ' '
    };

    // Round Lots Only, Inverse Indicator, Primary Market Maker, Printable, Open Eligibility Status
    inline constexpr std::array yes_no_values{
        YesNo::Yes,
        YesNo::No
    };

    // Short Sale Threshold Indicator, IPO Flag, ETP Flag
    inline constexpr std::array yes_no_or_na_values{
        YesNo::Yes,
        YesNo::No,
        YesNo::NotAvailable
    };

    inline constexpr std::array<bool, 256> valid_yes_no = make_valid_table(yes_no_values);
    inline constexpr std::array<bool, 256> valid_yes_no_or_na = make_valid_table(yes_no_or_na_values);

} // namespace itch::msg

#endif //ITCH_MSG_YES_NO_HPP

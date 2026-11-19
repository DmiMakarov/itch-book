#ifndef ITCH_MSG_IPO_QUOTING_PERIOD_UPDATE_HPP
#define ITCH_MSG_IPO_QUOTING_PERIOD_UPDATE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "itch/endian.hpp"

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/price.hpp"

namespace itch::msg
{
    enum class IpoReleaseQualifier: char
    {
        Anticipated = 'A',
        CanceledOrPostponed = 'C'
    };

    inline constexpr std::array ipo_release_qualifiers{
        IpoReleaseQualifier::Anticipated,
        IpoReleaseQualifier::CanceledOrPostponed
    };

    inline constexpr std::array<bool, 256> valid_ipo_release_qualifier = make_valid_table(ipo_release_qualifiers);

    struct IpoQuotingPeriodUpdateMsg
    {
        Header header{};
        Alpha<8> stock;
        std::uint32_t release_time_seconds{};   // seconds since midnight, not nanoseconds; 0 on cancellation
        IpoReleaseQualifier release_qualifier{};
        Price<4> ipo_price;

        bool operator==(const IpoQuotingPeriodUpdateMsg& msg) const = default;

        static constexpr MsgType type = MsgType::IPOQuotingPeriodUpdate;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr IpoQuotingPeriodUpdateMsg read_ipo_quoting_period_update_msg(const std::span<const std::byte, IpoQuotingPeriodUpdateMsg::length> bytes)
    {
        return IpoQuotingPeriodUpdateMsg{
            .header = read_header(bytes),
            .stock = Alpha<8>{bytes.subspan<11, 8>()},
            .release_time_seconds = itch::endian::be32(bytes.subspan<19, 4>()),
            .release_qualifier = read_enum<IpoReleaseQualifier, 23>(bytes, valid_ipo_release_qualifier),
            .ipo_price = Price<4>{itch::endian::be32(bytes.subspan<24, 4>())},
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_IPO_QUOTING_PERIOD_UPDATE_HPP

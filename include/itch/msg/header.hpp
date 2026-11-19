#ifndef ITCH_MSG_HEADER_HPP
#define ITCH_MSG_HEADER_HPP

#include<cstdint>
#include <span>
#include "itch/endian.hpp"

namespace itch::msg
{
    struct Header
    {
        std::uint16_t stock_locate{};
        std::uint16_t tracking_number{};
        std::uint64_t timestamp{};

        bool operator==(const Header& header) const = default;
    };

    [[nodiscard]] constexpr Header read_header(const std::span<const std::byte> bytes)
    {
        const std::uint16_t stock_locate = itch::endian::be16(bytes.subspan(1).first<2>());
        const std::uint64_t common_number = itch::endian::be64(bytes.subspan(3).first<8>());
        const std::uint16_t tracking_number = static_cast<std::uint16_t>(common_number >> 48);
        const std::uint64_t timestamp = common_number & 0xFFFF'FFFF'FFFF;

        Header header{.stock_locate=stock_locate, 
                      .tracking_number=tracking_number, 
                      .timestamp=timestamp};

        return header;
    }
} // namespace itch::msg

#endif //ITCH_MSG_HEADER_HPP
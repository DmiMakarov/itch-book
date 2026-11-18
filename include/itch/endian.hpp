#ifndef ITCH_ENDIAN_HPP
#define ITCH_ENDIAN_HPP

#include<cstddef>
#include<cstring>
#include<cstdint>
#include<bit>
#include<array>
#include<algorithm>
#include <concepts>
#include<span>

namespace itch::endian
{
    constexpr bool is_big_endian() noexcept
    {
        return std::endian::native == std::endian::big;
    }

    constexpr bool is_little_endian() noexcept
    {
        return std::endian::native == std::endian::little;
    }
    
    template <std::unsigned_integral T>
    constexpr T byteswap(T value) noexcept
    {
        if constexpr (is_big_endian())
        {
            return value;
        } 
        else
        {
            auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
            std::ranges::reverse(value_representation);
            return std::bit_cast<T>(value_representation);
        }
    }

    constexpr std::uint16_t be16(std::span<const std::byte, 2> src) noexcept
    {
        return (static_cast<std::uint16_t>(src[0]) << 8)  |
               (static_cast<std::uint16_t>(src[1]));
    }

    constexpr std::uint32_t be32(std::span<const std::byte, 4> src) noexcept
    {
        return (static_cast<std::uint32_t>(src[0]) << 24) |
               (static_cast<std::uint32_t>(src[1]) << 16) |
               (static_cast<std::uint32_t>(src[2]) << 8)  |
               (static_cast<std::uint32_t>(src[3]));
    }

    constexpr std::uint64_t be48(std::span<const std::byte, 6> src) noexcept
    {
        return (static_cast<std::uint64_t>(src[0]) << 40) |
               (static_cast<std::uint64_t>(src[1]) << 32) |
               (static_cast<std::uint64_t>(src[2]) << 24) |
               (static_cast<std::uint64_t>(src[3]) << 16) |
               (static_cast<std::uint64_t>(src[4]) << 8)  |
               (static_cast<std::uint64_t>(src[5]));
    }

    constexpr std::uint64_t be64(std::span<const std::byte, 8> src) noexcept
    {
        return (static_cast<std::uint64_t>(src[0]) << 56) |
               (static_cast<std::uint64_t>(src[1]) << 48) |
               (static_cast<std::uint64_t>(src[2]) << 40) |
               (static_cast<std::uint64_t>(src[3]) << 32) |
               (static_cast<std::uint64_t>(src[4]) << 24) |
               (static_cast<std::uint64_t>(src[5]) << 16) |
               (static_cast<std::uint64_t>(src[6]) << 8)  |
               (static_cast<std::uint64_t>(src[7]));
    }
} // namespace itch::endian
#endif // ITCH_ENDIAN_HPP
#ifndef ITCH_MSG_ENUM_VALIDATION_HPP
#define ITCH_MSG_ENUM_VALIDATION_HPP

#include <span>
#include <array> 
#include <cstdint>
#include <cstddef>
#include <format>
#include <type_traits>

#include "itch/error_handling/format_error.hpp"

namespace itch::msg
{
    template <typename T>
    concept Enum = std::is_enum_v<T>;

    template<Enum T, std::size_t N>
    constexpr std::array<bool, 256> make_valid_table(const std::array<T, N>& codes) noexcept
    {
        std::array<bool, 256> table{};

        for(auto code : codes)
        {
            table[static_cast<std::uint8_t>(code)] = true;
        }

        return table;
    }

    template <Enum T, std::size_t Offset, std::size_t N>
    [[nodiscard]] constexpr T read_enum(const std::span<const std::byte, N> bytes, const std::array<bool, 256>& valid_table)
    {
        static_assert(Offset < N);

        std::uint8_t code_ = std::to_integer<std::uint8_t>(bytes[Offset]);

        if(!valid_table[code_])
        {
            throw FormatError(std::format("Unknown code in message: 0x{:02x} at offset {}",
                                           code_, Offset), Offset);
        }

        return static_cast<T>(code_);
    }
} // namespace itch::msg

#endif //ITCH_MSG_ENUM_VALIDATION_HPP
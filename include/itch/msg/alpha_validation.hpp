#ifndef ITCH_MSG_ALPHA_VALIDATION_HPP
#define ITCH_MSG_ALPHA_VALIDATION_HPP

#include <array>
#include <cstddef>
#include <format>
#include <span>

#include "itch/error_handling/format_error.hpp"
#include "itch/msg/alpha.hpp"

namespace itch::msg
{
    // Многосимвольный код (Issue Sub-Type, Trading Action Reason): читает Alpha<Len> по смещению Offset
    // и проверяет по списку допустимых значений. Список короткий и сообщения редкие — линейного поиска хватает.
    template <std::size_t Len, std::size_t Offset, std::size_t N, std::size_t K>
    [[nodiscard]] constexpr Alpha<Len> read_alpha_code(const std::span<const std::byte, N> bytes,
                                                       const std::array<Alpha<Len>, K>& allowed)
    {
        static_assert(Offset + Len <= N);

        const Alpha<Len> code{bytes.template subspan<Offset, Len>()};

        for (const auto& candidate : allowed)
        {
            if (code == candidate)
            {
                return code;
            }
        }

        throw FormatError(std::format("Unknown code in message: '{}' at offset {}", code.view(), Offset), Offset);
    }
} // namespace itch::msg

#endif //ITCH_MSG_ALPHA_VALIDATION_HPP

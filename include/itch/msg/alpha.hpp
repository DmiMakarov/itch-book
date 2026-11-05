#ifndef ITCH_MSG_ALPHA_HPP
#define ITCH_MSG_ALPHA_HPP

#include <cstddef>
#include <stdexcept>
#include <compare>
#include <format>
#include <span>
#include <array>
#include <string_view>

namespace itch::msg
{

    template<std::size_t N>
    class Alpha
    {
    public:

        constexpr Alpha() {
            txt_.fill(' ');
        }

        explicit constexpr Alpha(const std::array<char, N>& txt): txt_{txt} {}
        explicit constexpr Alpha(std::span<const std::byte, N> txt) {
            for(std::size_t i = 0; i < N; i++)
            {
                txt_[i] = static_cast<char>(txt[i]);
            }
        }

        explicit constexpr Alpha(std::string_view txt)
        {
            if(txt.size() > N) throw std::invalid_argument(std::format("Text size greater than Alpha txt size: {} vs {}",
                                                        txt.size(), N));

            std::size_t i = 0;
            for(; i < txt.size(); i++) txt_[i] = txt[i];

            for(; i < N; i++) txt_[i] = ' ';
        }

        [[nodiscard]] constexpr std::string_view view() const noexcept
        {
            std::string_view sv{txt_.data(), N};

            std::size_t end_pos = sv.find_last_not_of(' ');
            //if end_pos == std::string_view::npos, then end_pos + 1 = 0 and sv is empty, as should be
            sv = sv.substr(0, end_pos + 1);

            return sv;
        }

        auto operator<=>(const Alpha&) const = default;

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return *this == Alpha{};
        }

    private:
        std::array<char, N> txt_;
    };

    static_assert(sizeof(Alpha<8>) == 8);
    static_assert(sizeof(Alpha<4>) == 4);

} // namespace itch::msg

#endif //ITCH_MSG_ALPHA_HPP
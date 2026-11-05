#ifndef ITCH_MSG_PRICE_HPP
#define ITCH_MSG_PRICE_HPP

#include <cstdint>
#include <compare>
#include <type_traits>

namespace itch::msg
{

    template <int Decimals>
    class Price
    {
    public: 
        using Rep = std::conditional_t<Decimals == 4, std::uint32_t, std::uint64_t>;
        
        Price() = default;
        explicit constexpr Price(Rep val): value_{val} {}
    
        auto operator<=>(const Price&) const = default;

        [[nodiscard]] constexpr Rep raw() const noexcept 
        {
            return value_;
        }

        [[nodiscard]] constexpr double to_double() const noexcept
        {
            return static_cast<double>(value_) / divisor;
        }
    
    private:
        static_assert(Decimals == 4 || Decimals == 8);
        
        static constexpr Rep divisor = Decimals == 4 ?  10'000 :  100'000'000;

        Rep value_{};
    };

    static_assert(sizeof(Price<4>) == 4);
    static_assert(sizeof(Price<8>) == 8);
} // namespace itch::msg

#endif //ITCH_MSG_PRICE_HPP
#ifndef ITCH_MSG_STOCK_TRADING_ACTION_HPP
#define ITCH_MSG_STOCK_TRADING_ACTION_HPP

#include <array>
#include <cstddef>
#include <span>

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/alpha_validation.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"

namespace itch::msg
{
    enum class TradingState: char
    {
        Halted = 'H',
        Paused = 'P',
        QuotationOnly = 'Q',
        Trading = 'T'
    };

    inline constexpr std::array trading_states{
        TradingState::Halted,
        TradingState::Paused,
        TradingState::QuotationOnly,
        TradingState::Trading
    };

    inline constexpr std::array<bool, 256> valid_trading_state = make_valid_table(trading_states);

    // Приложение C спецификации. Alpha<4>{} — четыре пробела, причина не указана
    inline constexpr std::array trading_action_reasons{
        Alpha<4>{"T1"},   Alpha<4>{"T2"},   Alpha<4>{"T3"},   Alpha<4>{"T5"},   Alpha<4>{"T6"},
        Alpha<4>{"T7"},   Alpha<4>{"T8"},   Alpha<4>{"T12"},  Alpha<4>{"H4"},   Alpha<4>{"H9"},
        Alpha<4>{"H10"},  Alpha<4>{"H11"},  Alpha<4>{"O1"},   Alpha<4>{"LUDP"}, Alpha<4>{"LUDS"},
        Alpha<4>{"MWC0"}, Alpha<4>{"MWC1"}, Alpha<4>{"MWC2"}, Alpha<4>{"MWC3"}, Alpha<4>{"MWCQ"},
        Alpha<4>{"IPO1"}, Alpha<4>{"IPOQ"}, Alpha<4>{"IPOE"}, Alpha<4>{"M1"},   Alpha<4>{"M2"},
        Alpha<4>{"R1"},   Alpha<4>{"R2"},   Alpha<4>{"R4"},   Alpha<4>{"R9"},   Alpha<4>{"C3"},
        Alpha<4>{"C4"},   Alpha<4>{"C9"},   Alpha<4>{"C11"},  Alpha<4>{}
    };

    struct StockTradingActionMsg
    {
        Header header;
        Alpha<8> stock;
        TradingState trading_state;
        // Байт 20 — Reserved: не хранится и не проверяется, спецификация не задаёт его значений
        Alpha<4> reason;

        bool operator==(const StockTradingActionMsg& msg) const = default;

        static constexpr MsgType type = MsgType::StockTradingAction;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr StockTradingActionMsg read_stock_trading_action_msg(const std::span<const std::byte, StockTradingActionMsg::length> bytes)
    {
        return StockTradingActionMsg{
            .header = read_header(bytes),
            .stock = Alpha<8>{bytes.subspan<11, 8>()},
            .trading_state = read_enum<TradingState, 19>(bytes, valid_trading_state),
            .reason = read_alpha_code<4, 21>(bytes, trading_action_reasons),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_STOCK_TRADING_ACTION_HPP

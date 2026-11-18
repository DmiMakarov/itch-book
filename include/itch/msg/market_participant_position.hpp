#ifndef ITCH_MSG_MARKET_PARTICIPANT_POSITION_HPP
#define ITCH_MSG_MARKET_PARTICIPANT_POSITION_HPP

#include <array>
#include <cstddef>
#include <span>

#include "itch/msg_type.hpp"
#include "itch/msg/alpha.hpp"
#include "itch/msg/enum_validation.hpp"
#include "itch/msg/header.hpp"
#include "itch/msg/yes_no.hpp"

namespace itch::msg
{
    enum class MarketMakerMode: char
    {
        Normal = 'N',
        Passive = 'P',
        Syndicate = 'S',
        PreSyndicate = 'R',
        Penalty = 'L'
    };

    inline constexpr std::array market_maker_modes{
        MarketMakerMode::Normal,
        MarketMakerMode::Passive,
        MarketMakerMode::Syndicate,
        MarketMakerMode::PreSyndicate,
        MarketMakerMode::Penalty
    };

    inline constexpr std::array<bool, 256> valid_market_maker_mode = make_valid_table(market_maker_modes);

    enum class MarketParticipantState: char
    {
        Active = 'A',
        ExcusedWithdrawn = 'E',
        Withdrawn = 'W',
        Suspended = 'S',
        Deleted = 'D'
    };

    inline constexpr std::array market_participant_states{
        MarketParticipantState::Active,
        MarketParticipantState::ExcusedWithdrawn,
        MarketParticipantState::Withdrawn,
        MarketParticipantState::Suspended,
        MarketParticipantState::Deleted
    };

    inline constexpr std::array<bool, 256> valid_market_participant_state = make_valid_table(market_participant_states);

    struct MarketParticipantPositionMsg
    {
        Header header{};
        Alpha<4> mpid;
        Alpha<8> stock;   // здесь после MPID, а не сразу после заголовка
        YesNo primary_market_maker{};
        MarketMakerMode market_maker_mode{};
        MarketParticipantState market_participant_state{};

        bool operator==(const MarketParticipantPositionMsg& msg) const = default;

        static constexpr MsgType type = MsgType::MarketParticipantPosition;
        static constexpr std::size_t length = msg_lengths[type_index(type)];
    };

    [[nodiscard]] constexpr MarketParticipantPositionMsg read_market_participant_position_msg(const std::span<const std::byte, MarketParticipantPositionMsg::length> bytes)
    {
        return MarketParticipantPositionMsg{
            .header = read_header(bytes),
            .mpid = Alpha<4>{bytes.subspan<11, 4>()},
            .stock = Alpha<8>{bytes.subspan<15, 8>()},
            .primary_market_maker = read_enum<YesNo, 23>(bytes, valid_yes_no),
            .market_maker_mode = read_enum<MarketMakerMode, 24>(bytes, valid_market_maker_mode),
            .market_participant_state = read_enum<MarketParticipantState, 25>(bytes, valid_market_participant_state),
        };
    }
} // namespace itch::msg

#endif //ITCH_MSG_MARKET_PARTICIPANT_POSITION_HPP

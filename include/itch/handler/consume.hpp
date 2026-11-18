#ifndef ITCH_HANDLER_CONSUME_HPP
#define ITCH_HANDLER_CONSUME_HPP

#include <array>
#include <cstdint>
#include "itch/itch_reader.hpp"
#include "itch/msg_type.hpp"


namespace itch::handler
{
struct ConsumeStats
{
    std::array<std::uint64_t, 256> per_type{};
    std::uint64_t total = 0;
};

template <typename OnMessage>
ConsumeStats consume(itch::ItchReader& reader, OnMessage on_message)
{
    ConsumeStats stats;

    while (auto msg = reader.next())
    {
        ++stats.per_type[itch::type_index(msg->type)];
        ++stats.total;
        on_message(*msg);
    }

    return stats;
}
} // namespace itch::handler

#endif //ITCH_HANDLER_CONSUME_HPP

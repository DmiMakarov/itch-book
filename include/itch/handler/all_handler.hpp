#ifndef ITCH_HANDLER_ALL_HANDLER_HPP
#define ITCH_HANDLER_ALL_HANDLER_HPP

#include <cstdint>
#include <cstddef>

// Accepts all message types: dispatch parses every one.
// Checksum over timestamps so the parse result is used and the optimizer can't discard it.

namespace itch::handler
{
struct AllHandler
{
    std::size_t count = 0;
    std::uint64_t checksum = 0;

    template <typename Msg>
    void on(const Msg& msg)
    {
        ++count;
        checksum ^= msg.header.timestamp;
    }
};
} // namespace itch::handler

#endif //ITCH_HANDLER_ALL_HANDLER_HPP
#ifndef ITCH_HANDLER_ALL_HANDLER_HPP
#define ITCH_HANDLER_ALL_HANDLER_HPP

#include <cstdint>
#include <cstddef>

// Принимает все типы сообщений: dispatch разбирает каждое.
// checksum по timestamp — чтобы результат разбора был использован и оптимизатор его не выкинул.

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
#ifndef ITCH_HANDLER_SYS_EVENT_HANDLER_HPP
#define ITCH_HANDLER_SYS_EVENT_HANDLER_HPP

#include<cstdint>
#include<cstddef>
#include<array>

#include "itch/msg/sys_event.hpp"

namespace itch::handler
{
struct SysEventHandler
{
    std::size_t total_count = 0;
    std::array<std::size_t, 256> distribution{};

    void on(const itch::msg::SysEventMsg& msg)
    {
        total_count += 1;
        distribution[static_cast<std::uint8_t>(msg.code)] += 1;
    }
};
} // namespace itch::handler

#endif //ITCH_HANDLER_SYS_EVENT_HANDLER_HPP
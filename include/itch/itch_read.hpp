#ifndef ITCH_READ
#define ITCH_READ

#include <cstddef>
#include<span>
#include<bit>
#include "itch/msg_type.h"

namespace itch
{

struct Message
{
    MsgType type;
    std::span<const std::byte> bytes;
    std::size_t offset;
}

class ItchRead
{
    std::span<const std::byte> bytes;
    std::size_t offset;
    
    public:

    ItchRead(std::span<const std::byte> bytes_, std::size_t offset_);

    next();
};


}

#endif
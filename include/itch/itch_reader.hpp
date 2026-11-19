#ifndef ITCH_ITCH_READER_HPP
#define ITCH_ITCH_READER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "itch/msg_type.hpp"
#include "itch/error_handling/format_error.hpp"


namespace itch
{

struct Message
{
    MsgType type;
    std::span<const std::byte> bytes; // the whole message from the type byte, without the 2-byte length prefix
    std::size_t offset;               // where the record starts in the buffer (at its length prefix)
    std::uint64_t timestamp;
};

class ItchReader
{
    std::span<const std::byte> bytes;
    std::size_t offset = 0;
    std::uint64_t prev_timestamp = 0;

public:
    explicit ItchReader(std::span<const std::byte> bytes_, std::size_t offset_ = 0);

    // Returns the next message: the type is known, the length matches msg_lengths,
    // and the timestamp is not less than the previous one.
    // std::nullopt: the buffer ended exactly on a record boundary.
    // Any format violation throws FormatError with the record offset.
    [[nodiscard]] std::optional<Message> next();
};

}

#endif // ITCH_ITCH_READER_HPP

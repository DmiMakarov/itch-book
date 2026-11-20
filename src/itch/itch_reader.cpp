#include "itch/itch_reader.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <stdexcept>

#include "itch/error_handling/format_error.hpp"
#include "itch/msg_type.hpp"

itch::ItchReader::ItchReader(std::span<const std::byte> bytes_, std::size_t offset_) : bytes{bytes_}, offset{offset_}
{
    if (offset > bytes.size())
    {
        throw std::out_of_range(std::format("start offset {} is past the end of buffer ({} bytes)", offset, bytes.size()));
    }
}

void itch::detail::throw_truncated_prefix(std::size_t offset, std::size_t left)
{
    throw FormatError(std::format("truncated length prefix at offset {}: only {} byte left", offset, left), offset);
}

void itch::detail::throw_empty_message(std::size_t offset)
{
    throw FormatError(std::format("empty message at offset {}", offset), offset);
}

void itch::detail::throw_truncated_message(std::size_t offset, std::size_t len, std::size_t left)
{
    throw FormatError(std::format("truncated message at offset {}: length {}, only {} bytes left", offset, len, left),
                      offset);
}

void itch::detail::throw_unknown_type(std::size_t offset, MsgType type)
{
    throw FormatError(std::format("unknown message type 0x{:02x} at offset {}", type_index(type), offset), offset);
}

void itch::detail::throw_length_mismatch(std::size_t offset, MsgType type, std::size_t expected_len, std::size_t len)
{
    throw FormatError(std::format("length mismatch for type '{}' at offset {}: expected {}, got {}",
                                  static_cast<char>(type), offset, expected_len, len),
                      offset);
}

void itch::detail::throw_timestamp_backwards(std::size_t offset, std::uint64_t timestamp, std::uint64_t prev_timestamp)
{
    throw FormatError(
        std::format("timestamp went backwards at offset {}: {} < previous {}", offset, timestamp, prev_timestamp),
        offset);
}

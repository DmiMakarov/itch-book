#include "itch/itch_reader.hpp"

#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <stdexcept>

#include "itch/endian.hpp"
#include "itch/msg_type.hpp"

itch::ItchReader::ItchReader(std::span<const std::byte> bytes_, std::size_t offset_) : bytes{bytes_}, offset{offset_}
{
    if (offset > bytes.size())
    {
        throw std::out_of_range(std::format("start offset {} is past the end of buffer ({} bytes)", offset, bytes.size()));
    }
}

std::optional<itch::Message> itch::ItchReader::next()
{
    if (offset == bytes.size())
    {
        return std::nullopt;
    }

    const std::size_t left = bytes.size() - offset;

    if (left < 2)
    {
        throw FormatError(std::format("truncated length prefix at offset {}: only {} byte left", offset, left), offset);
    }

    const std::size_t len = itch::endian::be16(bytes.subspan(offset).first<2>());

    if (len == 0)
    {
        throw FormatError(std::format("empty message at offset {}", offset), offset);
    }

    if (left - 2 < len)
    {
        throw FormatError(
            std::format("truncated message at offset {}: length {}, only {} bytes left", offset, len, left - 2),
            offset);
    }

    const MsgType type{std::to_integer<char>(bytes[offset + 2])};
    const std::size_t expected_len = msg_lengths[type_index(type)];

    if (expected_len == 0)
    {
        throw FormatError(std::format("unknown message type 0x{:02x} at offset {}", type_index(type), offset), offset);
    }

    if (len != expected_len)
    {
        throw FormatError(std::format("length mismatch for type '{}' at offset {}: expected {}, got {}",
                                      static_cast<char>(type), offset, expected_len, len),
                          offset);
    }

    const std::uint64_t timestamp = itch::endian::be64(bytes.subspan(offset + 2 + 3).first<8>()) & 0xFFFF'FFFF'FFFF;
    if (timestamp < prev_timestamp)
    {
        throw FormatError(std::format("timestamp went backwards at offset {}: {} < previous {}", offset, timestamp,
                                      prev_timestamp),
                          offset);
    }
    prev_timestamp = timestamp;

    const Message msg{type, bytes.subspan(offset + 2, len), offset, timestamp};
    offset += 2 + len;
    return msg;
}

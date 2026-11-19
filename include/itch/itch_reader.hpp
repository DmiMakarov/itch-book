#ifndef ITCH_ITCH_READER_HPP
#define ITCH_ITCH_READER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "itch/endian.hpp"
#include "itch/error_handling/format_error.hpp" // IWYU pragma: export (next() throws FormatError)
#include "itch/msg_type.hpp"

namespace itch
{

struct Message
{
    MsgType type;
    std::span<const std::byte> bytes; // the whole message from the type byte, without the 2-byte length prefix
    std::size_t offset;               // where the record starts in the buffer (at its length prefix)
    std::uint64_t timestamp;
};

namespace detail
{
    // Cold error paths of ItchReader::next(), defined in itch_reader.cpp. Formatting the message is a lot of code;
    // keeping it out of next() keeps the hot path small enough for the compiler to inline.
    [[noreturn]] [[gnu::cold]] [[gnu::noinline]] void throw_truncated_prefix(std::size_t offset, std::size_t left);
    [[noreturn]] [[gnu::cold]] [[gnu::noinline]] void throw_empty_message(std::size_t offset);
    [[noreturn]] [[gnu::cold]] [[gnu::noinline]] void throw_truncated_message(std::size_t offset, std::size_t len,
                                                                              std::size_t left);
    [[noreturn]] [[gnu::cold]] [[gnu::noinline]] void throw_unknown_type(std::size_t offset, MsgType type);
    [[noreturn]] [[gnu::cold]] [[gnu::noinline]] void throw_length_mismatch(std::size_t offset, MsgType type,
                                                                            std::size_t expected_len, std::size_t len);
    [[noreturn]] [[gnu::cold]] [[gnu::noinline]] void throw_timestamp_backwards(std::size_t offset,
                                                                                std::uint64_t timestamp,
                                                                                std::uint64_t prev_timestamp);
} // namespace detail

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
    [[nodiscard]] std::optional<Message> next()
    {
        if (offset == bytes.size())
        {
            return std::nullopt;
        }

        const std::size_t left = bytes.size() - offset;

        if (left < 2) [[unlikely]]
        {
            detail::throw_truncated_prefix(offset, left);
        }

        const std::size_t len = itch::endian::be16(bytes.subspan(offset).first<2>());

        if (len == 0) [[unlikely]]
        {
            detail::throw_empty_message(offset);
        }

        if (left - 2 < len) [[unlikely]]
        {
            detail::throw_truncated_message(offset, len, left - 2);
        }

        const MsgType type{std::to_integer<char>(bytes[offset + 2])};
        const std::size_t expected_len = msg_lengths[type_index(type)];

        if (expected_len == 0) [[unlikely]]
        {
            detail::throw_unknown_type(offset, type);
        }

        if (len != expected_len) [[unlikely]]
        {
            detail::throw_length_mismatch(offset, type, expected_len, len);
        }

        // There is no 6-byte load: read the 2-byte tracking number together with the timestamp and drop it.
        const std::uint64_t timestamp = itch::endian::be64(bytes.subspan(offset + 2 + 3).first<8>()) & 0xFFFF'FFFF'FFFF;

        if (timestamp < prev_timestamp) [[unlikely]]
        {
            detail::throw_timestamp_backwards(offset, timestamp, prev_timestamp);
        }
        prev_timestamp = timestamp;

        const Message msg{type, bytes.subspan(offset + 2, len), offset, timestamp};
        offset += 2 + len;
        return msg;
    }
};

} // namespace itch

#endif // ITCH_ITCH_READER_HPP

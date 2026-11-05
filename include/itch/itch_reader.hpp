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
    std::span<const std::byte> bytes; // всё сообщение от байта типа, без 2-байтового префикса длины
    std::size_t offset;               // где в буфере начинается запись (её префикс длины)
    std::uint64_t timestamp;
};

class ItchReader
{
    std::span<const std::byte> bytes;
    std::size_t offset = 0;
    std::uint64_t prev_timestamp = 0;

public:
    explicit ItchReader(std::span<const std::byte> bytes_, std::size_t offset_ = 0);

    // Возвращает следующее сообщение: тип известен, длина совпадает с msg_lengths,
    // timestamp не меньше предыдущего.
    // std::nullopt — буфер закончился ровно на границе записи.
    // Любое нарушение формата — FormatError со смещением записи.
    [[nodiscard]] std::optional<Message> next();
};

}

#endif // ITCH_ITCH_READER_HPP

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include "itch/itch_reader.hpp"
#include "itch/msg_type.hpp"

namespace
{
    // Builds a BinaryFILE record stream: [length be16][type][locate][tracking][timestamp be48][zeros up to length]
    class StreamBuilder
    {
    public:
        // Record with the table length for its type
        StreamBuilder& add(char type, std::uint64_t timestamp)
        {
            return add(type, timestamp, itch::msg_lengths[static_cast<std::uint8_t>(type)]);
        }

        // Record with an arbitrary length; a body shorter than 11 bytes is truncated
        StreamBuilder& add(char type, std::uint64_t timestamp, std::size_t length)
        {
            put_be(length, 2);

            std::vector<std::byte> message(length);
            if (length >= 1)
            {
                message[0] = static_cast<std::byte>(type);
            }
            for (std::size_t i = 0; i < 6 && 5 + i < length; ++i)
            {
                message[5 + i] = static_cast<std::byte>(timestamp >> (8 * (5 - i)));
            }
            bytes_.insert(bytes_.end(), message.begin(), message.end());
            return *this;
        }

        StreamBuilder& raw(std::initializer_list<std::uint8_t> bytes)
        {
            for (const auto b : bytes)
            {
                bytes_.push_back(static_cast<std::byte>(b));
            }
            return *this;
        }

        [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return bytes_; }

    private:
        void put_be(std::size_t value, std::size_t width)
        {
            for (std::size_t i = 0; i < width; ++i)
            {
                bytes_.push_back(static_cast<std::byte>(value >> (8 * (width - 1 - i))));
            }
        }

        std::vector<std::byte> bytes_;
    };

    // Checks that next() throws FormatError with the expected offset
    void expect_format_error_at(itch::ItchReader& reader, std::size_t offset)
    {
        try
        {
            [[maybe_unused]] auto msg = reader.next();
            FAIL() << "expected itch::FormatError";
        }
        catch (const itch::FormatError& e)
        {
            EXPECT_EQ(e.offset(), offset) << e.what();
        }
    }
}

TEST(ItchReaderTest, ReadsSequenceOfMessages)
{
    StreamBuilder stream;
    stream.add('S', 100).add('A', 200).add('D', 200);
    itch::ItchReader reader{stream.bytes()};

    const auto s_opt = reader.next();
    ASSERT_TRUE(s_opt.has_value());
    const itch::Message& s = s_opt.value();
    EXPECT_EQ(s.type, itch::MsgType::SystemEvent);
    EXPECT_EQ(s.offset, 0u);
    EXPECT_EQ(s.bytes.size(), 12u);
    EXPECT_EQ(s.bytes[0], std::byte{'S'}); // bytes starts at the type byte
    EXPECT_EQ(s.timestamp, 100u);

    const auto a_opt = reader.next();
    ASSERT_TRUE(a_opt.has_value());
    const itch::Message& a = a_opt.value();
    EXPECT_EQ(a.type, itch::MsgType::AddOrder);
    EXPECT_EQ(a.offset, 2u + 12u);
    EXPECT_EQ(a.bytes.size(), 36u);
    EXPECT_EQ(a.bytes.data(), stream.bytes().data() + a.offset + 2); // zero-copy
    EXPECT_EQ(a.timestamp, 200u);

    const auto d_opt = reader.next();
    ASSERT_TRUE(d_opt.has_value());
    const itch::Message& d = d_opt.value();
    EXPECT_EQ(d.type, itch::MsgType::OrderDelete);
    EXPECT_EQ(d.offset, 2u + 12u + 2u + 36u);
    EXPECT_EQ(d.timestamp, 200u); // equal timestamp is allowed

    EXPECT_FALSE(reader.next().has_value());
    EXPECT_FALSE(reader.next().has_value()); // nullopt repeats after the end
}

TEST(ItchReaderTest, ReadsFullTimestampRange)
{
    constexpr std::uint64_t max_timestamp = (std::uint64_t{1} << 48) - 1;
    StreamBuilder stream;
    stream.add('S', max_timestamp);
    itch::ItchReader reader{stream.bytes()};

    const auto msg_opt = reader.next();
    ASSERT_TRUE(msg_opt.has_value());
    const itch::Message& msg = msg_opt.value();
    EXPECT_EQ(msg.timestamp, max_timestamp);
}

TEST(ItchReaderTest, EmptyBufferReturnsNullopt)
{
    itch::ItchReader reader{std::span<const std::byte>{}};
    EXPECT_FALSE(reader.next().has_value());
}

TEST(ItchReaderTest, StartOffsetSkipsLeadingRecords)
{
    StreamBuilder stream;
    stream.add('S', 1).add('A', 2);
    itch::ItchReader reader{stream.bytes(), 2 + 12};

    const auto msg_opt = reader.next();
    ASSERT_TRUE(msg_opt.has_value());
    const itch::Message& msg = msg_opt.value();
    EXPECT_EQ(msg.type, itch::MsgType::AddOrder);
    EXPECT_EQ(msg.offset, 14u);
}

TEST(ItchReaderTest, StartOffsetPastEndThrows)
{
    StreamBuilder stream;
    stream.add('S', 1);
    EXPECT_THROW(itch::ItchReader(stream.bytes(), stream.bytes().size() + 1), std::out_of_range);
}

TEST(ItchReaderTest, TruncatedLengthPrefixThrows)
{
    StreamBuilder stream;
    stream.add('S', 1).raw({0x00}); // one byte instead of two
    itch::ItchReader reader{stream.bytes()};

    ASSERT_TRUE(reader.next().has_value());
    expect_format_error_at(reader, 14);
}

TEST(ItchReaderTest, TruncatedMessageThrows)
{
    StreamBuilder stream;
    stream.add('S', 1).raw({0x00, 0x24, 'A', 0x00}); // 36 bytes declared, 2 present
    itch::ItchReader reader{stream.bytes()};

    ASSERT_TRUE(reader.next().has_value());
    expect_format_error_at(reader, 14);
}

TEST(ItchReaderTest, ZeroLengthThrows)
{
    StreamBuilder stream;
    stream.raw({0x00, 0x00}); // last two bytes of the buffer: no type byte
    itch::ItchReader reader{stream.bytes()};

    expect_format_error_at(reader, 0);
}

TEST(ItchReaderTest, UnknownTypeThrows)
{
    StreamBuilder stream;
    stream.add('S', 1).add('Z', 2, 12);
    itch::ItchReader reader{stream.bytes()};

    ASSERT_TRUE(reader.next().has_value());
    expect_format_error_at(reader, 14);
}

TEST(ItchReaderTest, NonAsciiTypeThrows)
{
    StreamBuilder stream;
    stream.add(static_cast<char>(0xFF), 1, 12); // byte > 127: the table index must not become negative
    itch::ItchReader reader{stream.bytes()};

    expect_format_error_at(reader, 0);
}

TEST(ItchReaderTest, LengthMismatchThrows)
{
    StreamBuilder stream;
    stream.add('A', 1, 12); // Add Order must be 36 bytes
    itch::ItchReader reader{stream.bytes()};

    expect_format_error_at(reader, 0);
}

TEST(ItchReaderTest, TimestampGoingBackwardsThrows)
{
    StreamBuilder stream;
    stream.add('S', 500).add('A', 600).add('D', 599);
    itch::ItchReader reader{stream.bytes()};

    ASSERT_TRUE(reader.next().has_value());
    ASSERT_TRUE(reader.next().has_value());
    expect_format_error_at(reader, 2 + 12 + 2 + 36);
}

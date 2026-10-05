#include<gtest/gtest.h>
#include "itch/endian.hpp"

TEST(EndianTest, IsBigEndian)
{
    bool is_big_endian = itch::endian::is_big_endian();
    bool is_little_endian = itch::endian::is_little_endian();

    // Ensure that only one of the two functions returns true
    EXPECT_NE(is_big_endian, is_little_endian);
}

TEST(EndianTest, Byteswap)
{
    std::uint32_t original = 0x12345678;
    std::uint32_t swapped = itch::endian::byteswap(original);

    if (itch::endian::is_big_endian())
    {
        EXPECT_EQ(swapped, original);
    }
    else
    {
        EXPECT_EQ(swapped, 0x78563412);
    }
}

TEST(EndianTest, Be16)
{
    std::array<std::byte, 2> bytes = {std::byte{0x12}, std::byte{0x34}};
    std::uint16_t value = itch::endian::be16(bytes);
    EXPECT_EQ(value, 0x1234);
}

TEST(EndianTest, Be32)
{
    std::array<std::byte, 4> bytes = {std::byte{0x12}, std::byte{0x34}, std::byte{0x56}, std::byte{0x78}};
    std::uint32_t value = itch::endian::be32(bytes);
    EXPECT_EQ(value, 0x12345678);
}

TEST(EndianTest, Be48)
{
    std::array<std::byte, 6> bytes = {std::byte{0x12}, std::byte{0x34}, std::byte{0x56}, std::byte{0x78}, std::byte{0x9A}, std::byte{0xBC}};
    std::uint64_t value = itch::endian::be48(bytes);
    EXPECT_EQ(value, 0x123456789ABC);
}

TEST(EndianTest, Be64)
{
    std::array<std::byte, 8> bytes = {std::byte{0x12}, std::byte{0x34}, std::byte{0x56}, std::byte{0x78}, std::byte{0x9A}, std::byte{0xBC}, std::byte{0xDE}, std::byte{0xF0}};
    std::uint64_t value = itch::endian::be64(bytes);
    EXPECT_EQ(value, 0x123456789ABCDEF0);
}
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#include <unistd.h>

#include "itch/mapped_file.hpp"

namespace fs = std::filesystem;

// Type properties are checked at compile time
static_assert(!std::is_copy_constructible_v<itch::MappedFile>);
static_assert(!std::is_copy_assignable_v<itch::MappedFile>);
static_assert(std::is_nothrow_move_constructible_v<itch::MappedFile>);
static_assert(std::is_nothrow_move_assignable_v<itch::MappedFile>);
static_assert(!std::is_convertible_v<fs::path, itch::MappedFile>); // constructor is explicit

// Each test gets its own temporary directory, removed after the test
class MappedFileTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        dir_ = fs::temp_directory_path() / ("itch_mapped_file_" + std::to_string(::getpid()) + "_" + info->name());
        fs::create_directories(dir_);
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(dir_, ec);
    }

    [[nodiscard]] fs::path write_file(const std::string& name, const std::vector<std::byte>& content) const
    {
        const auto path = dir_ / name;
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
        return path;
    }

    // Deterministic content: different seeds produce different files
    [[nodiscard]] static std::vector<std::byte> make_bytes(std::size_t n, unsigned seed)
    {
        std::vector<std::byte> bytes(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            bytes[i] = static_cast<std::byte>(i * 31 + seed);
        }
        return bytes;
    }

    fs::path dir_;
};

TEST_F(MappedFileTest, MapsWholeFile)
{
    const auto content = make_bytes(10'000, 1);
    itch::MappedFile file{write_file("data.bin", content)};

    const auto bytes = file.bytes();
    ASSERT_EQ(bytes.size(), content.size());
    EXPECT_TRUE(std::ranges::equal(bytes, content));
}

TEST_F(MappedFileTest, SequentialPatternMapsWholeFile)
{
    const auto content = make_bytes(10'000, 2);
    itch::MappedFile file{write_file("data.bin", content), itch::AccessPattern::Sequential};

    EXPECT_TRUE(std::ranges::equal(file.bytes(), content));
}

TEST_F(MappedFileTest, MissingFileThrowsSystemErrorWithErrno)
{
    try
    {
        itch::MappedFile file{dir_ / "missing.bin"};
        FAIL() << "expected std::system_error";
    }
    catch (const std::system_error& e)
    {
        EXPECT_EQ(e.code(), std::errc::no_such_file_or_directory);
    }
}

TEST_F(MappedFileTest, DirectoryThrowsInvalidArgument)
{
    EXPECT_THROW(itch::MappedFile{dir_}, std::invalid_argument);
}

TEST_F(MappedFileTest, EmptyFileThrowsInvalidArgument)
{
    const auto path = write_file("empty.bin", {});
    EXPECT_THROW(itch::MappedFile{path}, std::invalid_argument);
}

TEST_F(MappedFileTest, MoveConstructorTransfersOwnership)
{
    const auto content = make_bytes(5'000, 3);
    itch::MappedFile source{write_file("data.bin", content)};
    const auto* data = source.bytes().data();

    itch::MappedFile target{std::move(source)};

    EXPECT_EQ(target.bytes().data(), data);
    EXPECT_TRUE(std::ranges::equal(target.bytes(), content));
    EXPECT_TRUE(source.bytes().empty()); // NOLINT(bugprone-use-after-move): checking the moved-from state
}

TEST_F(MappedFileTest, MoveAssignmentReplacesMapping)
{
    const auto first = make_bytes(5'000, 4);
    const auto second = make_bytes(7'000, 5);
    itch::MappedFile target{write_file("first.bin", first)};
    itch::MappedFile source{write_file("second.bin", second)};

    target = std::move(source);

    EXPECT_TRUE(std::ranges::equal(target.bytes(), second));
    EXPECT_TRUE(source.bytes().empty()); // NOLINT(bugprone-use-after-move)
}

TEST_F(MappedFileTest, SelfMoveAssignmentKeepsMapping)
{
    const auto content = make_bytes(5'000, 6);
    itch::MappedFile file{write_file("data.bin", content)};

    // Through a reference so the compiler doesn't warn about explicit self-move (-Wself-move)
    auto& alias = file;
    file = std::move(alias);

    EXPECT_TRUE(std::ranges::equal(file.bytes(), content));
}

TEST_F(MappedFileTest, MovedFromObjectCanBeReused)
{
    const auto first = make_bytes(5'000, 7);
    const auto second = make_bytes(3'000, 8);
    itch::MappedFile source{write_file("first.bin", first)};
    itch::MappedFile target{std::move(source)};

    source = itch::MappedFile{write_file("second.bin", second)}; // NOLINT(bugprone-use-after-move)

    EXPECT_TRUE(std::ranges::equal(source.bytes(), second));
    EXPECT_TRUE(std::ranges::equal(target.bytes(), first));
}

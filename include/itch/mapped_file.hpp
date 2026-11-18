#ifndef ITCH_MAPPED_FILE
#define ITCH_MAPPED_FILE

#include<cstddef>
#include<cstdint>
#include<span>
#include<filesystem>


namespace itch{

enum class AccessPattern : std::uint8_t { Normal, Sequential }; 

class MappedFile {
    const std::byte* addr = nullptr;
    std::size_t size = 0;
    AccessPattern pattern = AccessPattern::Normal;

    void reset() noexcept;

public:
    explicit MappedFile(const std::filesystem::path& filepath, AccessPattern pattern_ = AccessPattern::Normal);
    ~MappedFile();

    // Disable copy
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    //move

    MappedFile(MappedFile&& other) noexcept;
    MappedFile& operator=(MappedFile&& other) noexcept;

    [[nodiscard]] std::span<const std::byte> bytes() const noexcept;
};

}
#endif // ITCH_MAPPED_FILE
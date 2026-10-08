#ifndef MAPPED_FILE
#define MAPPED_FILE

#include<cstddef>
#include<span>
#include<filesystem>


namespace itch{

enum class AccessPattern { Normal, Sequential }; 

class MappedFile {
    const std::byte* addr = nullptr;
    std::size_t size = 0;
    AccessPattern pattern_ = AccessPattern::Normal;

    void reset() noexcept;

public:
    explicit MappedFile(const std::filesystem::path& filepath, AccessPattern pattern = AccessPattern::Normal);
    ~MappedFile();

    // Disable copy
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    //move

    MappedFile(MappedFile&& other) noexcept;
    MappedFile& operator=(MappedFile&& other) noexcept;

    // Get the size of the mapped file
    std::span<const std::byte> bytes() noexcept;
};

}
#endif // MAPPED_FILE
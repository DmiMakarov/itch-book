#include<cstddef>
#include<span>
#include<filesystem>
#include<exception>
#include "itch/mapped_file.hpp"

namespace itch{
    
    void MappedFile::reset() noexcept
    {

    }

    MappedFile::MappedFile(const std::filesystem::path& filepath, AccessPattern pattern) : pattern_(pattern)
    {

    }

    MappedFile::~MappedFile()
    {
        reset();
    }

    MappedFile::MappedFile(MappedFile&& other) noexcept
    {

    }
    
    MappedFile& MappedFile::operator=(MappedFile&& other) noexcept
    {
    
    }
    
    std::span<const std::byte> bytes() noexcept
    {
        
    }
}
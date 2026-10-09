#include<cstddef>
#include<span>
#include<filesystem>
#include <stdexcept>
#include "itch/mapped_file.hpp"
#include <sys/mman.h>
#include <system_error>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cerrno>
#include <utility>

namespace itch{
    
    void MappedFile::reset() noexcept
    {
        if (addr != nullptr && size > 0)
        {
            ::munmap(const_cast<std::byte*>(addr), size);
            size = 0;
            addr = nullptr;
        }
    }

    MappedFile::MappedFile(const std::filesystem::path& filepath, AccessPattern pattern_) : pattern(pattern_)
    {
        int fd = ::open(filepath.c_str(), O_RDONLY | O_CLOEXEC);

        if (fd == -1)
        {
            throw std::system_error(errno, std::generic_category(), "open: " + filepath.string());
        }

        struct stat file_stat{};
        
        if(::fstat(fd, &file_stat)  == -1)
        {
            int local_errno = errno;
            ::close(fd);
            throw std::system_error(local_errno, std::generic_category(), "Fstat failed: " + filepath.string());
        }

        if(!S_ISREG(file_stat.st_mode))
        {
            ::close(fd);
            throw std::invalid_argument("Not regular file: " + filepath.string());
        }

        size = static_cast<std::size_t>(file_stat.st_size);

        if(size == 0)
        {
            ::close(fd);
            throw std::invalid_argument("The file is empty: " + filepath.string());
        }

        void * tmp_addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
        int local_errno = errno;

        ::close(fd);

        if(tmp_addr == MAP_FAILED)
        {
            throw std::system_error(local_errno, std::generic_category(), "mmap failed: " + filepath.string());
        }

        madvise(tmp_addr, size, pattern == itch::AccessPattern::Normal ? MADV_NORMAL : MADV_SEQUENTIAL);

        addr = static_cast<const std::byte*>(tmp_addr);

    }

    MappedFile::~MappedFile()
    {
        reset();
    }

    MappedFile::MappedFile(MappedFile&& other) noexcept : addr(std::exchange(other.addr, nullptr)), size(std::exchange(other.size, 0)), pattern(other.pattern)
    {
    }
    
    MappedFile& MappedFile::operator=(MappedFile&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            addr = std::exchange(other.addr, nullptr);
            size = std::exchange(other.size, 0);
            pattern = other.pattern;
        }
        
        return *this;
    }
    
    std::span<const std::byte> MappedFile::bytes()  noexcept
    {
        return std::span{addr, size};
    }
}
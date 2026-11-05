#ifndef ITCH_ERROR_HANDLING_FORMAT_ERROR_HPP
#define ITCH_ERROR_HANDLING_FORMAT_ERROR_HPP

#include<stdexcept>
#include<string>
#include<cstddef>

namespace itch
{

class FormatError : public std::runtime_error
{
public:
    FormatError(const std::string& what, std::size_t offset) : std::runtime_error(what), offset_(offset) {}

    [[nodiscard]] std::size_t offset() const noexcept { return offset_; }

private:
    std::size_t offset_;
};
}

#endif //ITCH_ERROR_HANDLING_FORMAT_ERROR_HPP
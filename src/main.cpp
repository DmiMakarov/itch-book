#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <chrono>
#include <cstdint>
#include "itch/mapped_file.hpp"
#include "itch/itch_reader.hpp"

int main(int argc, char** argv)
{
    std::string filepath;
    itch::AccessPattern pattern = itch::AccessPattern::Normal;

    if (argc < 2)
    {
        filepath = "/home/lama/itch-book/data/12302019.NASDAQ_ITCH50";
    } 
    else 
    {
        filepath = argv[1];

        if(argc == 3 && std::string_view{argv[2]} == "--sequential") pattern = itch::AccessPattern::Sequential;
    }

    std::array<std::uint64_t, 256> counter{};
    std::uint64_t total_messages = 0;

    try {
    itch::MappedFile file{filepath, pattern};
    itch::ItchReader reader{file.bytes()};

    auto start = std::chrono::steady_clock::now();

    while(auto msg = reader.next())
    {
        ++counter[itch::type_index(msg->type)];
        ++total_messages;
    }

    auto end = std::chrono::steady_clock::now();

    auto total_milli = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    auto total_nano = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    std::cout << "Total milliseconds: " << total_milli << std::endl;
    if (total_messages != 0 ) std::cout << "Nanoseconds per message: " << static_cast<double>(total_nano.count()) / total_messages << std::endl;
    std::cout << "Message per nanoseconds: " << static_cast<double>(total_messages) / total_nano.count()  << std::endl;
    std::cout << "Total messages: " << total_messages << std::endl;
    std::cout << "Quantity per type: " << std::endl;

    for (std::size_t i = 0; i < itch::msg_infos.size(); i++)
    {
        std::uint64_t c = counter[itch::type_index(itch::msg_infos[i].type)];
        std::cout << "\tType: " << itch::msg_infos[i].name << ", quantity: " << c << ", part: "<< static_cast<double>(c) / total_messages << std::endl;
    }

    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    
}
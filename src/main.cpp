#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

#include "itch/mapped_file.hpp"
#include "itch/itch_reader.hpp"
#include "itch/msg/dispatcher.hpp"
#include "itch/handler/sys_event_handler.hpp"
#include "itch/handler/all_handler.hpp"
#include "itch/handler/consume.hpp"

enum class HandlerType : std::uint8_t
{
    Frame, // только фрейминг ItchReader, без dispatch
    All,   // dispatch с разбором всех 23 типов
    Sys    // dispatch с разбором только System Event
};


struct Options
{
    std::string filepath;
    itch::AccessPattern pattern = itch::AccessPattern::Normal;
    HandlerType handler = HandlerType::All;
};

struct ParseError
{
    std::string message;
};

struct HelpRequested{};

using ParseResult = std::variant<Options, HelpRequested, ParseError>;

constexpr std::string_view usage = "Usage: itch_app --file=<path> [--sequential] --handler=<frame|all|sys>\n";

void print_full_help()
{
    std::cout << usage
              << "\nOptions:\n"
              << "  --file=<path>                   ITCH 5.0 file to parse (required)\n"
              << "  --sequential                    use MADV_SEQUENTIAL for mmap\n"
              << "  --handler=<frame|all|sys>       launch mode\n"
              << "  --help                          show this message\n";
}

void print_error(std::string_view message)
{
    std::cerr << "error: " << message << '\n'
              << usage
              << "Try 'itch_app --help' for more information.\n";
}

// Чистый разбор: ничего не печатает, только возвращает результат.
// argv живёт до конца программы, поэтому ключи и значения храним как string_view.
ParseResult parse_args(int argc, char** argv)
{
    std::unordered_map<std::string_view, std::string_view> args;

    for (std::string_view arg : std::span(argv, static_cast<std::size_t>(argc)).subspan(1))
    {
        std::string_view key = arg;
        std::string_view val;

        if (const auto eq = arg.find('='); eq != std::string_view::npos)
        {
            key = arg.substr(0, eq);
            val = arg.substr(eq + 1);

            if (val.empty())
            {
                return ParseError{std::format("empty value for '{}'", key)};
            }
        }

        if (!args.emplace(key, val).second)
        {
            return ParseError{std::format("duplicate option '{}'", key)};
        }
    }

    // --help проверяем первым: он важнее любых других ошибок
    if (args.contains("--help"))
    {
        return HelpRequested{};
    }

    Options opt;

    // --file: обязателен, со значением
    const auto file = args.find("--file");

    if (file == args.end() || file->second.empty())
    {
        return ParseError{"option '--file=<path>' is required"};
    }

    opt.filepath = file->second;
    args.erase(file);

    // --handler: обязателен, со значением
    const auto handler = args.find("--handler");

    if (handler == args.end() || handler->second.empty())
    {
        return ParseError{"option '--handler=<frame|all|sys>' is required"};
    }

    if (handler->second == "frame")
    {
        opt.handler = HandlerType::Frame;
    }
    else if (handler->second == "all")
    {
        opt.handler = HandlerType::All;
    }
    else if (handler->second == "sys")
    {
        opt.handler = HandlerType::Sys;
    }
    else
    {
        return ParseError{std::format("unknown handler '{}', expected frame, all or sys", handler->second)};
    }

    args.erase(handler);

    // --sequential: флаг без значения
    if (const auto seq = args.find("--sequential"); seq != args.end())
    {
        if (!seq->second.empty())
        {
            return ParseError{"option '--sequential' takes no value"};
        }

        opt.pattern = itch::AccessPattern::Sequential;
        args.erase(seq);
    }

    // всё, что осталось, — неизвестные опции
    if (!args.empty())
    {
        return ParseError{std::format("unknown option '{}'", args.begin()->first)};
    }

    return opt;
}

// Тот же цикл consume, что и в бенчмарке; время меряем снаружи, вокруг вызова
template <typename OnMessage>
std::pair<itch::handler::ConsumeStats, std::chrono::nanoseconds> timed_consume(itch::ItchReader& reader, OnMessage on_message)
{
    const auto start = std::chrono::steady_clock::now();
    const auto stats = itch::handler::consume(reader, on_message);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    return {stats, elapsed};
}

void print_stats(const itch::handler::ConsumeStats& stats, std::chrono::nanoseconds elapsed)
{
    const auto total = static_cast<double>(stats.total);
    const auto nanos = static_cast<double>(elapsed.count());

    std::cout << "Total milliseconds: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed) << '\n';
    if (stats.total != 0 && elapsed.count() != 0)
    {
        std::cout << "Nanoseconds per message: " << nanos / total << '\n';
        std::cout << "Messages per second: " << total / nanos * 1e9 << '\n';
    }
    std::cout << "Total messages: " << stats.total << '\n';
    std::cout << "Quantity per type:\n";

    for (const auto& info : itch::msg_infos)
    {
        const std::uint64_t c = stats.per_type[itch::type_index(info.type)];
        std::cout << "\tType: " << info.name << ", quantity: " << c << ", part: " << static_cast<double>(c) / total << '\n';
    }
}

void run_frame(itch::ItchReader& reader)
{
    std::uint64_t checksum = 0;

    const auto [stats, elapsed] = timed_consume(reader, [&checksum](const itch::Message& msg) { checksum ^= msg.timestamp; });

    print_stats(stats, elapsed);
    std::cout << "Checksum: " << checksum << '\n';
}

void run_all(itch::ItchReader& reader)
{
    itch::handler::AllHandler handler{};

    const auto [stats, elapsed] = timed_consume(reader, [&handler](const itch::Message& msg) { itch::dispatch(msg, handler); });

    print_stats(stats, elapsed);
    std::cout << "Parsed messages: " << handler.count << '\n';
    std::cout << "Checksum: " << handler.checksum << '\n';
}

void run_sys(itch::ItchReader& reader)
{
    itch::handler::SysEventHandler handler{};

    const auto [stats, elapsed] = timed_consume(reader, [&handler](const itch::Message& msg) { itch::dispatch(msg, handler); });

    print_stats(stats, elapsed);
    std::cout << "SysEvent message count: " << handler.total_count << '\n';

    for (auto code : itch::msg::event_codes)
    {
        std::cout << "\t Code: " << static_cast<char>(code) << ", count: " << handler.distribution[static_cast<std::uint8_t>(code)] << '\n';
    }
}

// Вся логика программы. Исключения пробрасываются наверх, в main.
int run(int argc, char** argv)
{
    const ParseResult result = parse_args(argc, argv);

    if (std::holds_alternative<HelpRequested>(result))
    {
        print_full_help();
        return 0;
    }

    if (const auto* err = std::get_if<ParseError>(&result))
    {
        print_error(err->message);
        return 2;
    }

    const auto& opt = std::get<Options>(result);

    itch::MappedFile file{opt.filepath, opt.pattern};
    itch::ItchReader reader{file.bytes()};

    switch (opt.handler)
    {
        case HandlerType::Frame: run_frame(reader); break;
        case HandlerType::All:   run_all(reader);   break;
        case HandlerType::Sys:   run_sys(reader);   break;
    }

    return 0;
}

int main(int argc, char** argv)
{
    try
    {
        return run(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}

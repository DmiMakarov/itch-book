#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <span>
#include <string>
#include <vector>

#include "itch/mapped_file.hpp"
#include "itch/itch_reader.hpp"
#include "itch/msg/dispatcher.hpp"
#include "itch/handler/sys_event_handler.hpp"
#include "itch/handler/all_handler.hpp"
#include "itch/endian.hpp"
#include "itch/handler/consume.hpp"

namespace
{

std::vector<std::byte> load_prefix(const std::string& path, std::size_t limit)
{
    itch::MappedFile file{path};
    auto bytes = file.bytes();
    std::size_t offset = 0;

    while(offset + 2 <= bytes.size())
    {
        const std::size_t len = itch::endian::be16(bytes.subspan(offset).first<2>());
        std::size_t next = offset + 2 + len;
        if(next > limit || next > bytes.size()) break;
        offset = next;
    }

    const auto prefix = bytes.first(offset);
    
    return {prefix.begin(), prefix.end()};
}

// Path to the ITCH file: $ITCH_FILE, or data/12302019.NASDAQ_ITCH50 relative to the working directory
std::string sample_path()
{
    const char* env = std::getenv("ITCH_FILE");
    return env != nullptr ? std::string{env} : std::string{"data/12302019.NASDAQ_ITCH50"};
}

// First 256 MiB of the file, cut at a record boundary; loaded once, outside the timed region
const std::vector<std::byte>& sample()
{
    static const std::vector<std::byte> data = load_prefix(sample_path(), std::size_t{1} << 28);
    return data;
}
}

void BM_DispatchAll(benchmark::State& state)
{
    const std::vector<std::byte>* buf = nullptr;
    try { buf = &sample(); }
    catch (const std::exception& e) { state.SkipWithError(e.what()); return; }

    std::uint64_t total = 0;
    for (auto _ : state)
    {
        itch::ItchReader reader{*buf};           // fresh reader for every iteration
        itch::handler::AllHandler handler{};
        const auto counts = itch::handler::consume(reader, [&handler](const itch::Message& msg) { itch::dispatch(msg, handler); });
        benchmark::DoNotOptimize(handler.checksum);
        total += counts.total;
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(total));
    state.SetBytesProcessed(static_cast<std::int64_t>(buf->size() * state.iterations()));
}


void BM_DispatchFrame(benchmark::State& state)
{
    const std::vector<std::byte>* buf = nullptr;
    try { buf = &sample(); }
    catch (const std::exception& e) { state.SkipWithError(e.what()); return; }

    std::uint64_t total = 0;
    for (auto _ : state)
    {
        itch::ItchReader reader{*buf};           // fresh reader for every iteration
        std::uint64_t checksum = 0;
        const auto counts = itch::handler::consume(reader, [&checksum](const itch::Message& msg) { checksum ^= msg.timestamp; });
        benchmark::DoNotOptimize(checksum);
        total += counts.total;
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(total));
    state.SetBytesProcessed(static_cast<std::int64_t>(buf->size() * state.iterations()));
}

void BM_DispatchSys(benchmark::State& state)
{
    const std::vector<std::byte>* buf = nullptr;
    try { buf = &sample(); }
    catch (const std::exception& e) { state.SkipWithError(e.what()); return; }

    std::uint64_t total = 0;
    for (auto _ : state)
    {
        itch::ItchReader reader{*buf};           // fresh reader for every iteration
        itch::handler::SysEventHandler handler{};
        const auto counts = itch::handler::consume(reader, [&handler](const itch::Message& msg) { itch::dispatch(msg, handler); });
        benchmark::DoNotOptimize(handler.total_count);
        total += counts.total;
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(total));
    state.SetBytesProcessed(static_cast<std::int64_t>(buf->size() * state.iterations()));
}

BENCHMARK(BM_DispatchAll)->Unit(benchmark::kMillisecond); 
BENCHMARK(BM_DispatchFrame)->Unit(benchmark::kMillisecond);    
BENCHMARK(BM_DispatchSys)->Unit(benchmark::kMillisecond);    

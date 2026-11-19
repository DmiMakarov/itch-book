#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#include <unistd.h>

#include "itch/mapped_file.hpp"

namespace fs = std::filesystem;

namespace
{
    // Temporary file of the given size; removed when the program exits
    class TempFile
    {
    public:
        explicit TempFile(std::size_t size)
            : path_(fs::temp_directory_path() /
                    ("itch_bench_" + std::to_string(::getpid()) + "_" + std::to_string(size) + ".bin"))
        {
            std::vector<char> chunk(1 << 20);
            for (std::size_t i = 0; i < chunk.size(); ++i)
            {
                chunk[i] = static_cast<char>(i * 31);
            }

            std::ofstream out(path_, std::ios::binary);
            for (std::size_t left = size; left > 0;)
            {
                const auto n = std::min(left, chunk.size());
                out.write(chunk.data(), static_cast<std::streamsize>(n));
                left -= n;
            }
        }

        ~TempFile()
        {
            std::error_code ec;
            fs::remove(path_, ec);
        }

        TempFile(const TempFile&) = delete;
        TempFile& operator=(const TempFile&) = delete;

        [[nodiscard]] const fs::path& path() const noexcept { return path_; }

    private:
        fs::path path_;
    };

    // One file per size, created on first request (outside the timed region)
    const fs::path& temp_file(std::size_t size)
    {
        static std::map<std::size_t, TempFile> files;
        auto it = files.find(size);
        if (it == files.end())
        {
            it = files.try_emplace(size, size).first;
        }
        return it->second.path();
    }

    std::uint64_t sum_bytes(std::span<const std::byte> bytes) noexcept
    {
        std::uint64_t sum = 0;
        for (const auto b : bytes)
        {
            sum += std::to_integer<std::uint64_t>(b);
        }
        return sum;
    }
}

// open + fstat + mmap + munmap without touching the data: nearly independent of file size
static void BM_MapUnmap(benchmark::State& state)
{
    const auto& path = temp_file(static_cast<std::size_t>(state.range(0)));

    for (auto _ : state)
    {
        itch::MappedFile file{path};
        benchmark::DoNotOptimize(file.bytes().data());
    }
}

// Map and full pass on every iteration: includes a page fault for every page
// (the data is already in the page cache, so this is a warm run)
template <itch::AccessPattern Pattern>
static void BM_MapAndScan(benchmark::State& state)
{
    const auto& path = temp_file(static_cast<std::size_t>(state.range(0)));

    for (auto _ : state)
    {
        itch::MappedFile file{path, Pattern};
        benchmark::DoNotOptimize(sum_bytes(file.bytes()));
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}

// Repeated pass over already-mapped pages: no page faults, only memory bandwidth remains
static void BM_ScanMapped(benchmark::State& state)
{
    itch::MappedFile file{temp_file(static_cast<std::size_t>(state.range(0)))};
    const auto bytes = file.bytes();
    benchmark::DoNotOptimize(sum_bytes(bytes)); // warm-up: all pages are now mapped

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(sum_bytes(bytes));
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}

BENCHMARK(BM_MapUnmap)->Arg(4 << 10)->Arg(1 << 20)->Arg(64 << 20);
BENCHMARK(BM_MapAndScan<itch::AccessPattern::Normal>)->Arg(64 << 20)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_MapAndScan<itch::AccessPattern::Sequential>)->Arg(64 << 20)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ScanMapped)->Arg(64 << 20)->Unit(benchmark::kMillisecond);

#include <benchmark/benchmark.h>
#include<random>
#include<vector>
#include<span>
#include<cstddef>
#include "itch/endian.hpp"



static void Be16EndianBenchmark(benchmark::State& state)
{
    std::vector<std::byte> buf(4096);
    std::mt19937 rng(42);
    for (auto& b : buf) b = static_cast<std::byte>(rng());
    
    const std::span<const std::byte> bytes{buf};
    std::size_t off = 0;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(itch::endian::be16(bytes.subspan(off).first<2>()));
        off = (off + 1) & 1023;
    }
}

static void Be32EndianBenchmark(benchmark::State& state)
{
    std::vector<std::byte> buf(4096);
    std::mt19937 rng(42);
    for (auto& b : buf) b = static_cast<std::byte>(rng());
    
    const std::span<const std::byte> bytes{buf};
    std::size_t off = 0;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(itch::endian::be32(bytes.subspan(off).first<4>()));
        off = (off + 1) & 1023;
    }
}

static void Be48EndianBenchmark(benchmark::State& state)
{
    std::vector<std::byte> buf(4096);
    std::mt19937 rng(42);
    for (auto& b : buf) b = static_cast<std::byte>(rng());
    
    const std::span<const std::byte> bytes{buf};
    std::size_t off = 0;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(itch::endian::be48(bytes.subspan(off).first<6>()));
        off = (off + 1) & 1023;
    }
}

static void Be64EndianBenchmark(benchmark::State& state)
{
    std::vector<std::byte> buf(4096);
    std::mt19937 rng(42);
    for (auto& b : buf) b = static_cast<std::byte>(rng());
    
    const std::span<const std::byte> bytes{buf};
    std::size_t off = 0;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(itch::endian::be64(bytes.subspan(off).first<8>()));
        off = (off + 1) & 1023;
    }
}

BENCHMARK(Be16EndianBenchmark);
BENCHMARK(Be32EndianBenchmark);
BENCHMARK(Be48EndianBenchmark);
BENCHMARK(Be64EndianBenchmark); 
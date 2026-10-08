#include <benchmark/benchmark.h>
#include<random>
#include<vector>
#include<span>
#include<cstddef>
#include "itch/endian.hpp"


static std::vector<std::byte> generate_data()
{
    std::vector<std::byte> data_;
    std::mt19937 rng(42);
    data_.resize(4096);
    for (auto& b : data_) b = static_cast<std::byte>(rng());

    return data_;
}


template <std::size_t N, auto Fn>
static void BeEndianBenchmark(benchmark::State& state)
{

    std::vector<std::byte> data_ = generate_data();
    const std::span<const std::byte> bytes{data_};
    std::size_t off = 0;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(Fn(bytes.subspan(off).first<N>()));
        off = (off + 1) & 1023;
    }
}

BENCHMARK(BeEndianBenchmark<2, itch::endian::be16>);
BENCHMARK(BeEndianBenchmark<4, itch::endian::be32>);
BENCHMARK(BeEndianBenchmark<6, itch::endian::be48>);
BENCHMARK(BeEndianBenchmark<8, itch::endian::be64>); 
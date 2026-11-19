# itch-book

A C++20 parser for NASDAQ TotalView-ITCH 5.0 market data, built as the foundation for a low-latency order book.

The file is memory-mapped (`mmap`), records are framed by their length prefix, and every one of the 23 message
types is decoded zero-copy into a typed struct with strict validation against the official specification.
Messages are delivered through a static, `switch`-based dispatcher: a handler declares only the types it cares
about, and all other types are skipped without being decoded.

**Status:** parser, validation, tests and a performance baseline are done.
Next: order book reconstruction, latency histograms, and a UDP-multicast / DPDK receive path.

## Performance

In-memory parsing of the first 256 MiB of the 2019-12-30 file (~7.85 M messages), median of 20 runs,
AMD Ryzen 5 7500F, GCC 13.3, `-O2`:

| Mode | ns / message | M messages / s | IPC |
|---|---:|---:|---:|
| Framing only (`ItchReader::next()`) | 3.5 | 289 | 4.28 |
| Dispatch, one type decoded | 5.3 | 189 | 3.79 |
| Dispatch, all 23 types decoded and validated | 10.7 | 94 | 2.86 |

Baseline before optimizations: 4.2 / 5.8 / 12.2 ns.

The whole trading day (268,744,780 messages, 8.25 GB) parses in ~3.3 s of CPU time. Reading it from disk
takes 10–27 s, so end to end the bottleneck is I/O, not parsing.
Methodology, `perf stat` counters, the profile, optimization iterations and the remaining candidates are in
[docs/benchmarks.md](docs/benchmarks.md).

## Build and run

Requires CMake ≥ 3.25, a C++20 compiler, GoogleTest and Google Benchmark
(`sudo apt install libbenchmark-dev libgtest-dev`).

```bash
cmake --preset release && cmake --build --preset release
ctest --preset release

./build/release/main --file=data/12302019.NASDAQ_ITCH50 --handler=all                # decode every message
./build/release/main --file=data/12302019.NASDAQ_ITCH50 --handler=frame --sequential # framing only, MADV_SEQUENTIAL
./build/release/main --help

./build/release/bench                                        # micro-benchmarks
ITCH_FILE=/path/to/file ./build/release/bench                # benchmarks read data/12302019.NASDAQ_ITCH50 by default
```

Presets (`debug` with ASan + UBSan, `release`, `tsan`) and their flags are described in
[docs/building.md](docs/building.md).

**Data:** download `12302019.NASDAQ_ITCH50.gz` from <https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/> and unpack it
into `data/` (ignored by git). The unpacked file is 8,251,407,909 bytes.

## Writing a handler

A handler is any type with `on(const XxxMsg&)` overloads. `itch::dispatch` decodes a message only if the
handler accepts its type:

```cpp
#include "itch/itch_reader.hpp"
#include "itch/mapped_file.hpp"
#include "itch/msg/dispatcher.hpp"

struct ExecutionCounter
{
    std::uint64_t executed_shares = 0;

    void on(const itch::msg::OrderExecutedMsg& msg) { executed_shares += msg.executed_shares; }
    void on(const itch::msg::OrderExecutedWithPriceMsg& msg) { executed_shares += msg.executed_shares; }
};

itch::MappedFile file{"data/12302019.NASDAQ_ITCH50"};
itch::ItchReader reader{file.bytes()};
ExecutionCounter handler;

while (auto msg = reader.next())
{
    itch::dispatch(*msg, handler);   // Add Order, Order Delete, ... are framed but never decoded
}
```

A generic `template <typename Msg> void on(const Msg&)` accepts every type
(see `include/itch/handler/all_handler.hpp`).

## Layout

| Path | Contents |
|---|---|
| `include/itch/endian.hpp` | `be16` / `be32` / `be48` / `be64` from `std::span<const std::byte, N>` |
| `include/itch/mapped_file.hpp` | `MappedFile`: move-only RAII wrapper over `mmap`, exposes `std::span<const std::byte>` |
| `include/itch/msg_type.hpp` | `enum class MsgType`, type table and a 256-entry `constexpr` length table |
| `include/itch/itch_reader.hpp` | `ItchReader::next()`: next validated record, or `FormatError` with the record offset |
| `include/itch/msg/` | One header per message type: struct + `read_xxx_msg`; `Price<N>`, `Alpha<N>`, enum validation; `dispatcher.hpp` |
| `include/itch/handler/` | Reusable handlers and the shared `consume` loop used by both the app and the benchmarks |
| `src/main.cpp` | CLI: per-type counts and timing for the `frame` / `all` / `sys` modes |
| `tests/`, `bench/` | GoogleTest suites and Google Benchmark micro-benchmarks |

Message formats, lengths and field offsets: [docs/itch50_messages.md](docs/itch50_messages.md).
Design rationale (why `mmap`, page-fault behaviour): [docs/design-notes.md](docs/design-notes.md).

## Correctness

- **Framing:** every record's length matches the spec table; an unknown type, a truncated record, an empty
  record or a timestamp going backwards raises `FormatError` with the record offset. `next()` reports
  end-of-data only at an exact record boundary, so the parsed bytes always add up to the file size.
- **Decoding:** every enum and alphanumeric code field is checked against the set of values allowed by the
  2024 ITCH 5.0 specification. Fields are read with `std::span::subspan<Offset, Length>()`, so offsets are
  bounds-checked at compile time.
- **Whole file:** all 268,744,780 messages of 2019-12-30 decode under strict validation with zero errors.
- **Cross-check:** per-type counts match an independent Python implementation on the first 3 GB
  (97,526,600 messages).
- **Sanitizers:** the debug build (ASan + UBSan) runs clean on the first ~100 MB (3,409,528 messages).
- **Tests:** 111 GoogleTest cases — endianness, `MappedFile`, every branch of the `ItchReader` contract,
  field-by-field decoding of every message type, validation domains, and the dispatcher.
- **Static analysis:** `-Wall -Wextra -Wpedantic -Werror`, clean `clang-tidy` (`.clang-tidy`).

## Messages on 2019-12-30

268,744,780 messages in total.

| Type | Message | Count | Share |
|---|---|---:|---:|
| `A` | Add Order | 117,145,568 | 43.590 % |
| `D` | Order Delete | 114,360,997 | 42.554 % |
| `U` | Order Replace | 21,639,067 | 8.052 % |
| `E` | Order Executed | 5,722,824 | 2.129 % |
| `I` | NOII | 4,024,315 | 1.497 % |
| `X` | Order Cancel | 2,787,676 | 1.037 % |
| `F` | Add Order MPID | 1,485,888 | 0.553 % |
| `P` | Trade | 1,218,602 | 0.453 % |
| `L` | Market Participant Position | 215,161 | 0.080 % |
| `C` | Order Executed With Price | 99,917 | 0.037 % |
| `Q` | Cross Trade | 17,836 | 0.007 % |
| `Y` | Reg SHO Indicator | 9,013 | 0.003 % |
| `H` | Stock Trading Action | 8,966 | 0.003 % |
| `R` | Stock Directory | 8,906 | 0.003 % |
| `J` | LULD Auction Collar | 34 | < 0.001 % |
| `S` | System Event | 6 | < 0.001 % |
| `K` | IPO Quoting Period Update | 3 | < 0.001 % |
| `V` | MWCB Decline Level | 1 | < 0.001 % |
| `W` `h` `B` `N` `O` | MWCB Status, Operational Halt, Broken Trade, RPII, DLCR | 0 | — |

The six `S` messages describe a full trading day: start of messages, start of system hours, start of market
hours and the three corresponding end events.

## Environment

All numbers come from one machine: AMD Ryzen 5 7500F (6 cores / 12 threads), 15 GiB RAM, WSL2
(kernel 5.15.167.4-microsoft-standard), ext4 on the WSL virtual disk, GCC 13.3.
CPU frequency under WSL2 is controlled by Windows; expect 3–5 % run-to-run variation.

## License

[MIT](LICENSE)

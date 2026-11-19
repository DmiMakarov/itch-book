# itch-book benchmarks

This is the parser's performance baseline: every subsequent optimization is compared against it.
Rule: one change — one run against the baseline. If the improvement is smaller than cv, there is no improvement.

---

## Baseline v1 — 2026-11-18

**Commit:** `e27f636`

**Raw logs:** `docs/bench/` (`bench_tmp.log` — all benchmarks ×20, `perf_stat.log`, `perf_record.log`, `perf_report.log`)

### Environment

| | |
|---|---|
| CPU | AMD Ryzen 5 7500F, 6 cores / 12 threads, ~4.6–4.75 GHz under load (per `perf stat`) |
| Caches | L1d 32 KiB ×6, L2 1 MiB ×6, L3 32 MiB |
| RAM | 15 GiB (~6 GiB free for page cache) |
| OS | WSL2, kernel 5.15.167.4-microsoft |
| Compiler | g++ 13.3.0, C++20 |
| Build | `release` preset = `RelWithDebInfo` (`-O2 -g`), no `-march=native`, no LTO |
| Google Benchmark | 1.8.3 from the system package (built as DEBUG — the warning in the logs refers to the library, not to our code) |
| perf | `/usr/lib/linux-tools/6.8.0-149-generic/perf` (the `/usr/bin/perf` wrapper does not work with the WSL kernel) |

### What is measured

`bench/itch/bench_parse.cpp`. Input: the first 256 MiB of `12302019.NASDAQ_ITCH50` (~7.85 million messages),
truncated at a record boundary and copied into a `std::vector` **before** timing: the page cache and page faults do not affect the numbers;
only CPU parsing is measured. All three modes go through the same `itch::handler::consume` loop.

| Benchmark | What it does |
|---|---|
| `BM_DispatchFrame` | `ItchReader::next()` only — framing, type/length/timestamp-monotonicity checks |
| `BM_DispatchSys` | `dispatch` with a handler for System Event only: the other 22 types are skipped without parsing |
| `BM_DispatchAll` | `dispatch` with parsing and validation of all 23 types |

Message type distribution in the file (268,744,780 messages total): Add Order 43.6 %, Order Delete 42.6 %,
Order Replace 8.1 %, Order Executed 2.1 %, NOII 1.5 %, everything else < 1.1 % each.

### Results

Median of 20 repetitions (`--benchmark_repetitions=20`).
Per-message metrics from `perf stat` are estimates: the message count is computed as rate × user time, which also includes loading `sample()`.

| | ns/msg | M msg/s | GiB/s | cycles/msg | instr/msg | IPC | branch-miss/msg | cv |
|---|---|---|---|---|---|---|---|---|
| Frame | **4.2** | 236.5 | 7.54 | ~20 | ~77 | 3.97 | ≈0 | 3.7 % |
| Sys | **5.9** | 171.0 | 5.45 | ~28 | ~108 | 3.88 | ≈0 | 4.7 % |
| All | **12.2** | 81.8 | 2.61 | ~57 | ~163 | 2.87 | 0.35 | 3.1 % |

Breakdown by layer:

- `dispatch` with skipping (Sys − Frame): ~1.6 ns, ~8 cycles;
- field parsing and validation (All − Sys): ~6.4 ns, ~29 cycles, of which ~6 cycles are mispredictions of the `switch` on message type.

`BM_DispatchAll` profile (`perf record`, flat):

| % | Function |
|---|---|
| 36.0 | `itch::ItchReader::next()` |
| 35.4 | `itch::dispatch<AllHandler>` (with `read_*` for most types inlined) |
| 11.8 | `itch::msg::read_add_order_msg` — not inlined |
| 8.0 | `BM_DispatchAll` (`consume` loop + per-type counter) |
| 4.1 | `itch::msg::read_header` `[clone .isra.0]` — not inlined |
| 1.9 | `itch::msg::read_noii_msg` |
| 1.6 | `memmove` (loading `sample()`) |

Auxiliary measurements (`bench_endian.cpp`, `bench_mapped_file.cpp`), medians:

| | |
|---|---|
| `be16` / `be32` / `be64` | 0.43 / 0.44 / 0.48 ns |
| `be48` | **1.00 ns** — twice as slow as `be64` |
| `MapUnmap` 4 KiB … 64 MiB | ~4.8–5.3 µs, almost independent of size (mapping is lazy) |
| `MapAndScan` 64 MiB, Normal / Sequential | 2.48 / 2.48 GiB/s — with a hot page cache, `MADV_SEQUENTIAL` changes nothing |

Full file through `main` (8.25 GB, does not fit in the page cache): 39–44 ns/msg on a "warm" run, up to 84–140 ns on a cold one.

### Conclusions

1. **Frame and Sys are CPU-bound, not memory-bound.** IPC ~4 is near the core's limit, cache misses < 0.5 %,
   no branch mispredictions. The only way to speed them up is to reduce the instruction count, and ~77 instructions for framing alone is a lot.
2. **In All, about 10 % of the time goes to the unpredictable `switch` on message type** (0.35 misses per message × ~17 cycles ≈ 6 of ~57 cycles).
   This is a property of the data (types are interleaved) and is hard to eliminate on a single thread.
3. **A third of All's time is spent in `next()`, and it is a separate call.** The function is defined in a `.cpp` file and is not inlined into the loop:
   every message costs a call/ret and a `std::optional<Message>` passed through memory.
4. **The timestamp is read twice** (in `next()` and in `read_header`), both times through the slow `be48`.
5. **`read_add_order_msg` is not inlined,** even though Add Order is the most frequent type (43.6 %).
6. **When processing the file, the bottleneck is I/O, not the parser.** In memory, a full day is parsed in ~3.3 s of CPU time; from disk it takes 11–27 s.
   Parser optimization matters for per-message latency (live feed), not for file processing time.

### Optimization candidates

Each item is a separate run against this baseline.

- [ ] Make `ItchReader::next()` inlinable (move it to the header) or enable LTO.
- [ ] Avoid reading the timestamp twice: take `Message::timestamp` in `read_header`, or do not parse it in `next()`.
- [ ] Faster `be48`: e.g. `be16 << 32 | be32` — two loads instead of six.
- [ ] Inspect `perf annotate` for `dispatch`: what the `switch` compiles to in Sys (a single comparison?) and in All (a jump table?).
- [ ] Measure the effect of `-march=native` and `-O3`.

### Measurement limitations

- WSL2: background host load, cv 3–5 %. Differences smaller than ~5 % cannot be distinguished by this measurement.
- In `BM_DispatchAll`, only a checksum over `header.timestamp` is passed to `DoNotOptimize`: the compiler is free
  not to compute fields that do not feed into it (enum validation cannot be removed — it throws).
  The All figure may therefore be understated; verify with a variant that calls `DoNotOptimize(msg)` on every struct.
- No frame pointer — stacks in `perf record -g` are unreadable, so only a flat profile is available.

### How to reproduce

```bash
cmake --build --preset release

# benchmarks
./build/release/bench --benchmark_filter=BM_Dispatch \
    --benchmark_repetitions=20 --benchmark_report_aggregates_only=true

# counters
PERF=/usr/lib/linux-tools/6.8.0-149-generic/perf
$PERF stat -e cycles,instructions,branches,branch-misses,cache-references,cache-misses,page-faults \
    -- ./build/release/bench --benchmark_filter=BM_DispatchAll --benchmark_min_time=2s

# profile
$PERF record -- ./build/release/bench --benchmark_filter=BM_DispatchAll --benchmark_min_time=2s
$PERF report --stdio --no-children

# full file
./build/release/main --file=data/12302019.NASDAQ_ITCH50 --handler=all
```

---

## End-to-end from file — framing only (initial measurement)

The first measurement of the project, before message decoding existed: `ItchReader::next()` + per-type counter
over the whole 8.25 GB file via `mmap`. Time covers only the parsing loop (no `open`/`mmap`), median of three runs,
process pinned to core 2 (`taskset -c 2`).

| Run | `madvise` | Time | ns / message | M messages / s | GB / s |
|---|---|---:|---:|---:|---:|
| Cold | `NORMAL` | 10.22 s | 38.0 | 26.3 | 0.81 |
| Cold | `SEQUENTIAL` | 9.92 s | 36.9 | 27.1 | 0.83 |
| Warm | `NORMAL` | 1.83 s | 6.81 | 146.9 | 4.51 |
| Warm | `SEQUENTIAL` | 1.82 s | 6.75 | 148.1 | 4.55 |

Page faults per run (`/usr/bin/time`): warm ≈ 126,000 minor; cold ≈ 126,000 minor and ≈ 63,000 major.
Analysis of these numbers: [design-notes.md](design-notes.md).

**Method:** cold run — the file's pages are dropped from the page cache with `posix_fadvise(POSIX_FADV_DONTNEED)`
before the run (no `sudo` for `drop_caches` under WSL; `free` shows the cache drop from 9.0 to 1.3 GiB).
Warm run — after one warm-up pass, with the whole file in the page cache. At the time ~9.4 GiB was free;
with less free memory (as in the baseline above) the file no longer fits and a "warm" run is partly cold.

**Caveat:** CPU frequency under WSL2 is managed by Windows; cold runs vary by ±10 %
(the first run after a build took 13.5 s and did not affect the median).

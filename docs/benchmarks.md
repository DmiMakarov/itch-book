# itch-book benchmarks

This is the parser's performance baseline: every subsequent optimization is compared against it.
Rule: one change — one run against the baseline. If the improvement is smaller than cv, there is no improvement.

---

## Baseline v1 — 2026-11-18

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

- [x] Make `ItchReader::next()` inlinable (move it to the header) or enable LTO. Done in
  [iteration 3](#iteration-3--cold-error-paths-and-inlining--2026-11-19): moving alone was not enough, the error paths had to go out of line; LTO gave mixed results.
- [x] Avoid reading the timestamp twice. Measured in [iteration 1](#iteration-1--faster-be48--2026-11-19): upper bound at noise level, not worth an API change.
- [x] Faster `be48`: e.g. `be16 << 32 | be32` — two loads instead of six. Done in [iteration 1](#iteration-1--faster-be48--2026-11-19).
- [ ] Inspect `perf annotate` for `dispatch`: what the `switch` compiles to in Sys (a single comparison?) and in All (a jump table?).
- [ ] Measure the effect of `-march=native`. (`-O3`: no gain on the opaque benchmark, see iteration 3.)

### Measurement limitations

- WSL2: background host load, cv 3–5 %. Differences smaller than ~5 % cannot be distinguished by this measurement.
- In `BM_DispatchAll`, only a checksum over `header.timestamp` is passed to `DoNotOptimize`: the compiler is free
  not to compute fields that do not feed into it (enum validation cannot be removed — it throws).
  The All figure may therefore be understated; verify with a variant that calls `DoNotOptimize(msg)` on every struct.
  Confirmed in [iteration 3](#the-allhandler-benchmark-overstates-gains): with aggressive inlining the gap reaches 2×.
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

## Iteration 1 — faster `be48` — 2026-11-19

### Change

GCC fuses the byte-by-byte `be16` / `be32` / `be64` into one load plus a byte swap, but there is no 6-byte load,
so the old `be48` compiled into six separate loads, shifts and ORs. It is now composed from the two fast functions:

```cpp
return (static_cast<std::uint64_t>(be16(src.first<2>())) << 32) | static_cast<std::uint64_t>(be32(src.last<4>()));
```

### Method

- **A/B on the same machine, same session:** both versions built in separate worktrees, runs interleaved
  (baseline, new, baseline, new), 20 repetitions each. The baseline numbers above were taken on another day; comparing
  against them directly would mix in CPU frequency drift.
- **Exact per-message counters:** `perf stat` with a fixed number of iterations (`--benchmark_min_time=10x` and `60x`),
  difference divided by 50 × 7,844,756 messages. Loading `sample()` cancels out, so instructions per message come out as
  whole numbers. The "~" estimates in the baseline table used rate × user time and were off by 5–10 %.

### Results

Median of 20 repetitions, two A/B rounds (ranges are the two rounds):

| | Baseline, ns/msg | `be48`, ns/msg | Δ | instr/msg | IPC |
|---|---:|---:|---:|---|---|
| Frame | 4.20–4.23 | **3.71** | **−12 %** | 85 → 78 | 4.48 → 4.05 |
| Sys | 5.80–5.82 | 5.61–5.64 | −3 % (noise) | 111 → 104 | 3.98 → 4.01 |
| All | 11.9–12.3 | **10.9–11.1** | **−9 %** | 173 → 158 | 2.93 → 3.00 |

Branch misses are unchanged (0.37 per message in All). Microbenchmark: `be48` 1.00 → 0.68 ns
(`be64` 0.48 ns, `be16` / `be32` 0.43 ns).

### Conclusions

1. **Each `be48` call was ~7 instructions too long.** Frame and Sys read the timestamp once (in `next()`) and lost
   exactly 7 instructions per message; All reads it twice (`next()` and `read_header`) and lost ~15.
2. **Frame gained the most in time, Sys did not.** The same 7 instructions are a larger share of Frame's work;
   Sys's difference is within cv.
3. **The duplicated timestamp read is not worth an API change.** Upper bound measured by temporarily replacing the read
   in `read_header` with `0` (tests do not compile, so only the `bench` target was built): All 10.8 → 10.4 ns, ~4 %,
   i.e. at the noise level — and an overestimate, since `AllHandler`'s checksum over the timestamp becomes constant too.
   Passing `Message::timestamp` into all 23 `read_*_msg` functions is not justified by this.

### Next

- Read the timestamp in `next()` with a single `be64` over the tracking number + timestamp, masked to 48 bits —
  done in [iteration 2](#iteration-2--timestamp-via-one-be64--2026-11-19).

---

## Iteration 2 — timestamp via one `be64` — 2026-11-19

### Change

There is no 6-byte load, but the timestamp is always preceded by the 2-byte tracking number inside the same
message header. So the 8 bytes at message offset 3 (tracking number + timestamp) are read with one `be64`, and the
timestamp is the low 48 bits:

```cpp
// next()
const std::uint64_t timestamp = be64(bytes.subspan(offset + 2 + 3).first<8>()) & 0xFFFF'FFFF'FFFF;

// read_header(): the same load also yields the tracking number
const std::uint64_t common_number = be64(bytes.subspan(3).first<8>());
const std::uint16_t tracking_number = static_cast<std::uint16_t>(common_number >> 48);
const std::uint64_t timestamp = common_number & 0xFFFF'FFFF'FFFF;
```

The two places were measured separately and together: (1) `read_header` only, (2) `next()` only, (3) both.

### Method

Same as iteration 1: four builds in separate worktrees (iteration 1 and variants 1–3), exact per-message counters
from fixed-iteration `perf stat` differences, then two interleaved A/B rounds × 20 repetitions.

### Results

Instructions per message (exact):

| | Frame | Sys | All |
|---|---:|---:|---:|
| Iteration 1 (`be48`) | 78 | 104 | 157.8 |
| 1: `read_header` only | 78 | 104 | 154.4 |
| 2: `next()` only | 71 | 97 | 150.8 |
| 3: both | **71** | **97** | **147.4** |

Time, ns/msg, medians of two A/B rounds:

| | Frame | Sys | All |
|---|---|---|---|
| Iteration 1 (`be48`) | 3.75–3.80 | 5.53–5.74 | 10.93–11.03 |
| 1: `read_header` only | 3.77–4.04 | 5.48–5.65 | 11.07–11.10 |
| 2: `next()` only | 3.43–3.74 | 5.33–5.41 | 10.94–11.74 |
| 3: both | **3.46** | **5.25–5.33** | **10.64–10.66** |

### Conclusions

1. **The effects add up exactly.** `next()` runs for every message and saves 7 instructions in every mode;
   `read_header` runs only for decoded messages and saves ~3 per decoded message, so it shows up only in All.
2. **Time follows instructions only where the code is instruction-bound.** Frame: −8 % (3.78 → 3.46 ns), consistent
   across rounds and confirmed by −5 % cycles. Sys: ~−5 %, borderline. All: −3 %, within noise — at IPC ≈ 3 it is
   limited by `switch` mispredictions and dependency chains, so ten fewer instructions are almost free.
3. **Variant 3 is kept.** The gain is small but the code is no more complex, and the instruction reduction is exact.
   `be48` stays in `endian.hpp` (tested and benchmarked) but is no longer used on the hot path.

---

## Iteration 3 — cold error paths and inlining — 2026-11-19

### Change

1. **`ItchReader::next()` moved into the header, its error paths moved out of line.** Every `throw FormatError(std::format(...))`
   is replaced by a call to a `[[noreturn]] [[gnu::cold]] [[gnu::noinline]]` function in `itch::detail`, defined in
   `itch_reader.cpp`; the branches are marked `[[unlikely]]`. `<format>` left the header.
2. **The same for validation:** `read_enum` (`enum_validation.hpp`) and `read_alpha_code` (`alpha_validation.hpp`) call
   non-template cold functions defined `inline` in the header, one copy per program.
3. **`dispatch` is declared `inline`.**

### How we got there

The compiler explains each decision with `-fopt-info-inline-missed`. Intermediate variants, all measured:

| Variant | Result |
|---|---|
| LTO (`-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON`) | Mixed: Sys −22 %, All −9 %, **Frame +29 %**. `next()` still not inlined; `dispatch` was inlined into the loop, but `be64` got outlined from `next()` (+12 instructions) |
| `next()` moved to the header as is | **Worse everywhere** (+12 instructions): still not inlined, and `be64` inside it got outlined, as with LTO |
| Header + `[[gnu::always_inline]]` on `next()` | Upper bound for inlining: Frame 3.5 → 1.71 ns |
| Header + cold error paths (change 1) | Inlined without forcing, and **better than the forced variant**: Frame 1.69 ns, 34 instructions |

Why: GCC estimates a function's size including the code that formats and throws the exception, even though it never
runs. Five `std::format` + `throw` blocks made the 40-instruction hot path of `next()` look too big. The same blocks,
inlined from `read_enum` into `dispatch<AllHandler>`, made it ~500 lines of assembly with 9 copies of exception
construction.

`inline` on `dispatch` matters because GCC applies different limits: a template that is not declared `inline` gets the
low `max-inline-insns-auto` threshold; declared `inline`, it gets `max-inline-insns-single`. Without the keyword
`dispatch` was not inlined into the loop (`-fopt-info`: `max-inline-insns-auto limit reached`).

### Results

Per-message counters from fixed-iteration `perf stat` differences (exact, frequency-independent).
`Opaque` is `BM_DispatchOpaque`: the handler copies every decoded struct into `benchmark::DoNotOptimize`, so no field
can be optimized away (see [below](#the-allhandler-benchmark-overstates-gains)).

| | Iteration 2 | Cold paths in `next()` | **+ cold validation, `inline dispatch`** | Δ cycles |
|---|---|---|---|---:|
| Frame | 71 instr / 19.3 cyc | 34 / 8.5 | **34 / 8.4** | **−56 %** |
| Sys | 95 / 28.4 | 67 / 19.4 | **37 / 8.8** | **−69 %** |
| All | 148 / 59.1 | 120 / 49.6 | **110 / 45.5** | −23 % |
| Opaque | 131 / 58.6 | 94 / 45.0 | **83 / 42.5** | **−27 %** |

Time, interleaved A/B in one session, ns/msg (the machine ran ~30 % slower than in iterations 1–2, so only the ratios
matter): Frame 4.6 → 2.15 (−53 %), Sys 6.75 → 2.2 (−67 %), Opaque 14.2 → 10.7 (−25 %).

Final figures on a quiet machine (median of 20, cv 2.4–4.7 %), after `BM_DispatchOpaque` was added to the repository:

| | ns/msg | M msg/s | instr/msg | cyc/msg | IPC |
|---|---:|---:|---:|---:|---:|
| Frame | **1.67** | 600 | 34.0 | 8.0 | 4.23 |
| Sys | **1.84** | 544 | 37.0 | 8.3 | 4.44 |
| All | 8.97 | 111 | 110.8 | 43.0 | 2.58 |
| Opaque | **8.71** | 115 | 83.8 | 40.7 | 2.06 |

Against the baseline: Frame 4.2 → 1.67 ns (2.5×), Sys 5.8 → 1.84 ns (3.2×).

Sys now costs Frame + 3 instructions: with `dispatch` inlined, skipping 22 uninteresting types is a single comparison.

### The `AllHandler` benchmark overstates gains

After change 3, `read_header` and `read_add_order_msg` were still calls inside `dispatch<AllHandler>`
(`-fopt-info`: `inline-unit-growth limit reached` and `max-inline-insns-single limit reached`). Forcing them in looked
promising on `BM_DispatchAll`, so three variants were measured on both benchmarks:

| Variant | All, cyc/msg | Opaque, cyc/msg |
|---|---:|---:|
| Current | 46.5 | **42.8** |
| `[[gnu::always_inline]]` on `read_header` and `read_add_order_msg` | 45.0 | 46.0 |
| `-O3` | 32.2 | 44.8 |
| `--param inline-unit-growth=200 max-inline-insns-single=400 max-inline-insns-auto=200` | 22.5 | 54.5 |

`AllHandler` uses only `header.timestamp`. Once everything is inlined down to the handler, the compiler is free to skip
computing the other fields, so a "faster" All means "more work thrown away". On the opaque benchmark none of the
variants helps, and the raised limits make it worse (code bloat). The same effect inflated change 2: −15 % on All,
−7.5 % on Opaque.

A second trap: `inline-unit-growth` is a budget for the whole translation unit. Adding `BM_DispatchOpaque` to
`bench_parse.cpp` changed `BM_DispatchAll`'s instruction count (103.7 → 110.1) without touching its code.

### Conclusions

1. **Keep cold code out of hot functions.** Moving the error paths out of line is what made `next()` and `dispatch`
   inlinable; forcing with `always_inline` was worse.
2. **`inline` is not only an ODR keyword for GCC:** it selects a higher inlining limit.
3. **Do not chase inlining with `-O3` / `--param` / `always_inline`:** no gain on a realistic handler.
4. **Benchmarks must consume what they measure.** Decoding results should be compared on the opaque benchmark;
   `BM_DispatchAll` is a lower bound.

### Next

- Add `BM_DispatchOpaque` to `bench_parse.cpp` and use it as the main "decode everything" figure.
- All/Opaque remain at ~43 cycles: look at `perf annotate` — `switch` mispredictions (0.45 per message × ~17 cycles ≈ 8
  cycles) and struct materialization.

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

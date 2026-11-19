# Design notes

## Why `mmap` instead of `read()` into a buffer

**Simpler parsing.** With `mmap` the whole file is one contiguous `std::span<const std::byte>`.
When reading in blocks, a record (12–50 bytes) can start at the end of one block and finish at the start
of the next; that requires "move the tail to the front of the buffer and read more" logic — a common source of rare bugs.

**No copying.** `read()` copies data from the page cache into a user-space buffer; with `mmap`
the process reads page cache pages directly. There is also no system call per block.

**Groundwork for zero-copy parsing.** Zero-copy field parsing requires the bytes to already be contiguous in memory.

**Downsides:**
- I/O errors and a file truncated while mapped arrive as a `SIGBUS` signal rather than a return code;
- every first access to a page is a page fault (see below), while `read()` with large blocks enters
  the kernel less often; for sequential reads, `read()` with a 1–4 MB buffer is often no slower;
- works only with regular files: a pipe, a socket or a `.gz` file cannot be mapped;
- the kernel, not the program, decides what stays in memory.

For this reason the parser takes a `std::span` rather than a `MappedFile`: the byte source can be swapped
(a `read()` buffer, a MoldUDP64 network packet) without touching the parser.

## Page faults

`mmap` only reserves a range of virtual addresses. Page table entries are created lazily:
the first access to an unmapped address raises a CPU exception, and the kernel finds the corresponding
file page and maps it.

- **Minor fault** — the page is already in the page cache; the kernel only adds a page table entry.
- **Major fault** — the page is not in memory; the kernel reads it from disk while the thread waits.

**How many for this file.** 8,251,407,909 bytes / 4096 = **2,014,504 pages** — that is how many faults
there would be if pages were mapped one at a time. Measured (`/usr/bin/time`, see [benchmarks.md](benchmarks.md#end-to-end-from-file--framing-only-initial-measurement)):

| Run | Minor | Major | Explanation |
|---|---:|---:|---|
| Warm | ≈ 126,160 | 0 | 2,014,504 / 16 = 125,906: fault-around maps 64 KB (16 pages) per fault |
| Cold | ≈ 126,160 | ≈ 62,970 | 2,014,504 / 32 = 62,953: readahead reads 128 KB (32 pages) from disk at a time (`read_ahead_kb = 128`) |

So the kernel batches pages on its own, and there are 16 times fewer faults than pages. Possible next steps
to reduce them further: `MAP_POPULATE`, huge pages (2 MB), `madvise(MADV_WILLNEED)`.

## Cold vs. warm runs

- **Cold:** the file is not in the page cache, pages are read from disk. 10 s, 38 ns/message, 0.8 GB/s —
  time is dominated by the disk (the WSL2 virtual disk), not the code.
- **Warm:** the whole file is in the page cache. 1.8 s, 6.8 ns/message, 4.5 GB/s — time is determined
  by the CPU and memory, i.e. by the code itself.

**Which one to report.** Both, clearly labeled and with the methodology. For comparing code versions (before/after),
use the warm run: it measures the code rather than the disk, and it is stable (spread < 1 %). The cold run answers a different
question — how long it takes to process the whole file from disk — and depends heavily on the machine. In a production system
data arrives from the network into memory, so the warm run is closer to what the parser actually sees.

Pitfall: a warm run is only possible if the file fits in free memory. At the time of measurement
≈ 9.4 GiB was available for a 7.7 GiB file; with insufficient memory, the beginning of the file is evicted by the end of the pass
and a "warm" run partially becomes a cold one.

## `MADV_SEQUENTIAL`

No effect beyond noise: warm 6.81 vs. 6.75 ns/message, cold — medians of 10.2 and 9.9 s
with ±10 % spread, and the same number of major faults. The kernel detects sequential access
and enables readahead without the hint; the hint could help under memory pressure
(dropping already-read pages more aggressively), but here the file fits in the cache.

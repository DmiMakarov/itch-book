# Building and running

Quick reference for the current build configuration: `CMakeLists.txt` + `CMakePresets.json`.

## Commands

```bash
cmake --list-presets                 # list available presets

cmake --preset release               # configure → build/release/
cmake --build --preset release       # build
ctest --preset release               # run tests

./build/release/tests                # run tests directly (shows GoogleTest output)
./build/release/bench                # run benchmarks
```

Replace `release` with `debug` or `tsan` as needed. Each preset builds into its own directory `build/<preset>/`, so builds never mix.

If the compiler or preset flags changed and rebuilds behave oddly, delete the directory and configure again: `rm -rf build/debug && cmake --preset debug`. CMake caches the compiler and variables on the first configure.

## Presets

| Preset | Build type | Effective flags | Purpose |
|---|---|---|---|
| `debug` | `Debug` | `-O0 -g` + ASan + UBSan | Finding memory errors and UB. Tests and runs on the first 100 MB of the file |
| `release` | `RelWithDebInfo` | `-O2 -g -DNDEBUG` | All measurements and benchmarks. Published numbers come only from this preset |
| `tsan` | `Debug` | `-O1 -g` + TSan | Finding data races once threads are introduced |

ASan and TSan cannot be enabled in the same build, so `tsan` is a separate preset.

## Compiler flags

### Common to all presets (`CMakeLists.txt`)

| Flag / setting | Effect |
|---|---|
| `CMAKE_CXX_STANDARD 20` | `-std=c++20` |
| `CMAKE_CXX_STANDARD_REQUIRED ON` | If the compiler does not support C++20, fail instead of silently falling back to an older standard |
| `CMAKE_CXX_EXTENSIONS OFF` | `-std=c++20` rather than `-std=gnu++20`: no GCC extensions, so the code stays portable to Clang |
| `-Wall` | Main set of warnings |
| `-Wextra` | Additional warnings (unused parameters, signed/unsigned comparison, etc.) |
| `-Wpedantic` | Warn about anything that is not standard ISO C++ |
| `-Werror` | Treat every warning as a build error |

Warnings are set via `target_compile_options` on the `itch` target rather than in the presets: they are a project requirement, not a property of a build variant. Targets that link against `itch` (`tests`, `bench`, `main`) inherit them automatically.

### From the build type (`CMAKE_BUILD_TYPE`)

| Flag | Effect |
|---|---|
| `-O0` | No optimization. Code maps to the source line by line, convenient for debugging |
| `-O1` | Light optimization. Needed for TSan, which is too slow without it |
| `-O2` | Main optimization level for measurements |
| `-g` | Debug info: function names and line numbers in gdb, perf and sanitizer reports. Does not affect code speed |
| `-DNDEBUG` | Disables `assert`. Only in `release` |

`Debug` itself adds only `-g`; `-O0` / `-O1` come from the preset's `CMAKE_CXX_FLAGS`.

### Sanitizers (`CMAKE_CXX_FLAGS` in the preset)

| Flag | Effect |
|---|---|
| `-fsanitize=address` | ASan: out-of-bounds access (heap, stack, globals), use-after-free, double free, leaks. ~2× slower, ~3× memory |
| `-fsanitize=undefined` | UBSan: signed overflow, shifts by the type width or more, misaligned access, `nullptr`, invalid `bool`/`enum` values |
| `-fsanitize=thread` | TSan: data races between threads. 5–15× slower, 5–10× memory |
| `-fno-sanitize-recover=all` | The first UBSan error terminates the program. Without it, UBSan prints a report and keeps running, and the test stays green |
| `-fno-omit-frame-pointer` | Keeps the frame pointer, giving readable stack traces in sanitizer reports |

`CMAKE_CXX_FLAGS` is passed to both compilation and linking — a sanitizer needs both (its runtime library is linked in at link time).

ASan does not track `mmap`-ed memory: it will not catch a read past the end of a mapped file. Bounds must be checked explicitly when parsing the file.

### Other (`base`)

| Setting | Effect |
|---|---|
| `binaryDir: build/${presetName}` | Separate build directory per preset |
| `CMAKE_EXPORT_COMPILE_COMMANDS ON` | Generates `build/<preset>/compile_commands.json` — the exact compile command for each file. Required by clangd |
| `hidden: true` | `base` cannot be used on its own; it exists only to be inherited |

## Build targets

| Target | What it is | Links with |
|---|---|---|
| `itch` | Static library: `src/itch/` + headers from `include/` + warning flags | — |
| `tests` | GoogleTest tests | `itch`, `GTest::gtest_main` (no custom `main` needed) |
| `bench` | Google Benchmark benchmarks | `itch`, `benchmark::benchmark_main` (no custom `main` needed) |
| `main` | Command-line program that parses a full ITCH file | `itch` |

## Benchmarks

```bash
taskset -c 2 ./build/release/bench --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
```

| Option | Effect |
|---|---|
| `taskset -c 2` | Pin the process to core 2 so the scheduler does not migrate it between cores |
| `--benchmark_repetitions=10` | Repeat the measurement 10 times |
| `--benchmark_report_aggregates_only=true` | Print only `mean` / `median` / `stddev` |
| `--benchmark_filter=be48` | Run only benchmarks whose name matches the regular expression |

`release` only. Numbers under ASan are meaningless.

## Verifying the setup

- `build/debug/compile_commands.json` — commands include `-fsanitize=address,undefined`.
- A deliberate use-after-free in `debug` → `heap-use-after-free` report; in `release` it passes silently.
- Disassembly of a function: `objdump -d --no-show-raw-insn -C build/release/bench | less`, then search for the function name.

## Environment

- **clangd** takes its flags from `compile_commands.json`. Without it, clangd does not know about C++20 or the include paths and reports false errors. Point it to `build/<preset>/compile_commands.json` — via a symlink in the project root or `cmake.copyCompileCommands` in CMake Tools.
- **IntelliSense** in the Microsoft C/C++ extension should be disabled (`C_Cpp.intelliSenseEngine: disabled`) so it does not conflict with clangd. Do not uninstall the extension itself — it is needed for the debugger.
- **WSL2:** CPU frequency scaling cannot be disabled, so Google Benchmark prints a warning. This is a limitation of the environment; the CPU model is listed in [benchmarks.md](benchmarks.md).
- **TSan on WSL2** may fail with `unexpected memory mapping` — fix with `sudo sysctl vm.mmap_rnd_bits=28`.

## Known issues

- **`_GLIBCXX_ASSERTIONS` is not enabled in `debug`.** With it, `std::span`, `std::vector`, etc. check bounds and abort on out-of-range access. To enable, add `-D_GLIBCXX_ASSERTIONS` to the `CMAKE_CXX_FLAGS` of the `debug` preset.

# itch-book

Разбор дампа NASDAQ TotalView-ITCH 5.0 на C++20: файл отображается в память (`mmap`), записи
нарезаются по префиксу длины и проверяются по таблице типов из спецификации.

## Сборка и запуск

Нужны CMake ≥ 3.25, компилятор с C++20, GoogleTest и Google Benchmark
(`sudo apt install libbenchmark-dev libgtest-dev`).

```bash
cmake --preset release && cmake --build --preset release
ctest --preset release

./build/release/main data/12302019.NASDAQ_ITCH50                 # MADV_NORMAL
./build/release/main data/12302019.NASDAQ_ITCH50 --sequential    # MADV_SEQUENTIAL
./build/release/bench                                            # микробенчмарки
```

Пресеты (`debug` с ASan и UBSan, `release`, `tsan`) и флаги описаны в [docs/helper.md](docs/helper.md).

Данные: `12302019.NASDAQ_ITCH50.gz` с `https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/`,
распаковать в `data/` (каталог в `.gitignore`). Распакованный файл — 8 251 407 909 байт.

## Устройство

| Файл | Что делает |
|---|---|
| `include/itch/endian.hpp` | `be16` / `be32` / `be48` / `be64` из `std::span<const std::byte, N>` |
| `include/itch/mapped_file.hpp` | `MappedFile`: RAII над `mmap`, только перемещение, наружу `std::span<const std::byte>` |
| `include/itch/msg_type.hpp` | `enum class MsgType : char`, таблица типов и `constexpr`-таблица длин на 256 элементов |
| `include/itch/itch_reader.hpp` | `ItchReader::next()`: следующее проверенное сообщение или `FormatError` со смещением |
| `src/main.cpp` | Подсчёт сообщений по типам и замер времени |

Таблица «тип → длина» и формат записи: [docs/itch50_messages.md](docs/itch50_messages.md).

## Проверки корректности

- Длина каждого сообщения совпадает с табличной; неизвестный тип, обрыв записи, пустое сообщение,
  время назад — `FormatError` со смещением записи.
- `next()` возвращает «конец» только при смещении, равном размеру буфера: разобранные байты в сумме
  равны размеру файла.
- Время не убывает от сообщения к сообщению.
- Сверка с независимой реализацией на Python по первым 3 ГБ (97 526 600 сообщений): счётчики совпадают.
- Debug-сборка с ASan и UBSan на первых 99 999 985 байтах (≈ 100 МБ, ровно по границе записи) —
  3 409 528 сообщений, без замечаний санитайзеров.
- 27 тестов GoogleTest: `be*`, `MappedFile` (ошибки, перемещение), `ItchReader` (каждая ветка контракта).

## Типы сообщений за 30.12.2019

Всего **268 744 780** сообщений.

| Тип | Сообщение | Количество | Доля |
|---|---|---:|---:|
| `A` | Add Order | 117 145 568 | 43.590 % |
| `D` | Order Delete | 114 360 997 | 42.554 % |
| `U` | Order Replace | 21 639 067 | 8.052 % |
| `E` | Order Executed | 5 722 824 | 2.129 % |
| `I` | NOII | 4 024 315 | 1.497 % |
| `X` | Order Cancel | 2 787 676 | 1.037 % |
| `F` | Add Order MPID | 1 485 888 | 0.553 % |
| `P` | Trade | 1 218 602 | 0.453 % |
| `L` | Market Participant Position | 215 161 | 0.080 % |
| `C` | Order Executed With Price | 99 917 | 0.037 % |
| `Q` | Cross Trade | 17 836 | 0.007 % |
| `Y` | Reg SHO Indicator | 9 013 | 0.003 % |
| `H` | Stock Trading Action | 8 966 | 0.003 % |
| `R` | Stock Directory | 8 906 | 0.003 % |
| `J` | LULD Auction Collar | 34 | < 0.001 % |
| `S` | System Event | 6 | < 0.001 % |
| `K` | IPO Quoting Period Update | 3 | < 0.001 % |
| `V` | MWCB Decline Level | 1 | < 0.001 % |
| `W` `h` `B` `N` `O` | MWCB Status, Operational Halt, Broken Trade, RPII, DLCR | 0 | — |

6 сообщений `S` — полный торговый день: начало сообщений, системных часов, рынка и их закрытия.

## Baseline

Цикл `next()` + счётчик по типу, без разбора полей. Время — только цикл разбора (без `open`/`mmap`),
медиана трёх прогонов.

| Прогон | `madvise` | Время | нс / сообщение | Млн сообщений / с | ГБ / с |
|---|---|---:|---:|---:|---:|
| Холодный | `NORMAL` | 10.22 с | 38.0 | 26.3 | 0.81 |
| Холодный | `SEQUENTIAL` | 9.92 с | 36.9 | 27.1 | 0.83 |
| Тёплый | `NORMAL` | 1.83 с | 6.81 | 146.9 | 4.51 |
| Тёплый | `SEQUENTIAL` | 1.82 с | 6.75 | 148.1 | 4.55 |

Page fault'ы на прогон (`/usr/bin/time`): тёплый — ≈ 126 000 minor, холодный — ≈ 126 000 minor
и ≈ 63 000 major. Подробности и разбор чисел: [docs/notes/week1.md](docs/notes/week1.md).

**Среда:** AMD Ryzen 5 7500F (6 ядер / 12 потоков), WSL2 (ядро 5.15.167.4-microsoft-standard),
ext4 на виртуальном диске WSL, 15 ГиБ RAM, GCC 13.3, `-O2 -g` (`release`), процесс привязан к ядру 2
(`taskset -c 2`).

**Методика:** холодный прогон — перед запуском страницы файла выброшены из page cache через
`posix_fadvise(POSIX_FADV_DONTNEED)` (в WSL нет `sudo` для `drop_caches`; по `free` кэш падает
с 9.0 до 1.3 ГиБ). Тёплый — после одного прогрева, весь файл в page cache.

**Ограничения:** частотой процессора в WSL2 управляет Windows; разброс холодных прогонов ±10 %
(первый после сборки — 13.5 с, в медиану не влияет).

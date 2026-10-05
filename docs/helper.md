# Сборка и запуск

Шпаргалка по текущей конфигурации: `CMakeLists.txt` + `CMakePresets.json`.

## Команды

```bash
cmake --list-presets                 # какие пресеты есть

cmake --preset release               # конфигурация → build/release/
cmake --build --preset release       # сборка
ctest --preset release               # тесты

./build/release/tests                # тесты напрямую (видно вывод GoogleTest)
./build/release/bench                # бенчмарки
```

Вместо `release` — `debug` или `tsan`. Каждый пресет собирается в свой каталог `build/<пресет>/`, сборки не смешиваются.

Если поменялся компилятор или флаги в пресете, а пересборка ведёт себя странно, удалить каталог и сконфигурировать заново: `rm -rf build/debug && cmake --preset debug`. CMake кэширует компилятор и переменные при первой конфигурации.

## Пресеты

| Пресет | Тип сборки | Итоговые флаги | Для чего |
|---|---|---|---|
| `debug` | `Debug` | `-O0 -g` + ASan + UBSan | Поиск ошибок памяти и UB. Тесты и прогон на первых 100 МБ файла |
| `release` | `RelWithDebInfo` | `-O2 -g -DNDEBUG` | Все замеры и бенчмарки. Цифры в README только отсюда |
| `tsan` | `Debug` | `-O1 -g` + TSan | Поиск гонок данных, когда появятся потоки |

ASan и TSan нельзя включить в одной сборке, поэтому `tsan` — отдельный пресет.

## Флаги компиляции

### Общие для всех пресетов (`CMakeLists.txt`)

| Флаг / настройка | Что делает |
|---|---|
| `CMAKE_CXX_STANDARD 20` | `-std=c++20` |
| `CMAKE_CXX_STANDARD_REQUIRED ON` | Если компилятор не умеет C++20 — ошибка, а не тихий откат на старый стандарт |
| `CMAKE_CXX_EXTENSIONS OFF` | `-std=c++20`, а не `-std=gnu++20`: без расширений GCC, код переносим на Clang |
| `-Wall` | Основной набор предупреждений |
| `-Wextra` | Дополнительные предупреждения (неиспользуемые параметры, сравнение signed/unsigned и т.д.) |
| `-Wpedantic` | Предупреждать обо всём, что не по стандарту ISO C++ |
| `-Werror` | Любое предупреждение — ошибка сборки |

Предупреждения заданы в `target_compile_options` на цели `itch`, а не в пресетах: это требование проекта, а не варианта сборки. Цели, которые линкуются с `itch` (`tests`, `bench`), получают их автоматически.

### Из типа сборки (`CMAKE_BUILD_TYPE`)

| Флаг | Что делает |
|---|---|
| `-O0` | Без оптимизаций. Код соответствует исходнику строка в строку, удобно отлаживать |
| `-O1` | Лёгкие оптимизации. Для TSan: без них он слишком медленный |
| `-O2` | Основной уровень оптимизаций для замеров |
| `-g` | Отладочная информация: имена функций и номера строк в gdb, perf и отчётах санитайзеров. На скорость кода не влияет |
| `-DNDEBUG` | Отключает `assert`. Есть только в `release` |

`Debug` сам добавляет только `-g`; `-O0` / `-O1` приходят из `CMAKE_CXX_FLAGS` пресета.

### Санитайзеры (`CMAKE_CXX_FLAGS` в пресете)

| Флаг | Что делает |
|---|---|
| `-fsanitize=address` | ASan: выход за границы (куча, стек, глобальные), use-after-free, double free, утечки. Медленнее ~2×, памяти ~3× |
| `-fsanitize=undefined` | UBSan: переполнение знаковых, сдвиги на ширину типа и больше, невыровненный доступ, `nullptr`, недопустимые `bool`/`enum` |
| `-fsanitize=thread` | TSan: гонки данных между потоками. Медленнее в 5–15×, памяти в 5–10× |
| `-fno-sanitize-recover=all` | Первая же ошибка UBSan завершает программу. Без него UBSan печатает отчёт и работает дальше, и тест остаётся зелёным |
| `-fno-omit-frame-pointer` | Сохраняет указатель кадра стека — читаемые стектрейсы в отчётах санитайзеров |

`CMAKE_CXX_FLAGS` попадает и в компиляцию, и в линковку — санитайзеру нужно и то, и другое (на линковке подключается его runtime-библиотека).

ASan не видит память из `mmap`: чтение за концом отображённого файла он не поймает. Границы при разборе файла нужно проверять явно.

### Прочее (`base`)

| Настройка | Что делает |
|---|---|
| `binaryDir: build/${presetName}` | Отдельный каталог сборки на каждый пресет |
| `CMAKE_EXPORT_COMPILE_COMMANDS ON` | Генерирует `build/<пресет>/compile_commands.json` — точные команды компиляции каждого файла. Нужен clangd |
| `hidden: true` | `base` нельзя запустить сам по себе, он только для наследования |

## Цели сборки

| Цель | Что это | Линкуется с |
|---|---|---|
| `itch` | INTERFACE-библиотека: только заголовки из `include/` + флаги предупреждений | — |
| `tests` | Тесты GoogleTest | `itch`, `GTest::gtest_main` (свой `main` не нужен) |
| `bench` | Бенчмарки Google Benchmark | `itch`, `benchmark::benchmark_main` (свой `main` не нужен) |

## Бенчмарки

```bash
taskset -c 2 ./build/release/bench --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
```

| Опция | Что делает |
|---|---|
| `taskset -c 2` | Привязать процесс к ядру 2, чтобы планировщик не перекидывал его между ядрами |
| `--benchmark_repetitions=10` | Повторить замер 10 раз |
| `--benchmark_report_aggregates_only=true` | Печатать только `mean` / `median` / `stddev` |
| `--benchmark_filter=be48` | Запустить только бенчмарки, имя которых совпадает с регулярным выражением |

Только `release`. Под ASan цифры бессмысленны.

## Проверка, что всё настроено

- `build/debug/compile_commands.json` — у команд есть `-fsanitize=address,undefined`.
- Намеренный use-after-free в `debug` → отчёт `heap-use-after-free`; в `release` молча проходит.
- Ассемблер функции: `objdump -d --no-show-raw-insn -C build/release/bench | less`, поиск по имени функции.

## Окружение

- **clangd** берёт флаги из `compile_commands.json`. Без него он не знает про C++20 и пути к заголовкам и показывает ложные ошибки. Нужно дать ему путь к `build/<пресет>/compile_commands.json` — симлинком в корень проекта или `cmake.copyCompileCommands` в CMake Tools.
- **IntelliSense** расширения Microsoft C/C++ надо отключить (`C_Cpp.intelliSenseEngine: disabled`), чтобы не спорил с clangd. Само расширение не удалять — оно нужно для отладчика.
- **WSL2:** изменение частоты процессора не отключить — Google Benchmark будет предупреждать. Записать в README как ограничение среды вместе с моделью процессора.
- **TSan в WSL2** может упасть с `unexpected memory mapping` — лечится `sudo sysctl vm.mmap_rnd_bits=28`.

## Не настроено / известные проблемы

- **Пути к исходникам в `CMakeLists.txt` не совпадают с файлами.** Там `tests/be_test.cpp` и `bench/be_bench.cpp`, а на диске `tests/itch/test_endian.cpp` и `bench/itch/bench_endian.cpp`. Пока не поправить, `cmake --preset ...` падает.
- **`_GLIBCXX_ASSERTIONS` в `debug` не включён.** С ним `std::span`, `std::vector` и др. проверяют границы и падают при выходе за них. Добавить `-D_GLIBCXX_ASSERTIONS` в `CMAKE_CXX_FLAGS` пресета `debug`.

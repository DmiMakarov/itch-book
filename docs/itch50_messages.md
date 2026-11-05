# ITCH 5.0: типы сообщений и их длины

Длина указана **без** 2-байтового префикса длины из файла (BinaryFILE): в цикле разбора сдвиг на `2 + длина`.
Длина = Offset последнего поля + его Length в таблице формата сообщения (раздел 4 спецификации).

Общий заголовок у всех сообщений — первые 11 байт:

| Поле | Offset | Length |
|---|---|---|
| Message Type | 0 | 1 |
| Stock Locate | 1 | 2 |
| Tracking Number | 3 | 2 |
| Timestamp (нс от полуночи) | 5 | 6 |

## Таблица «тип → длина»

| Тип | Hex | Сообщение | Длина |
|---|---|---|---|
| **Системные** | | | |
| `S` | `0x53` | System Event | 12 |
| **По инструменту** | | | |
| `R` | `0x52` | Stock Directory | 39 |
| `H` | `0x48` | Stock Trading Action | 25 |
| `Y` | `0x59` | Reg SHO Short Sale Price Test Restricted Indicator | 20 |
| `L` | `0x4C` | Market Participant Position | 26 |
| `V` | `0x56` | MWCB Decline Level | 35 |
| `W` | `0x57` | MWCB Status | 12 |
| `K` | `0x4B` | IPO Quoting Period Update | 28 |
| `J` | `0x4A` | LULD Auction Collar | 35 |
| `h` | `0x68` | Operational Halt | 21 |
| **Добавление заявки** | | | |
| `A` | `0x41` | Add Order (No MPID Attribution) | 36 |
| `F` | `0x46` | Add Order with MPID Attribution | 40 |
| **Изменение заявки** | | | |
| `E` | `0x45` | Order Executed | 31 |
| `C` | `0x43` | Order Executed With Price | 36 |
| `X` | `0x58` | Order Cancel | 23 |
| `D` | `0x44` | Order Delete | 19 |
| `U` | `0x55` | Order Replace | 35 |
| **Сделки** | | | |
| `P` | `0x50` | Trade (Non-Cross) | 44 |
| `Q` | `0x51` | Cross Trade | 40 |
| `B` | `0x42` | Broken Trade | 19 |
| **Аукционы** | | | |
| `I` | `0x49` | Net Order Imbalance Indicator (NOII) | 50 |
| `N` | `0x4E` | Retail Price Improvement Indicator (RPII) | 20 |
| `O` | `0x4F` | Direct Listing with Capital Raise Price Discovery | 48 |

`H` и `h` — разные сообщения: регистр важен.

`O` добавлен в спецификацию позже, чем записан файл `12302019.NASDAQ_ITCH50`; в нём этот тип не ожидается.

## Сверка с данными

Первые 3 ГБ файла `12302019.NASDAQ_ITCH50` (≈ 99 млн сообщений): у каждого встреченного типа
все длины совпадают с таблицей. Встретились `S R H Y L V K J A F E C X D U P Q I`;
`W h B N` в этом диапазоне не попались — их длины взяты только из спецификации.

## Поля по типам

Во всех таблицах ниже пропущен общий заголовок (offset 0–10), поля начинаются с offset 11.
Последняя строка каждой таблицы: Offset + Length = длина сообщения из таблицы выше.

Списки значений сверены с официальной спецификацией (NQTVITCHspecification.pdf, редакция 2024) и с фактическими
значениями по всему файлу `12302019.NASDAQ_ITCH50`: всё, что встречается в файле, входит в списки спецификации.

Типы полей (раздел 1 спецификации):

| Тип | Что это |
|---|---|
| Integer | беззнаковое целое, big-endian (2, 4, 6 или 8 байт) |
| Price(4) | Integer 4 байта, фиксированная точка, 4 знака после запятой: `1234500` = 123.45 |
| Price(8) | Integer 8 байт, фиксированная точка, 8 знаков после запятой |
| Alpha | ASCII, выровнено влево, дополнено пробелами справа (тикер `AAPL` = `"AAPL    "`) |

У системных сообщений (`S`, `V`, `W`) Stock Locate = 0: они не относятся к инструменту.

### `S` System Event — 12

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Event Code | 11 | 1 | Alpha | `O` начало сообщений, `S` начало системных часов, `Q` начало рыночных часов, `M` конец рыночных часов, `E` конец системных часов, `C` конец сообщений |

### `R` Stock Directory — 39

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Market Category | 19 | 1 | Alpha | `Q` Global Select, `G` Global Market, `S` Capital Market, `N` NYSE, `A` NYSE American, `P` NYSE Arca, `Z` BATS, `V` IEX, пробел — нет данных |
| Financial Status Indicator | 20 | 1 | Alpha | `N` норма; `D` deficient, `E` delinquent, `Q` bankrupt, `S` suspended, `G` D+Q, `H` D+E, `J` E+Q, `K` D+E+Q; `C` у ETP приостановлены creations/redemptions; пробел — не Nasdaq-листинг |
| Round Lot Size | 21 | 4 | Integer | обычно 100 |
| Round Lots Only | 25 | 1 | Alpha | `Y` / `N` |
| Issue Classification | 26 | 1 | Alpha | приложение D: `A` ADS, `B` bond, `C` common stock, `F` depository receipt, `I` 144A, `L` LP, `N` notes, `O` ordinary share, `P` preferred, `Q` other, `R` right, `S` shares of beneficial interest, `T` convertible debenture, `U` unit, `V` units/benif int, `W` warrant |
| Issue Sub-Type | 27 | 2 | Alpha | приложение E: ~50 кодов из 1–2 символов (`A `, `AI`, `B `, `C `, `CB`, ..., `X `, `Y `, `Z `) |
| Authenticity | 29 | 1 | Alpha | `P` live/production, `T` тест |
| Short Sale Threshold Indicator | 30 | 1 | Alpha | `Y` / `N` / пробел |
| IPO Flag | 31 | 1 | Alpha | `Y` / `N` / пробел |
| LULD Reference Price Tier | 32 | 1 | Alpha | `1` / `2` / пробел |
| ETP Flag | 33 | 1 | Alpha | `Y` / `N` / пробел |
| ETP Leverage Factor | 34 | 4 | Integer | |
| Inverse Indicator | 38 | 1 | Alpha | `Y` / `N` |

### `H` Stock Trading Action — 25

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Trading State | 19 | 1 | Alpha | `H` halted, `P` paused, `Q` quotation only, `T` trading |
| Reserved | 20 | 1 | Alpha | |
| Reason | 21 | 4 | Alpha | приложение C: `T1`, `T2`, `T5`, `T6`, `T8`, `T12`, `H4`, `H9`, `H10`, `H11`, `O1`, `LUDP`, `LUDS`, `MWC0`–`MWC3`, `IPO1`, `M1`, `M2`, `T3`, `T7`, `R4`, `R9`, `C3`, `C4`, `C9`, `C11`, `MWCQ`, `R1`, `R2`, `IPOQ`, `IPOE`; пробелы — причина не указана |

### `Y` Reg SHO Short Sale Price Test Restricted Indicator — 20

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Reg SHO Action | 19 | 1 | Alpha | `0` ограничения нет, `1` введено (падение цены), `2` действует |

### `L` Market Participant Position — 26

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| MPID | 11 | 4 | Alpha | идентификатор участника |
| Stock | 15 | 8 | Alpha | |
| Primary Market Maker | 23 | 1 | Alpha | `Y` / `N` |
| Market Maker Mode | 24 | 1 | Alpha | `N` normal, `P` passive, `S` syndicate, `R` pre-syndicate, `L` penalty |
| Market Participant State | 25 | 1 | Alpha | `A` active, `E` excused/withdrawn, `W` withdrawn, `S` suspended, `D` deleted |

Внимание: здесь Stock стоит после MPID, а не сразу после заголовка.

### `V` MWCB Decline Level — 35

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Level 1 | 11 | 8 | Price(8) | |
| Level 2 | 19 | 8 | Price(8) | |
| Level 3 | 27 | 8 | Price(8) | |

### `W` MWCB Status — 12

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Breached Level | 11 | 1 | Alpha | `1` / `2` / `3` |

### `K` IPO Quoting Period Update — 28

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| IPO Quotation Release Time | 19 | 4 | Integer | секунды от полуночи (не наносекунды!) |
| IPO Quotation Release Qualifier | 23 | 1 | Alpha | `A` ожидаемое время, `C` отменено/отложено |
| IPO Price | 24 | 4 | Price(4) | |

### `J` LULD Auction Collar — 35

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Auction Collar Reference Price | 19 | 4 | Price(4) | |
| Upper Auction Collar Price | 23 | 4 | Price(4) | |
| Lower Auction Collar Price | 27 | 4 | Price(4) | |
| Auction Collar Extension | 31 | 4 | Integer | |

### `h` Operational Halt — 21

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Market Code | 19 | 1 | Alpha | `Q` Nasdaq, `B` BX, `X` PSX |
| Operational Halt Action | 20 | 1 | Alpha | `H` остановлено, `T` торговля возобновлена |

### `A` Add Order (No MPID Attribution) — 36

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | уникален в пределах дня |
| Buy/Sell Indicator | 19 | 1 | Alpha | `B` покупка, `S` продажа |
| Shares | 20 | 4 | Integer | |
| Stock | 24 | 8 | Alpha | |
| Price | 32 | 4 | Price(4) | |

### `F` Add Order with MPID Attribution — 40

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | |
| Buy/Sell Indicator | 19 | 1 | Alpha | `B` / `S` |
| Shares | 20 | 4 | Integer | |
| Stock | 24 | 8 | Alpha | |
| Price | 32 | 4 | Price(4) | |
| Attribution | 36 | 4 | Alpha | MPID участника |

Первые 36 байт совпадают с `A`.

### `E` Order Executed — 31

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | |
| Executed Shares | 19 | 4 | Integer | |
| Match Number | 23 | 8 | Integer | |

Цена исполнения — цена заявки из `A`/`F`: её нужно помнить в стакане.

### `C` Order Executed With Price — 36

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | |
| Executed Shares | 19 | 4 | Integer | |
| Match Number | 23 | 8 | Integer | |
| Printable | 31 | 1 | Alpha | `Y` / `N` — учитывать ли в объёме и ленте сделок |
| Execution Price | 32 | 4 | Price(4) | может отличаться от цены заявки |

### `X` Order Cancel — 23

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | |
| Cancelled Shares | 19 | 4 | Integer | частичная отмена: заявка уменьшается, но остаётся |

### `D` Order Delete — 19

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | заявка удаляется целиком |

### `U` Order Replace — 35

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Original Order Reference Number | 11 | 8 | Integer | эта заявка удаляется |
| New Order Reference Number | 19 | 8 | Integer | новая заявка: та же сторона и тикер, новые цена и объём |
| Shares | 27 | 4 | Integer | |
| Price | 31 | 4 | Price(4) | |

Сторона и тикер в сообщении отсутствуют — берутся из исходной заявки.

### `P` Trade (Non-Cross) — 44

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | скрытая заявка, в стакане её нет |
| Buy/Sell Indicator | 19 | 1 | Alpha | `B` / `S`; с 2014 года всегда `B` |
| Shares | 20 | 4 | Integer | |
| Stock | 24 | 8 | Alpha | |
| Price | 32 | 4 | Price(4) | |
| Match Number | 36 | 8 | Integer | |

Стакан не меняет.

### `Q` Cross Trade — 40

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Shares | 11 | 8 | Integer | **8 байт**, не 4 |
| Stock | 19 | 8 | Alpha | |
| Cross Price | 27 | 4 | Price(4) | |
| Match Number | 31 | 8 | Integer | |
| Cross Type | 39 | 1 | Alpha | `O` открытие, `C` закрытие, `H` IPO/halt |

### `B` Broken Trade — 19

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Match Number | 11 | 8 | Integer | сделка с этим номером отменена |

### `I` Net Order Imbalance Indicator (NOII) — 50

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Paired Shares | 11 | 8 | Integer | |
| Imbalance Shares | 19 | 8 | Integer | |
| Imbalance Direction | 27 | 1 | Alpha | `B` покупка, `S` продажа, `N` нет дисбаланса, `O` недостаточно заявок, `P` paused (с 2023) |
| Stock | 28 | 8 | Alpha | |
| Far Price | 36 | 4 | Price(4) | |
| Near Price | 40 | 4 | Price(4) | |
| Current Reference Price | 44 | 4 | Price(4) | |
| Cross Type | 48 | 1 | Alpha | `O` открытие, `C` закрытие, `H` IPO/halt, `A` extended trading close (с 2022) |
| Price Variation Indicator | 49 | 1 | Alpha | `L` < 1%, `1`–`9`, `A`, `B`, `C` — диапазоны отклонения, пробел — не рассчитано |

Stock стоит не сразу после заголовка, а на offset 28.

### `N` Retail Price Improvement Indicator (RPII) — 20

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Interest Flag | 19 | 1 | Alpha | `B` интерес на покупку, `S` на продажу, `A` обе стороны, `N` нет |

### `O` Direct Listing with Capital Raise Price Discovery — 48

| Поле | Offset | Length | Тип | Значения |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Open Eligibility Status | 19 | 1 | Alpha | `N` / `Y` |
| Minimum Allowable Price | 20 | 4 | Price(4) | |
| Maximum Allowable Price | 24 | 4 | Price(4) | |
| Near Execution Price | 28 | 4 | Price(4) | |
| Near Execution Time | 32 | 8 | Integer | наносекунды от полуночи |
| Lower Price Range Collar | 40 | 4 | Price(4) | |
| Upper Price Range Collar | 44 | 4 | Price(4) | |


1. Тесты reader (tests/itch/test_itch_reader.cpp) на маленьких буферах, собранных вручную, по одному тесту на каждую ветку контракта: корректная последовательность из двух-трёх сообщений, пустой буфер, обрыв в префиксе, обрыв в сообщении, len == 0, неизвестный тип, несовпадение длины, время назад. В тестах заодно проверяй e.offset().
2. Программа подсчёта (например, src/main.cpp и отдельный add_executable в CMake): путь к файлу из аргументов, MappedFile → ItchReader → счётчики в std::array<std::uint64_t, 256>, засечь время. Вывод: таблица «тип — количество — доля» (названия из msg_infos), всего сообщений, сообщений в секунду, нс на сообщение.
3. Оставшиеся проверки плана в программе: первое сообщение S с кодом события 'O' (байт 11 в msg.bytes). Проверка «разобрано ровно столько байт, сколько в файле» уже следует из контракта: чистый конец бывает только при offset == size.
4. Сверка с Python на первом миллионе сообщений: твой собственный короткий скрипт со struct.unpack('>H', ...), числа должны совпасть.
5. ASan и UBSan: debug-пресет на срезе в 99 999 985 байт.
6. Замеры: AccessPattern::Normal и Sequential, холодный и тёплый прогон. Для холодного в WSL кэш сбрасывается командой sync; echo 3 | sudo tee /proc/sys/vm/drop_caches. Помни, что весь файл в доступные ~5.5 ГБ не поместится, мы это обсуждали.
7. README и заметки: команда сборки и запуска, таблица типов, baseline, модель процессора; ответы на три вопроса из плана.

Пункты 2–4 это главная самостоятельная часть недели, так что код программы подсчёта я писать не буду, если не попросишь, а помогу разбором, как раньше.
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

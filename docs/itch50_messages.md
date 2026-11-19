# ITCH 5.0: message types and lengths

Lengths are given **without** the 2-byte length prefix used in the file (BinaryFILE): the parsing loop advances by `2 + length`.
Length = Offset of the last field + its Length in the message format table (section 4 of the specification).

All messages share a common 11-byte header:

| Field | Offset | Length |
|---|---|---|
| Message Type | 0 | 1 |
| Stock Locate | 1 | 2 |
| Tracking Number | 3 | 2 |
| Timestamp (ns since midnight) | 5 | 6 |

## Type → length table

| Type | Hex | Message | Length |
|---|---|---|---|
| **System** | | | |
| `S` | `0x53` | System Event | 12 |
| **Stock-related** | | | |
| `R` | `0x52` | Stock Directory | 39 |
| `H` | `0x48` | Stock Trading Action | 25 |
| `Y` | `0x59` | Reg SHO Short Sale Price Test Restricted Indicator | 20 |
| `L` | `0x4C` | Market Participant Position | 26 |
| `V` | `0x56` | MWCB Decline Level | 35 |
| `W` | `0x57` | MWCB Status | 12 |
| `K` | `0x4B` | IPO Quoting Period Update | 28 |
| `J` | `0x4A` | LULD Auction Collar | 35 |
| `h` | `0x68` | Operational Halt | 21 |
| **Add Order** | | | |
| `A` | `0x41` | Add Order (No MPID Attribution) | 36 |
| `F` | `0x46` | Add Order with MPID Attribution | 40 |
| **Modify Order** | | | |
| `E` | `0x45` | Order Executed | 31 |
| `C` | `0x43` | Order Executed With Price | 36 |
| `X` | `0x58` | Order Cancel | 23 |
| `D` | `0x44` | Order Delete | 19 |
| `U` | `0x55` | Order Replace | 35 |
| **Trades** | | | |
| `P` | `0x50` | Trade (Non-Cross) | 44 |
| `Q` | `0x51` | Cross Trade | 40 |
| `B` | `0x42` | Broken Trade | 19 |
| **Auctions** | | | |
| `I` | `0x49` | Net Order Imbalance Indicator (NOII) | 50 |
| `N` | `0x4E` | Retail Price Improvement Indicator (RPII) | 20 |
| `O` | `0x4F` | Direct Listing with Capital Raise Price Discovery | 48 |

`H` and `h` are different messages: case matters.

`O` was added to the specification after the `12302019.NASDAQ_ITCH50` file was recorded; this type is not expected in it.

## Validation against real data

First 3 GB of `12302019.NASDAQ_ITCH50` (≈ 99 million messages): for every type encountered,
all lengths match the table. Types seen: `S R H Y L V K J A F E C X D U P Q I`;
`W h B N` did not occur in this range — their lengths are taken from the specification only.

## Fields by message type

All tables below omit the common header (offsets 0–10); fields start at offset 11.
In the last row of each table, Offset + Length = the message length from the table above.

Value lists were checked against the official specification (NQTVITCHspecification.pdf, 2024 revision) and against the
actual values in the entire `12302019.NASDAQ_ITCH50` file: every value found in the file is in the specification's lists.

Field types (section 1 of the specification):

| Type | Meaning |
|---|---|
| Integer | unsigned integer, big-endian (2, 4, 6 or 8 bytes) |
| Price(4) | 4-byte Integer, fixed point with 4 decimal places: `1234500` = 123.45 |
| Price(8) | 8-byte Integer, fixed point with 8 decimal places |
| Alpha | ASCII, left-justified, padded with spaces on the right (ticker `AAPL` = `"AAPL    "`) |

System-wide messages (`S`, `V`, `W`) have Stock Locate = 0: they do not refer to a specific security.

### `S` System Event — 12

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Event Code | 11 | 1 | Alpha | `O` start of messages, `S` start of system hours, `Q` start of market hours, `M` end of market hours, `E` end of system hours, `C` end of messages |

### `R` Stock Directory — 39

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Market Category | 19 | 1 | Alpha | `Q` Global Select, `G` Global Market, `S` Capital Market, `N` NYSE, `A` NYSE American, `P` NYSE Arca, `Z` BATS, `V` IEX, space — not available |
| Financial Status Indicator | 20 | 1 | Alpha | `N` normal; `D` deficient, `E` delinquent, `Q` bankrupt, `S` suspended, `G` D+Q, `H` D+E, `J` E+Q, `K` D+E+Q; `C` ETP creations/redemptions suspended; space — not a Nasdaq-listed security |
| Round Lot Size | 21 | 4 | Integer | usually 100 |
| Round Lots Only | 25 | 1 | Alpha | `Y` / `N` |
| Issue Classification | 26 | 1 | Alpha | Appendix D: `A` ADS, `B` bond, `C` common stock, `F` depository receipt, `I` 144A, `L` LP, `N` notes, `O` ordinary share, `P` preferred, `Q` other, `R` right, `S` shares of beneficial interest, `T` convertible debenture, `U` unit, `V` units/benif int, `W` warrant |
| Issue Sub-Type | 27 | 2 | Alpha | Appendix E: ~50 codes of 1–2 characters (`A `, `AI`, `B `, `C `, `CB`, ..., `X `, `Y `, `Z `) |
| Authenticity | 29 | 1 | Alpha | `P` live/production, `T` test |
| Short Sale Threshold Indicator | 30 | 1 | Alpha | `Y` / `N` / space |
| IPO Flag | 31 | 1 | Alpha | `Y` / `N` / space |
| LULD Reference Price Tier | 32 | 1 | Alpha | `1` / `2` / space |
| ETP Flag | 33 | 1 | Alpha | `Y` / `N` / space |
| ETP Leverage Factor | 34 | 4 | Integer | |
| Inverse Indicator | 38 | 1 | Alpha | `Y` / `N` |

### `H` Stock Trading Action — 25

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Trading State | 19 | 1 | Alpha | `H` halted, `P` paused, `Q` quotation only, `T` trading |
| Reserved | 20 | 1 | Alpha | |
| Reason | 21 | 4 | Alpha | Appendix C: `T1`, `T2`, `T5`, `T6`, `T8`, `T12`, `H4`, `H9`, `H10`, `H11`, `O1`, `LUDP`, `LUDS`, `MWC0`–`MWC3`, `IPO1`, `M1`, `M2`, `T3`, `T7`, `R4`, `R9`, `C3`, `C4`, `C9`, `C11`, `MWCQ`, `R1`, `R2`, `IPOQ`, `IPOE`; spaces — reason not specified |

### `Y` Reg SHO Short Sale Price Test Restricted Indicator — 20

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Reg SHO Action | 19 | 1 | Alpha | `0` no price test in place, `1` restriction in effect due to an intraday price drop, `2` restriction remains in effect |

### `L` Market Participant Position — 26

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| MPID | 11 | 4 | Alpha | market participant identifier |
| Stock | 15 | 8 | Alpha | |
| Primary Market Maker | 23 | 1 | Alpha | `Y` / `N` |
| Market Maker Mode | 24 | 1 | Alpha | `N` normal, `P` passive, `S` syndicate, `R` pre-syndicate, `L` penalty |
| Market Participant State | 25 | 1 | Alpha | `A` active, `E` excused/withdrawn, `W` withdrawn, `S` suspended, `D` deleted |

Note: here Stock comes after MPID, not immediately after the header.

### `V` MWCB Decline Level — 35

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Level 1 | 11 | 8 | Price(8) | |
| Level 2 | 19 | 8 | Price(8) | |
| Level 3 | 27 | 8 | Price(8) | |

### `W` MWCB Status — 12

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Breached Level | 11 | 1 | Alpha | `1` / `2` / `3` |

### `K` IPO Quoting Period Update — 28

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| IPO Quotation Release Time | 19 | 4 | Integer | seconds since midnight (not nanoseconds!) |
| IPO Quotation Release Qualifier | 23 | 1 | Alpha | `A` anticipated release time, `C` canceled/postponed |
| IPO Price | 24 | 4 | Price(4) | |

### `J` LULD Auction Collar — 35

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Auction Collar Reference Price | 19 | 4 | Price(4) | |
| Upper Auction Collar Price | 23 | 4 | Price(4) | |
| Lower Auction Collar Price | 27 | 4 | Price(4) | |
| Auction Collar Extension | 31 | 4 | Integer | |

### `h` Operational Halt — 21

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Market Code | 19 | 1 | Alpha | `Q` Nasdaq, `B` BX, `X` PSX |
| Operational Halt Action | 20 | 1 | Alpha | `H` operationally halted, `T` trading resumed |

### `A` Add Order (No MPID Attribution) — 36

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | unique within the trading day |
| Buy/Sell Indicator | 19 | 1 | Alpha | `B` buy, `S` sell |
| Shares | 20 | 4 | Integer | |
| Stock | 24 | 8 | Alpha | |
| Price | 32 | 4 | Price(4) | |

### `F` Add Order with MPID Attribution — 40

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | |
| Buy/Sell Indicator | 19 | 1 | Alpha | `B` / `S` |
| Shares | 20 | 4 | Integer | |
| Stock | 24 | 8 | Alpha | |
| Price | 32 | 4 | Price(4) | |
| Attribution | 36 | 4 | Alpha | participant MPID |

The first 36 bytes are identical to `A`.

### `E` Order Executed — 31

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | |
| Executed Shares | 19 | 4 | Integer | |
| Match Number | 23 | 8 | Integer | |

The execution price is the order price from `A`/`F`, so the order book must keep it.

### `C` Order Executed With Price — 36

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | |
| Executed Shares | 19 | 4 | Integer | |
| Match Number | 23 | 8 | Integer | |
| Printable | 31 | 1 | Alpha | `Y` / `N` — whether to include in volume and the time-and-sales display |
| Execution Price | 32 | 4 | Price(4) | may differ from the order price |

### `X` Order Cancel — 23

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | |
| Cancelled Shares | 19 | 4 | Integer | partial cancellation: the order is reduced but stays on the book |

### `D` Order Delete — 19

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | the entire order is removed |

### `U` Order Replace — 35

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Original Order Reference Number | 11 | 8 | Integer | this order is removed |
| New Order Reference Number | 19 | 8 | Integer | new order: same side and stock, new price and size |
| Shares | 27 | 4 | Integer | |
| Price | 31 | 4 | Price(4) | |

The message carries no side or stock; they are taken from the original order.

### `P` Trade (Non-Cross) — 44

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Order Reference Number | 11 | 8 | Integer | non-displayed order, not on the book |
| Buy/Sell Indicator | 19 | 1 | Alpha | `B` / `S`; always `B` since 2014 |
| Shares | 20 | 4 | Integer | |
| Stock | 24 | 8 | Alpha | |
| Price | 32 | 4 | Price(4) | |
| Match Number | 36 | 8 | Integer | |

Does not change the order book.

### `Q` Cross Trade — 40

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Shares | 11 | 8 | Integer | **8 bytes**, not 4 |
| Stock | 19 | 8 | Alpha | |
| Cross Price | 27 | 4 | Price(4) | |
| Match Number | 31 | 8 | Integer | |
| Cross Type | 39 | 1 | Alpha | `O` opening, `C` closing, `H` IPO/halt |

### `B` Broken Trade — 19

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Match Number | 11 | 8 | Integer | the trade with this match number is broken |

### `I` Net Order Imbalance Indicator (NOII) — 50

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Paired Shares | 11 | 8 | Integer | |
| Imbalance Shares | 19 | 8 | Integer | |
| Imbalance Direction | 27 | 1 | Alpha | `B` buy imbalance, `S` sell imbalance, `N` no imbalance, `O` insufficient orders to calculate, `P` paused (since 2023) |
| Stock | 28 | 8 | Alpha | |
| Far Price | 36 | 4 | Price(4) | |
| Near Price | 40 | 4 | Price(4) | |
| Current Reference Price | 44 | 4 | Price(4) | |
| Cross Type | 48 | 1 | Alpha | `O` opening, `C` closing, `H` IPO/halt, `A` extended trading close (since 2022) |
| Price Variation Indicator | 49 | 1 | Alpha | `L` < 1%, `1`–`9`, `A`, `B`, `C` — deviation ranges, space — not calculated |

Stock is not right after the header but at offset 28.

### `N` Retail Price Improvement Indicator (RPII) — 20

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Interest Flag | 19 | 1 | Alpha | `B` RPI interest on the buy side, `S` on the sell side, `A` on both sides, `N` none |

### `O` Direct Listing with Capital Raise Price Discovery — 48

| Field | Offset | Length | Type | Values |
|---|---|---|---|---|
| Stock | 11 | 8 | Alpha | |
| Open Eligibility Status | 19 | 1 | Alpha | `N` / `Y` |
| Minimum Allowable Price | 20 | 4 | Price(4) | |
| Maximum Allowable Price | 24 | 4 | Price(4) | |
| Near Execution Price | 28 | 4 | Price(4) | |
| Near Execution Time | 32 | 8 | Integer | nanoseconds since midnight |
| Lower Price Range Collar | 40 | 4 | Price(4) | |
| Upper Price Range Collar | 44 | 4 | Price(4) | |

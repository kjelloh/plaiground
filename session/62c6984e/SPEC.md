# cratchit — specification

Living document. Edit freely; mark anything uncertain with **(?)**.

## 1. Goals

cratchit shall eventually be able to:

1. Produce a VAT return (Momsdeklaration) as an eskd file to send to SKV. **← current focus**
2. Close the books for a financial year (Bokslut).
3. Produce the annual report (Årsredovisning).
4. Produce the income tax return (Inkomstdeklaration 2, INK2).

## 2. Company facts

- Legal form: aktiebolag
- Financial year: From SIE-file / app setting (1 May – 30 April)
- VAT reporting period: From app setting - quarterly (Period = last month of quarter, e.g. `202409`)
- Current bookkeeping software: Fortnox (source of SIE exports)
- Use app settings for data required to amend / complete VAT returns report
  - Settings file: `.cratchit/settings` (plain text, one `name = value` per line)
  - Org number is read from SIE `#ORGNR`, not stored in settings

## 2b. User interface

- Command line tool, e.g. `cratchit vat --period 202409`
  (reads `_sie_in/`, writes the eskd file to `_to_skv/`)

## 3. Inputs and outputs

| Kind | Format | Where | Notes |
|---|---|---|---|
| Books | SIE type 4 | `_sie_in/` | Fortnox export, `#FORMAT PC8` (CP437), CRLF line endings, one file per financial year (2016/17 – 2026/27) |
| Bank statements: To bee defined later.
| SKV tax account statements: To be defined later
| Vouchers / receipts: To be defined later
| VAT return | eSKDUpload 6.0 XML, ISO-8859-1 | `_to_skv/` | See `_docs/README.md` |
| Updated books | SIE type 4 | `_sie_out/` | |

## 4. Milestone 1 — reproduce filed VAT returns

**Goal:** given a SIE file and a VAT period, cratchit generates an eskd file
equal to the one that was actually filed.

### Acceptance tests (local, real data)

| Period | SIE file (FY) | Expected file in `_eskd_irl/` |
|---|---|---|
| 202409 | 2024-05-01 – 2025-04-30 | `eskd_…_20241106_1717.eskd` |
| 202412 | 2024-05-01 – 2025-04-30 | `eskd_…_20250211_1804.eskd` |
| 202503 | 2024-05-01 – 2025-04-30 | `eskd_…_20250510_1842.eskd` |

"Equal" means: Same reported values (After SKV rounding rules applied)

### Rules (known so far)

- Amounts in the eskd file are whole kronor. Whole SEK provided as defined by SKV.
  - Observed: 2025Q1 input VAT 1997.50 was reported as 1997.
- `MomsBetala` (ruta 49) = output VAT − input VAT; negative means refund.
- Boxes are provided as required by SKV (probably when value is available?)
- Vouchers with dates in a period counts.
- A VAT quarter can span two financial years (Apr–Jun when the FY starts 1 May),
  so cratchit must combine vouchers from several SIE files by date.
- Voucher corrections applies to the period of the correction date.
- Vat returns sign determine if posted to BAS 2650 or BAS 1650.

### BAS account → VAT box mapping (draft)

| Box (ruta) | eskd element | BAS accounts (?) |
|---|---|---|
| 05 | `ForsMomsEjAnnan` | 3001, 30xx … |
| 10 | `MomsUtgHog` | 2610–2619 |
| 11 | `MomsUtgMedel` | 2620–2629 |
| 12 | `MomsUtgLag` | 2630–2639 |
| 48 | `MomsIngAvdr` | 2640–2649 |
| 49 | `MomsBetala` | computed |

## 5. Later milestones (sketch)

- M2: Write SIE (round-trip: read → write gives an equivalent file).
- M3: Import bank / SKV account statements and propose vouchers.
- M4: Infer postings for new events from earlier vouchers.
- M5+: Bokslut, Årsredovisning, INK2.

## 6. Open questions

- Mapping BAS accounts to VAT Returns box needs to be established and confirmed by online spec (SKV, BAS org, etc.,) or stated locally by specified template.
  - The BAS org may have csv-file that cratchit can parse?
- Vat Returns Box to eskd-file members needs to be established by online spec or stated locally by specified template.
  - SKV may have excel or csv-file that cratchit can parse?

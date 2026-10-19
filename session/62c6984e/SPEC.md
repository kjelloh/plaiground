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

Implemented in `src/vat_calculation.cpp` and `src/vat_account_mapping.cpp`.

- Amounts in the eskd file are whole kronor. Each box is **truncated** (öre dropped), as the
  settlement vouchers in our books show (öre difference posted to 3740).
- `MomsBetala` (ruta 49) = output VAT boxes (10–12, 30–32, 60–62) − box 48, computed from the
  truncated box values; negative means refund. Always present (SKV requires it).
- Boxes that are zero are left out.
- Vouchers with dates in a period count.
- A VAT quarter can span two financial years (Apr–Jun when the FY starts 1 May),
  so cratchit must combine vouchers from several SIE files by date.
- Voucher corrections apply to the period of the correction date.
- The VAT settlement vouchers ("Momsrapport", Fortnox series `M`) are left out: they only move
  the period's VAT to 1650/2650. Setting: settlement series, default `M`.
- Vat returns sign determine if posted to BAS 2650 or BAS 1650.
- A posting on a 26xx account without a box mapping is an error (except 2650, 2651).

Verified: reproduces the three filed returns (202409, 202412, 202503) exactly. Box 49 also
matches the settlement vouchers for 27 of 35 quarters 2017Q4–2026Q2 (see open questions).

### BAS account → VAT box mapping

| Box (ruta) | eskd element | Accounts | Sign |
|---|---|---|---|
| 05 | `ForsMomsEjAnnan` | 3001, 3002, 3003 **(?)** | credit |
| 10 | `MomsUtgHog` | 2610–2613 | credit |
| 11 | `MomsUtgMedel` | 2620–2623 | credit |
| 12 | `MomsUtgLag` | 2630–2633 | credit |
| 20 | `InkopVaruAnnatEg` | 9021 (Fortnox) | debit |
| 30 / 31 / 32 | `MomsInkopUtg…` | 2614, 2617 / 2624, 2627 / 2634, 2637 | credit |
| 39 | `ForsTjSkskAnnatEg` | 3308 **(?)** | credit |
| 48 | `MomsIngAvdr` | 2640–2642, 2645–2647 (not 2648 vilande) | debit |
| 49 | `MomsBetala` | computed | |
| 50 | `MomsUlagImport` | 9022 (Fortnox) | debit |
| 60 / 61 / 62 | `MomsImportUtg…` | 2615 / 2625 / 2635 | credit |

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
- **202506:** a year-end voucher (series I) that corrects 2641 is dated in the quarter, so cratchit
  counts it; the Fortnox settlement voucher did not. Should it count? Details: `_local/vat_findings.md`.
- **202606:** Fortnox split the quarter at the FY boundary into two settlement vouchers, each truncated
  separately; cratchit truncates the quarter total once (1 kr difference). Which is right?
- Older quarters that differ from their settlement voucher: 201712, 201803, 201806, 202112, 202203, 202212.
  Not investigated.

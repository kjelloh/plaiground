# Progress

Claude's working journal for session 7d4475fb (the user's journal is `session.md`).
Goal: `cratchit_ink2 <file.se>` makes the Swedish inkomstdeklaration 2 (INK2) of an aktiebolag
from its SIE file, checked against the INK2 the company filed in `_Resources/_ink2/`.

## How to build and check

```
cd cratchit
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset conan-debug && cmake --build --preset conan-debug
ctest --preset conan-debug
build/Debug/cratchit_ink2 ../_Resources/_sie/<file>.se      # INK2R to stdout, errors and notes to stderr
```

`_Resources/` is git ignored; the tests that compare with the filed INK2 skip when its files are missing.

## Agreed with the user (2026-11-03)

- App in `cratchit/`, executable `cratchit_ink2`, cloned from cratchit_ar (session 5b35b2b9) the way
  that session built it: CMake, Conan (gtest only), C++23, the core library plus a thin `main.cpp`.
- Same working agreement as 5b35b2b9: the user reviews and commits each step, Claude does not commit;
  no data files the user has not agreed to; `progress.md` at the session root.
- `cratchit/MANUAL.md` (English) is the user manual, built in and printed by `--help` / `-h`.
  Every step that changes what the user sees updates MANUAL.md in the same step.
- Of the three 2024 filings, the latest (`The ITfied AB 556782-8172`, 2025-08-15) is the reference.
  Their INK2R are the same; they differ in INK2S 7763 and so in 7770 / INK2 7114.
- Start with INK2R only (steps 1-3); INK2S and INK2 page 1 once we see what is missing.

## Pairs to verify against

| Fiscal year | SIE file (`_Resources/_sie/`) | Filed INK2 (`_Resources/_ink2/`) |
|---|---|---|
| 2021-05-01 - 2022-04-30 | TheITfiedAB20221124_192100.se | deklaration 2022/blanketter.sru |
| 2022-05-01 - 2023-04-30 | TheITfiedAB20230503_152345.se | deklaration 2023/blanketter.sru |
| 2023-05-01 - 2024-04-30 | TheITfiedAB20240727_213539.se | deklaration 2024/The ITfied AB 556782-8172/blanketter.sru |
| 2024-05-01 - 2025-04-30 | TheITfiedAB20250817_172409.se | deklaration 2025/BLANKETTER.SRU |
| 2025-05-01 - 2026-04-30 | sie_2025-26_all.se (books not closed) | to be made |

## Steps

### 1. Scaffold and clone - done (2026-11-03)

Cloned from `../5b35b2b9/cratchit` as is: `amount`, `bas`, `charset/*`, `csv`, `date`, `log`,
`manual`, `sie/*`, `terminal_encoding` and their tests. Adapted: `closing` (from `ar/closing`, into
namespace `cratchit`) and `balances` (`Balances`, `year_balances`, `to_kronor`, from `ar/statement`).
Not cloned: the ÅR layout and its account set parser; INK2's BAS table writes accounts differently.

### 2. The mapping - done (2026-11-03)

`Resources/bas_2023_to_ink2_2026.csv` (the user's, unchanged) is built in like the manual.
- `ink2/accounts`: BAS's notation `10xx`, `151x–155x` (en dash), `(exkl. ...)` for the terms
  before it since the last exclusion (so `28xx (exkl. 286x-287x), 2473, 2873` keeps 2873),
  `(Om netto +/-)` and `+ 899x` / `– 899x`.
- `ink2/mapping`: headings, rows (2.1-2.50, 3.1-3.27) and the second (minus) field of a row.
- Signs by SRU code: 72xx and 75xx as booked, 73xx and 74xx negated; 7450/7550 by 899x (debit = profit).
- `Om netto +` / `-`: by the net of the account group; zero goes to the + field.

### 3. INK2R - done (2026-11-03)

`ink2/form` sums each field (rounded half away from zero), errors for an account in no field or
two, notes for accounts outside class 1-8 and for sums that do not add up in whole kronor.
`ink2/output` prints the form; `main` adds the closing notes and the preview of books not closed.
`sru` reads SKV's BLANKETTER.SRU for the tests in `test_ink2_filed.cpp`.

Against the filed INK2R (each difference is in the test with its reason):

- 2024-25: all fields the same.
- 2023-24: 7369 25025 made, 25024 filed: the filer rounded down so the balansräkning adds up.
- 2022-23: 7550 84193 / 84192 (the same rounding); 7413 -129 / 67: the filer put 8710 (195.50,
  not in BAS 2023's table, an error here) in övriga rörelseintäkter; 7369 6270 / not filed: the
  filed balansräkning lacks övriga skulder (2641, 2893, 2898) and does not add up by 6270 kr.
- 2021-22: 7261 751 / 1179 and 7369 -428 / 0: the filer moved moms 2650 (debit 428) to
  receivables, as BAS's closing asks (1650); 7281 and 7365 rounded down by the filer to balance.

## Decisions and findings (2026-11-03, after step 3)

1. Rounding: sum the BAS accounts of a field exactly, then round the field (as now). SKV's field
   table for INK2R (`INK2R_SKV2002-34-01-26-04.xls`, in SKV's zip "Ändringar från och med 2026P4" on
   skatteverket.se, Teknisk information om filöverföring) has no sum fields and its only rules are
   that 7450 and 7550 must not both be given with an amount. So no rule that the balansräkning or
   resultaträkning add up in whole kronor; the filers' adjustments were their own. The rounding mode
   (half away from zero now) is not stated there; still open whether SKV says one.
2. Follow BAS's table as is. Reclassifications such as moms with a debit balance to 7261 would be
   a separate INK2 processing step, if a rule asks for it.
3. The SIE files' `#SRU` (by Fortnox) agree with BAS's table for every account with an amount but:
   1226/1227 (own sub-accounts, no #SRU), 8710 -> 7418 (an older code, not on INK2R 2026) and
   8999 -> 7450 always (#SRU has one code per account, so no plus/minus split).
   Decided: BAS's table decides; #SRU is a cross-check (note) and decides only where the table
   cannot: an account in two fields (its #SRU chooses) or in none (its #SRU, when a field of INK2R).
4. FLAG 40xx-47xx (3.5 Råvaror och förnödenheter / 3.6 Handelsvaror): BAS's table lists them under
   both; which one is by the nature of the goods (råvaror consumed vs goods sold), not by account
   number. Now the account's #SRU chooses (Fortnox: 4010 -> 7511); an error without one.
   Still to confirm what SKV requires; these books have no such accounts.

### 3b. #SRU cross-check and fallback - done (2026-11-03)

`ink2::sru_codes(document)` and `build_form(lines, balances, sru)`. On the session's files: one note
per year (1226, 1227 have no #SRU) and 2022-23's 8710 stays an error (its #SRU 7418 is not on INK2R).

## Next

4. INK2S and INK2 page 1: what can be made from the books (7650/7750 from 3.26/3.27, 7651
   non-deductible costs, ...) and what needs input. E.g. 7763: 2023's equals 2022's 7770 (92519)
   and 2025's equals 7770 of the corrected 2024 filing (205963), not of the latest one (114130).
5. SRU files for SKV, and tracing amounts back to accounts and vouchers.

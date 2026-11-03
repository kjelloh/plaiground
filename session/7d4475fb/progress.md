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
build/Debug/cratchit_ink2 ../_Resources/_sie/<file>.se "../_Resources/_ink2/deklaration <last year>"
                                    # INK2R, INK2S, INK2 to stdout, errors and notes to stderr
```

`_Resources/` is git ignored; the tests that compare with the filed INK2 skip when its files are missing.

## Agreed with the user (2026-11-03)

- App in `cratchit/`, executable `cratchit_ink2`, cloned from cratchit_ar (session 5b35b2b9) the way
  that session built it: CMake, Conan (gtest only), C++23, the core library plus a thin `main.cpp`.
- Same working agreement as 5b35b2b9: the user reviews and commits each step, Claude does not commit;
  no data files the user has not agreed to; `progress.md` at the session root.
- `cratchit/MANUAL.md` (English) is the user manual, built in and printed by `--help` / `-h`.
  Every step that changes what the user sees updates MANUAL.md in the same step.
- Of the three 2024 filings, the corrected one (`Korrigerad 20241130`) is the reference (changed from
  the latest after step 3; see step 4a). Their INK2R are the same; they differ in INK2S 7763 and so
  in 7770 / INK2 7114.
- Start with INK2R only (steps 1-3); INK2S and INK2 page 1 once we see what is missing.

## Pairs to verify against

| Fiscal year | SIE file (`_Resources/_sie/`) | Filed INK2 (`_Resources/_ink2/`) |
|---|---|---|
| 2021-05-01 - 2022-04-30 | TheITfiedAB20221124_192100.se | deklaration 2022/blanketter.sru |
| 2022-05-01 - 2023-04-30 | TheITfiedAB20230503_152345.se | deklaration 2023/blanketter.sru |
| 2023-05-01 - 2024-04-30 | TheITfiedAB20240727_213539.se | deklaration 2024/Korrigerad 20241130/blanketter.sru |
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

## Agreed with the user (2026-11-03, before step 4)

- `bas_2023_to_ink2_2026.csv` holds only mappings from BAS accounts to fields, for all forms (INK2R
  and INK2S). How fields depend on each other (4.1 = 3.26, 4.15/4.16 sums, 1.1/1.2 = 4.15/4.16) is
  hard coded in the app.
- `Resources/skv_ink2_2026P4.csv` (agreed): SKV's fields of INK2, INK2R and INK2S, made from SKV's field
  tables `INK2*_SKV2002-34-01-26-04.xls` (in SKV's zip "Ändringar från och med 2026P4", Teknisk
  information om filöverföring). Columns form;sru;ink2;text;sign;type (type, amount or mark, added
  for the yes/no fields). Made by a script kept outside the repo (xlrd); rerun for a new SKV version.
- The app always needs last year's return, told from the SIE file by its ending (.sru, or a folder
  of BLANKETTER.SRU), or instead `--previous-deficit <kronor>` (last year's 4.16), never both.
  So the app chains over the years once it writes SRU files (step 4b), starting from the filed ones.
- 4b: write a folder `<orgnr>_<start>_<end>/` with BLANKETTER.SRU, INFO.SRU and a copy of the SIE
  file, mirroring cratchit_ar.
- The 2024 deficit: the higher one (corrected filing, 205963). The user remembers forgetting to carry
  a deficit somewhere and adding it. The arithmetic agrees: 2022-23's 4.16 is 176026, which the
  corrected 2024 filing carries; the latest (2025-08-15) carries 84193 (2022-23's loss only).
  The user may want to check what SKV registered for 2024.

### 4a. INK2S and INK2 page 1 to the terminal - done (2026-11-03)

- INK2S rows in the BAS CSV (block at the end, marked as cratchit's): 4.3a `89xx (exkl. 899x)`,
  4.3c `5982, 6072, 6342, 6392, 6982, 6992, 7622, 7623, 7632, 8423`, 4.5c `8254, 8314`, from the
  accounts the BAS e-book (BAS 2025) names ej avdragsgill / skattefri. For the user to review.
  (The e-book's own INK2 table covers INK2R only; there is no BAS table for INK2S.)
- `ink2/skv` (SKV's fields), `ink2/previous` (last year's return: orgnr, fiscal year, 7770, marks,
  opening balance check), `ink2/tax` (the rules), new arguments, output of INK2S and INK2 page 1.
- Against the filed returns, each year chained from the year before (in `test_ink2_filed.cpp`):
  - 2023-24 against the corrected filing: INK2S and INK2 the same.
  - 2022-23: 4.16 176032 made, 176026 filed: 3.27 rounding (84193/84192) and 8314 (5) not filed.
  - 2024-25: 4.16 251460 made, 252665 filed: 6992 (1217, ej avdragsgill) and 8314 (12) not filed.
  - The latest 2024 filing: 4.14a 84193 filed, 176026 made.
- 2025-26 (books not closed), from the filed 2025 return: 4.16 = 24989 + 252665 + 8 = 277662.

### 4b. SRU files - done (2026-11-03)

- `ink2/sru_output`: BLANKETTER.SRU (blocks INK2, INK2R, INK2S, e.g. `INK2R-2025P1`; #IDENTITET with
  the orgnr as 16 + ten digits and the run's date and time; 7011/7012 and the fields in SKV's order)
  and INFO.SRU (#ORGNR, #NAMN, #POSTNR, #POSTORT), ISO 8859-1, LF; written with a copy of the SIE
  file to `<orgnr>_<start>_<end>/` in the working directory, as cratchit_ar does.
- Format from SKV's page "Teknisk information om filöverföring" (it replaces SKV 269, discontinued
  2024-01-01): ISO 8859-1, records start with #, LF ends them, negative amounts with '-', amounts
  signed relative to the form's printed sign (as the app shows them), orgnr SSÅÅMMDDNNNK (16 for
  legal persons). The block name and period (P1-P4) follow the filed returns and SKV's rule for 7012.
- Not written with errors, nor while the books are not closed (the files are for filing).
- Postnummer/postort: SIE #ADRESS, else last year's INFO.SRU next to its BLANKETTER.SRU.
- Tests: exact BLANKETTER.SRU and INFO.SRU texts, writing, and the app chaining its own returns:
  from the filed 2022 return, 4.16 is 176032 (2022-23), 205969 (2023-24), 251466 (2024-25).
- Run by hand: 2024-25 with the corrected 2024 return writes the folder; 2025-26 with that folder
  reads 251460 and writes nothing (books not closed), and its #IB matches the written INK2R.

## Open

- Test the written files with SKV's test function in Filöverföring (the user; Claude cannot).
- SKV's field tables used are for periods from 2026P4; a fiscal year ending 2026-04-30 is 2026P1,
  which the previous version of the tables covers. Check whether they differ for INK2/INK2R/INK2S.
- Postal address: the SIE file's #ADRESS wins over last year's INFO.SRU. The 2024-25 SIE file says
  17668 Järfälla, the 2024 INFO.SRU and the 2025-26 SIE file 17141 Solna.
- 4.3b (nedskrivning av finansiella tillgångar) left out: deductible or not depends on the asset.
- The rounding mode SKV wants, if any (now half away from zero).

## Next

5. Tracing amounts back to accounts and vouchers (`--trace`), and a report file like cratchit_ar's
   with the notes, if wanted.

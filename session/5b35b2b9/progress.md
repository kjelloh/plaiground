# Progress

Claude's working journal for session 5b35b2b9 (the user's journal is `session.md`).
Goal: `cratchit_ar <file.se>` prints a Swedish årsredovisning (K2, aktiebolag) as plain text,
mirroring the filed ÅR in `_Resources/_årsredovisningar/`.

## How to build and check

```
cd cratchit
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset conan-debug && cmake --build --preset conan-debug
ctest --preset conan-debug
build/Debug/cratchit_ar ../_Resources/_sie/<file>.se             # the ÅR
build/Debug/cratchit_ar --accounts ../_Resources/_sie/<file>.se  # accounts behind each post, in öre
```

`_Resources/` is git ignored; the tests comparing against the PDFs skip when its SIE files are missing.

## Agreed with the user (2026-11-02)

- App in `cratchit/`, executable `cratchit_ar`.
- Expected amounts typed in from the PDFs may be test files (the PDFs are public).
- Texts not in SIE (verksamhet, medelantal anställda, utdelningsförslag, underskrifter, flerårsöversikt
  beyond two years) are left out for now.
- `Resources/bas_to_ar_2024.csv` defines every line of the ÅR, not just BAS ranges.
- Never add data files the user has not agreed to.
- The user reviews and commits each step; Claude does not commit.
- `cratchit/MANUAL.md` (English) is the user manual, built in and printed by `--help` / `-h`.
  **Every step that changes what the user sees updates MANUAL.md in the same step**
  (options, output, errors, what is not yet included). The test `Manual.NamesEveryOption` lists the options.

## Pairs to verify against

| SIE file (`_Resources/_sie/`) | ÅR PDF | Expected (`cratchit/test/expected_ar/`) |
|---|---|---|
| TheITfiedAB20230503_152345.se | Årsredovisning 220501-230430.pdf | 220501-230430.csv |
| TheITfiedAB20240727_213539.se | Årsredovisning The ITfied AB 2024.pdf | 230501-240430.csv |
| TheITfiedAB20250817_172409.se | Årsredovisning 240501-250430.pdf | 240501-250430.csv |

All from Fortnox, SIE 4, fiscal year May-April, each with `#RAR 0` and `#RAR -1`.

## Design

- SIE stack copied unchanged from `session/60cae9fe/cratchit` (lexer, document, check, charset, amount, date,
  log, csv) with its tests. Only what the SIE parser needs; no REPL, workspace or VAT code.
- `core/ar/layout`: parses the mapping CSV `Nivå;Typ;BAS Range;ÅR`, built into the program by CMake
  (`layout_text.cpp.in`). Typ: `Rubrik`, `Post` (has BAS accounts), `Summa`, `Utanför` (accounts on no line).
  BAS Range grammar: numbers and ranges separated by `,` / `eller` / `och`, `(förutom ...)` excludes from the
  whole set.
- `core/ar/statement`: amounts per year from `#UB` (balance) and `#RES` (result). A Summa adds the posts after
  the last Rubrik at its Nivå or above, so Nivå 1 sums in the RR are cumulative (Rörelseresultat,
  Resultat efter finansiella poster, ...). Shown sign: class 1 as booked, all others negated.
  Exact öre are summed; each line is rounded to kronor on its own (half away from zero), so a sum may be
  1 kr off its shown posts, but RR = BR result and Tillgångar = EK+skulder hold to the öre.
  A post is shown when any account in it has an amount in either year; headings and sums when a post under
  them is shown.
- Errors: account with an amount on no line or on two; checks: BR balance and Årets resultat RR vs BR.
- `core/ar/text`: plain text, 56 wide text column, 14 wide amount columns, long texts wrapped.

## Mapping changes vs the original 2024 CSV (from arsredovisning-online.se)

The original is in git history (commit 36ad56e). Deliberate changes:
- 8710 Extraordinära intäkter -> Övriga rörelseintäkter; 8750 Extraordinära kostnader -> Övriga rörelsekostnader
  (K2 has no extraordinary posts; this reproduces the 22/23 PDF's "Övriga rörelseintäkter 67").
- 2099 -> Årets resultat in the BR (was unmapped).
- 8999 (Fortnox closing post in #RES) and 9000-9999 (class 9) are Utanför.

## Findings

- 22/23 PDF is wrong: omits Redovisningsmedel 157, and its BR does not balance (307 026 vs 283 723).
  The SIE balances at 307 183, which the 23/24 PDF also shows for that year.
- The PDF tools put 2650 Redovisningskonto för moms with a debit balance (428, 2022-04-30) under Övriga fordringar;
  we put it under Övriga skulder (-428). Needs a sign rule.
- 24/25 PDF nets 8422 dröjsmålsränta (2,66) into ränteintäkter (we keep it separate: kvittningsförbud) and puts
  1999 (named "OBS" in the user's chart, 336 kr) in Kassa och bank (we: Redovisningsmedel, per BAS).
  Both noted in the expected file. 1999 "OBS" holding money at year end may be worth clearing in the books.
- The 23/24 PDF tool rounds per account and sums rounded amounts; the 22/23 and 24/25 tool sums exact amounts.
  We sum exact; tests allow ±1 kr.
- Fortnox SIE files have 2099 booked with the year's result at #UB 0, so the BR balances without adding the RR result.
  An SIE file from books not yet closed would fail the checks (not handled).

## Steps

1. Skeleton, SIE stack, header printed. Done.
2. Trial balance (`--accounts`). Done.
3. Mapping parser and full K2 layout CSV. Done; no account 1000-9999 on two lines.
4. Amounts per ÅR post, unmapped accounts are errors. Done; all 3 files map fully.
5. RR/BR structure, sums, signs, checks. Done; all 3 years balance and agree to the öre.
6. Plain text rendering. Done; amounts match all 3 PDFs within ±1 kr except the findings above.
   124 tests pass, no warnings. Not committed yet.
7. User manual: `MANUAL.md` built in like the mapping (`core/manual`, `manual_text.cpp.in`);
   `--help` / `-h` prints it raw to stdout, exit 0; a wrong command line (also an unknown `-` option)
   prints usage and `See cratchit_ar --help`, exit 2. 126 tests pass, no warnings. Done.
8. Next: sign rule for accounts that are a fordran or a skuld by their balance (2650, 1630/2510 ...),
   in the CSV, e.g. a debit/credit column. Then 2650 in the 22/23 expected file follows the PDF.
9. Förändringar i eget kapital from IB/UB of 2081/209x and the dividend paid (check: pages 3 of the PDFs).
10. Not 3 Inventarier from 122x IB/UB and the year's purchases in the vouchers (check: page 8, 22/23 and 24/25).
11. Resultatdisposition (needs the proposed dividend, not in SIE; agree input with the user first).
12. `bas_to_ar_2026.csv` from https://www.arsredovisning-online.se/bas-kontoplan/ in the same form;
    diff against 2024 and rerun the 3 years.
13. Later: flerårsöversikt over several SIE files, soliditet, the texts not in SIE, iXBRL.

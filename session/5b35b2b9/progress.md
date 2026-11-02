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
- `core/ar/statement`: each account with an amount is on exactly one line; amounts per year from `#UB` (balance) and `#RES` (result). A Summa adds the posts after
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
- (Step 8 added balance rules for 2510, 2650 and 1630; step 8b removed them again: closing entries belong in the books.)
- 8999 (Fortnox closing post in #RES) and 9000-9999 (class 9) are Utanför.

## Findings

- 22/23 PDF is wrong: omits Redovisningsmedel 157, and its BR does not balance (307 026 vs 283 723).
  The SIE balances at 307 183, which the 23/24 PDF also shows for that year.
- The PDF tools put 2650 Redovisningskonto för moms with a debit balance (428, 2022-04-30) under Övriga fordringar,
  but kept 2641 Ingående moms (debit 1 331, 2023-04-30) netted in Övriga skulder. By BAS both should have been
  moved to 1650 in the closing (step 8b notes them).
- 24/25 PDF nets 8422 dröjsmålsränta (2,66) into ränteintäkter (we keep it separate: kvittningsförbud) and puts
  1999 (named "OBS" in the user's chart, 336 kr) in Kassa och bank (we: Redovisningsmedel, per BAS).
  Both noted in the expected file. 1999 "OBS" holding money at year end may be worth clearing in the books.
- The 23/24 PDF tool rounds per account and sums rounded amounts; the 22/23 and 24/25 tool sums exact amounts.
  We sum exact; tests allow ±1 kr.
- Fortnox SIE files have 2099 booked with the year's result at #UB 0, so the BR balances without adding the RR result.
  An SIE file from books not yet closed would fail the checks (not handled).

### Year-end closing, deduced from the vouchers (2026-11-02)

- The user's own practice: each quarterly VAT report (M series) books a refund to 1650 Momsfordran.
  The 428 debit on 2650 at 2022-04-30 breaks that practice (it is cleared by A31 2022-05-16 "Skattekonto").
  The 21/22 vouchers are not in any SIE file here, so how it got there is not visible. Booked to 1650, it
  would be in Övriga fordringar by plain BAS, with no balance rule needed.
- 1630 never has a credit and 2510 never a debit balance at a year end in the 3 files: those two rules
  are only defensive.
- 23/24 already has a closing entry of this kind: I5 2024-04-30 "balans av momsfordran och skuld" moves
  2 639 between 1650 and 2650.
- 1999 ("OBS") is not Redovisningsmedel. At 2023-04-30 it holds 156,75 of unreported ingående moms
  (I5 2023-04-30 moves it from 2641, I1 2023-05-01 moves it back). At 2025-04-30 it holds 336 = 179,25 + 156,75,
  "Obokad/utebliven moms" (I5, I6 2025-04-30). At 2022-04-30, 161,25 from a double payment (A68 2022-11-23).
  These are receivables (VAT to claim, money to get back), so Övriga fordringar, not Kassa och bank.
- 2641 Debiterad ingående moms has a debit balance at every year end (1 331,46 / 156,75 / 67,60): the VAT
  quarters are calendar quarters, the fiscal year ends 04-30, so April's VAT is not yet reported. Normal,
  but as a receivable it is netted into Övriga skulder (both PDF tools did the same).

### What BAS says (Bokföringsboken BAS 2026, the user's licensed copy, read 2026-11-02; not copied into the repo)

- 2650 / 1650 (pp. 181, 285): VAT is kept in group 26 during the year. If the sum at the balance day of
  VAT periods not yet due is a receivable, the balance is moved to 1650 Momsfordran. A closing entry for
  the group's net, not per account.
- 2510 / 1640 (pp. 181, 265): if the balance of group 25 is a receivable at the balance day, it is moved
  to 1640 Skattefordringar. Again the group's net.
- 1630 (p. 180): a negative (credit) balance at the balance day should be moved to 2850 Avräkning för
  skatter och avgifter, to avoid netting assets and debts. 2850 is Övriga skulder, NOT Skatteskulder:
  the step 8 rule `1630 vid kreditsaldo` on Skatteskulder is wrong.
- 1990 Redovisningsmedel (p. 204): money received on behalf of others (brokers, lawyers), with a matching
  debt on 2830. The user's 1999 "OBS" use does not fit it.
- 2999 OBS-konto (pp. 315-316) is BAS's suspense account; its balance must always be zero at year end.
- So: these are closing entries the books should hold. Done in the books, a plain BAS mapping gives the
  right ÅR. Deduced effect on the user's year ends:
  - 2022-04-30: group 26 net 428 debit -> 1650 (what step 8 shows).
  - 2023-04-30: group 26 net 1 331,46 (2641) + 156,75 parked on 1999 -> 1650 1 488,21.
    Övriga fordringar 2 493, Övriga skulder 7 602, no Redovisningsmedel (both PDFs netted instead).
  - 2024-04-30: 2641 156,75 -> 1650 (with the 1 026 already there).
  - 2025-04-30: 2641 67,60 and the 336 of unbooked VAT on 1999 -> 1650.

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
8. Accounts on a line by their balance. BAS Range takes `vid debetsaldo` / `vid kreditsaldo` per account
   or range (not inside `förutom`). Per account and year, a fitting qualified line wins over the line
   for any balance; more than one fitting line is an error. Sign now follows the line (first range class 1:
   as booked), not the account. Mapping: 2510 and 2650 vid debetsaldo -> Övriga fordringar, 1630 vid
   kreditsaldo -> Skatteskulder (per account, as agreed; the whole VAT group netted would not match the PDFs).
   Only 2650 actually moves in the 3 files (2022-04-30, 428); 22/23 year -1 now matches the PDF.
   MANUAL.md updated. 130 tests pass, no warnings. Done.
8b. Closing belongs in the books (option A, agreed after reading the BAS 2026 book). The built-in mapping is
   plain BAS again (no `vid debetsaldo/kreditsaldo`).
   `core/ar/closing`: notes on stderr per year shown, exit code unchanged: moms 2610-2659 net debit -> 1650,
   2500-2599 net debit -> 1640, 1630 credit -> 2850, 1990-1999 not matched by 2830-2839, 2999 not zero.
   The 3 files give exactly the deduced notes (test ArClosing.FiledYearsLackTheVatClosingEntry).
   Fixed: imbalance findings now give exit 1, as MANUAL.md said. 22/23 expected year -1 is the books again
   (751 / -428), the PDF value noted. MANUAL.md: Year-end closing notes. 137 tests pass, no warnings. Done.
8c. Balance qualifiers removed from the code too, to keep the mapping as simple as we can get away with:
   `AccountSet` is included/excluded ranges again, an account is on the one line that holds it, signs
   follow the account class. Kept because the data needs them: `förutom`, the `,`/`eller`/`och`
   separators, `Utanför` (8999, class 9), `Nivå`/`Typ`. Can come back if a real need shows.
   Output of the 3 files unchanged. MANUAL.md updated.
9. Förändringar i eget kapital from IB/UB of 2081/209x and the dividend paid (check: pages 3 of the PDFs).
10. Not 3 Inventarier from 122x IB/UB and the year's purchases in the vouchers (check: page 8, 22/23 and 24/25).
11. Resultatdisposition (needs the proposed dividend, not in SIE; agree input with the user first).
12. `bas_to_ar_2026.csv` from https://www.arsredovisning-online.se/bas-kontoplan/ in the same form;
    diff against 2024 and rerun the 3 years.
13. Later: flerårsöversikt over several SIE files, soliditet, the texts not in SIE, iXBRL.

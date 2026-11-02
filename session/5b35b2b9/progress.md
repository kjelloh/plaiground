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
build/Debug/cratchit_ar --trace ../_Resources/_sie/<file>.se     # accounts and vouchers behind each amount
```

`_Resources/` is git ignored; the tests comparing against the PDFs skip when its SIE files are missing.

## Agreed with the user (2026-11-02)

- App in `cratchit/`, executable `cratchit_ar`.
- Expected amounts typed in from the PDFs may be test files (the PDFs are public).
- Texts not in SIE (verksamhet, medelantal anställda, utdelningsförslag, underskrifter, flerårsöversikt
  beyond two years) are left out for now.
- The mapping CSV (`Resources/bas_to_ar_<year>.csv`, built in: 2026) defines every line of the ÅR, not just BAS ranges.
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
  (`layout_text.cpp.in`; since step 12 `Resources/bas_to_ar_2026.csv`). Typ: `Rubrik`, `Post` (has BAS accounts), `Summa`, `Utanför` (accounts on no line).
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

### Equity vouchers (step 9)

- 24/25 books a dividend 125 000 (I2 2024-07-27) and cancels it (I3 2025-04-30 "Makulerad I2"): no dividend
  that year, as the PDF's empty Utdelning row says.
- The 23/24 SIE file (exported 2024-07-27) closes with 2091 -348 498,23 and 2098 278 845,50; the 24/25 file
  opens that year with 2091 -69 652,73 and 2098 0. The books were changed after the export. Balanserat
  resultat is the same (69 653) either way.

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
9. Förändringar i eget kapital (`core/ar/equity`), printed before the resultaträkning. Columns: the BR posts
   under the Rubrik "Eget kapital" with an amount, then Totalt. Rows: #IB 0, the year's vouchers on those
   accounts classified by their other accounts (none: Balanseras i ny räkning; 8999: Årets resultat;
   2898: Utdelning; all on 2093: Erhållna aktieägartillskott; else Övriga förändringar + note), #UB 0.
   Error unless opening + changes = closing per column, to the öre. Zero change rows left out, zero cells
   blank. Text column 30 wide, column titles wrapped. Expected tables typed from the PDFs:
   `test/expected_ar/*_eget_kapital.csv`; all 3 match within 1 kr. 24/25 PDF is wrong: puts the
   aktieägartillskott 100 000 in the Årets resultat column and closes Balanserat resultat at 39 106 (BR: 139 106).
   MANUAL.md updated. 145 tests pass, no warnings. Done.
9b. `--trace` replaces `--accounts` (agreed; no filter for now). `core/ar/trace` is the model: each shown
   RR/BR item -> accounts (#UB or #RES per year, #IB 0 for class 1-2) -> year 0 postings by date
   (#BTRANS left out). Equity rows keep their accounts (first/last row) or vouchers (changes).
   `format_trace` in `core/ar/text`, in report order; amounts as booked. Year -1 only to account level
   (no vouchers in the file). Tests: the trace adds up to the öre on all 3 files (line = its accounts,
   account = IB + postings, equity row = its accounts/voucher transactions). 22/23 trace is ~900 lines.
   MANUAL.md: --trace section with real excerpts. Done.
10. Not 3 Inventarier, verktyg och installationer. Done. Agreed design: no new column or file; the note is
    ordinary rows in bas_to_ar_2024.csv under a new statement heading `1;Rubrik;;Noter`:
    `2;Rubrik;;Not 3 Inventarier, verktyg och installationer`, three `Post` rows Anskaffningsvärden
    (12x0-12x7), Avskrivningar (12x9), Nedskrivningar (12x8) within the BR post "1220-1279 (förutom 1260)",
    and `2;Summa;;Redovisat värde`. The note number is just the CSV text.
    - Rule changes: an account may be on one RR/BR line and one note line (checked per group; "on no line"
      only for RR/BR). `StatementLine::in_notes`. RR/BR printing stops at Noter.
    - `core/ar/notes`: per section #IB 0, vouchers, #UB 0; rows by section text (Inköp / Försäljningar och
      utrangeringar; Återförda / Årets avskrivningar|nedskrivningar). Disposal = voucher also on another
      section of the note or 3970-3979/7970-7979. Errors: section not adding up, Redovisat värde != BR post
      (post found from the title "Not <n> <post>"), unknown section text. Year 0 only.
    - Output: Noter after the Balansräkning. `--trace` lists each note row's accounts / vouchers (by date);
      a voucher's transactions show the transaction text when it has one (e.g. I6 2023-04-30 names each
      depreciated item and its period, "Mac Pro 2013 (5 år)"), else the account name. 22/23 trace ~1 180 lines.
    - All 3 years: no errors. 22/23 and 24/25 match PDF page 8 to the krona
      (`test/expected_ar/*_not_3.csv`); 23/24 (no Not 3 in its PDF) now gets one: 304 597 + 11 098 = 315 695,
      -236 868 - 20 685 = -257 553, -15 408, 42 734.
    - Tests: note sections split the BR post exactly (1000-9999), purchase/return/depreciation/scrapping,
      errors, trace adds up for note rows. 158 tests pass, no warnings. MANUAL.md: Noter section.
    - Not yet: the "Not 3" reference on the BR line; Not 1/Not 2 text notes.
    What we knew before:
    - The 22/23 and 24/25 Not 3 tables can be reproduced from #IB/#UB and vouchers to the krona
      (22/23: 220 215 + 84 382 = 304 597; -165 970 - 70 897 = -236 868; -15 408; 52 321.
      24/25: 315 695 + 43 752 = 359 447; -257 553 - 20 562 = -278 115; -15 408; 65 924).
    - Account roles by BAS numbering: ...8 ackumulerade nedskrivningar, ...9 ackumulerade avskrivningar,
      the others anskaffningsvärde (1218/1219, 1228/1229, 1258/1259 in the BAS 2026 book).
    - 22/23 credits on 1226 (10 248) are returns and credit notes (A18, A41, A42, A70, D37), netted into
      Inköp by the PDF. A sale or utrangering would also touch 1229 or 3970/7970.
    - The 23/24 PDF has no Not 3 although the BR shows Inventarier 42 734.
    - Numbering: K2 also requires Not 1 Redovisningsprinciper and Not 2 Medelantal anställda (not in SIE).
11. Flerårsöversikt (2 years), Resultatdisposition and the Not column. Done.
    - `core/ar/overview`: Nettoomsättning and Resultat efter finansiella poster (tkr, half away from zero),
      Soliditet = (Summa eget kapital + 0,794 x Summa obeskattade reserver) / Summa tillgångar in tenths of a
      percent ("-" without assets). Printed first. Two years from one SIE file; four (more files) later.
    - `core/ar/disposition`: the shown posts under "Fritt eget kapital" (year 0) and their Summa; the proposal
      rows "Utdelas till aktieägare" and "Balanseras i ny räkning" are kept with amount TBD and the line
      "TBD: koppling till stämmobeslut saknas" (user, 2026-11-02). Also a stderr note. Printed after
      Förändringar i eget kapital (PDF order).
    - Not column: `note_references` from note titles "Not <n> <post>"; the balansräkning gets a 6 wide Not
      column at the end of the text column, only when there are notes.
    - `--trace` starts with both, showing the statement lines they are computed from.
    - All 3 years match the PDFs (`test/expected_ar/*_flerarsoversikt.csv`, `*_resultatdisposition.csv`)
      except where the 24/25 PDF is wrong: soliditet 85,3 (copied from the year before; books: 99,9) and
      Balanserat resultat 39 106 / Summa -7 596 in its Resultatdisposition (books: 139 106 / 92 404, the
      aktieägartillskott 100 000 left out, as in its Förändringar i eget kapital).
    - 166 tests pass, no warnings. MANUAL.md: Flerårsöversikt, Resultatdisposition, The Not column.
12. `Resources/bas_to_ar_2026.csv`, built in instead of 2024 (option a; the 2024 file kept for comparison;
    `layout_2024_text()` renamed `layout_text()`). Done.
    - The web mapping (arsredovisning-online.se/bas-kontoplan, fetched 2026-11-02 with curl, parsed locally)
      is unchanged since 2023-11-30 (`article:modified_time`): identical to the user's 2024 extract, 97 rows.
    - Checked against the BAS 2026 chart of accounts (the user's Bokföringsboken 2026, local text only, not
      in the repo): 60 accounts were on no line. Corrected from the book:
      - Class 4: groups 40-42 are Handelsvaror, 43-48 Råvaror och förnödenheter (book, class 4, "Redovisning
        i resultaträkningen"). Was: 4000-4799 all Råvaror. Now Handelsvaror 4000-4299, 4960-4969, 4980-4989;
        Råvaror 4300-4899, 4910-4929 (4910/4920 are the only 491x-492x main accounts); Förändring av lager
        4900-4999 except those.
      - New K2 post Övriga immateriella anläggningstillgångar 1090-1099 (e.g. 1092 Tomträtter, new in 2026).
      - Group 12 reorganised: 1230, 1240 free accounts for maskiner; 1250, 1260 for inventarier; 127x gone.
        Maskiner 1210-1219, 1230-1249; Inventarier 1220-1229, 1250-1269; Not 3 sections follow.
      - Utanför 8990-8999 (group Resultat; was only 8999).
      Left on no line (on purpose): the 10 "(EJ K2)" accounts and equity of other company forms (2010-2072,
      2080, 2088). No account on two lines.
    - The user's books give byte-identical reports and traces for all 3 years (no moved account used).
    - Note: the user's chart names differ from BAS 2026 for accounts they use: 1226 "Elektroniklabb -
      Verktyg och maskiner" (BAS 2026: Bilar och transportmedel), 1227 "Elektroniklabb - Lager" (BAS 2026:
      Finansiellt leasade inventarier, Ej K2). Both are still Inventarier in the mapping.
    - Test ArLayout.BuiltInMappingFollowsBas2026 pins each placement. 167 tests pass, no warnings.
      MANUAL.md: the 2026 file, its origin and corrections.
13. Later: flerårsöversikt over several SIE files (4 years), the board's proposal in the Resultatdisposition,
    the texts not in SIE (förvaltningsberättelse, Not 1, Not 2, underskrifter), iXBRL.

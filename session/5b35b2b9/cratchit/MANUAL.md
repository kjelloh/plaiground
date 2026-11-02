# cratchit_ar

Prints the Swedish årsredovisning (annual report) of a company's books, read from an SIE 4 file.

## Synopsis

    cratchit_ar <file.se>
    cratchit_ar --accounts <file.se>
    cratchit_ar --help
    cratchit_ar -h

## What it prints

As plain text, laid out as for a K2 aktiebolag (BFNAR 2016:10):

1. Förändringar i eget kapital (changes in equity) of fiscal year 0.
2. The resultaträkning (income statement).
3. The balansräkning (balance sheet).

For the resultaträkning and balansräkning:

- The first column is fiscal year 0 of the SIE file (`#RAR 0`), the second the year
  before (`#RAR -1`), when the file has it.
- Amounts are in whole kronor, with a space between thousands, e.g. `-84 193`.
- A post is shown when any account in it has an amount in either year. Headings and
  sums are shown when a post under them is.

Errors and notes go to stderr, so the report can be redirected to a file:

    cratchit_ar TheITfiedAB20250817_172409.se > ar.txt

## Förändringar i eget kapital

The table shows how each part of the company's equity changed during fiscal year 0:
where it started, what happened to it, and where it ended. ÅRL 6 kap. 2 § asks for it
(in the förvaltningsberättelse for a K2 aktiebolag).

### How to read it

An example, for the fiscal year 2022-05-01 - 2023-04-30:

                                                    Balanserat         Årets
                                    Aktiekapital      resultat      resultat        Totalt

    Belopp vid årets ingång              100 800       119 680       284 165       504 645
    Utdelning                                         -125 000                    -125 000
    Balanseras i ny räkning                            284 165      -284 165             0
    Årets resultat                                                   -84 193       -84 193
    Belopp vid årets utgång              100 800       278 845       -84 193       295 453

- **Columns** are the posts under Eget kapital in the balansräkning, so each column
  ends where that post's line in the balansräkning does. Totalt is their sum.
  - **Aktiekapital**: the share capital. It only changes with a new share issue or a
    reduction.
  - **Balanserat resultat**: profits and losses kept from earlier years, together with
    shareholder contributions (2093).
  - **Årets resultat**: the result of a single year. At the start of the year it holds
    last year's result, at the end this year's.
- **Rows** read top to bottom, as the year went:
  1. **Belopp vid årets ingång**: equity at the start of the year. It equals the
     comparison column of the balansräkning (here 504 645).
  2. **Utdelning**: the dividend decided by the årsstämma (annual general meeting) and
     taken from Balanserat resultat (here 125 000).
  3. **Balanseras i ny räkning**: last year's result moved from Årets resultat into
     Balanserat resultat. It is a move within equity, so its Totalt is 0.
  4. **Årets resultat**: this year's result, the same as Årets resultat in the
     resultaträkning (here a loss of 84 193).
  5. **Belopp vid årets utgång**: equity at the end of the year. Each column equals
     its post in the balansräkning, and Totalt equals Summa eget kapital (295 453).
- Every column adds up: the rows from Belopp vid årets ingång down to the last change
  sum to Belopp vid årets utgång. In Balanserat resultat above:
  119 680 - 125 000 + 284 165 = 278 845. Each amount is rounded to kronor on its own,
  so a sum may differ by a krona from the shown amounts.
- A blank cell means the change did not touch that column. A change that is zero in
  every column is left out. For example, a dividend that was booked and then
  cancelled during the year gives no Utdelning row.

### How it is generated

From the SIE file alone, for fiscal year 0:

1. **Columns.** The posts under the heading `Eget kapital` in the mapping
   (`Resources/bas_to_ar_2024.csv`), e.g. Aktiekapital (2081, 2083, 2084), Balanserat
   resultat (2090, 2091, 2093-2095, 2098) and Årets resultat (2099). A post becomes a
   column only when its accounts have an amount.
2. **Belopp vid årets ingång** is the opening balance (`#IB 0`) of each column's
   accounts. **Belopp vid årets utgång** is their closing balance (`#UB 0`). Both are
   shown as equity: credit positive.
3. **The changes** are the year's vouchers (`#VER`) that post to any of these accounts.
   The voucher's other accounts decide its row:

   | Row | The voucher's other accounts | Typical voucher |
   |---|---|---|
   | Erhållna aktieägartillskott | not 8999 or 2898, and all its equity amounts are on 2093 | bank 1920 against 2093 |
   | Utdelning | include 2898 Outtagen vinstutdelning | the årsstämma's dividend: 2098 against 2898 and 2091 |
   | Balanseras i ny räkning | none: only equity accounts | last year's result: 2099 against 2098 |
   | Årets resultat | include 8999 Årets resultat | the year-end booking: 2099 against 8999 |
   | Övriga förändringar | anything else | noted on stderr with the voucher's series, number, date and text |

   A voucher adds its equity amounts to its row, each in its account's column.
   Amounts struck from a voucher (`#BTRANS`) do not count.
4. **The check.** In every column, Belopp vid årets ingång plus the changes must equal
   Belopp vid årets utgång, to the öre. If not, `cratchit_ar` says which column and
   exits with 1. That would mean the SIE file's balances and vouchers do not agree.

To check a row against the books, find the vouchers on the equity accounts (2081-2099)
in your bookkeeping program. Each one is in exactly one row.

## --accounts

Lists each shown post with the BAS accounts in it and their amounts as booked, in
kronor and öre, per year. Use it to check which accounts make up a post.

## --help, -h

Prints this manual. Long output can be paged: `cratchit_ar --help | less`.

## How the amounts are made

- **Source:** the closing balance (`#UB`) of balance accounts and the result (`#RES`)
  of result accounts, for each year.
- **Mapping:** the file `Resources/bas_to_ar_2024.csv` is built into the program. It
  lists every line of the årsredovisning in order, with the columns
  `Nivå;Typ;BAS Range;ÅR`:
  - `Nivå`: the level, 1 for the statements themselves and deeper for their parts.
  - `Typ`: one of
    - `Rubrik`: a heading.
    - `Post`: a post with BAS accounts.
    - `Summa`: adds up the posts after the last `Rubrik` at its level or above.
    - `Utanför`: accounts that are not on the årsredovisning, e.g. 8999 and class 9.
  - `BAS Range`: four digit accounts and ranges separated by `,`, `eller` or `och`, e.g.
    `1590-1619, 1630-1659 eller 1680-1689`. `(förutom ...)` excludes accounts, e.g.
    `4900-4999 (förutom 4910-4931 och 4980-4989)`.
  - `ÅR`: the text on the årsredovisning.
- **Signs:** assets (class 1) are shown as booked. Everything else is negated, so
  income, equity and debt are positive and costs negative.
- **One line per account:** each account is on one line whatever its balance. Moving
  a receivable or a debt to its proper account is a closing entry in the books (see
  Year-end closing notes).
- **Rounding:** each line is rounded to whole kronor from its exact amount, half away
  from zero. A sum may therefore differ by a krona from the shown posts it adds.

## Errors and checks

`cratchit_ar` exits with 1 and says why on stderr when:

- The SIE file cannot be read or is not a valid SIE 4 file.
- An account has an amount but is on no line of the mapping, or on more than one.
- The balansräkning does not balance (Summa tillgångar differs from Summa eget
  kapital och skulder), to the öre.
- Årets resultat differs between resultaträkning and balansräkning, to the öre. The
  year's result must be booked to 2099, as in a closed fiscal year.
- In Förändringar i eget kapital, a column's opening balance and changes do not add
  up to its closing balance, to the öre.

A wrong command line prints the usage and exits with 2.

## Year-end closing notes

The årsredovisning shows the books as they are. For each year shown, `cratchit_ar`
also notes on stderr the closing entries that BAS asks for at the balance day and
that the books lack. Notes do not change the exit code.

- `Moms (2610-2659) nets to a receivable`: the VAT not yet due is a receivable;
  book it to 1650 Momsfordran.
- `Skatteskulder (2500-2599) net to a receivable`: book it to 1640 Skattefordringar.
- `1630 Skattekonto has a credit balance`: book it to 2850, so that a debt is not
  netted against the assets.
- `Redovisningsmedel (1990-1999) ... is not matched by Avräkning för annans räkning
  (2830-2839)`: BAS 1990 is money held for others, booked against a debt on 2830. It is
  not a suspense account.
- `2999 OBS-konto is ...`: the suspense account must be zero at year end.

For example:

    Note: Moms (2610-2659) nets to a receivable 1331.46 at 2023-04-30; BAS: move it to 1650 Momsfordran

## Not yet included

Förvaltningsberättelse texts, flerårsöversikt, resultatdisposition, noter and
underskrifter.

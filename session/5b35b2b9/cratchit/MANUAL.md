# cratchit_ar

Prints the Swedish årsredovisning (annual report) of a company's books, read from an SIE 4 file.

## Synopsis

    cratchit_ar <file.se>
    cratchit_ar --accounts <file.se>
    cratchit_ar --help
    cratchit_ar -h

## What it prints

The resultaträkning (income statement) and balansräkning (balance sheet) as plain text,
laid out as for a K2 aktiebolag (BFNAR 2016:10).

- The first column is fiscal year 0 of the SIE file (`#RAR 0`), the second the year
  before (`#RAR -1`), when the file has it.
- Amounts are in whole kronor, with a space between thousands, e.g. `-84 193`.
- A post is shown when any account in it has an amount in either year. Headings and
  sums are shown when a post under them is.

Errors and notes go to stderr, so the report can be redirected to a file:

    cratchit_ar TheITfiedAB20250817_172409.se > ar.txt

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

Förvaltningsberättelse, flerårsöversikt, förändringar i eget kapital,
resultatdisposition, noter and underskrifter.

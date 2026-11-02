# cratchit_ar

Prints the Swedish årsredovisning (annual report) of a company's books, read from an SIE 4 file.

## Synopsis

    cratchit_ar <file.se>
    cratchit_ar --trace <file.se>
    cratchit_ar --help
    cratchit_ar -h

## What it prints

As plain text, laid out as for a K2 aktiebolag (BFNAR 2016:10):

1. Flerårsöversikt (multi-year overview) of fiscal year 0 and the year before.
2. Förändringar i eget kapital (changes in equity) of fiscal year 0.
3. Resultatdisposition (the disposition of the funds) of fiscal year 0.
4. The resultaträkning (income statement).
5. The balansräkning (balance sheet), with a Not column naming the note of a post.
6. Noter (notes) of fiscal year 0, for the posts of the balansräkning that have one in
   the mapping: for now Not 3 Inventarier, verktyg och installationer.

The first three are part of the förvaltningsberättelse (directors' report), whose
texts are not yet included.

For the resultaträkning and balansräkning:

- The first column is fiscal year 0 of the SIE file (`#RAR 0`), the second the year
  before (`#RAR -1`), when the file has it.
- Amounts are in whole kronor, with a space between thousands, e.g. `-84 193`.
- A post is shown when any account in it has an amount in either year. Headings and
  sums are shown when a post under them is.

Errors and notes go to stderr, so the report can be redirected to a file:

    cratchit_ar TheITfiedAB20250817_172409.se > ar.txt

## Flerårsöversikt

Three key figures for fiscal year 0 and the year before, to compare the years at a
glance:

                                                                2022-05-01    2021-05-01
                                                               -2023-04-30   -2022-04-30

    Nettoomsättning (tkr)                                                2           336
    Resultat efter finansiella poster (tkr)                            -84           284
    Soliditet (%)                                                     96,2         100,0

- **Nettoomsättning** (net sales) and **Resultat efter finansiella poster** (result
  after financial items, before year-end appropriations and tax) are the lines of the
  resultaträkning, in thousands of kronor (tkr), rounded half away from zero.
- **Soliditet** (equity ratio) is how much of the assets the owners' capital stands
  for: (Summa eget kapital + 79,4 % of Summa obeskattade reserver) / Summa tillgångar,
  from the balansräkning, in percent with one decimal. The untaxed reserves count with
  79,4 % as 20,6 % of them is deferred tax. A `-` means there are no assets.
- An SIE file holds two years, so the overview shows two. The filed årsredovisningar
  show four.

## Resultatdisposition

The funds at the årsstämma's (annual general meeting's) disposal, and the board's
proposal for them:

    Till årsstämmans förfogande står följande medel
      Balanserat resultat                                          278 845
      Årets resultat                                               -84 193
    Summa                                                          194 653

    Styrelsen föreslår att medlen disponeras enligt följande
      Utdelas till aktieägare                                          TBD
      Balanseras i ny räkning                                          TBD
    Summa                                                          194 653

    TBD: koppling till stämmobeslut saknas

- **The funds** are the posts under Fritt eget kapital in the balansräkning (e.g.
  Balanserat resultat and Årets resultat), at the end of fiscal year 0. Their Summa
  equals Summa fritt eget kapital.
- **The proposal** (how much to pay out as dividend and how much to carry forward) is
  the board's decision and not in the SIE file, so its amounts are `TBD`. A note on
  stderr says so too. The SIE file only shows the dividend a year later, when the
  årsstämma's decision is booked (see Utdelning in Förändringar i eget kapital).

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

To see the vouchers behind each row, run `cratchit_ar --trace` (see below). Each
voucher on the equity accounts is in exactly one row.

## The Not column

When there are notes, the balansräkning has a `Not` column with the number of the note
that breaks a post down:

                                                         Not    2023-04-30    2022-04-30
          Inventarier, verktyg och installationer          3        52 321        38 836

The number comes from the note's title in the mapping, `Not <n> <post>`.

## Noter

A note breaks a post of the balansräkning down: how it changed during fiscal year 0,
from what it was at the start to what it is at the end. For now there is one, for the
post Inventarier, verktyg och installationer (equipment, tools and installations).
The note numbers are those of the filed årsredovisningar: Not 1 and Not 2 are text
notes not yet included.

### How to read it

An example, for the fiscal year ending 2023-04-30:

    Not 3 Inventarier, verktyg och installationer               2023-04-30

    Anskaffningsvärden
      Ingående anskaffningsvärden                                  220 215
      Inköp                                                         84 382
      Utgående anskaffningsvärden                                  304 597

    Avskrivningar
      Ingående avskrivningar                                      -165 970
      Årets avskrivningar                                          -70 897
      Utgående avskrivningar                                      -236 868

    Nedskrivningar
      Ingående nedskrivningar                                      -15 408
      Utgående nedskrivningar                                      -15 408

    Redovisat värde                                                 52 321

- **Anskaffningsvärden** (cost): what the company paid for the equipment it holds.
  It grows with **Inköp** (purchases, net of returns and credit notes) and shrinks
  with **Försäljningar och utrangeringar** (sold or scrapped equipment).
- **Avskrivningar** (accumulated depreciation, negative): the cost written off so far.
  It grows with **Årets avskrivningar** (this year's depreciation, the same amount as
  in the resultaträkning) and shrinks with **Återförda avskrivningar** (the
  depreciation of sold or scrapped equipment, taken out with it).
- **Nedskrivningar** (accumulated write-downs, negative): value written off beyond
  the plan, with **Årets nedskrivningar** and **Återförda nedskrivningar** the same way.
- Each section reads from its opening (Ingående) through the year's changes to its
  closing (Utgående) amount: here 220 215 + 84 382 = 304 597.
- **Redovisat värde** (book value) is the three closing amounts added up, and equals
  the post in the balansräkning (52 321).
- A change that is zero is left out. Each amount is rounded to kronor on its own, so
  a sum may differ by a krona from the shown amounts.

### How it is generated

From the SIE file alone, for fiscal year 0:

1. **The note and its sections are rows in the mapping**
   (`Resources/bas_to_ar_2024.csv`), under the statement heading `Noter`:

       1;Rubrik;;Noter
       2;Rubrik;;Not 3 Inventarier, verktyg och installationer
       3;Post;1220-1227, 1230-1237, 1240-1247, 1250-1257, 1261-1267 eller 1270-1277;Anskaffningsvärden
       3;Post;1229, 1239, 1249, 1259, 1269 eller 1279;Avskrivningar
       3;Post;1228, 1238, 1248, 1258, 1268 eller 1278;Nedskrivningar
       2;Summa;;Redovisat värde

   The split follows BAS: 12x0-12x7 hold the cost, 12x8 the accumulated write-downs
   and 12x9 the accumulated depreciation. The sections together hold exactly the
   accounts of the post in the balansräkning. The title `Not <n> <post>` names that post.
2. **Ingående** and **Utgående** are the opening (`#IB 0`) and closing (`#UB 0`)
   balances of the section's accounts, as booked: debit positive.
3. **The changes** are the year's vouchers that post to the section's accounts. A
   voucher is a sale or scrapping (Försäljningar och utrangeringar, Återförda
   avskrivningar or Återförda nedskrivningar) when it also posts to another section of
   the same note, or to 3970-3979 or 7970-7979 (gain or loss on disposal). Any other
   voucher is Inköp, Årets avskrivningar or Årets nedskrivningar.
4. **The checks.** Each section's opening balance and changes must add up to its
   closing balance, and Redovisat värde must equal the post in the balansräkning, to
   the öre. If not, `cratchit_ar` says which and exits with 1.
5. A note is shown when any account in it has an amount in either year.

## --trace

Prints, instead of the årsredovisning, what each of its amounts is made of: the BAS
accounts and the vouchers behind it. Use it to check an amount against the books, or
to find out why a post holds what it does. It is long (several hundred lines for a
small company), so page it or save it:

    cratchit_ar --trace TheITfiedAB20230503_152345.se | less

### How to read it

The trace follows the order of the report: Flerårsöversikt and Resultatdisposition
(with the lines of the statements they are computed from), Förändringar i eget
kapital, the resultaträkning, the balansräkning and the notes. Each shown amount is a
heading line, in whole kronor as on the årsredovisning, with what makes it up
indented below it in kronor and öre **as booked: debit positive, credit negative**.

In the resultaträkning and balansräkning, each post lists its accounts, and each
account its vouchers in fiscal year 0, by date:

    Redovisningsmedel                                               157 kr        161 kr
      1999 OBS                                                      156.75        161.25
        Ingående balans 2022-05-01                                  161.25
        A 68 2022-11-23 Reglera OBS-konto                          -161.25
        I 5 2023-04-30 Ej rapporterad Moms till OB-konto            156.75

- The post line has one column per year, like the report.
- The account line (number and `#KONTO` name) has the account's closing balance
  (`#UB`) for a balance account (class 1 and 2) or its result (`#RES`) for a result
  account, per year.
- Below it, for fiscal year 0: `Ingående balans` (`#IB 0`) for a balance account, then
  every voucher on the account, as series, number, date and text, with the amount
  booked on the account. The opening balance and the vouchers add up to the account's
  year 0 amount: here 161.25 - 161.25 + 156.75 = 156.75.
- The year before can only be traced to the accounts: the SIE file has its balances
  but not its vouchers.
- The post's amount is its accounts added up, with the sign the årsredovisning shows:
  class 1 as booked, all others negated (so a credit income of -636.05 is 636 kr of
  income).
- An account is listed when it has an amount in either year, an opening balance or a
  voucher, so an account that ends the year at 0 still shows how it got there.
- Sums are not traced again: they add up the posts above them.

In Förändringar i eget kapital, the first and last row list the equity accounts'
opening and closing balances. A change row lists its vouchers, each with its
transactions on the equity accounts:

    Utdelning                                                  -125 000 kr
      I 2 2022-11-24 Årsstämma resultatfördelning
        2098 Vinst eller förlust från föregående år              284164.89
        2091 Balanserad vinst eller förlust                     -159164.89

Here the dividend of 125 000 is the net of the two: 284 164.89 - 159 164.89 taken
from Balanserat resultat. The voucher's other transaction (2898, the dividend owed)
is not listed, as it is not equity.

In the notes, each section lists its rows the same way: the accounts' balances for
Ingående and Utgående, the vouchers for each change. A voucher's transactions show
the account with the transaction's own text when it has one, else the account's name,
so the depreciation of each item is visible:

      Årets avskrivningar                                       -70 897 kr
        I 6 2023-04-30 Planenliga Avskrivningar för 20220501
        - 20230430
          1229 Mac Pro 2013 (5 år)                                -4986.00

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
- **One line per account:** each account is on one line of the resultaträkning and
  balansräkning whatever its balance. Moving a receivable or a debt to its proper
  account is a closing entry in the books (see Year-end closing notes). Under the
  statement heading `Noter`, an account may also be on one line of a note, which
  breaks its post down (see Noter).
- **Rounding:** each line is rounded to whole kronor from its exact amount, half away
  from zero. A sum may therefore differ by a krona from the shown posts it adds.

## Errors and checks

`cratchit_ar` exits with 1 and says why on stderr when:

- The SIE file cannot be read or is not a valid SIE 4 file.
- An account has an amount but is on no line of the resultaträkning and
  balansräkning, or on more than one, or on more than one line of the notes.
- The balansräkning does not balance (Summa tillgångar differs from Summa eget
  kapital och skulder), to the öre.
- Årets resultat differs between resultaträkning and balansräkning, to the öre. The
  year's result must be booked to 2099, as in a closed fiscal year.
- In Förändringar i eget kapital, a column's opening balance and changes do not add
  up to its closing balance, to the öre.
- In a note, a section's opening balance and changes do not add up to its closing
  balance, Redovisat värde differs from its post in the balansräkning, or a section
  under `Noter` is not Anskaffningsvärden, Avskrivningar or Nedskrivningar.

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

Förvaltningsberättelse texts, the board's proposal in the resultatdisposition (shown as
TBD), flerårsöversikt for more than two years, the text notes (Not 1
Redovisningsprinciper, Not 2 Medelantal anställda), and underskrifter.

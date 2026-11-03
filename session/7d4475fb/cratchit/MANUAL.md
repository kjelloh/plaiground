# cratchit_ink2

Makes INK2R, the räkenskapsschema (balance sheet and income statement) of the Swedish
inkomstdeklaration 2 for aktiebolag, from a company's books read from an SIE 4 file.

## Synopsis

    cratchit_ink2 <file.se>
    cratchit_ink2 --help
    cratchit_ink2 -h

A wrong command line prints the usage and exits with 2.

## What it prints

    cratchit_ink2 _Resources/_sie/TheITfiedAB20250817_172409.se

prints INK2R of fiscal year 0 of the SIE file (`#RAR 0`) to the terminal:

1. The head: the company name and orgnr, the fiscal year with the fields 7011 and 7012
   (its first and last day, as SKV writes them), and the mapping used.
2. Each line of the form in the order of the mapping: its number on the form (2.1-2.50 for
   the balansräkning, 3.1-3.27 for the resultaträkning), its text, its SRU code (SKV's
   fältkod) and its amount in whole kronor. The amount is blank when the books have no
   account with an amount in the field, so `0` means accounts that add up to zero.
3. The sums of the fields as shown: tillgångar against eget kapital och skulder, and the
   fields of the resultaträkning against årets resultat (3.26 less 3.27).

Lines whose amount goes to a plus or a minus field by its sign (e.g. 3.2, 3.12, 3.26/3.27)
show both fields, marked `+` and `-`.

Errors and notes go to stderr, after the form, one per line, starting with `Error:` or
`Note:`.

## How the amounts are made

The mapping is BAS's table from BAS 2023 to INK2 (`Resources/bas_2023_to_ink2_2026.csv`,
built into the program), with the BAS accounts written as BAS writes them, e.g.
`12xx (exkl. 128x)` or `151x–155x, 158x`.

- Each field is the sum of its accounts at the end of fiscal year 0: the closing balance
  (`#UB`) of balance accounts and the result (`#RES`) of result accounts.
- Signs: tillgångar (72xx) are shown as booked; eget kapital och skulder (73xx) negated, so
  a debt is positive; the plus fields of the resultaträkning (74xx, income) negated and the
  minus fields (75xx, costs) as booked, so both are positive when they are what they say.
  A field may thus be negative, e.g. leverantörsskulder with a debit balance.
- `(Om netto +)` and `(Om netto -)`: the accounts before it go to this field only when they
  together add to (+, a credit) or take from (-, a debit) the year's result; zero counts as +.
- 3.26 Årets resultat, vinst (7450) and 3.27 förlust (7550): BAS's `+ 899x` and `– 899x`.
  899x carries the year's result to the balansräkning, so a debit on 899x is a profit.
- Each field is the exact sum of its accounts rounded to whole kronor, half away from zero.
  The sums of the fields may then differ by a krona or two; this is a note. SKV's field
  table for INK2R has no sum fields and no rule that they add up; its only rule is that
  7450 and 7550 are not both given.
- Accounts outside 1000-8999 (e.g. class 9 for internal accounting) are left out, with a note.

## Books not closed

When the books of fiscal year 0 are not closed (nothing on 8990-8999, and the balance
accounts lack exactly the year's result), the form is a preview that shows the result as if
booked to 2099 against 8999: the head says PRELIMINÄR and a note says why.

## Errors

- An SIE file that cannot be read, or that is not a valid SIE 4 file: the errors, exit 1,
  and no form.
- An account of class 1-8 with an amount that is in no field of the mapping, or in more than
  one (BAS lists 40xx-47xx under both 3.5 and 3.6): the form is printed, with the error, and
  the exit code is 1. The form then does not add up.

## Notes

- `Closing:` closing entries BAS asks for at the balance day that the books lack (e.g. moms
  netting to a receivable, which BAS moves to 1650 Momsfordran); the form shows the books as
  they are.
- `Rounding:` the sums of the fields differ from rounding each field on its own.
- `Left out`: accounts that are not BAS accounts of class 1-8.

## Not yet included

- INK2S (skattemässiga justeringar) and INK2 page 1 (the tax computation).
- SRU files to send to SKV (`BLANKETTER.SRU`, `INFO.SRU`).
- Tracing each amount back to its accounts and vouchers.

## Options

- `--help`, `-h`: print this manual.

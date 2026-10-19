# Hints from earlier work, and where they went

Sources: Todo mails (`…/todo_mails/markdown_domain/_mail/`, referenced by id), the old cratchit repo
(`~/Documents/GitHub/cratchit`: `docs/thinking`, `todo/`, `issues/`, `resources/`, `skv_specs/`), and
take 1 (`../62c6984e`).

## Interfaces and formats

| Hint | Source | Used in |
|---|---|---|
| Two returns run all year: VAT (quarterly for us) and employer declaration (monthly). Due dates are the 12th, see the SKV calendar | mail 4be299a3 | `vat/vat_rules.cspec` deadlines, `employer_declaration` |
| Zero-salary employer declaration: fill box 001 at payee level | mail a28bf624 | `employer_declaration` (listed) |
| eSKD file: all amounts unsigned except box 49. Example of a refund return | mail 394f1543 | `vat/eskd_6p0.cspec` |
| EC sales list file: `SKV574008`, header row, buyer rows. SKV's examples are inconsistent about trailing `;` | mail f11ad335, 63a32cbf | `vat/ec_sales_list.cspec` |
| SRU upload is two files: INFO.SRU + BLANKETTER.SRU (SKV 269) | mail f11ad335 | `ink2/sru_files.cspec` |
| SKV 269 xls files and BAS INK2 interval files are already downloaded | old repo `skv_specs/`, `resources/BAS/` | `ink2/sru_files.cspec` sources |
| Annual report = fastställelseintyg + förvaltningsberättelse + RR + BR + noter | mails 7cdfdf41, a661d71a | `annual_report/k2_ixbrl.cspec` |
| Bolagsverket accepts digital K2 annual reports (iXBRL via API) | mail 39248095 | `annual_report_ixbrl` |
| "No codes for annual report lines": the K2 XBRL taxonomy is exactly those codes | mail 94d5fe3e | `annual_report/k2_ixbrl.cspec` |
| BAS → annual report is many-to-many and uses patterns like `54xx` | mail 9e55894e | planned `bas_to_k2` |
| Nordea CSV and SKV tax account CSV layouts, with real example rows. SKV rows mix kinds (header, balance, transaction) | old repo issue `#ISSUE20260114_SKV_CSV` | `statements/*.cspec`, `row.require_date` |
| Own accounts 9021/9022/9099 carry EU/import values for boxes 20 and 50 | mail 6d2ccfb3 | `vat/bas_to_vat_box.cspec` |
| Import VAT: box 50 = customs value + duties + extra costs, box 60 = VAT on box 50 | mail 64293332 | `vat/vat_boxes.cspec` titles |

## Bookkeeping practice that the code should support

| Hint | Source | Becomes |
|---|---|---|
| Fix date, amount and heading first; edit vouchers in an open (staged) mode before posting | mail 789d1d1b | financial events (HAD) + planned append-only journal with staging |
| Invoice and payment are two events of one business event; aggregate HADs | old repo todo 04292213 | `field.group` in `events/financial_event.cspec` |
| Propose vouchers from earlier ones (same accounts, same debit/credit pattern, same words) | mail cff894eb, todo 04292213 | `event_to_voucher` (planned, M3) |
| A voucher with two VAT rates (goods + shipping) must still be accepted | old repo todo 7154ef31 | voucher checks only require balance, not a "VAT type" |
| SIE re-import showed false "CONFLICTED" for identical vouchers (text differs only in `\|` vs `/`) | old repo todo 79d96613 | compare vouchers by accounts, amounts and dates. Fortnox changes texts |
| cratchit couldn't add accounts that are not in the SIE file; account names lived in a global | old repo todo 03bf6e28 | `Books::m_accounts` per file + `bas/account_types.cspec` |
| Natural signs for users: income +, cost − | mail a66cf09a | `sign_convention` (future) |
| Year-end checklist: reconcile 1630 with the SKV account, 1920/1930 with the bank, VAT accounts zeroed, 2440 matches open invoices, 8314 holds all SKV interest, IB = last year's UB | mails a809a814, a74f47ba, 804d6f8a | `year_end_checklist` mechanism (M4) |
| **Fortnox did not carry forward opening balances** (twice) | mails a809a814, a74f47ba | year-end check: IB(year) = UB(year−1) |
| Duplicate supplier invoice (D17/D20) was "corrected" by a new receivable instead of reversing the debt | mail 804d6f8a | correction_by_voucher must reverse the original postings |
| INK2S 4.14a: last year's confirmed deficit was missed | mail a4962c14 | `ink2/sru_files.cspec` lesson |
| Annual report: soliditet, changes in equity from 2091/2099, flerårsöversikt in tkr | mail a1851123 | `annual_report/k2_ixbrl.cspec` lessons |
| `-omslutning` per account (IN, period, OUT) is the input to the annual report sheet | mail 3da31823 | planned trial balance report (M4) |

## From take 1

- Truncate each box to whole kronor; box 49 from truncated boxes; omit zero boxes; settlement series M
  excluded; a quarter can span two SIE files. All of this is now in `vat/vat_rules.cspec`, and take 2
  proves it on 14 filed returns.
- The account → box table is now `vat/bas_to_vat_box.cspec`.
- BAS's machine-readable JSON costs money. The xlsx is free but has no account type column, so
  `bas/account_types.cspec` derives types by class and group.

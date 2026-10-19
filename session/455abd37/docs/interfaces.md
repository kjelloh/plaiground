# cratchit interfaces

The machine-readable registry is [`specs/interfaces.cspec`](../specs/interfaces.cspec).
This page is the human view. `cratchit interfaces` prints the registry, and
`cratchit check-specs` checks that it matches the code.

Status: **skeleton** = code exists and is tested · **planned** = near term (M4–M6) ·
**future** = later · **listed** = named so it is not forgotten.

## Data flow

```text
  Fortnox ──SIE 4──▶ sie4_import ──▶ sie_to_journal ──▶ ┌─────────┐ ──▶ journal_to_vat_boxes ──▶ vat_return_eskd ──▶ SKV
  Bank ────CSV────▶ bank_statement_import ─┐            │ Journal │ ──▶ journal_to_k2 ─────────▶ annual_report_ixbrl ─▶ Bolagsverket
  SKV ─────CSV────▶ skv_account_statement ─┼▶ events ──▶│ (Books) │ ──▶ journal_to_sru ────────▶ ink2_sru ───────────▶ SKV
  Receipts, user ─────────────────────────┘  (HADs)    └─────────┘ ──▶ year_end_closing (I series vouchers)
                                                          │   ▲
  Filed eSKD files ──▶ filed_vat_return_import ───────────┘   └── sie4_export ──▶ Fortnox, accountant, archive
  Reference data: bas_chart_of_accounts, bas_sru_mapping, skv_sru_field_definitions, settings
  Every output: provenance record (audit_trail)
```

## Inbound

| Interface | Counterpart | Format | Spec | Status |
|---|---|---|---|---|
| `sie4_import` | Fortnox, accountant | SIE 4, PC8/CP437 | `sie/sie4.cspec` | skeleton, M1 |
| `filed_vat_return_import` | Earlier filings | eSKD 6.0 | `vat/eskd_6p0.cspec` | skeleton, M1 |
| `bank_statement_import` | Nordea | CSV | `statements/nordea_csv.cspec` | skeleton, M3 |
| `skv_account_statement_import` | Skatteverket | CSV (later: Skattekonto API) | `statements/skv_account_csv.cspec` | skeleton, M3 |
| `financial_event_listing` | User, imports | cspec listing | `events/financial_event.cspec` | skeleton, M3 |
| `voucher_documents` | Suppliers, mail, scanner | PDF/images by hash | – | listed, M3 |
| `bas_chart_of_accounts` | bas.se | xlsx → cspec | `bas/account_types.cspec` | skeleton, M2 |
| `bas_sru_mapping` | bas.se | xlsx/pdf → cspec | (in `ink2/sru_files.cspec`) | planned, M6 |
| `skv_sru_field_definitions` | SKV 269 | xls → cspec | (in `ink2/sru_files.cspec`) | planned, M6 |
| `settings` | User | `.cratchit/settings.cspec` | `settings/settings.cspec` | skeleton, M1 |
| `user_cli` | User | `cratchit <command>` | – | skeleton, M1 |

## Outbound

| Interface | Counterpart | Format | Spec | Status |
|---|---|---|---|---|
| `vat_return_eskd` | SKV, file upload | eSKD 6.0, ISO-8859-1 | `vat/eskd_6p0.cspec`, `vat/vat_boxes.cspec` | skeleton, M1 |
| `vat_return_api` | SKV Momsdeklaration API (live since Nov 2025) | REST/JSON | – | future |
| `ec_sales_list` | SKV Filöverföring | `SKV574008` text file | `vat/ec_sales_list.cspec` | future |
| `employer_declaration` | SKV (AGI) | XML or API | – | listed |
| `sie4_export` | Fortnox, accountant, archive | SIE 4 | `sie/sie4.cspec` | planned, M2 |
| `year_end_closing` | The books | I series vouchers + checklist | – | planned, M4 |
| `annual_report_ixbrl` | Bolagsverket | iXBRL, K2 taxonomy | `annual_report/k2_ixbrl.cspec` | planned, M5 |
| `ink2_sru` | SKV Filöverföring | INFO.SRU + BLANKETTER.SRU | `ink2/sru_files.cspec` | planned, M6 |
| `ledger_reports` | User, auditor | text/Markdown | – | listed, M2 |
| `audit_trail` | User, auditor | cspec log | – | listed, M2 |

## Internal transformations

| Interface | From → to | Spec | Status |
|---|---|---|---|
| `sie_to_journal` | SIE records → Books | `sie/sie4.cspec` (`use = read`) | skeleton |
| `statement_to_events` | CSV rows → financial events | statement specs | skeleton |
| `event_to_voucher` | events + earlier vouchers → voucher candidate | – | planned, M3 |
| `journal_to_vat_boxes` | postings → boxes | `vat/bas_to_vat_box.cspec`, `vat/vat_rules.cspec` | skeleton |
| `journal_to_k2` | balances → K2 line items | `annual_report/k2_ixbrl.cspec` | planned, M5 |
| `journal_to_sru` | balances → SRU fields | `ink2/sru_files.cspec` | planned, M6 |
| `sign_convention` | BAS debit/credit ↔ natural signs | `bas/account_types.cspec` | future |

## How an interface is pinned down

1. A spec file in `specs/` says *what* crosses the interface: fields, types, encodings, mappings,
   rules. It names its sources (`spec.source.*`) and how sure we are (`spec.status`, `….status`).
2. Code reads the spec at run time where that is practical. Examples: SIE field lists and types,
   VAT box elements, eSKD element order, account → box mapping, CSV columns, settings.
   Where code can't read the spec, a check makes sure the two agree. Examples: SIE `use = read`
   against `sie_to_journal`, legal mechanisms against `legal::mechanisms()`, and reports against
   the interface status.
3. A test with real or SKV-published data proves the spec. For example, `tests/real_files_test.cpp`
   reproduces all 14 filed VAT returns.
4. Every generated file gets a provenance record listing the inputs, spec versions and vouchers used.

To add an interface: register it in `specs/interfaces.cspec`, write its spec, write the code,
then run `cratchit check-specs`.

## Generating specs from external sources (next step)

| Source (free, official) | Becomes | Tool (to write) |
|---|---|---|
| BAS kontoplan xlsx (bas.se) | `bas/accounts_<year>.cspec`: names, K2 flag | xlsx → cspec converter |
| BAS "INK2 P1 intervall" xlsx (bas.se/kontoplaner/sru) | `ink2/bas_to_sru_<year>.cspec` | xlsx → cspec |
| SKV 269 xls per form (INK2, INK2R, INK2S) | `ink2/fields_<form>_<year>.cspec` | xls → cspec |
| K2 XBRL taxonomy (Bolagsverket/taxonomier.se) | `annual_report/k2_elements_<version>.cspec` | xsd → cspec |
| SIE 4C PDF (sie.se) | `sie/sie4.cspec` checked by hand | – |

Each generated file records the tool, its version and the source file's fingerprint in `spec.derived_by`
and `spec.source.*`, so a regenerated spec shows up as a diff.

## Open questions

- **Q1** Converter tooling: write the xlsx/xls → cspec converters in C++ (a cratchit sub-command)
  or as small Python scripts next to the specs? Python is quicker. C++ keeps one toolchain.
- **Q2** The BAS → K2 annual report mapping: BAS sells a kopplingstabell, and the BOKSLUTSBOKEN
  pages are in todo mail 9e55894e. Do we build `bas_to_k2` by hand from your own Numbers
  sheet `Årsredovisning.numbers` (todo mail 0830a134)?
- **Q3** Should the journal stay "SIE file is the truth" (Fortnox-first, as now), or should cratchit
  become the book of record with append-only vouchers? This decides how much of the legal
  scaffolding cratchit itself must implement.
- **Q4** BAS account meanings change over time. In 2016 our books had 2615 for EU goods; BAS 2026
  uses 2615 for import VAT. Should `bas_to_vat_box.cspec` be split per BAS version with an
  `applies.from` date?
- **Q5** take 1's quarters with differences (202506, 202606 and some older ones, see
  `../62c6984e/_local/vat_findings.md`): none of the filed files here hit them. Investigate when
  the next return is filed?

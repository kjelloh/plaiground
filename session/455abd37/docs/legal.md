# Legal requirements and how cratchit adheres to them

The machine-readable list is [`specs/legal/requirements.cspec`](../specs/legal/requirements.cspec).
Each **requirement** points to one or more **mechanisms**. A mechanism is something cratchit does,
or will do, to comply. The code-side list of mechanisms is `legal::mechanisms()` in
`src/legal/compliance.cpp`, and `cratchit check-specs` checks that it agrees with the spec.

> Scaffolding, not legal advice. Every summary is marked `unverified` until someone checks it
> against the law text (sources are listed in the spec header).

| Requirement | What it asks (short) | Mechanisms | Mechanism status |
|---|---|---|---|
| BFL 4:1 | Bookkeeping duty: record, vouchers, close, archive | voucher checks, append-only journal, archive | partly implemented |
| BFL 5:1 | Registration order and account order | registration order, ledger listing | planned |
| BFL 5:2 | Cash next working day, other events as soon as possible | booking lag warning | planned |
| BFL 5:5 | Correct, never erase; show when and by whom | append-only journal, correction voucher, audit log | planned |
| BFL 5:6 | A voucher for every business event | voucher fields, document link | fields implemented |
| BFL 5:7 | What a voucher must contain | voucher fields, document link | fields implemented |
| BFL 5:11 | System documentation and processing history | spec files, provenance records, audit log | partial |
| BFL 6:1 | An AB closes the year with an annual report | year-end checklist | planned |
| BFL 7:1–2 | Keep accounting records 7 years, durable and accessible | archive retention, document link | planned |
| ÅRL 2:1 | Parts of the annual report | K2 structure | planned |
| ÅRL 8:3 | File the adopted report with Bolagsverket | deadline calendar | planned |
| SFL 26 kap. | VAT return each period, on time | VAT rules from spec, deadline calendar, provenance | partial |
| ML (2023:200) | Taxable supplies, rates, reverse charge, deduction | VAT rules from spec | partial |
| SFL (INK) | Income tax return each year | deadline calendar | planned |

## What already works

- **Voucher checks** (`legal::check_voucher`): every voucher balances, and has series, number, text
  and at least two postings. Findings go into the provenance record of every VAT return.
- **Provenance records** (`*.provenance.cspec` next to each output): input files with fingerprints,
  spec ids, versions and status, and the vouchers used. This is the first part of *behandlingshistorik*.
- **Spec files as system documentation**: the `specs/` folder describes, line by line, how cratchit turns
  input into output, and git keeps its history. This is the first part of *systemdokumentation*.

## The design choice behind the rest (open question Q3)

As long as Fortnox holds the books and cratchit reads SIE files, Fortnox carries most of the BFL duties
(registration order, immutability, archive). Once cratchit books vouchers itself, it needs the planned
mechanisms first: an append-only journal, correction vouchers and an audit log.

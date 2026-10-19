# cratchit take 2: a baseline with interfaces pinned down from the start

A Swedish bookkeeping app in C++ for an aktiebolag. Every interface (to files, to Skatteverket,
to Bolagsverket, and between internal steps) is written down in a plain-text spec file **before**
the code that uses it. The code reads those specs, and tests check that the code and the specs agree.

## Where to start

| Read | For |
|---|---|
| [`docs/interfaces.md`](docs/interfaces.md) | All interfaces, the data flow, and open questions |
| [`docs/cspec_format.md`](docs/cspec_format.md) | The `path = value` spec format |
| [`docs/legal.md`](docs/legal.md) | Legal requirements (BFL, ÅRL, SFL, ML) and how cratchit adheres |
| [`docs/hints_from_previous_work.md`](docs/hints_from_previous_work.md) | What the Todo mails, the old repo and take 1 taught us |
| [`specs/`](specs) | The spec files themselves |

## Layout

```text
specs/                       machine-readable specs (.cspec)
  interfaces.cspec           registry of every interface
  legal/requirements.cspec   requirements → mechanisms
  sie/sie4.cspec             SIE 4 records, fields, types
  vat/                       boxes, BAS → box mapping, rules, eSKD layout, EC sales list
  statements/                Nordea and SKV tax account CSV layouts
  events/                    financial event (HAD) listing schema
  bas/account_types.cspec    account type by class/group
  settings/settings.cspec    settings schema with defaults
  ink2/, annual_report/      planned: INK2 SRU files, K2 iXBRL
src/
  cspec/                     spec file parser
  core/                      Amount (öre), Date, accounts, Books, FinancialEvent, AccountTypes
  sie/                       spec-driven SIE reader, CP437, SIE → Books
  vat/                       VAT spec loader, calculation, eSKD writer and reader
  statements/                spec-driven CSV statement reader
  reports/                   ReportGenerator registry: vat (done), bokslut, arsredovisning, ink2 (planned)
  legal/                     mechanisms and voucher checks
  app/                       settings, spec/code consistency check
  main.cpp                   CLI
tests/                       GoogleTest; real-data tests skip when the local files are absent
testdata/                    fictional examples + SKV's public eSKD example 6
```

## Build and run

```sh
conan install . --build=missing -s compiler.cppstd=23
cmake --preset conan-release
cmake --build --preset conan-release
ctest --preset conan-release

./build/Release/cratchit check-specs
./build/Release/cratchit interfaces
./build/Release/cratchit report vat --period 202409 --sie testdata/example.se --out _to_skv
./build/Release/cratchit events nordea_csv testdata/nordea_example.csv
```

## Status (2026-10-19)

- Builds with zero warnings (`-Wall -Wextra -Wpedantic -Wconversion -Wshadow`). 26 tests pass.
- `check-specs`: specs and code agree.
- On local real data: all 11 Fortnox SIE files parse under the strict spec. All 14 filed VAT returns
  (2020–2025) are reproduced value for value, 11 of them byte for byte (the other 3 write zero boxes explicitly).

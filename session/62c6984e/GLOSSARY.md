# Glossary

Swedish domain term ↔ English ↔ name used in code. Keep code names consistent with this table.

| Swedish | English | Code name | Notes |
|---|---|---|---|
| Bokföring | Bookkeeping | | |
| Räkenskapsår | Financial year | `FinancialYear` | Here 1 May – 30 April. SIE `#RAR` |
| Verifikation | Voucher | `Voucher` | SIE `#VER` |
| Verifikationsserie | Voucher series | `VoucherSeries` | e.g. `A` |
| Kontering / transaktion | Posting / transaction | `Posting` | SIE `#TRANS` |
| Konto | Account | `Account` | SIE `#KONTO` |
| Kontoplan (BAS) | Chart of accounts (BAS) | `ChartOfAccounts` | |
| Ingående balans | Opening balance | `OpeningBalance` | SIE `#IB` |
| Utgående balans | Closing balance | `ClosingBalance` | SIE `#UB` |
| Resultat | Result (P&L balance) | | SIE `#RES` |
| Periodsaldo | Period balance | | SIE `#PSALDO` |
| SRU-kod | SRU code (tax form mapping) | `SruCode` | SIE `#SRU` |
| Moms | VAT | `Vat` | |
| Utgående moms | Output VAT | `OutputVat` | BAS 261x–263x |
| Ingående moms | Input VAT | `InputVat` | BAS 264x |
| Momsdeklaration | VAT return | `VatReturn` | |
| Ruta | Box (on a tax form) | `Box` | e.g. ruta 48 |
| Redovisningsperiod | Reporting period | `VatPeriod` | eskd `<Period>` YYYYMM |
| Skattekonto | Tax account (at SKV) | `TaxAccount` | BAS 1630 |
| Skatteverket (SKV) | Swedish Tax Agency | | |
| Bokslut | Year-end closing | `YearEndClosing` | |
| Årsredovisning | Annual report | `AnnualReport` | |
| Inkomstdeklaration 2 (INK2) | Corporate income tax return | `Ink2` | |
| Organisationsnummer | Corporate ID number | `OrgNr` | |
| Öre | 1/100 krona | | Amounts stored as integer öre / cents |

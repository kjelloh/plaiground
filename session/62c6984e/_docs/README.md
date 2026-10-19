# Reference documents

## SIE file format

- `SIE_filformat_ver_4C_2025-08-06.pdf` — official spec from SIE-Gruppen (40 pages).
  Source: https://sie.se/wp-content/uploads/2026/02/SIE_filformat_ver_4C_2025-08-06.pdf
- Older version 4B: https://sie.se/wp-content/uploads/2020/05/SIE_filformat_ver_4B_080930.pdf
- Our Fortnox exports declare `#SIETYP 4` and `#FORMAT PC8` (CP437).

## SKV VAT return file (eSKDUpload 6.0)

- Source: Skatteverket, "Lämna momsdeklaration via fil i e-tjänsten"
  https://www.skatteverket.se/foretag/moms/deklareramoms/skapaochskickainmomsdeklarationviafil.4.2fb39afe18dabf1e4d223cc.html
- The DTD URL in our filed eskd files (`https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd`)
  returns 404 as of 2026-10-19. **Missing:** the DTD itself (defines element order).

**Caution:** the summary below was extracted by a web tool, not copied by hand.
Check it against the SKV page before relying on it.

File structure:

```xml
<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE eSKDUpload PUBLIC "-//Skatteverket, Sweden//DTD Skatteverket eSKDUpload-DTD Version 6.0//SV" "https://www1.skatteverket.se/demoeskd/eSKDUpload_6p0.dtd">
<eSKDUpload Version="6.0">
<OrgNr>NNNNNN-NNNN</OrgNr>
<Moms>
<Period>YYYYMM</Period>
... one element per non-empty box ...
</Moms>
</eSKDUpload>
```

(Our filed files include the DOCTYPE line and put everything after it on one line.
SKV's own examples have no DOCTYPE.)

Amounts: whole kronor, digits only, optional leading `-` (no space).

| Ruta | Element | Meaning |
|---|---|---|
| 05 | `ForsMomsEjAnnan` | Taxable sales not in 06–08 |
| 06 | `UttagMoms` | Taxable withdrawals |
| 07 | `UlagMargbesk` | Taxable base, profit-margin taxation |
| 08 | `HyrinkomstFriv` | Rental income, voluntary tax liability |
| 10 | `MomsUtgHog` | Output VAT 25 % |
| 11 | `MomsUtgMedel` | Output VAT 12 % |
| 12 | `MomsUtgLag` | Output VAT 6 % |
| 20 | `InkopVaruAnnatEg` | Purchase of goods from another EU country |
| 21 | `InkopTjanstAnnatEg` | Purchase of services from another EU country |
| 22 | `InkopTjanstUtomEg` | Purchase of services from outside the EU |
| 23 | `InkopVaruSverige` | Purchase of goods in Sweden, reverse charge |
| 24 | `InkopTjanstSverige` | Purchase of services in Sweden, reverse charge |
| 30 | `MomsInkopUtgHog` | Output VAT 25 % on boxes 20–24 |
| 31 | `MomsInkopUtgMedel` | Output VAT 12 % on boxes 20–24 |
| 32 | `MomsInkopUtgLag` | Output VAT 6 % on boxes 20–24 |
| 35 | `ForsVaruAnnatEg` | Sale of goods to another EU country |
| 36 | `ForsVaruUtomEg` | Sale of goods outside the EU |
| 37 | `InkopVaruMellan3p` | Intermediary purchase, triangulation |
| 38 | `ForsVaruMellan3p` | Intermediary sale, triangulation |
| 39 | `ForsTjSkskAnnatEg` | Sale of services to taxable person in another EU country |
| 40 | `ForsTjOvrUtomEg` | Other sales of services outside Sweden |
| 41 | `ForsKopareSkskSverige` | Sales where buyer is liable in Sweden |
| 42 | `ForsOvrigt` | Other sales etc. |
| 48 | `MomsIngAvdr` | Input VAT to deduct |
| 49 | `MomsBetala` | VAT to pay (+) or receive (−) |
| 50 | `MomsUlagImport` | Taxable base on import |
| 60 | `MomsImportUtgHog` | Output VAT 25 % on import |
| 61 | `MomsImportUtgMedel` | Output VAT 12 % on import |
| 62 | `MomsImportUtgLag` | Output VAT 6 % on import |
| — | `TextUpplysningMoms` | Free text, max 300 characters |

## BAS chart of accounts

- Each SIE export has the full `#KONTO` list with names and `#SRU` codes, so no separate BAS file is needed for now.
- `BAS_kontoplan_2026_v2.xlsx` — free download from https://www.bas.se/kontoplaner/ .
  One sheet, layout made for print: columns A/B = main account + name, C/D = sub-account + name,
  `#` suffix = not allowed under K2, plus group heading rows (e.g. `10 IMMATERIELLA …`).
  **No account type column.**
- `Kontoplan-Dataformat.pdf` — spec of BAS's machine-readable `listOfAccounts.json`
  (fields incl. `normalBalance` Debet/Kredit, `contraAccounts`, `SRU` per form NE/INK2/INK3/INK4).
  The JSON itself is **paid** (4 000 SEK excl. VAT for 2026):
  https://www.bas.se/kontoplaner/kontoplanen-i-maskinlasbart-format/
- Account type from account class (BAS convention):

  | Class | Meaning | SIE `#KTYP` |
  |---|---|---|
  | 1 | Assets (tillgångar) | T |
  | 2 | Equity and liabilities (eget kapital och skulder) | S |
  | 3 | Operating income (intäkter) | I |
  | 4–7 | Operating costs (kostnader) | K |
  | 8 | Financial items, tax, result — mixed: income (I) and cost (K) | I / K per account **(?)** |

  Our Fortnox exports contain no `#KTYP`, so cratchit must derive the type.
  **(?)** Check in the SIE 4C spec how a missing `#KTYP` should be handled.

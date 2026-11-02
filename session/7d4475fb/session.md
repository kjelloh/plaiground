# Consider a cratchit app that maps an SIE file to a Swedish SKV Ink_2?

```#7d4475fb```

## 20261102

It seems I am now ready to tackle the task of parsing an SIE file and create an SKV VAT Returns Report 'Inkomstdeklaration 2'?

* I decide to download and use [BAS mapping to SRU and INK2 form boxes](https://www.bas.se/wp-content/uploads/2024/11/INK2_P1-231002_241119.pdf)

We should be able to parse the BAS ranges in this file using the same parser as the cratchit_ar app developed in session 5b35b2b9 (See ../session/5b35b2b9/cratchit)?

So we can ask Claude to clone relevant code and make a 'cratchit_ink2' app?

* It shall mirror the approach taken by cratchit_ar app developed in session/5b35b2b9 (See ../session/5b35b2b9/cratchit)

    * Clone and re-use the SIE file parser and code it depends on.

* I created the initial scaffolding for a 'BAS to INK2 SRU and Form id' csv file

    * [bas_2023_to_ink2_2026](./cratchit/Resources/bas_2023_to_ink2_2026.csv)
    * Naming is not easy?
    * The document from BAS states it is for BAS 2023
    * But it is unclear if the mapping is valid to SKV INK2 as of 2026
    * For now I will assume it is as it is the one shown today?
    * We may have to come back to naming (version control of BAS vsn INK2 later)
    * It may be the case that Swedish accounting apps has to patch on some processing of their own to ensure correctness?
    * For now, fingers crossed, these mappings changes so little and seldom that we will get accurate result in any case (close enough?)

Anyhow, I have now created some folder and project scaffolding.

.
├── _Resources
│   ├── _bas
│   │   ├── INK2_P1-231002_241119.csv               // BAS official, exported as csv
│   │   ├── INK2_P1-231002_241119.pdf               // BAS Official, downloaded pdf
│   │   └── INK2_P1-231002_241119.xlsx              // BAS official, downloaded excel
│   ├── _sie
│   │   ├── TheITfiedAB20230503_152345.se           // Reference SIE file
│   │   ├── TheITfiedAB20240727_213539.se           // Reference SIE file
│   │   ├── TheITfiedAB20250817_172409.se           // Reference SIE file
│   │   └── sie_2025-26_all.se                      // Reference SIE file
│   └── _skv
│       └── 2002_33_web_2025-05-01_2026-04-30.pdf   // SKV INK2,INK2R,INK2S form
├── cratchit
│   └── Resources
│       └── bas_2023_to_ink2_2026.csv
└── session.md

So now we have something we can start working with.

* Create a C++ app called 'cratchit_ink2'
* Clone SIE File parser and BAS account range parser from cratchit_ar (@../session/5b35b2b9/cratchit)
* The app shall parse an SIE file and produce the amounts for the SKV INK2 form.
* It shall use the [bas_2023_to_ink2_2026](./cratchit/Resources/bas_2023_to_ink2_2026.csv) to map BAS accounts to SKV INK2 form entries (and associated SRU codes 'SKV fältkoder')
* The SRU codes can then later be mapped to other formats defined by SKV to electronically upload the INK2 to SKV.

As a start we can make cratchit_ink2 print the INK2 form text and amounts to the console?

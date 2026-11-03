# Consider a cratchit app that maps an SIE file to a Swedish SKV Ink_2?

```#7d4475fb```

## 20261103

I have now also added my INK2 for 2022 to 2025 (four years)

* So our session _Resources (NOT in git repo = local only) should now be complete to cross reference SIE files to actual historical INK2 (Fingers crossed without any known larger errors).

```sh
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/7d4475fb % tree _Resources 
_Resources
├── _bas                                // bas.se official mapping to INK2
│   ├── INK2_P1-231002_241119.csv
│   ├── INK2_P1-231002_241119.pdf
│   └── INK2_P1-231002_241119.xlsx
├── _ink2                               // My historical INK2 as SRU files
│   ├── deklaration 2022
│   │   ├── blanketter.sru
│   │   └── info.sru
│   ├── deklaration 2023
│   │   ├── blanketter.sru
│   │   └── info.sru
│   ├── deklaration 2024
│   │   ├── 20240727
│   │   │   ├── blanketter.sru
│   │   │   └── info.sru
│   │   ├── Korrigerad 20241130
│   │   │   ├── blanketter.sru
│   │   │   └── info.sru
│   │   └── The ITfied AB 556782-8172
│   │       ├── blanketter.sru
│   │       └── info.sru
│   └── deklaration 2025
│       ├── BLANKETTER.SRU
│       └── INFO.SRU
├── _sie                                // My historical SIE files
│   ├── TheITfiedAB20221124_192100.se
│   ├── TheITfiedAB20230503_152345.se
│   ├── TheITfiedAB20240727_213539.se
│   ├── TheITfiedAB20250817_172409.se
│   └── sie_2025-26_all.se
└── _skv                                // SKV form(s) (empty)
    └── 2002_33_web_2025-05-01_2026-04-30.pdf   // INK2 form

12 directories, 20 files
kjell-olovhogdahl@MacBook-Pro ~/Documents/GitHub/plaiground/session/7d4475fb % 
```

So now we can ask Claude to implement a C++ console app that parses an SIE file and generates an INK2 'Inkomstdeklaration'.

* The app shall be named 'cratchit_ink2'
* The app shall clone code from '../session/5b35b2b9/cratchit' to parse SIE files and BAS account ranges.
* The app shall be cmake based
* The app shall consume conan package manager packages when appropriate
* The app shall implement its own logic where using a shared package is not worth the price.
* The app shall provide a user manual and an help argument option.
* Claude shall use a progress.md to keep track of progress.
* Claude is allowed to read '/Users/kjell-olovhogdahl/Resilio Sync/Dropbox/The ITfied/Köpta Digitala Media/BAS/svensk_redovisning_bokforing.pdf' e-book for BAS account specifications.

It seems [Teknisk information om filöverföring](https://www.skatteverket.se/foretag/inkomstdeklaration/forredovisningsbyraer/tekniskinformationomfiloverforing.4.13948c0e18e810bfa0cca8.html) provides zip-archives with technical specifications for SRU-files that SKV supports in its file upload service?

* Claude also found this page and downloaded specs for INK2R to verify rounding rules.

So now Claude has created the cratchit_ink2 app that takes an SIE file and produces a printout of IN2R ok.

* It seems it is the INK2R we actually can generate from SIE file (the books)?
* The INK2S seems to be 'skattemässiga justeringar' that needs amounts kept outside the books and relates to how we have chosen to apply Tax Return rules?
    * It seems we should be able to provide some TBD mechanism as in cratchit_ar to begin with.
    * And form that design a mechanism to keep track of and provide amounts and data required to create a full INK2?

I also now started to think about what checks teh app shall do to ensure a valid INK2 is created.

* It seems we need to detect amounts on BAS account that does not map to anything on INK2.
    * This should be an error?
    * We should not have amounts in the books that does not show up on INK2?
    * Can we verify this somehow?
* It seems we need to validate the BAS to SRU in the SIE file against the mapping we have from bas_to_ink2 csv-file?
    * The SIE file should take precedence?
    * AS the SIE file seems to define the BAS account plan the user actually uses.
    * Still, if the SIE file maps to and SRU thats is not on the INK2, this is an error?
    * Also, we should detect mapping in SIE file that breaks with the bas_to_ink2 mapping?
    * It seems the user may map added BAS accounts as long as the mapping dows not break with bas_to_ink2 official mapping.
    * And the app is allowed to 'fill in the blanks' when teh usr SIE file uses accounts with missing SRU mapping (use bas_to_ink2 mapping)
        * We should inform the user so they can update the account plan.

I now pressed on.

* Claude now made cratchit_ink2 generate FULL INK2 (first page, INK2r and INK2S)
* It reads last year SRU-file or argument --previous-deficit to chain in accumulated (saved) deficit over years


It does NOT YET generate SRU-files to upload to SKV and read in for coming year INK2 generation.

Claude asked for the document SKV 269 that specifies the upload interface and file format for SRU files to SKV.

* [Tekniskt beskrivning − filöverföring av Inkomstdeklaration 1 och 2 samt bilagor till Inkomstdeklaration 3 och 4](https://www.skatteverket.se/download/18.96cca41179bad4b1aac472/1708607388657/Använd%20ejSKV269_27_(2021P4).pdf)
* I placed it in _Resources/_skv

BUT! It seems the SRU file format for INK2 terminates Dec 2026?

```text
Tjänsten Inkomstdeklaration 2 avvecklas 29 december 2026
Använd gärna någon av våra andra digitala tjänster för att deklarera. Det kan du göra redan nu.
```

* It is a bit confusing?
* But it seems information on this webpage replaces SKV 269 [Teknisk information om filöverföring](https://www.skatteverket.se/foretag/inkomstdeklaration/forredovisningsbyraer/tekniskinformationomfiloverforing.4.13948c0e18e810bfa0cca8.html)

    * It has a specific text about SKV 269 being replaced by this page?

    ```text
    Den här sidan ersätter broschyren Teknisk information om filöverföring (SKV 269)
Broschyren Information om filöverföring av Inkomstdeklaration 1 och 2 samt bilagor och fritextbrev till Inkomstdeklaration 1, 2, 3 och 4 (SKV 269) har utgått från och med den 1 januari 2024. Informationen på den här sidan ersätter innehållet i broschyren.
    ```

    * AND: The SRU-file content seems to be defined ON THE WEB PAGE ITSELF?!!

BUT - Claude picked this up by itself!

```text
Step 4b is done: the app now writes the return as SRU files, and each year's folder works as next year's "last year's return"
...
What a run writes (in the working directory, as cratchit_ar does)
556782-8172_2024-05-01_2025-04-30/
    BLANKETTER.SRU   blocks INK2, INK2R, INK2S (e.g. INK2R-2025P1)
    INFO.SRU         #ORGNR, #NAMN, #POSTNR, #POSTORT
    556782-8172_2024-05-01_2025-04-30.se   copy of the SIE file
- Format is from SKV's page "Teknisk information om filöverföring", which replaced SKV 269 in 2024:
  - ISO 8859-1, with the orgnr written 165567828172
  - negative amounts with -
  - block names and periods (P1 for a year ending 30 April, and so on) as in your filed returns
...
```

So next step is for me to test the generated files with SKV 'file upload test site' [Filöverföringstjänst](https://www1.skatteverket.se/fv/fv_web/login.do)

* NOTE: I need to be logged in!
* It is unclear if this is the same web service as the live one?
* But I can upload without saving to test files (so either way is ok?)

But this is SLOPPY by SKV?!!

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

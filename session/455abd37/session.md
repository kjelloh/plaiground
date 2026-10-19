# Consider a vibe coded book keeping app 'cratchit' take 2?

```#455abd37```

## 20261019

I want to tackle vide coding a swedish book keeping app in a more 'based' way.

* We still want a to-the-point swedish book keeping app in C++.
* But I want to ensure we pin down all the interfaces as correctly as we can already from start!

Being a Swedish book keeping app there are a lot of technical and juridical details to comply with!

1. What does the cratchit app interface?

    * Hint: It must be able to read and write SIE-files.
    * Hint: It must enable a user to perform book keeping operations.
    * Hint: It must be able to interface SKV to provide reports (e.g., eskd-files but also XML-files or web APIs if such exist?)
    * Hint: It must be able to transform and/or map information between interfaces (e.g.)
    * What more should we list and pin down right at the beginning?

2. What do we know from before?

    * Hint: We have a lot of writings as Todo-mails, locally at '/Users/kjell-olovhogdahl/Downloads/Privat/arkiv/filer/mail_export/todo_mails/markdown_domain/_mail/index.md'
    * Hint: We have previous work on a cratchit app locally at '/Users/kjell-olovhogdahl/Documents/GitHub/cratchit'
    * We have the previous session [Consider vibe coded book keeping app cratchit take 1?](62c6984e/session.md) with its produced documents and files.

3. What can we do to pin down how we interact with each interface?

    * Can we imagine a local text file that defines how cratchit implements each interface?
    * Can we generate these text files from external specs?
    * Can we figure out those specs from hinted previous information and code?

So what could be a first base line for a C++ swedish book keeping app that is very clear in code and through its own spec text files how it processes data to and from files and SKV?

I now asked Claude to read my introduction above at it came back very reassuring.

```sh
I've read session.md and skimmed the sources it points to.

What I understand

This is take 2 of cratchit, a Swedish bookkeeping app written in C++. Take 1 (session 62c6984e) produced working code quickly: SIE parsing, a VAT return, an eskd writer and passing tests. But it left you unsure whether the output actually meets SKV's requirements, and whether BAS accounts map correctly to VAT boxes and eskd. The unsolved problems were BAS account types (especially class 8, and Fortnox exports with no #KTYP) and the BAS → VAT box mapping.

So the goal this time is to pin down every interface before writing code:

1. List what cratchit interfaces with. This covers SIE read and write, the bookkeeping operations a user performs, and reports to SKV (eskd, XML, maybe web APIs). It also covers the transformations between these, plus anything else that should be listed from the start.
2. Gather what we already know.
   - Your Todo mails: 3,206 entries. About 76 index lines match bookkeeping or tax keywords, though some hits are probably noise.
   - The old cratchit repo: it has sie_specs/, skv_specs/ and resources/. In resources/ I found 2021_SRU.csv, BAS chart-of-accounts files for 2022 and 2024, SKV XML examples and annual-report material. The repo also has a docs/thinking/ folder.
   - The take 1 session: SPEC.md, GLOSSARY.md, _eskd_irl and other artifacts.
3. Write a local "interface spec" text file per interface. Each file would be readable by people, generated or derived from external specs where possible, and would drive or document how cratchit processes data. The aim is that the code and these specs together show clearly what goes in and out.

The deliverable for this session is a first baseline: the interface inventory, the spec-file concept, and how the C++ code relates to the specs.

Questions

1. Wrong link? Under item 2, session.md calls the previous session "Consider how to have a github push trigger social media postings – take 1" (771ae935). That one is about GitHub → social media. Did you mean the cratchit take 1 session 62c6984e? Or is something from the social-media session relevant, such as its event/queue thinking?
2. What should this session produce? Should it be only documents (an interface inventory, a spec-file format, an architecture note), or should it also include a code skeleton? And should take 1's code be reused or deliberately ignored?
3. How much of the legal side to cover? Should the inventory include the legal requirements as interfaces too? For example: Bokföringslagen (BFL) rules on verifikationer, how long records must be kept, systemdokumentation and behandlingshistorik, and immutability once entries are booked. Plus BFNAR guidance (K1, K2, K3).
4. Which reports to SKV? Momsdeklaration (eskd / XML / SKV's API), arbetsgivardeklaration (AGI), INK2 with SRU files, the annual report to Bolagsverket (iXBRL), and the K10 or private tax return? Which ones matter for milestone 1, and which are just "list now, build later"?
5. What other inputs? Bank exports (CSV, ISO 20022 camt), receipts and PDFs, and Fortnox exports beyond SIE. Should these count as interfaces?
6. BAS licensing: take 1 found that the BAS chart of accounts isn't freely available. Can the spec files be built from the BAS files you already have in cratchit/resources/, or should cratchit define its own account-type template and only refer to BAS?
7. Spec file format: do you have a preference (Markdown with embedded tables, TOML/YAML, CSV, or a custom DSL)? Should the C++ code load these files at runtime, or are they documentation that tests check the code against?
8. The Todo mails: should I search all 3,206 for bookkeeping-related content, or do you know of specific threads, such as cratchit-tagged ones?

My suggested first step: write interfaces.md in this session folder, listing every interface with its external spec source, direction, and what we already have locally. Then we choose a spec-file format together.
```

I answered Claudes questions and off it went!

* Claude really went to work and produced a lot of code and files!

```sh
I've built a first baseline in session/455abd37/: an interface inventory, spec files in your path = value format, legal scaffolding, and a C++ skeleton that reads its behaviour from those spec files. It builds with no warnings, all 26 tests pass, and cratchit check-specs reports that specs and code agree. Nothing is committed and session.md is untouched.

The strongest result: the spec-driven VAT code reproduces all 14 VAT returns in your local _eskd_irl folder (2020–2025) with the same values, and 11 of them byte for byte. The other three (two from 2020–21 and one from the old cratchit) also write the zero boxes. Take 1 had verified 3 returns. All 11 real Fortnox SIE files parse under the strict SIE spec.

What's in the baseline:
- Interfaces: about 30, registered in specs/interfaces.cspec and described in docs/interfaces.md. Each one lists:
  - its direction (in, out or internal) and counterpart
  - its format and spec file
  - the code that implements it, its legal requirements and its milestone

  Besides SIE, eSKD, bank and tax-account statements and the financial-event listing, it also lists:
  - SKV's VAT return API, which has been live since November 2025
  - the EC sales list (periodisk sammanställning)
  - the employer declaration (AGI)
  - INK2 as SRU files
  - the annual report to Bolagsverket as Inline XBRL (iXBRL, the format for digital filing) using the K2 taxonomy
  - an audit trail, and reading back already-filed returns
- Spec format (docs/cspec_format.md): one path = value per line. Every file names its sources and marks how sure it is (draft, checked or verified, with .status/.note on single entries). Each line stands alone, so you can grep it and diffs stay clear.
- Spec-driven code: at run time the code reads:
  - the SIE field lists and types
  - the VAT boxes and the BAS account → box mapping, plus the rounding rules
  - the eSKD element order, the CSV columns and the settings

  Where code can't read a spec, check-specs verifies they match. For example, the SIE records the code handles are checked against the spec, and the legal mechanisms in code against requirements.cspec.
- Reports: VAT return works. Bokslut, Årsredovisning and INK2 are registered as planned, so each one only needs its generate() written.
- Legal: requirements from BFL, ÅRL, SFL and ML each point to a mechanism in cratchit. Voucher checks (balance and required fields) and a provenance file next to every output already work. The append-only journal, corrections, audit log and archiving are planned.
- Your notes: docs/hints_from_previous_work.md maps about 25 findings from the Todo mails, the old repo and take 1 to where they are used. Examples: Fortnox not carrying forward opening balances, your 9021/9022 accounts, and the K2 taxonomy being the "codes" you were missing in 2019.

Not yet checked:
- The legal summaries are marked unverified. I wrote them from memory and notes, not from the law text.
- I couldn't render the SIE 4C PDF here, so sie4.cspec was written from 4B and take 1. It is proven by your real files, not yet compared to 4C.
- The Bolagsverket and taxonomier.se URLs are not checked.
- Some spec sources use absolute paths under your home folder.

Open questions (also at the end of docs/interfaces.md):
1. Should the converters that generate spec files from BAS and SKV xlsx/xls be written in C++ or Python?
2. Should we build the BAS → K2 mapping by hand from your Årsredovisning.numbers?
3. Is the SIE file from Fortnox still the truth, or should cratchit become the book of record? This decides how much of the BFL scaffolding cratchit must implement itself.
4. BAS account meanings change over the years (2615 meant EU goods purchases in 2016 but import VAT in BAS 2026). Should the VAT mapping be split by BAS version?
5. Should we look into take 1's mismatched quarters (for example 202506 and 202606) now? None of the 14 filed returns hit them.

Start with README.md, then docs/interfaces.md.

Sources:
- Skatteverket: VAT return API (Fortnox article) (https://www.fortnox.se/fortnox-foretagsguide/driva-foretag/skatt/forenkla-momsdeklarationen-med-skatteverkets-nya-api)
- Skatteverket: VAT return API terms (https://www7.skatteverket.se/portal-wapi/open/apier-och-oppna-data/utvecklarportalen/v1/getFile/allmanna-villkor-momsdeklaration)
- Skatteverket: tax account API service description (https://www7.skatteverket.se/portal-wapi/open/apier-och-oppna-data/utvecklarportalen/v1/getFile/tjanstebeskrivning-skattekonto)
- Skatteverket: employer declaration API service description (https://www7.skatteverket.se/portal-wapi/open/apier-och-oppna-data/utvecklarportalen/v1/getFile/tjanstebeskrivning-agd-inlamning-v1)
- BAS: INK4 interval table (example of the SRU tables) (https://www.bas.se/wp-content/uploads/2024/11/INK4_P1_Intervall-241119.pdf)
- Wolters Kluwer: digital filing of annual reports (https://www.wolterskluwer.com/sv-se/expert-insights/fakta-om-digital-inlamning-av-arsredovisning)
- FAR: digital filing of annual reports (https://www.faronline.se/dokument/rr_digitalinlamningavarsredovisning/)

✻ Sautéed for 21m 54s · done 2:49 PM

⏺ Unknown command: /btw:. Did you mean /btw?

⏺ Args from unknown skill: Can you create local '_' prefixed folders for information you find applicable to this session from hinted external sources?

※ recap: I built the cratchit take 2 baseline: spec files, legal scaffolding and a C++ skeleton that reproduces all 14 of your filed VAT returns, with all tests passing. Next, I need your answers to the five open questions in docs/interfaces.md. (disable recaps in /config)
```


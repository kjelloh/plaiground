# Status

## Klart

* 2026-11-15: `cratchit/` kopierad från 0a37da4b (155 git-spårade filer, utan `build/` och `_try/`). Debug byggt i `cratchit/build/Debug/`, 458 tester går igenom på macOS (clang).
* 2026-11-15: Installationen provad på macOS, på samma sätt som i källan. Release byggt (458 tester går igenom), `cmake --install build/Release --prefix <tillfällig mapp>` installerar `bin/cratchit` och `bin/.cratchit/` (framework och manual), och programmet startar via `PATH` från en annan mapp med HOME i en tillfällig mapp. `install.py --try --no-tests` fungerar också, startat från en annan mapp. Inte installerat i `~/.local` på riktigt. Linux och Windows inte provade (ingen CI för den här sessionen).

## Bygg, kör och installera

Se [cratchit/README.md](cratchit/README.md).

## Att känna till

* GitHub Actions-filen `.github/workflows/cratchit-0a37da4b.yml` bygger 0a37da4b, inte den här sessionen. Filen ligger utanför sandlådan. En workflow för 029b12e4 får användaren lägga till.
* Testdata (`The ITfied AB`, `_dev_home`) är inte kopierade. Läs dem i 0a37da4b eller kopiera dem hit vid behov.

## Källor

Det som går att återanvända. Ta in en sak i taget och notera det under *Klart* eller i `beslut.md`.

### 0a37da4b – grunden (kopierad)

* [beslut.md](../0a37da4b/beslut.md): byggstegen 2026-11-06–07, datamodellen för bokföringen 2026-11-10 (affärshändelse, kontering, utkast, verifikation, underlag, kontoutdragsrad, avstämning; åtgärderna bokför, ändra, slå ihop, dela upp, ta tillbaka, rätta, länka underlag), Årsflödet och utkast till nästa år 2026-11-13.
* [status.md](../0a37da4b/status.md): steg 1–77, *Att känna till* och *Nästa steg*. Där finns noteringarna om forcerad SIE-import och om BAS-kontoplanen i framework.
* [session.md](../0a37da4b/session.md): användarens önskelista (2026-11-09), avstämning i bokslut (2026-11-11), formulärskissen för att ändra en verifikation och felet med att D1 matchar D11 (2026-11-10).

### 5b35b2b9 – årsredovisning (`cratchit_ar`)

* `cratchit/src/core/ar/*`: balansräkning, resultaträkning, noter, förvaltningsberättelse, resultatdisposition, eget kapital, utdata och spårning.
* Läser SIE (`core/sie/*`, en egen kopia). I cratchit ska den få sina data från bokföringen.
* Kontrollerad mot tidigare årsredovisningar och mot ett utkast från Årsredovisning Online. Referensdata finns i `_Resources/_årsredovisningar`, `_bfn` och `_sie`.
* [session.md](../5b35b2b9/session.md), [progress.md](../5b35b2b9/progress.md), [MANUAL.md](../5b35b2b9/cratchit/MANUAL.md).

### 7d4475fb – inkomstdeklaration 2 (`cratchit_ink2`)

* `cratchit/src/core/ink2/*`: blankett, mappning från BAS, skatt, föregående år, rapport och SRU-utdata. Dessutom `core/sru`, `core/balances` och `core/closing`.
* Kontrollerad mot tidigare SRU-filer. Användaren har lämnat årets deklaration med den. Referensdata finns i mapparna `556782-8172_*` och i `_Resources/_ink2`, `_skv` och `_bas`.
* [session.md](../7d4475fb/session.md), [progress.md](../7d4475fb/progress.md), [MANUAL.md](../7d4475fb/cratchit/MANUAL.md).

### 60cae9fe – take 3

* `cratchit/src/core/skv/vat_return/*`: momsrapport och eSKD, för *Årsflöde > Momsrapport*.
* `cratchit/src/core/statement/*`, `core/books/statements`, `match` och `hint`: kontoutdrag och matchning, för avstämningen i *Bokslut*.
* Delar av `core/books` och `core/sie` finns redan i 0a37da4b.
* `_resources/`: BAS-kontoplaner, BAS-SRU, Fortnox kontoplan, eSKD och kontoutdrag.
* [session.md](../60cae9fe/session.md), [progress.md](../60cae9fe/progress.md), [MANUAL.md](../60cae9fe/cratchit/MANUAL.md).

### 9fe36d28 – walk_pdf

* `walk_pdf/src/*`: PDF till struktur och klartext (filter, typsnitt, CMap, kryptering), för att länka underlag och skapa utkast från en PDF.
* Har sin egen `charset` med fler kodningar (UTF-16, WinAnsi, MacRoman, Adobe Standard).
* [session.md](../9fe36d28/session.md), [progress.md](../9fe36d28/progress.md), [API.md](../9fe36d28/walk_pdf/API.md).

### Dubbletter att slå ihop

`sie`, `charset`, `amount`, `date`, `bas`, `csv`, `log` och `terminal_encoding` finns i flera sessioner, i olika versioner. 0a37da4b:s version gäller tills något annat beslutas.

## Nästa steg

* Användaren väljer. Två förslag:
  * Koppla in årsredovisning och INK2 under *Årsflöde*. De är redan kontrollerade och ger nytta direkt.
  * Bygg själva bokföringen (bokför, ändra och slå ihop utkast). Det är kärnan som saknas.

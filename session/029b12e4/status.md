# Status

## Klart

* 2026-11-15: `cratchit/` kopierad från 0a37da4b (155 git-spårade filer, utan `build/` och `_try/`). Debug byggt i `cratchit/build/Debug/`, 458 tester går igenom på macOS (clang).
* 2026-11-15: Installationen provad på macOS, på samma sätt som i källan. Release byggt (458 tester går igenom), `cmake --install build/Release --prefix <tillfällig mapp>` installerar `bin/cratchit` och `bin/.cratchit/` (framework och manual), och programmet startar via `PATH` från en annan mapp med HOME i en tillfällig mapp. (`install.py` fanns då, se steg 3.) Inte installerat i `~/.local` på riktigt. Linux och Windows inte provade lokalt; se GitHub Actions under *Att känna till*.
* Steg 1: cratchit skriver först vilket bygge som körs, en uppgift per rad: `Byggd: 11:19:20 2026-11-15 (Debug)`, `Från: …/build/Debug/cratchit` och `Version: 0.1.0` (användarens önskan att ha dem på var sin rad). Samma rader hamnar i `log.txt`. Målet `cratchit_build_stamp` kör `cmake/build_stamp.cmake` vid varje bygge och skriver `build/<typ>/generated/build_stamp.cpp` (`build_time()` och `build_type()` i `core/build_stamp.hpp`). Den filen länkas bara in i programmet. `startup_lines` ligger i `core/version`, och `program_path` i `core/dot_folder`. Manualen och README är uppdaterade. 462 tester. Provkört på macOS: Debug byggt två gånger utan andra ändringar (ny tid, bara stämpeln kompilerad och programmet omlänkat), och Release installerat i en tillfällig mapp (visar `Release` och installationens sökväg).
* Steg 2: Startas cratchit i en mapp med `.cratchit/project.txt` öppnas den bokföringen direkt (`Cratchit > Samlingsmapp > <bokföring>`), och samlingsmappen kommer ihåg den i `workspace.txt` (`ui/start`, `open_working_folder_project`, anropad från `main`). Bara arbetsmappen, inte föräldramapparna. Ingen ny fil eller nytt fält: "komma ihåg" betyder att bokföringen står i samlingsmappens lista (användarens val; att öppna den senast använda bokföringen från en annan mapp är inte infört). Går bokföringen inte att öppna startar cratchit i roten och säger varför. En kopia med samma `uuid` ersätter originalet i listan, och cratchit säger det. Manualen uppdaterad (*Starta i en bokföring*). 465 tester. Provkört med pipe på macOS mot en kopia av 0a37da4b:s `The ITfied AB` med HOME i en tillfällig mapp: start i bokföringen, start där igen, och start i en annan mapp där samlingsmappen listar den.
* Steg 3: `install.py` borttaget, och cratchit installeras för hela datorn med `cmake --install` utan `--prefix` (beslut 2026-11-15). README beskriver det (med `sudo`, och administratörsterminal och `PATH` på Windows), liksom avinstallationen. `_try/` borttagen ur `.gitignore`. Manualens exempel visar `/usr/local/bin/cratchit`. Provat på macOS med `DESTDIR` i en tillfällig mapp: installationen hamnar i `usr/local/bin/`. Inte installerat i `/usr/local` på riktigt. Windows och Linux utan `--prefix` inte provade.
* Steg 4: `1 Ny bokföring här` i roten när arbetsmappen saknar bokföring (beslut 2026-11-15). Samma formulär som *Från uppgifter* (`make_project_here` i `ui/details_project_menu`), men namnet är bara text och bokföringen skapas i arbetsmappens `.cratchit/` (`create_project_here` i `core/details_project`, som `create_project_from_details` nu använder). `check_project_here` vägrar mappar med `.cratchit`, hemkatalogen, samlingsmappen och mappar inuti en bokföring; då leder `1` till *Fel* med orsaken. Efter Enter går cratchit tillbaka till roten, som då säger att arbetsmappen har bokföringen, och samlingsmappen kommer ihåg den. `make_root_menu` tar arbetsmappen (från `main`). Nya hjälpämnen `ny-bokforing-har` och `har-namn`; manualen har avsnittet *Ny bokföring här*. 471 tester. Provkört med pipe på macOS med HOME i en tillfällig mapp: skapad i en påhittad företagsmapp (undermappen `Avtal` orörd), listad i samlingsmappen, öppnad direkt vid nästa start, och vägrad i hemkatalogen.
* Steg 5: `1 Ny bokföring här` har samma val som *Lägg till > Ny*: `1 Från SIE` och `2 Från uppgifter` (användarens önskan; `make_new_project_here` i `ui/add_project_menu`). *Från SIE* börjar sökvägen i arbetsmappen, utan ifylld fil (användarens val: användaren letar upp fröet), och skapar bokföringen i arbetsmappens `.cratchit/` (`plan_project_from_sie(file, here)`, `SieProjectPlan::here`). SIE-filen flyttas till arbetsmappens `importerade_sie_filer/`, som överallt annars (användarens val); ligger den redan där får den ligga kvar, och en annan fil med samma namn där skrivs aldrig över. Formuläret heter `Från uppgifter` även här. Manualen uppdaterad. 473 tester. Provkört med pipe på macOS mot en kopia av `~/Documents/cratchit/_itfied_2025_innan_bokslut` med HOME i en tillfällig mapp: 116 verifikationer, SIE-filen i `importerade_sie_filer/`, och bokföringen öppnad direkt vid nästa start.
* Steg 6: När användaren letar upp en SIE-fil visar Tab vad varje SIE-fil innehåller, på en egen indragen rad under namnet: räkenskapsår (`#RAR 0`), skapad (`#GEN`), antal verifikationer och senaste datum, och sista numret per serie (`summarize_sie_file`, `summary_line`, `describe_sie_file` i `core/sie_summary`; läser filen utan att logga). När Tab fyller i en enda fil, eller hela namnet redan står där, visas raden under sökvägen. pyroceph: `complete_path` tar `describe_file`, och `Completion::notes` visas av promptloopen under raden. Gäller *Ny bokföring här > Från SIE*, *Lägg till > Ny > Från SIE*, *Räkenskapsår > Importera från SIE* och *Exportera till SIE > Skillnad*. `Bekräfta` (ny bokföring och import) har raden `Innehåll: …`. Manualen uppdaterad (*Skriva en sökväg*, exemplen i `Bekräfta`). 480 tester. Provkört med pipe på macOS mot en kopia av `_itfied_2025_efter_bokslut`: Tab listar båda filerna med innehåll, och en ensam fil visar raden.
* Steg 7: Tab grupperar SIE-filerna på räkenskapsår (`#RAR 0`, senaste först), med en rubrik per år och en rad per fil i kolumner: namn, skapad, antal verifikationer, senaste datum och sista numret per serie. Inom ett år står den senast skapade filen först (användarens val), och alla år märks lika (ingen märkning av årets grupp vid import, användarens val). Sist *Utan räkenskapsår*, *Går inte att läsa* (med orsaken) och *Mappar* (`list_sie_files` i `core/sie_summary`). pyroceph: `complete_path` tar `list` (`ListCandidates`), där klienten själv lägger upp kandidaterna. En ensam fil visas som i steg 6. Manualen uppdaterad. 482 tester. Provkört med pipe på macOS mot användarens mapp med 30 SIE-filer (bara läst, HOME i en tillfällig mapp).
* Steg 8: `Bekräfta` i *Från SIE* (både *Ny bokföring här* och *Lägg till > Ny*) visar SIE-filens uppgifter på var sin rad med etikett: företag, räkenskapsår, skapad (`#GEN` och `#PROGRAM`, nya fält i `SieProjectPlan`), verifikationer med senaste datum och serier, konton. Raderna om var bokföringen skapas och att filen flyttas är borttagna (användarens val: det sker bakom kulisserna). Valen är `J Ja` och `N Nej` (stora eller små bokstäver); Enter visar valen igen som överallt. `N` och `-` går tillbaka till *Från SIE*, vars sökväg då börjar i mappen där filen valdes. *Importera från SIE* är oförändrad. Manualen uppdaterad. 483 tester. Provkört med pipe på macOS mot en kopia av ett frö: Enter, `N` och `J`.
* Steg 9: Räkenskapsårets status i en SIE-fil (beslut 2026-11-15): `pågående`, `ofullbordat` (med ` !` när året är äldre än förra året), `fullbordat` och `misstämmer` (`status_of`, `status_text`, `SieSummary::balance` och `result_booked` i `core/sie_summary`). Visas som kolumn efter namnet i Tab-listan, i raden för en ensam fil och i `Innehåll` vid import, och i `Bekräfta` för *Från SIE* som `Räkenskapsår: …, <status>` med en rad `Varning` för ett ofullbordat äldre år och för `misstämmer`. Manualen har avsnittet *Räkenskapsårets status i en SIE-fil*. 490 tester. Provkört (bara läst) mot fröerna: `_innan_bokslut` ofullbordat, `_efter_bokslut` fullbordat, `_innan_M2` pågående; och mot användarens 30 SIE-filer.
* Steg 10: `J` och `N` i `Bekräfta` för *Från SIE* matchas exakt; `j` och `n` gör ingenting (beslut 2026-11-15, tangenter matchas exakt). Manualen säger det under *Välja med en tangent*. 490 tester.
* Steg 11: cratchit flyttar inte längre SIE-filer (beslut 2026-11-15, verka utan att märkas), vid *Ny bokföring här > Från SIE*, *Lägg till > Ny > Från SIE* och *Importera från SIE*. I stället skrivs en post i räkenskapsårets förteckning (`core/inventory`: `record_sie_use`, `read_inventory` med två nivåer, `add_to_inventory`; `core/sha256`). `import_sie` tar dagens datum och ger förteckningens post (`SieImported::entry`); `imported_sie_folder_name` och flytten är borttagna, liksom raden om flytten i importens `Bekräfta` och i rapporten. Loggen nämner hashen; gränssnittet inte. Manualen har avsnittet *Dina filer*; README beskriver `inventory.csv`. 497 tester. Provkört med pipe på macOS mot en kopia av ett frö: SIE-filen ligger kvar, bara `.cratchit/` tillkommer, och hashen i förteckningen är densamma som `shasum -a 256` ger.

## Bygg, kör och installera

Se [cratchit/README.md](cratchit/README.md).

## Att känna till

* GitHub Actions: `.github/workflows/cratchit-029b12e4.yml` (användaren gav tillstånd att skapa den utanför sandlådan, 2026-11-15). Den är en kopia av 0a37da4b:s workflow med sessionens sökvägar. Den körs vid push som ändrar `session/029b12e4/cratchit/**` eller själva workflow-filen, och för hand (`workflow_dispatch`). Den bygger Debug, kör testerna, kör programmet och installerar det på Linux, macOS och Windows. Den är inte körd än. 0a37da4b:s workflow (`cratchit-0a37da4b.yml`) är borttagen på användarens begäran, så 0a37da4b byggs inte längre i CI. Dess `beslut.md` och `status.md` nämner den fortfarande.
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

* Minnesanteckning (användarens önskan): återkom med enkla kontroller som en SIE-fil måste klara för att vara *fullbordat* (annars *misstämmer*). Kandidater: årets ingående balanser lika med förra årets utgående; beloppet på 8999 lika med summan av resultatkontona; inga verifikationer utanför räkenskapsåret; inga luckor i serierna; momskontona avräknade vid årets slut.

* Användaren väljer. Två förslag:
  * Koppla in årsredovisning och INK2 under *Årsflöde*. De är redan kontrollerade och ger nytta direkt.
  * Bygg själva bokföringen (bokför, ändra och slå ihop utkast). Det är kärnan som saknas.

# Status

## Klart

* Steg 1: Tomt C++-projekt i `cratchit/` med CMake som konsument av Conan-paket.
* Steg 2: Biblioteket `charset` i `cratchit/src/charset/` med tester i `cratchit/test/`. `gtest` är första Conan-paketet. `main` är fortfarande tom och använder inte `charset`.
* Steg 3: `terminal_encoding` i `cratchit/src/core/` med tester, i biblioteket `cratchit_core`.
* Steg 4: `main` anropar `terminal_encoding` och skriver varningar till stderr. Utan varningar skriver programmet inget.
* Steg 5: Loggningen `core/log` med tester, inklusive CMake-flaggan `CRATCHIT_DEVELOPMENT_TRACE`.
* Steg 6: `open_dot_folder` och `core/path_value` med tester. `main` öppnar `.cratchit` i startmappen och loggar till `log.txt`.
* Steg 7: Provkört att `runtime` i `framework.txt` följs och att loggen hamnar i runtime-mappens `.cratchit`. Ingen kod behövde ändras.
* Steg 8: `.cratchit` öppnas i hemkatalogen, med kod för macOS, Linux och Windows.
* Steg 9: Workflow för GitHub Actions i `.github/workflows/cratchit-0a37da4b.yml`. Inte körd än.
* Steg 10: `Transcoder`, `BasicTranscoder` och `make_transcoder` i `cratchit/src/charset/transcoder.*` med tester i `test/test_transcoder.cpp`. Används ännu inte av `main`.
* Steg 11: Alla decoders har `failure_count()` (fel i parsningen av indata, alltid 0 för CP437). Alla encoders har `unmapped_count()` (kodpunkter utan mappning).
* Steg 12: `BasicTranscoder` och `Transcoder` lämnar ut `failure_count()` och `unmapped_count()`.
* Steg 13: `pyroceph::tui` med `RawMode`, `KeyReader` och `run_prompt_loop`, plus `charset::Decoder`. `main` kör promptloopen sist. Provkört med pipe på macOS, inte i en riktig terminal.
* Steg 14: Träd av `State` och `Navigator` i `pyroceph/tui/state`. Promptloopen navigerar ett demoträd i `main` (Huvudmeny, Projekt, Öppna projekt, Bokslut). Provkört med pipe på macOS.
* Steg 15: Prompten visar sökvägen med statens namn (`Navigator::path()`, `prompt_of`). Provkört med pipe på macOS.
* Steg 16: Gemensamma val listas inte i menyn. `?` visar dem (`common_keys`, `show_common_keys`). Provkört med pipe på macOS.
* Steg 17: Enter visar valen igen (`is_enter`). Provkört med pipe på macOS.
* Steg 18: Indata i `InputBuffer` och `State::on_input`. Redigering börjar när en tangent inte är en handling. Demo: `Bokslut` tar emot ett år. Provkört med pipe på macOS.
* Steg 19: `KeyParser` och `FunctionKey`. Esc tömmer indata, Delete suddar, pilar med mera känns igen men gör inget. Provkört med pipe och med pseudoterminal (`script`) på macOS. Windows-delen är bara skriven, inte byggd.
* Steg 20: `State` är en basklass, och trädet byggs medan användaren navigerar (`on_key` ger `OnKeyResult`, `options()` ger `OptionsList`). `Navigator` äger statena och förstör dem när användaren lämnar dem. Provkört med pipe på macOS.
* Steg 21: `q` avslutar vid navigering (`quit_key`). Provkört med pipe på macOS.
* Steg 22: `install()`-regel i `CMakeLists.txt` och Release-bygge. Provinstallerat till en tillfällig mapp på macOS och kört från `PATH`. Inte installerat i `~/.local` på riktigt. Windows och Linux inte provade.
* Steg 23: `cratchit/README.md` på engelska, för den som bygger och installerar. Byggstegen flyttade dit från `status.md`.
* Steg 24: Framework bredvid programmet (`<prefix>/bin/.cratchit/framework.txt` med `version`), runtime i HOME. `program_folder()` per plattform. Provkört på macOS: utvecklarbygget, installerat i en tillfällig mapp och startat via `PATH` från en annan mapp, via en symbolisk länk, och med framework borttaget (fel, exit 1). Linux- och Windows-grenarna av `program_folder()` är skrivna men inte byggda.
* Steg 25: GitHub Actions kontrollerar att framework ligger bredvid programmet och att runtime skapas i HOME (utan `framework.txt`). Nytt steg installerar (Debug) och kör det installerade programmet via `PATH` från en annan mapp, på alla tre plattformar. Stegets kommandon provade lokalt på macOS, workflow inte körd.
* Steg 26: `cratchit/install.py` (`--prefix`, `--try`, `--no-tests`). Provkört på macOS: `--try` från `/tmp`, `--try` i en flyttad kopia (byggmappen gjordes om, andra gången inte), `--prefix` till en tillfällig mapp. Inte installerat i `~/.local` på riktigt. Windows-grenen (`conan-default`, `--config Release`, `cratchit.exe`) inte provad.
* Steg 27: `cratchit/framework/` heter nu `cratchit/resources/`.
* Steg 28: MSVC-felet C4702 i `charset/decoder.cpp` rättat. Windows-bygget har troligen aldrig kommit förbi `charset`, så fler fel kan komma i `pyroceph`, `main.cpp` och testerna.
* Steg 29: Med MSVC kompilerar nu `charset`, `cratchit_core`, `pyroceph` och `main.cpp` (enligt CI-loggen). Två varningar i `test/test_prompt_loop.cpp` rättade (C4996 `tmpfile`, C4244). Länkning och körning av testerna på Windows ännu inte sedda.
* 2026-11-07: GitHub Actions grönt på Linux, macOS och Windows: bygge, alla tester, körning från byggmappen och installation (Debug) med körning via `PATH`. Därmed är Windows- och Linux-grenarna i `dot_folder` (`program_folder`, `user_home`), `terminal_encoding`, `RawMode` och `KeyReader` byggda och körda i CI (utan riktig terminal). `install.py` är inte körd på Windows eller Linux.
* Steg 30: Ny rot `Cratchit` med `9 Testmeny`. Testmenyn och roten i `src/ui/` (biblioteket `cratchit_ui`) med tester i `test/test_ui.cpp`. 165 tester. Provkört med pipe på macOS.
* Steg 31: `0 Samlingsmapp` i roten, ett tomt workspace-state (`src/ui/workspace_menu`). 167 tester. Provkört med pipe på macOS.
* Steg 32: Ett state har två delar: vy (`State::view()`, typen `View`, rader) och val (`options()`). `show()` visar vyns rader ovanför valen. Alla befintliga state har tom vy. 168 tester. Provkört med pipe på macOS, inget syns annorlunda.
* Steg 33: Felmeddelande-state `make_error_state(message)` i `pyroceph/tui/error_state` med tester i `test/test_error_state.cpp`. Namnet är `Fel`, vyn är meddelandet, inga val, inga tangenter mappade, så `-` går tillbaka. Ännu inte använt av något state i cratchit. 171 tester.
* Steg 34: Testmenyns `Projekt` har `1 Öppna` (lyckas) och `2 Öppna (misslyckas)`, som leder till felstatet med "Kunde inte öppna projektet.". 172 tester. Provkört med pipe på macOS.
* Steg 35: `0 Samlingsmapp` öppnar `<Dokument>/cratchit` och skapar den första gången (`open_workspace`, `documents_folder`, `default_workspace_folder` i `core/dot_folder`). Vyn visar mappen, och `Ny samlingsmapp skapad.` när den skapades. Går det inte att öppna mappen kommer man till `Fel` med orsaken. 182 tester. Provkört med pipe på macOS med HOME i en tillfällig mapp (skapad, öppnad igen, och `Documents` saknas leder till `Fel`). Inte provat mot den riktiga `~/Documents`, där macOS först frågar om Terminal får använda mappen. Windows-grenen (`SHGetKnownFolderPath`) är skriven men inte byggd.
* Steg 36: GitHub Actions sparar Conans paket (`~/.conan2/p`) mellan körningarna med `actions/cache@v6`. Orsak: macOS-bygget föll när github.com inte svarade (503, timeout) då `gtest` skulle hämtas för att byggas från källkoden. Conan-versionen står en gång, i `CONAN_VERSION`. Workflow inte körd.
* Steg 37: `Samlingsmapp` listar projekten (undermappar med `.cratchit/project.txt`) som 1–9 och öppnar dem (`list_projects`, `open_project` i `core/project`, `make_project_menu` i `src/ui/project_menu`). Projektets state har projektets namn i sökvägen och mappen som vy, inga val än. Ett projekt som inte går att öppna leder till `Fel`. 194 tester. Provkört med pipe på macOS med HOME i en tillfällig mapp och en kopia av `The ITfied AB`.
* Steg 38: Samlingsmappen söker inte längre i sina undermappar. Den listar projekten den kommer ihåg i `workspace.txt` (`project.<uuid> = <mapp>`, som i 'take 3'), och mappen kan ligga var som helst. `open_project` kontrollerar att `uuid` i `project.txt` stämmer. cratchit skriver ännu inget i `workspace.txt`, så projekt läggs dit för hand. 198 tester. Provkört med pipe på macOS (HOME i en tillfällig mapp, `The ITfied AB` utanför samlingsmappen och ihågkommen, och ett projekt i en undermapp som inte var ihågkommet och inte syntes).

## Bygg, kör och installera

Se [cratchit/README.md](cratchit/README.md). Byggstegen står bara där.

## Att känna till

* `detect` innehåller bara `is_utf8`, som testar om bytes är UTF-8. Plattformens och terminalens kodning tas reda på av `terminal_encoding`.
* `terminal_encoding` frågar inte macOS, utan antar UTF-8 där. Windows- och Linux-grenarna är byggda och körda i GitHub Actions, inte lokalt.
* 'Take 3' använder de gamla namnen `FromUnicodeBuffer` och `ToUnicodeBuffer`. Här heter de `Encoder` och `Decoder`.

* När utmatningen faller tillbaka till UTF-8 på en terminal som inte är UTF-8 kan å, ä och ö i den svenska varningen bli fel, eftersom varningen själv skrivs som UTF-8.

* Programmet skapar `.cratchit` med `runtime.txt` i hemkatalogen, om den saknas. Startmappen spelar ingen roll. Framework läses från `.cratchit` bredvid programmet.
* Bara macOS är byggt och kört lokalt. Linux och Windows byggs, testas och installeras i `.github/workflows/cratchit-0a37da4b.yml`, men utan en riktig terminal (indata från en tom pipe).

## Nästa steg

* Bestäms av användaren. Troliga steg: att cratchit själv lägger till ett projekt i `workspace.txt` (öppna bokföring på annat ställe, eller ny bokföring), och kunna komma ihåg en samlingsmapp på ett annat ställe (i `runtime.txt`).

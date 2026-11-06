# Beslut

Kort logg över vad vi beslutat och varför.

## 2026-11-06

* Kod på engelska, UI-texter på svenska.
* Bokföringstermer som `Bokslut` och kommentarer kan vara på svenska.
* Minnet av sessionen hålls i `beslut.md` och `status.md`. `session.md` är användarens egen text.
* Användaren testar själv och committar. Claude committar inte.
* C++23, CMake och Conan 2, samma grund som 'take 3' (`session/60cae9fe`).
* `charset` tas från walk_pdf (`session/9fe36d28`) oförändrat: `unicode`, `utf8`, `utf16`, `cp437` och `detect`. Användaren vill ha `detect` redan nu eftersom appen behöver anpassa sig till plattform och runtime.
* PDF-kodningarna `winansi`, `macroman` och `adobe_standard` väntar till PDF-läsningen.
* Varningar räknas som fel för alla mål, som i walk_pdf.
* `terminal_encoding` tas från 'take 3' (`session/60cae9fe`) till `src/core/`, i ett nytt bibliotek `cratchit_core`. Enda ändringen i koden är att varningstexterna är översatta, eftersom de är UI.
* MSVC kompilerar med `/utf-8`, så att svenska UI-texter i källkoden blir rätt.
* Loggningen `core/log` tas från 'take 3'. Felet när loggen inte kan öppnas är översatt, eftersom det är UI. Loggvisaren `core/log_view` väntar, eftersom den hör till REPL:en.
* `open_data_folder` från 'take 3' heter här `open_dot_folder` (fil `core/dot_folder`, struct `DotFolder`). `core/path_value` följde med som beroende. Felen är översatta eftersom de är UI.
* Loggen är `log.txt` bredvid `runtime.txt`, som i 'take 3'. Loggtexterna från `main` är på svenska.
* Filformatet behålls: `.cratchit/framework.txt`, `.cratchit/runtime.txt` och nycklarna `created` och `runtime`.
* Kedjan i 'take 3' är framework -> runtime -> workspace -> project. Tills vidare öppnas bara framework och runtime. Loggen ligger i runtime-mappens `.cratchit`. Workspace och project väntar.
* `.cratchit` ligger i hemkatalogen (`HOME`, på Windows `USERPROFILE`), inte i startmappen och inte bredvid binären. Det gör att appen kan installeras i `PATH`. Avviker från 'take 3'.
* Ingen egen miljövariabel som `CRATCHIT_HOME` för nu: vi har inget behov av den ännu, och fler alternativ ger fler sätt att gå fel. Hemkatalogen måste vara en hel sökväg till en mapp som finns, annars blir det fel. Vid test pekas `HOME` om för den enskilda körningen.
* GitHub Actions: en workflow per session i `.github/workflows/`. Här `cratchit-0a37da4b.yml`, som bara startar vid ändringar i `session/0a37da4b/cratchit/**` och i workflow-filen. Användaren gav uttryckligt tillstånd att skriva i `.github/workflows/`, annars gäller sandlådan.
* GitHub Actions använder Conan 2.34.0, eftersom 2.17.1 inte känner till Apple clang 21 på `macos-latest`. Lokalt är Conan 2.17.1 kvar.

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
* Steg 11: Alla decoders har `failure_count()` (fel i parsningen av indata, alltid 0 för CP437). Alla encoders har `unmapped_count()` (kodpunkter utan mappning). `Transcoder` lämnar ännu inte ut räknarna.

## Bygg och kör

```sh
cd cratchit
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset conan-debug
cmake --build --preset conan-debug
ctest --preset conan-debug
mkdir -p ../_dev_home   # en gång
HOME="$PWD/../_dev_home" ./build/Debug/cratchit
```

Utan `HOME=...` används den riktiga `~/.cratchit`.

## Att känna till

* `detect` innehåller bara `is_utf8`, som testar om bytes är UTF-8. Plattformens och terminalens kodning tas reda på av `terminal_encoding`.
* `terminal_encoding` frågar inte macOS, utan antar UTF-8 där. Windows- och Linux-grenarna är inte byggda eller testade här.
* 'Take 3' använder de gamla namnen `FromUnicodeBuffer` och `ToUnicodeBuffer`. Här heter de `Encoder` och `Decoder`.

* När utmatningen faller tillbaka till UTF-8 på en terminal som inte är UTF-8 kan å, ä och ö i den svenska varningen bli fel, eftersom varningen själv skrivs som UTF-8.

* Programmet skapar `.cratchit` i hemkatalogen, om den saknas. Startmappen spelar ingen roll.
* Bara macOS-grenen är byggd och körd lokalt. Windows- och Linux-grenarna i `dot_folder` och `terminal_encoding` provas först när `.github/workflows/cratchit-0a37da4b.yml` har körts på GitHub.

## Nästa steg

* Bestäms av användaren. Väntar på användarens test av steg 10.

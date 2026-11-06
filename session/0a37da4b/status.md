# Status

## Klart

* Steg 1: Tomt C++-projekt i `cratchit/` med CMake som konsument av Conan-paket.
* Steg 2: Biblioteket `charset` i `cratchit/src/charset/` med tester i `cratchit/test/`. `gtest` är första Conan-paketet. `main` är fortfarande tom och använder inte `charset`.
* Steg 3: `terminal_encoding` i `cratchit/src/core/` med tester, i biblioteket `cratchit_core`. `main` använder den inte än.

## Bygg och kör

```sh
cd cratchit
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset conan-debug
cmake --build --preset conan-debug
ctest --preset conan-debug
./build/Debug/cratchit
```

## Att känna till

* `detect` innehåller bara `is_utf8`, som testar om bytes är UTF-8. Plattformens och terminalens kodning tas reda på av `terminal_encoding`.
* `terminal_encoding` frågar inte macOS, utan antar UTF-8 där. Windows- och Linux-grenarna är inte byggda eller testade här.
* 'Take 3' använder de gamla namnen `FromUnicodeBuffer` och `ToUnicodeBuffer`. Här heter de `Encoder` och `Decoder`.

## Nästa steg

* Bestäms av användaren. Väntar på användarens test av steg 3.

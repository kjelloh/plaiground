# Status

## Klart

* Steg 1: Tomt C++-projekt i `cratchit/` med CMake som konsument av Conan-paket. Inga paket är tillagda än.

## Bygg och kör

```sh
cd cratchit
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset conan-debug
cmake --build --preset conan-debug
./build/Debug/cratchit
```

## Nästa steg

* Bestäms av användaren. Väntar på användarens test av steg 1.

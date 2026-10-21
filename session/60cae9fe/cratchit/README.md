# cratchit

Swedish bookkeeping app in C++23, built with CMake and Conan 2.

## Build

```sh
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset conan-debug
cmake --build --preset conan-debug
ctest --preset conan-debug
```

Run: `./build/Debug/cratchit` (Windows/multi-config generators: `build/Debug/cratchit.exe` or as reported by CMake).

For a release build, replace `Debug`/`debug` with `Release`/`release`.

`LOG_DEVELOPMENT_TRACE` events are compiled into Debug builds only. Set the CMake cache
variable `CRATCHIT_DEVELOPMENT_TRACE` to `ON` (every build), `OFF` (none) or `DEBUG`
(the default), e.g. `cmake --preset conan-debug -DCRATCHIT_DEVELOPMENT_TRACE=OFF`.

## Use

See [MANUAL.md](MANUAL.md). It is built into the program and shown by the `h` key,
so its headings are the help menu. CMake rebuilds it whenever the file changes.

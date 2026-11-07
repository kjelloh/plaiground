# cratchit

A Swedish bookkeeping app for the terminal.

* The user interface is in Swedish. cratchit is made for Swedish bookkeeping (SIE files, *bokslut* — year-end closing — and *årsredovisning* — annual report).
* The code and the developer documentation are in English.
* Early work in progress: so far cratchit opens its settings folder and lets you navigate a small demo menu.

## Requirements

* CMake 3.25 or later
* [Conan 2](https://conan.io) (and Python, which Conan runs on)
* A C++23 compiler: Apple clang (Xcode), GCC 14 or MSVC (Visual Studio 2022)

Conan needs a profile once per machine: `conan profile detect`.

## Build and install

From this folder (`cratchit/`):

```sh
# 1. Fetch the dependencies for a Release build and generate the CMake setup.
#    Compiles nothing of cratchit. Only needed again when dependencies or settings change.
conan install . --build=missing -s build_type=Release -s compiler.cppstd=23

# 2. Configure: read CMakeLists.txt and write the build files to build/Release/.
#    Rarely needed again; step 3 reconfigures by itself when CMakeLists.txt changes.
cmake --preset conan-release

# 3. Compile and link the libraries, cratchit and the tests. Only what changed is rebuilt.
cmake --build --preset conan-release

# 4. Run the tests (optional, but a good check before installing).
ctest --preset conan-release

# 5. Copy the program to <prefix>/bin, here ~/.local/bin/cratchit. Builds nothing.
cmake --install build/Release --prefix ~/.local
```

The folder `<prefix>/bin` must be in your `PATH`. After a code change, run steps 3 to 5 again; otherwise the old program stays installed.

Only the single program file is installed. The libraries are static and built into it.

### Windows

Visual Studio is a multi-config generator, so steps 2 and 5 differ:

```sh
cmake --preset conan-default
cmake --install build --config Release --prefix <folder>
```

Add `<folder>\bin` to `PATH` yourself. The Windows install steps are not tried yet.

## Build for development

A Debug build sits next to the Release build, in `build/Debug/`, and does not affect it:

```sh
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset conan-debug
cmake --build --preset conan-debug
ctest --preset conan-debug
```

To run it without touching your real `~/.cratchit`, point `HOME` to a folder of its own for that run:

```sh
mkdir -p ../_dev_home   # once
HOME="$PWD/../_dev_home" ./build/Debug/cratchit
```

In Debug builds `LOG_DEVELOPMENT_TRACE` is compiled in; in Release builds it is left out (CMake option `CRATCHIT_DEVELOPMENT_TRACE`: `ON`, `OFF` or `DEBUG`, the default).

## Where cratchit keeps its files

cratchit keeps its settings in `.cratchit` in your home folder (`HOME`, on Windows `USERPROFILE`), no matter where the program is installed or started from. It is created on the first run.

| File | Holds |
| --- | --- |
| `framework.txt` | `created` and `runtime`, the folder whose `.cratchit` holds `runtime.txt` (`.` is the home folder itself) |
| `runtime.txt` | `created` |
| `log.txt` | The log, next to `runtime.txt` |

## Using cratchit

cratchit shows a menu and a prompt with the path to where you are, e.g. `Huvudmeny > Projekt> `.

| Key | Does |
| --- | --- |
| `1`, `2`, ... | The options listed in the menu |
| `-` | Back (*Tillbaka*) |
| `?` | List the common keys (*Visa gemensamma val*) |
| Enter | Show the menu again |
| `q` | Quit (*Avsluta*) |
| Ctrl-D, Ctrl-C, Ctrl-Z | Quit, also while typing |

A key that is no option starts text input, ended with Enter. While typing, Backspace erases and Esc (or Ctrl-U) empties the input.

## Uninstall

Delete the program file, e.g. `~/.local/bin/cratchit`. To remove all settings and logs as well, delete `~/.cratchit`.

## Licence

As for all of this repository: source code under GNU GPL v2, documentation under CC0 1.0. See [LICENCE.md](../../../LICENCE.md).

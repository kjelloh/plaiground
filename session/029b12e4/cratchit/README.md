# cratchit

A Swedish bookkeeping app for the terminal.

* The user interface is in Swedish. cratchit is made for Swedish bookkeeping (SIE files, *bokslut* — year-end closing — and *årsredovisning* — annual report).
* The code and the developer documentation are in English.
* Early work in progress: so far cratchit opens its framework and its settings folder and lets you navigate a small demo menu.

## Requirements

* CMake 3.25 or later
* [Conan 2](https://conan.io) (and Python, which Conan runs on)
* A C++23 compiler: Apple clang (Xcode), GCC 14 or MSVC (Visual Studio 2022)

Conan needs a profile once per machine: `conan profile detect`.

## Build and install

### With install.py

`install.py` runs all the steps below in one go (Python is there already, since Conan runs on it):

```sh
python3 install.py                  # build, test and install into ~/.local (program in ~/.local/bin)
python3 install.py --prefix <dir>   # install into <dir>/bin instead
python3 install.py --try            # install into _try/ here and run it with _try/home as home folder
python3 install.py --no-tests       # skip the tests
```

On Windows, start it with `py install.py`.

* It prints each command before running it and stops at the first failing step.
* It takes every path from its own folder, so it works from any current folder and after the `cratchit` folder is moved.
* `--try` leaves your real `~/.cratchit` alone. `_try/` is ignored by Git; `_try/home` is kept between tries, so delete `_try/` to start afresh.
* CMake keeps full paths in its build folder, so a build folder made before the `cratchit` folder was moved cannot be used. `install.py` notices that, removes the Release build folder (generated files only) and builds afresh. A Debug build folder from before a move must be removed by hand: `build/Debug/`.

### Step by step

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

# 5. Copy the program and its framework to <prefix>/bin, here ~/.local/bin/cratchit
#    and ~/.local/bin/.cratchit/. Builds nothing.
cmake --install build/Release --prefix ~/.local
```

The folder `<prefix>/bin` must be in your `PATH`. After a code change, run steps 3 to 5 again; otherwise the old program stays installed.

The program file and its framework folder (see [Where cratchit keeps its files](#where-cratchit-keeps-its-files)) are installed. The libraries are static and built into the program.

### Windows

Visual Studio is a multi-config generator, so steps 2 and 5 differ:

```sh
cmake --preset conan-default
cmake --install build --config Release --prefix <folder>
```

Add `<folder>\bin` to `PATH` yourself. GitHub Actions installs a Debug build this way on Windows, Linux and macOS and runs it via `PATH`; `install.py` itself is only tried on macOS.

## Build for development

A Debug build sits next to the Release build, in `build/Debug/`, and does not affect it:

```sh
conan install . --build=missing -s build_type=Debug -s compiler.cppstd=23
cmake --preset conan-debug
cmake --build --preset conan-debug
ctest --preset conan-debug
```

It finds its framework in `build/Debug/.cratchit/`. To run it without touching your real `~/.cratchit`, point `HOME` to a folder of its own for that run:

```sh
mkdir -p ../_dev_home   # once
HOME="$PWD/../_dev_home" ./build/Debug/cratchit
```

In Debug builds `LOG_DEVELOPMENT_TRACE` is compiled in; in Release builds it is left out (CMake option `CRATCHIT_DEVELOPMENT_TRACE`: `ON`, `OFF` or `DEBUG`, the default).

## Folder layout

| Folder or file | Holds | In Git |
| --- | --- | --- |
| `src/charset/` | Text encodings: Unicode, UTF-8, UTF-16, CP437, and `Transcoder` | yes |
| `src/pyroceph/tui/` | The terminal UI library `pyroceph`: keys, states and the prompt loop. Knows nothing of cratchit. | yes |
| `src/core/` | cratchit's own logic: settings folders, log, terminal encoding | yes |
| `src/ui/` | cratchit's own UI states (library `cratchit_ui`): the root menu, the workspace and its projects, and the test menu kept to remember how states and navigation work | yes |
| `src/main.cpp` | The program: starts up and runs the prompt loop from the root menu | yes |
| `test/` | The tests (GoogleTest) | yes |
| `docs/` | The user manual, in Swedish (`anvandarmanual.md`) | yes |
| `resources/` | The sources of the framework that is installed next to the program (see [Where cratchit keeps its files](#where-cratchit-keeps-its-files)). So far `framework.txt.in`; later the rules for Swedish bookkeeping. | yes |
| `CMakeLists.txt`, `conanfile.py` | The build | yes |
| `install.py` | Build, test and install in one go (see [With install.py](#with-installpy)) | yes |
| `build/` | Everything built, per build type (`build/Debug/`, `build/Release/`). Safe to delete. | no |
| `CMakeUserPresets.json` | Generated by `conan install` | no |
| `_try/` | Made by `install.py --try`. Safe to delete. | no |

## Where cratchit keeps its files

cratchit has two `.cratchit` folders:

| Folder | Holds | Written by |
| --- | --- | --- |
| *Framework*: `.cratchit` next to the program, e.g. `~/.local/bin/.cratchit/` | What comes with the installation. Later the rules for Swedish bookkeeping, tax, *bokslut* and *årsredovisning*. | The installation only. cratchit only reads it. |
| *Runtime*: `.cratchit` in your home folder (`HOME`, on Windows `USERPROFILE`) | Your own files. Created on the first run. | cratchit |

| File | Holds |
| --- | --- |
| `<program folder>/.cratchit/framework.txt` | `version`, the cratchit version the framework belongs to |
| `<program folder>/.cratchit/anvandarmanual.md` | A copy of `docs/anvandarmanual.md`. `?` shows its help texts (see below). |
| `~/.cratchit/runtime.txt` | `created`, the day it was created |
| `~/.cratchit/log.txt` | The log, next to `runtime.txt` |

When the framework is missing, cratchit stops with an error: the installation is not whole.

The workspace (*samlingsmapp*) and each project (*bokföring*, the books of one juridical person) have a `.cratchit` folder of their own:

| File | Holds |
| --- | --- |
| `<Documents>/cratchit/.cratchit/workspace.txt` | `created`, and one `project.<uuid> = <folder>` per project the workspace remembers. A project folder may be anywhere. |
| `<project folder>/.cratchit/project.txt` | `created`, `uuid`, `name` and `orgnr`, in the format of take 3 (which may add more). |
| `<project folder>/.cratchit/books/books.txt` | `chart_type`, `company_id`, `address.*` and one `year.<start> = <end>` per fiscal year |
| `<project folder>/.cratchit/books/accounts.csv`, `dimensions.csv`, `objects.csv` | The chart of accounts, dimensions and objects |
| `<project folder>/.cratchit/books/<year start>/opening.csv`, `vouchers.csv`, `postings.csv` | A fiscal year's reported opening balances, vouchers and their postings |

The books are kept as take 3 keeps them (`core/books`, ported from take 3, as is the SIE reader `core/sie`). *Ny bokföring > Från SIE* plans the import of a whole SIE file into empty books (`plan_project_from_sie`), and on the user's confirmation creates `<the SIE file's folder>/<#FNAMN>/` with `project.txt` and the books, and moves the SIE file into it (`create_project_from_sie`). No copy is kept in `.cratchit/imports`. cratchit works without SIE files; SIE is only one way in: *Ny bokföring > Från uppgifter* creates `<workspace>/<name>/` from the name, organisation number (checked with its Luhn digit) and first fiscal year the user types (`core/details_project`, `ui/details_project_menu`), writing only `project.txt` and a `books.txt` holding the year. The books read a missing `accounts.csv`, `dimensions.csv` or `objects.csv`, and a year without a folder of its own, as empty. There is no chart of accounts for such books yet; a BAS chart in the framework is planned.

The workspace and its `.cratchit` are created the first time the user opens it. *Öppna befintlig* adds or replaces the project's line in `workspace.txt`, so a moved project folder is remembered at its new place (same `uuid`).

`<Documents>` is `$HOME/Documents` on macOS, `XDG_DOCUMENTS_DIR` (else `$HOME/Documents`) on Linux, and the Documents known folder on Windows (it follows a move to OneDrive). macOS may ask the user once whether the terminal may use Documents; when denied, cratchit cannot open the workspace.

The framework's sources are in `resources/`. CMake puts them together in the build folder (`framework.txt` gets the version from `project()` in `CMakeLists.txt`), copies them next to the built program on every build, and installs them next to the installed program. So a development build and an installed program find their framework the same way.

### How cratchit finds its own folder

To find the framework, cratchit needs the folder of the program file that is running. `argv[0]` cannot tell it: it is only what the starter (usually the shell) passed, and the shell passes what you typed. Started from `PATH`, `argv[0]` is the bare name `cratchit`; started via a link, it is the link's path; and the starter may pass anything at all.

So cratchit asks the platform (`program_folder()` in `src/core/dot_folder.cpp`):

| Platform | Asks |
| --- | --- |
| macOS | `_NSGetExecutablePath` (from dyld, the macOS loader) |
| Linux | the link `/proc/self/exe` |
| Windows | `GetModuleFileNameW` |

Then `std::filesystem::canonical` resolves links and `..` (on macOS and Linux like POSIX `realpath`, on Windows like `GetFinalPathNameByHandleW`). So a link to cratchit in another folder still leads to the framework next to the real program file. The folder of the resolved file is the program folder.

## Using cratchit

The user manual is in Swedish: [docs/anvandarmanual.md](docs/anvandarmanual.md). It describes what cratchit can do so far and is updated with every step that changes what the user sees.

`?` in the program shows the common keys. When the state has a help text, the `?` line says so, and `?` pressed again right after shows the help text from the manual. A third `?` shows the common keys again; any other key in between starts over. The manual is the only source of the help texts:

* A help text starts after a line `<!-- hjälp: <topic> -->` and ends at `<!-- /hjälp -->` (or the next comment or heading). Comments are invisible when the Markdown is shown. A help text may hold several paragraphs, lists and code blocks, and should fit on one screen.
* Each state names its topic (`State::help_topic()`); cratchit's topics are in `src/ui/help_topics.hpp`. A state without a topic shows only the common keys.
* The manual is copied into the framework at build time and read at start (`core/manual`). The program shows the help text as plain text, without `**`, `*`, backticks and link targets; code blocks show as they are, without their fences.
* The tests check that the manual has a help text for every topic, and no help text for an unknown topic.

## Trying another day

Some of cratchit depends on today's date, e.g. which fiscal year is the current one (key 0). To try another day, e.g. the turn of a fiscal year, set `CRATCHIT_TODAY`:

```sh
CRATCHIT_TODAY=2027-05-02 cratchit
```

cratchit says on stderr which day it takes as today. The tests run on a fixed day (`test/test_today.cpp`).

## Uninstall

Delete the program file and the framework next to it, e.g. `~/.local/bin/cratchit` and `~/.local/bin/.cratchit/`. To remove your own files and logs as well, delete `~/.cratchit`.

## Licence

As for all of this repository: source code under GNU GPL v2, documentation under CC0 1.0. See [LICENCE.md](../../../LICENCE.md).

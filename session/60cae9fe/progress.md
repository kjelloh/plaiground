# Progress

## Now

Task 1: CMake + Conan scaffold in `cratchit/`. Done.

Task 2: Single-key REPL (`h` help, `q` / Ctrl-D quit). Builds without warnings, 8 tests pass, verified in a pty and with piped input. In review.

Task 3: Seeded user manual `cratchit/MANUAL.md`; CLAUDE.md now requires keeping it current. In review.

Task 4: `h` browses the built-in manual (`core/manual`, `core/help`). Number keys drill down, `-` goes back,
`q` leaves help, Esc does nothing, 0/9 scroll lists of more than 9, long output pages like `more`.
39 tests pass, no warnings, checked with piped input and in a 10-row pty. Not built on Windows yet. In review.

Task 5: On start cratchit opens `./.cratchit` (`core/data_folder`). Missing: creates it with `framework.txt`
(`created = <local date>`) and reports `Initiated <file> in <folder>` per new file. Existing: never changed;
an error (exit 1) when it is a file or lacks `framework.txt`. In review.

Task 6: `framework.txt` entry `runtime = <folder>` (fresh start writes `runtime = .`; missing entry means `.`)
points to a folder whose `.cratchit` holds `runtime.txt` (`created = <date>`). Relative to the framework's folder.
runtime.txt is added only to the own `.cratchit`; a runtime elsewhere must exist, else error. Missing runtime
folder is an error. New `core/path_value` parser (trimmed `path = value`, blank lines skipped, no comments,
malformed line or repeated path is an error). 64 tests pass, no warnings, checked in a scratch folder. In review.

## Next

To be given in chat.

## Agreed in chat

- 2026-10-19: Tasks are given in chat, not in files. Conventions, architecture and domain knowledge live in the code the user approves, not in separate docs.
- 2026-10-19: Implement natively, or consume from Conan when appropriate.
- 2026-10-19: Targets: `cratchit_core` library (all logic) + thin `cratchit` executable + `cratchit_tests` (GoogleTest via Conan).
- 2026-10-19: Classic headers (`#pragma once`), no C++20 modules.
- 2026-10-20: Single-key input is plain code (termios / conio), not a Conan package. `q` quits.
- 2026-10-20: User manual lives in `cratchit/MANUAL.md`; every new or changed user operation updates it.
- 2026-10-20: Help is MANUAL.md built into the binary. Headings are the menu. `-` backs out (not Esc, which
  arrow keys also send). `q` in help leaves help only. Over 9 entries: 1-8 plus `0 ...up` / `9 ...down`.
  Long output pages with space/Enter at `--More--`.
- 2026-10-20: Data lives in `.cratchit/` in the current folder. Planned chain of dot-files:
  framework -> runtime -> workspace -> project. For now `framework.txt` holds only `created = YYYY-MM-DD`.
  Every new file in `.cratchit` is reported as `Initiated ...`. An existing `.cratchit` missing required content is an error,
  except that cratchit may add missing files to its own `.cratchit` (e.g. runtime.txt when `runtime = .`).
  Creating a runtime elsewhere is not done on start; it needs a fresh start or a future special operation.

## Open questions

- Should `#` start a comment line in `path = value` files? Currently not (such a line is an error unless it has `=`).
- Should warnings be errors (`-Werror` / `/WX`)? Currently not.
- Is a CI setup (e.g. GitHub Actions on macOS/Linux/Windows) wanted to verify cross-platform builds?

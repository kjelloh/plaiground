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

Task 7: Workspace (`core/workspace`, `core/path_editor`). On start and on `w`, cratchit opens the folder named by
`workspace = <folder>` in runtime.txt (`workspace>` prompt: `h`, `-` back, `q` quits). Not set or missing folder:
choose 1) `<Documents>/cratchit` (per platform) 2) start folder 3) type a path in cratchit's own buffer, starting at the
root, Tab completes folders like a shell, a missing folder asks `Create it? (y/n)`. The choice is saved as a full path
in runtime.txt (`Updated ...`). A fresh location gets `.cratchit/workspace.txt` (`created = <date>`); a saved
workspace whose `.cratchit` lacks it is an error (stays at `cratchit>`). 107 tests pass, no warnings, checked in a pty
and with piped input. Windows Documents lookup (SHGetKnownFolderPath) not built yet. In review.

Task 8: Charset transcoding scaffold, CP437 to Unicode (`core/charset/unicode`, `core/charset/cp437`).
`charset::unicode::CodePoint` (char32_t), `charset::cp437::CodePoint` (uint8_t). `cp437::ToUnicodeBuffer::push(byte)`
returns `std::optional<unicode::CodePoint>` (always a value for CP437); `cp437::to_unicode(bytes)` decodes a text.
0x00-0x7F is ASCII including control characters, 0x80-0xFF the standard CP437 table. No user operation, so no manual
change. 112 tests pass, no warnings. In review.

Task 9: Unicode to UTF-8 (`core/charset/utf8`). `charset::utf8::CodePoint` (uint8_t). `utf8::FromUnicodeBuffer::push(code
point)` returns `utf8::Bytes` (`std::array<CodePoint, 4>` + `size`, iterable). Surrogates and values above U+10FFFF give
U+FFFD. `utf8::from_unicode(code points)` returns a `std::string`. Test chains CP437 -> Unicode -> UTF-8.
119 tests pass, no warnings. In review.

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

- 2026-10-20: cratchit may change an existing file when the user asks for it (choosing a workspace updates runtime.txt).
  The workspace is chosen from a menu; its path is typed key by key in cratchit's own buffer with shell-like Tab
  completion. Paths are shown and saved in full. `q` quits from `workspace>`. The workspace opens on start.

- 2026-10-21: Transcoding goes through two push buffers: charset -> Unicode (`push(byte)` returns an optional code
  point) and Unicode -> charset (`push(code point)` returns the byte or bytes). Each charset has its own namespace
  `charset::<name>` with its own `CodePoint` type; Unicode lives in `charset::unicode`. CP437 bytes 0x00-0x1F and 0x7F
  are ASCII control characters, not the IBM screen glyphs.

## Open questions

- Should `#` start a comment line in `path = value` files? Currently not (such a line is an error unless it has `=`).
- Should warnings be errors (`-Werror` / `/WX`)? Currently not.
- How should the user change to another workspace once one is set? Today only by editing runtime.txt, which is also
  the only way out when a saved workspace has a `.cratchit` without workspace.txt.
- Is a CI setup (e.g. GitHub Actions on macOS/Linux/Windows) wanted to verify cross-platform builds?

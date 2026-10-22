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
returns `std::optional<unicode::CodePoint>` (always a value for CP437).
0x00-0x7F is ASCII including control characters, 0x80-0xFF the standard CP437 table. No user operation, so no manual
change. 112 tests pass, no warnings. In review.

Task 9: Unicode to UTF-8 (`core/charset/utf8`). `charset::utf8::CodePoint` (uint8_t). `utf8::FromUnicodeBuffer::push(code
point)` returns `utf8::Bytes` (`std::array<CodePoint, 4>` + `size`, iterable). Surrogates and values above U+10FFFF give
U+FFFD. Test chains CP437 -> Unicode -> UTF-8. 119 tests pass, no warnings. In review.

Task 10: Terminal encoding (`core/terminal_encoding`). On start cratchit asks the platform for the terminal's input and
output encoding and keeps the choice for the run (in memory, not saved). macOS and Android: UTF-8 at compile time.
Linux/POSIX: `nl_langinfo(CODESET)` of the user's locale (process locale restored after). Windows: `GetConsoleCP` /
`GetConsoleOutputCP`, else `GetACP`. `Charset` names many encodings (incl. GBK, GB18030, Big5, Shift_JIS, EUC-JP/KR);
each direction adapts when a transcoder exists (read: UTF-8, CP437; write: UTF-8), else UTF-8 plus a warning on stderr.
Not yet wired into actual terminal I/O. Manual has a new section "Terminal encoding". 126 tests pass, no warnings.
Linux and Windows branches not built yet (Linux query logic checked on macOS). In review.

Task 11: Removed all Unicode code point buffers: `cp437::to_unicode(text)`, `utf8::from_unicode(text)` and the
`unicode::CodePoints` (u32string) alias. Only the two push buffers remain; the CP437 -> UTF-8 test pairs them in its
own loop. No user operation, so no manual change. 124 tests pass, no warnings. In review.

Task 12: Logging facade (`core/log`), own code, no Conan package. `LOG_BUSINESS`, `LOG_DEVELOPMENT_TRACE`,
`LOG_DESIGN_INSUFFICIENCY` take `std::format` arguments and append one line `<local time> <KIND> <file>:<line> <message>`
to `log.txt` next to runtime.txt (one log for all runs, never on screen). Events before the log is opened are kept in
memory and written on open. CMake cache variable `CRATCHIT_DEVELOPMENT_TRACE` = ON/OFF/DEBUG (default DEBUG: Debug
builds only); when off, the trace is still type-checked but never runs. Log viewer (`core/log_view`): `l` at
`cratchit>` and `workspace>` shows the log newest first, paged with `--More--` (space/Enter older, `-`/`q` back),
ends with `(start of log)`. Logged so far: start, initiated files, chosen/opened workspace, quit (business);
terminal encoding fallback (design insufficiency, still also warned on stderr); page lines (trace). Manual has
section "The log"; README documents the CMake flag. 139 tests pass, no warnings with trace on or off; checked with
piped input and in a 10-row pty. In review.

Task 13: UTF-8 to Unicode (`utf8::ToUnicodeBuffer` in `core/charset/utf8`). `push(byte)` returns an optional code point;
`finish()` ends the input. Only well-formed UTF-8 decodes (no overlong forms, surrogates or values above U+10FFFF,
checked from the second byte). A failed sequence consumes its bytes including the breaking byte (even if that byte is
good on its own) and gives one U+FFFD. `failure_count()` counts failed sequences, `failed_bytes()` keeps all their bytes
flat and in order, without a cap. `unicode::replacement_character` moved to `core/charset/unicode.hpp`. The buffer does
not log (clients decide). No user operation, so no manual change. 151 tests pass, no warnings. In review.

Task 14: Warnings are errors (`-Werror` / `/WX` on the shared `cratchit_warnings` target). Clean Debug rebuild with
trace DEBUG, OFF and ON builds and passes 151 tests on macOS. Release, Linux and Windows not built. In review.

Task 15: REPL keys are decoded to Unicode before they are acted on (`core/key_input`). `KeyReader(charset, byte source)`
holds the push buffer for the input charset detected on start (UTF-8 or CP437, else UTF-8) and `next()` gives one code
point per key, calling `finish()` at end of input. A sequence that fails gives U+FFFD (unknown key) and one
`LOG_DESIGN_INSUFFICIENCY` with its bytes. `KeySource` and all `on_key`/`ignores` take `unicode::CodePoint`. Echo is
written as UTF-8 (the only output encoding so far). The path editor keeps its text as UTF-8 (agreed for now) and drops
C0/C1 controls and U+FFFD. Windows `_getch` kept as is: 0xE0 (alpha in CP437, O-acute in CP850) cannot be typed.
Manual section "Terminal encoding" updated. 163 tests pass, no warnings; checked with piped input. In review.

Task 16: Switch workspace. Start is unchanged. `w` at `cratchit>` now shows a workspace menu (`workspace?>`): `w` opens
the saved workspace (left out when none is saved or its folder is missing), `b` browses, `-` back. Browse is the folder
menu, reordered: 1) start folder 2) Documents/cratchit (hidden if not found) 3) type a path; its `-` returns to the
workspace menu after `b`, else to `cratchit>`. Choosing the saved folder again leaves runtime.txt as is. A real change
logs `Switched workspace from <old> to <new>` (business). A broken saved workspace can now be replaced via `w` `b`.
Manual: new "The workspace menu", choices reordered, piped example now `1q`. 169 tests pass, no warnings; checked
with piped input. In review.

Task 17: Projects (`core/project`). A project is the books of one juridical person; its folder (anywhere) has
`.cratchit/project.txt` (`created`, `uuid` (random v4, own code), `name` = official name). workspace.txt lists
`project.<uuid> = <folder>` (full path written; relative read from the workspace folder) and `project = <uuid>` (last
opened). `p` at `workspace>` shows the project menu (`project?>`): numbers open (0/9 scroll over 9), `n` new (name typed
at `name>`, then folder: 1) `<workspace>/<name>` (hidden if not a valid folder name) 2) type a path from the workspace
folder, missing asks `Create it?`), `a` adds an existing project folder, `-` back. `project>` prompt: `h`, `l`, `-`, `q`.
Opening a workspace (start, `w w`, switch) reopens its last project. Listed uuid must match project.txt. A uuid already
listed with another folder (copy) is an error + design insufficiency log. Business log: created, added, opened. Manual:
new section "Projects", files, typing paths, log. 201 tests pass, no warnings; checked with piped input.
Not done: remove/rename a project, update a listed folder that moved (re-adding it is now the copy error). In review.

Task 18: SIE import, step 1: push-based SIE lexer (`core/sie/lexer`, namespace `cratchit::sie`). `Lexer::push(code
point)` gives an optional `Lexed` (`std::expected<Line, std::string>`) once a line ends; `finish()` gives a last line
without LF. `Line`: kind (record / lone `{` / lone `}`), label without `#`, fields (UTF-8 text or `ObjectList` of item
texts), line number. CR ignored, blank lines skipped, space/tab separate, `\"` in quotes is `"`, other `\` kept. Errors
name the line; lexing continues on the next line. No logging (clients decide). No user operation, so no manual change.
217 tests pass, no warnings. Also lexed all 11 Fortnox sample files in `../62c6984e/_sie_in` (CP437 -> lexer, scratch
program, not in the repo): 0 errors. In review.

Task 19: SIE file encoding check (`core/sie/file`) and seed of charset detection (`core/charset/detect`).
`charset::detect::is_utf8(bytes)`: well-formed UTF-8 with at least one multi-byte character (pure ASCII gives false).
`sie::is_expected_text(code point)`: tab, CR, LF, printable ASCII, U+00A0-U+00FF; CP437 box drawing, blocks, Greek,
math and other controls are unexpected. `sie::lex_bytes(bytes)` reads CP437; on the first unexpected character it
applies UTF-8 if `is_utf8` (leading BOM skipped) and sets `alert` (names the character and line), else returns error
text "Unknown encoding ...". `LexedFile` = applied `Charset`, `alert`, lexed lines. `sie::lex_file(path)` logs
`Read SIE file <file> as <charset>` and the alert (business), an unknown encoding as design insufficiency; alert and
error name the file. The alert is not on screen yet (no import UI); the dry-run task shows it. No manual change.
231 tests pass, no warnings. All 11 samples read as CP437, no alert; a UTF-8 copy reads as UTF-8 with alert
(U+251C on line 8); a Windows-1252 copy is an unknown encoding (U+03A3). In review.

## Next

SIE import plan (proposed 2026-10-22, see chat): 20) SIE document: identification + chart of accounts records and
parser from lines; 21) balances, vouchers (`#RTRANS` is followed by a duplicate `#TRANS` to skip), dimensions;
22) file checks (KSUMMA, balanced vouchers, known accounts, IB + transactions = UB); 23) `i` at `project>` dry run
(summary only, shows the encoding alert); 24) books model + files; 25) import (confirm, write books, stash file +
import record, log, manual); 26) reports (saldobalans, voucher list, account ledger). Next task to be given in chat.

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

- 2026-10-21: Cratchit has its own internal encoding (Unicode code points), separate from the platform's. It determines
  and remembers the terminal in/out encodings, adapts to them when it can transcode, and otherwise falls back to UTF-8
  with a warning. Transcoders are added as the need arises.

- 2026-10-21: Logging is cratchit's own code behind a facade (swap to spdlog later only if needed). Three kinds:
  business (information for the user, NOT bookkeeping data), development trace (CMake flag, on in Debug for now),
  design insufficiency. One log for all runs in the runtime's `.cratchit/log.txt`. Nothing is logged to the screen.
  A rudimentary viewer shows the log newest first.

- 2026-10-21: Every task considers what to log. Log sparsely but deliberately: important events only (what the user
  did or cratchit changed as `LOG_BUSINESS`, every not-designed-for path as `LOG_DESIGN_INSUFFICIENCY`, traces only
  where they help development). No logging in loops or per key press.

- 2026-10-21: Results come from return values, never out-parameters. Unicode code points exist only one at a time
  between two push buffers, never in a buffer of their own. There are no whole-text transcode helpers: each client
  pairs the buffers it needs in its own algorithm.

- 2026-10-21: A failed UTF-8 sequence consumes all its bytes including the byte that breaks it, gives one U+FFFD, and
  its bytes are kept in a flat, uncapped buffer (to e.g. detect a match with another charset). The decoder counts
  failures. `finish()` ends the input; an unfinished sequence there is a failure.

- 2026-10-21: Warnings are errors in all builds of cratchit's own targets (`-Werror` / `/WX`).

- 2026-10-21: REPL keys are transcoded from the detected terminal input encoding to Unicode before acting on them.
  A key that fails to decode is logged as a design insufficiency. Path editor text is UTF-8 for now. Windows keeps
  `_getch` (0xE0 clash with CP437/CP850 accepted).

- 2026-10-21: `w` at `cratchit>` shows a menu: `w` opens the saved workspace, `b` browses for a new one (folder menu
  1) start folder 2) Documents/cratchit 3) type a path). The browse choice switches the saved workspace.

- 2026-10-21: A project is the bookkeeping of one juridical person (private person, company or organisation, one
  personal or organisation number). Each project has a uuid, kept in its `.cratchit/project.txt` with the official
  `name`. workspace.txt lists `project.<uuid> = <folder>`; the folder may be anywhere. The last opened project is
  remembered and reopened. The same uuid in two folders is an error for now (merge or override may come later).

- 2026-10-22: SIE import: SIE 4E (full, with transactions) only, no SIE 5 yet. Sample files in `../62c6984e/_sie_in`
  (Fortnox, PC8, CRLF). Strict CP437, abort when decoding shows another encoding. The first import sets the project's
  org number and metadata (name, fiscal years, ...); a later file that does not match is an error. The chart of
  accounts is taken as is, designed for a later check against BAS. Import adds new financial events but never changes
  existing ones. The imported file is stashed in the project together with metadata about the import.

- 2026-10-22: An SIE file is read as CP437 unless that gives an unexpected character (box drawing etc.); then it is
  read as UTF-8 if it is UTF-8, with an alert to the user and a log of the applied encoding; else it is an error.
  Charset detection is seeded in `charset::detect`, one test per charset, added as needed.

## Open questions

- Should `#` start a comment line in `path = value` files? Currently not (such a line is an error unless it has `=`).
- Should the terminal encoding be saved (e.g. in runtime.txt) or only detected each start, as now?
- Is a CI setup (e.g. GitHub Actions on macOS/Linux/Windows) wanted to verify cross-platform builds?

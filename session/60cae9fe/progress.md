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

Task 20-21: SIE document (`core/sie/document`). `parse_document(lexed lines)` gives `Parsed{Document, errors}`.
Document holds every SIE 4 record true to the file: identification (#FLAGGA ... #PROSA), chart (#KONTO with #KTYP,
#SRU, #ENHET), #DIM/#UNDERDIM/#OBJEKT, #IB/#UB/#RES, #OIB/#OUB, #PSALDO/#PBUDGET, #VER with transactions
(`Transaction::Kind` normal / added (#RTRANS, its duplicate #TRANS skipped) / removed (#BTRANS, kept, not counted)),
unknown labels counted and ignored, #KSUMMA (CRC-32 per SIE 4B 10.x over `#LABEL` + field texts in CP437, no
separators/quotes/braces/block lines; an escaped quote counts as `"` without its backslash, as the underlined
example in SIE 4B 10.15 shows; a mismatch stops the import, as 10.3 requires checking). Amounts in öre
(`core/amount`, max 2 decimals), dates `std::chrono::year_month_day` (`core/date`). In review.

Task 22: File checks (`core/sie/check`, `core/bas`). Errors: not type 4, no #FNAMN/#ORGNR/#RAR 0, non-SEK, voucher
without series/number, duplicate, outside year 0, unknown account, unbalanced, checksum mismatch. Notes: #FORMAT not
PC8, unknown labels, IB0+trans vs UB0, trans vs RES0, IB0 vs UB-1. Account kind by #KTYP else BAS class (1-2
balance, 3-8 result). In review.

Task 23+25: Import UI and import (`core/books/import`, repl). `i` at `project>`: path typed from the start folder (Tab
completes files too), summary (encoding, program, company, year, accounts, vouchers, notes, errors), `Import it?
(y/n)`; errors -> "Nothing is imported."; all known -> "Nothing to import". `plan_import(books, identity, document)`:
first import sets orgnr + name (project.txt via `save_project_identity`), later mismatch = error; year overlap / other
end = error; identical voucher skipped, differing voucher = error (no mutation), missing vouchers noted; master data
changes only from the latest year (noted); balances checked only, with possible-cause vouchers. `save_import`: writes
books, stashes `.cratchit/imports/<today>_<n>_<file>` + `.txt` record. Logged: read, checked, imported, each note.
In review.

Task 24: Books (`core/books/books`): accounts, dimensions, objects, fiscal years (reported opening + vouchers) in
`<project>/.cratchit/books/` (books.txt, accounts/dimensions/objects.csv, `<start>/opening|vouchers|transactions.csv`,
own CSV module `core/csv`). Only changed files are written. `balances()`: a year directly after another opens with its
closing balances (balance accounts); else the reported opening. In review.

Task 26: Reports (`core/books/report`, `core/pager`). `r` at `project>`: years newest first (0/9 scroll), then
`report?>` 1 trial balance (saldobalans, notes carried-vs-reported opening differences), 2 vouchers (added/removed
marked), 3 account ledger (`account>`), paged with `--More--`. In review.

Tasks 20-26: manual updated (files, typing paths, in a project, "Importing an SIE file", "Reports", log kinds).
309 tests pass, no warnings (Debug, trace DEBUG). Checked with the 11 Fortnox samples: all import through the app via
piped input in mixed year order, re-import gives "Nothing to import", both orders give identical books, and an
independent script confirms every year's closing balances and results equal Fortnox's #UB/#RES except 2026/27
1650/2641, the noted opening difference (cratchit carries its 2025/26 closing: 1650 89.00, 2641 0.10; Fortnox opens
with 0.00 and 90.08, possibly voucher M 5 2026-04-30 Momsrapport). Not built on Linux/Windows or Release; trace OFF/ON
not rebuilt (no trace added).

Tasks 20-26 fix (after review: no way to find the import key): at `cratchit>`, `workspace>` and `project>`, Enter
lists the keys of that prompt (`Keys at project>:` with i, r, l, -, h, q); an unknown key prints `Unknown key.` and
the same list (was `Unknown key. Press h for help.`). Space and tab are still ignored. Manual "Using the prompt" and
the key tables updated. 313 tests pass, no warnings; checked in the app with piped input. In review.

Task 27: Enter a voucher (`core/books/entry`, repl). `e` at `project>` (no books: "No books yet ..."). `voucher>`: heading,
amount and date on one line in any order (`parse_event`: last date word, else today; last other number is the amount,
decimal comma or point, groups of three before a number with decimals, no-break spaces dropped; rest is the heading).
Errors and a date outside the books' years ("Refused: ...") keep the line for editing; `OK? (y/n)`. `series>`: A-N
(Fortnox names), Enter takes the default = series of the voucher sharing most heading words (latest on tie), else A;
`h` is series H here (no help), `q` quits. Number = highest in series and year + 1. `post>`: `account amount`, account
alone = what remains (first row: the amount), not an account = list matches by number prefix or name (max 20), `-`
removes last row / drops the voucher, Enter when balanced shows it (+ note if debit total != amount) and `Save as D 4?
(y/n)`; `n` back to rows. Saved into the year's vouchers.csv/transactions.csv via write_books, registered = today,
logged (business). Re-import of a Fortnox file keeps it and notes it as missing in the file. Manual: new section
"Entering a voucher", key tables, files, log kinds. 331 tests pass, no warnings; checked with piped input on a Fortnox
sample. In review.

Task 27 fix (after review): a voucher saves only when debit and credit each add up to the amount on the voucher line
(`Draft::is_balanced`, `Draft::status`: "Remaining X (debit, credit, amount)", "...: debit and credit must each reach
the amount", "...: more than the amount"); the save note is gone. An account alone on rows that add up to zero (but not
to the amount) asks for an amount. `parse_event` now returns every reading of a spaced amount (last word alone, then
with each three-digit group before it: `87 500` -> 500.00 or 87500.00); several readings must be chosen at `amount?>`
(number, `-` back to the line, `h`, `q`). A no-break space inside a number stays clear. Manual updated. 335 tests pass,
no warnings; checked with piped input on a Fortnox sample.

Task 28: Fiscal years. `y` at `project>` lists the years numbered as SIE (0 = newest), with open/closed; ten per page,
`>` older, `<` newer (keys restart at 0, heading names year of key 0); `year?>` prompt. Opening saves `year = <start>`
in project.txt (`save_project_year`) and is logged; the project reopens it on open (missing year = error, stays at
`project>`). Year prompt is the year itself (`2026/27>`, `2025>`): `e` enter a voucher (only when open), `r` that year's
reports (`-` back to the year), `c` close / `o` open for new vouchers (`closed.<start> = yes` in books.txt,
`FiscalYear::closed`, logged), `l`, `-` back to `project>`, Enter lists keys. `e` moved from `project>` to the year.
Voucher date must be in the open year ("Refused: <date> is outside fiscal year ..."); no date + today outside the year
asks for the date (`parse_event` takes an optional today; `year_for` removed, `FiscalYear::holds` added). An SIE
import into a closed year is not stopped. Manual: new "Fiscal years", files, key tables, entry, reports, log.
341 tests pass, no warnings; checked with all 11 Fortnox samples via piped input (two pages, close/open, restart).
In review.

Task 29: Financial event = Event + Postings (refactor, same UI). `books::Event` (heading, amount, date) moved from
entry.hpp to books.hpp. `books::Transaction` renamed `Posting`; `Postings` = `std::vector<Posting>` with `debit`,
`credit`, `sum` (removed postings do not count). `Voucher` = series, number, `Event event`, registered, signature,
`Postings postings` (was date, text, transactions). A voucher always carries its amount, without sign: entered =
`Draft::target()`, imported = debit total (`voucher_of`). vouchers.csv gets an `amount` column
(`series,number,date,text,amount,registered,signature`); old books without it fail to read (re-import needed).
transactions.csv keeps its name. `Draft` holds `postings` (was `rows`); its debit/credit moved to the free functions.
Import `difference` also names a differing amount. Manual: files table. 343 tests pass, no warnings (Debug, trace
DEBUG); checked in a scratch folder: Fortnox sample imports, re-import gives "Nothing to import", voucher report. In review.

Task 30: Waiting events (events waiting for postings). `books::WaitingEvent` (Event, noted date);
`FiscalYear::waiting` in the order noted. Stored in `<start>/events.csv` (`date,text,amount,noted`), which exists only
while the year has waiting events: `write_books` removes it after the other writes (`Written::removed`, shown as
`Removed ...`). Year prompt: `n` notes an event (`event>` line, same parsing, refusals and `amount?>` as `e`, then
`OK? (y/n)`; amount kept without sign); `w` lists them (`waiting?>`, 1-9, 0/9 scroll over 9): a number posts it
(straight to `series>`, `-` there back to the list; `-` at `post>` with no rows drops the voucher, the event stays);
saving the voucher takes the event off the list. `d` + number + `Delete it? (y/n)` deletes (`delete?>`). Closed year:
`n` and posting refused, listing and deleting allowed. Key list shows `w  Waiting events (<n>)`. Logged (business):
noted, deleted, saved voucher "from a waiting event". Fixed in passing: an empty `voucher>` line went back to
`project>` instead of the year. Manual: new "Waiting events", files, year keys, prompt, log kinds. 347 tests pass, no
warnings; checked in the app on a Fortnox sample (note two, post one as D 27, delete the other, file removed). In review.

Task 31: Template vouchers. After the voucher line (and when posting a waiting event) `template?>` lists at most 5
earlier vouchers like the event (`books::find_templates`): heading shares words (as `guess_series`), series A-N, amount
not zero, at least two counted postings; ranked by shared words, then same sign of amount as the event (seed for
negative amounts), then latest; one per posting pattern (account + side). Each shown with its rows. Enter = 1, a number
picks, `n` none (series guessed as before), `-` back to the line (or the waiting list); `h` help. No candidate: straight to
`series>`. The series default is the template's series; `-` at `series>` goes back to the template list when one was
shown. `books::guide_of` scales each counted posting by amount / template amount (`scaled()` in `core/amount`, exact,
half away from zero; a negative amount flips the rows) and the last row balances; accounts not in the books are left out
with a note. `Draft::guide`, `Draft::from_template`, `Draft::suggestion()`: the guide row at the number of rows so far
(unless balanced), the last one gets what remains. `post>` shows `Suggested <account> <amount> <name>` before the
prompt; empty Enter takes it, a typed row replaces it, `-` steps back. Save log adds `with template D 3`. Manual: new
"The template", series default, rows, waiting list. 355 tests pass, no warnings; checked with piped input on a Fortnox
sample (Telia template D 3 scaled to 1245.00, saved as D 4). In review.

Task 32: View vouchers. `v` at the year prompt: `series?>` lists the year's series (name, count, last number and date;
letter picks, `h` is series H so no help, `-` back to the year), `voucher?>` lists one series newest (highest number)
first (1-9, 0/9 scroll over 9) marked `(entered)` / `(imported)`, a number shows the voucher at prompt `D 4>` (rows with
`(added)`/`(removed)`, registered, origin). Origin is derived, not stored: `books::imported_vouchers(project folder)` reads
the stashed SIE copies in `.cratchit/imports` (not the .txt records; `lex_bytes`, no logging) and gives `VoucherKey`s
(year start, series, number); anything else is entered. No file format change, so existing books keep working.
Key list shows `v`. In review.

Task 33: Correct and template from a voucher. At `D 4>`: `c` corrects an entered voucher (imported: refused, "correct it
in the program it came from and import the file again"): the voucher line starts as `Rättelse av D 4: <heading>
<amount>` (no date = today), then `OK?`, series (default the corrected one's), rows = the corrected rows reversed
(`guide_of` with the negated amount). Saved: `D 5 reverses D 4. ...` + `Enter the right voucher now? (y/n)`; `y`
starts the line with the old heading and amount and the old voucher as template. `t` starts a voucher with the viewed
one as the template (no template list). Both only in an open year. `Draft::corrects`; save log adds `correcting D 4`.
No deletion of vouchers. Manual: new "Viewing and correcting vouchers", year keys, saving, log. 358 tests pass, no
warnings; checked with piped input on a Fortnox sample (imported D 3 refused, D 4 corrected by D 5). In review.

Task 34: Account statement document (`core/statement`, namespace `cratchit::statement`), in memory only, no UI yet.
`decode`: the file must start with a UTF-8 BOM (else "Save it as UTF-8 with a program that writes a BOM"), then strict
UTF-8 via the push buffers (error names the line). `split(text, separator)`: quoted cells with doubled quotes, CR
dropped, trimmed; the empty cell after a line-ending separator is left out only when every line ends with one (Nordea).
`find_separator`: `;`, tab or `,`, most rows with the same cell count (>= 3). `date_of`: pure `YYYY-MM-DD` /
`YYYY/MM/DD`. `amount_of`: pure `-`, digits in optional spaced groups of three (space, U+00A0, U+202F), decimal comma
or point with 1-2 decimals, max 15 digits. `parse_statement(bytes)` -> `Document` (separator, date/amount/saldo/text
columns, `Order` oldest_first/newest_first/unknown, optional `opening`/`closing` saldo, `postings` oldest first:
date, amount, optional saldo, text, line; first/last line, notes). Posting rows: exactly one date cell and an amount;
the postings are the longest run of same-shape rows (two equally long = error). Amount column = amount on every row;
saldo candidates = amount or empty, >= 2 amounts; every pair and order is walked (rows without saldo carry the sum);
exactly one must add up, dates must not go against it. No saldo: only with one amount column and no other candidate,
order from the dates (+ note). One posting row: only with exactly one amount. Text: the other cells joined with ` | `,
empty and repeated cells left out, columns the same on every row dropped (two rows or more). Opening = first saldo
minus the amounts up to it; closing = last saldo plus the amounts after it. Does not log. No manual change (no user
operation). 369 tests pass, no warnings. All 24 files in `_resources/_from_bank_or_skv` run through a scratch program
(not in the repo): 19 parse (Nordea newest first, saldo col 9; SKV oldest first, saldo col 4; each Plusgiro and
Sparkonto file's opening equals the previous file's closing), 4 single-posting files refused (2-3 amounts), the old
ISO-8859-1 `bokf_trans_165567828172.skv` refused (no BOM). In review.

Task 35: Import an account statement (`core/books/statements`, repl). `s` at `project>` and at the year prompt (only
the postings in that year; the rest listed as `Outside`, not imported). `path>` from the start folder (Tab completes
files); a folder lists its files at `statement?>` (1-9, 0/9 scroll over 9), each read: postings, dates, `new` / `<n>
already imported` / `already imported`, or the parse error. A chosen file shows postings, saldo before/after, notes,
then `account>` prefilled with `guess_statement_account`: most postings with the same date and amount on an account in
the books' vouchers, else most postings in statements imported for an account, else the account whose latest
statement ends with this one's opening saldo; a tie suggests nothing. Not an account: lists matches. Then New / Known
/ Outside and `Import it? (y/n)`; nothing new: "Nothing to import". Postings are derived, not stored (agreed b):
`save_statement` stashes the file as `.cratchit/statements/<today>_<n>_<file>` + `.txt` record (imported, source,
account, from, to, postings, added, known, outside); `read_statement_imports` reads them back, `counted_postings`
keeps a stash's `from`-`to` days (a year's days at its prompt, else first year start to last year end, so a posting
left out stays out), `known_postings` counts a posting once over overlapping files (most copies in one file),
`most_known` for the folder list. Logged (business): read, account chosen (as suggested + reason, or typed), imported,
nothing new; a file that cannot be read (design insufficiency). No view of the waiting statement postings yet.
Manual: new "Importing an account statement", files, project and year keys, log kinds. 376 tests pass, no warnings.
Checked with piped input on a scratch project with all 11 Fortnox SIE years and the 24 statement files: suggestions
1920 for PlusGiro (15 of 15), 1630 for SKV (3 of 4), 1930 for Sparkonto (5 of 8); an overlapping file gives "Nothing to
import"; at 2024/25 a file spanning the year imports 2 and lists 6 outside. In review.

Task 36: Naming (no new behaviour). A line of a statement is an *entry*, an amount on a BAS account a *posting*:
`statement::Posting` -> `statement::Entry`, `Document::postings` -> `entries`, `counted_postings` / `known_postings` ->
`counted_entries` / `known_entries`, `StatementPlan::postings` -> `entries` (new records write `entries = <n>`; old
records' `postings` line is not read). UI and messages say entries ("15 of 15 entries match postings on 1920 in the
books", "Line 1 is not an entry."). A year's `transactions.csv` is now `postings.csv`: read from `transactions.csv`
when `postings.csv` is missing, written as `postings.csv` with `transactions.csv` removed (`Initiated` / `Removed`).
Manual: files table, statement section (with the entry/posting words). 377 tests pass, no warnings; checked on the
scratch project with all 11 Fortnox years: one write renamed every year's file, vouchers read back. In review.

Task 37: Waiting events carry postings, from statement entries. `WaitingEvent::postings` (each with date and text);
`Posting::source` (stash:line of the entry; kept on waiting events only, cleared in `Draft::voucher`). Stored in
`<start>/event_postings.csv` (`event,account,amount,date,text,source`, `event` = row in `events.csv` from 1), which exists
only while some waiting event has postings; `events.csv` unchanged. `books::open_entries(books, imports, year)`: the
imported entries (each once over overlapping files) in the year's days (or any fiscal year), by account and date,
minus those booked by a posting on the account with the same date and amount, in a voucher (not removed) or a waiting
event (each posting books one entry). `a` at `project>` and the year prompt: `entries?>` list (1-9, 0/9 scroll over 9);
a number shows the entry (`entry?>`): `n` new waiting event (`heading>` prefilled with the entry text; date, amount
without sign, the entry as its posting), `w` add it to a waiting event of its year (`event?>`); refused in a closed
year; back to the list after, or to the prompt when none are left. The waiting list (and `event?>`) shows an event's
postings and `Draft::status`. Posting a waiting event starts `post>` with its postings (shown first); a template's
rows on accounts the event has are left out (`without_present`) and suggestions start after them
(`Draft::guide_start`); a balanced event saves with Enter. The import ends "Press a to book them." Logged (business):
event noted from an entry, entry added to an event. Manual: new "Statement entries", waiting list, files, key tables,
log kinds. 381 tests pass, no warnings. Checked on the scratch project with all 11 Fortnox years: the 2026-10-20
PlusGiro file lists exactly its 7 entries not in the books (earlier files: all booked); the Loopia entry became a
waiting event and, with Fortnox's E 3 (1920/2440) as template, voucher E 4 (1920 -1308.75, 2440 1308.75). In review.

Task 38: Waiting events by date. `FiscalYear::waiting` is kept oldest date first, ties in the order noted.
`FiscalYear::place_waiting(event)` is the only way an event enters the list (inserted after those of the same date,
returns its index; used by `n` and by `n` on a statement entry, a failed write erases that index). `read_books` sorts
stably after linking `event_postings.csv` rows, so files written in noted order read in date order with their
postings and are rewritten sorted on the next write (no format change). Both lists (`waiting?>`, `event?>`) follow.
Manual: waiting list, `event?>`, files table. 384 tests pass, no warnings (Debug, trace DEBUG). In review.

Task 39: Debit and credit columns (display only; typing at `post>` and the files stay signed). New `core/books/columns`:
`sides_of(amount)` (zero and up debit, below zero credit, without sign), `posting_heading()` (`Account Debit Credit`),
`posting_row(account, amount, rest)`, `posting_total(postings)` (removed not counted), `posting_brief(posting)`
(`1930 C 1118.00`). Used by: voucher view and save preview (heading + rows + Total), rows and `Suggested` at `post>`,
a waiting event's postings (heading + rows + Total, then the status), template list (brief, one line), voucher report
(heading + rows + Total; marks now `(added by X)` / `(removed by X)` after the text), account ledger (Debit and Credit
columns, balance still signed). Trial balance, statement entries and import messages unchanged. Manual: rows, template
list, saving, voucher view, waiting list, reports. 388 tests pass, no warnings (Debug, trace DEBUG). In review.

Task 40: Whole suggested voucher. `Draft::suggested_postings()`: the postings followed by every `suggestion()` in turn.
After the series, when a guide exists (template, `t`, the reversed rows of `c`) and the postings plus suggestions
balance, the whole voucher is shown (`Suggested from D 9, scaled to the amount:` / `The rows of D 4, reversed:`, then
`show_draft`: voucher line, heading, rows, Total) at `Save as D 10? (y/n)`: `y` saves, `n`/`-` goes to `post>` with
the rows in place (`show_row_help` with "The suggested rows are in place. - removes the last one and suggests it
again ..."; `Repl::whole_suggested_`). Not balancing (e.g. a waiting event's postings that do not fit): row by row as
before. Waiting events: event postings first, then the suggestions. Manual: The rows, correcting, waiting list.
389 tests pass, no warnings (Debug, trace DEBUG). Not covered by a REPL test: a waiting event with some postings plus
a template (covered by the Draft test). In review.

Task 41: Flip the template at `postings>`. The save prompt `Save as D 10? (y/n)` (whole suggested voucher and the
preview after Enter at `post>`) is now the prompt `postings>` with a key list: `s` save as D 10, `f` flip the
template's rows (only with a guide: template, `t`, `c`), `-` change the rows (was `n`/`-`), `h`, `q`; Enter lists the
keys (`Keys at postings>:`), an unknown key prints `Unknown key.` and the list. `Draft::flip_guide()` negates the guide,
toggles `Draft::flipped` and drops the postings after `guide_start` (a waiting event's own postings stay, unflipped);
`Repl::apply_guide` (the step after the series, moved out of `on_series_key`) applies it again: whole voucher at
`postings>` when it balances (`Suggested from A 2, scaled to the amount and flipped:` / `The rows of D 4, reversed and
flipped:`), else row by row at `post>`. Flipping twice gives the template back. No flip at `post>`, none of a single
posting, no special case for corrections. Save log adds `, flipped`. Manual: new "Flipping the template", The rows,
Saving (postings> key table), correcting, waiting list, Using the prompt, log kinds. 391 tests pass, no warnings
(Debug, trace DEBUG). Not covered by a REPL test: flipping with a waiting event's postings (covered by the Draft test).
In review.

Task 42: Find matches among waiting events (`core/books/match`). `m` at `waiting?>` lists possible matches
(`match?>`): two waiting events (`same`) or a waiting event and a voucher of the year (`books`), with the same
amount and at most `books::match_window_days` = 4 calendar days apart (no holidays), same fiscal year only.
`find_matches` ranks: fits first (same: both have postings, on different accounts, balancing together; books: the
event's postings all in the voucher), then more shared heading words (`books::shared_words`, new in entry.hpp, as
guess_series), then fewer days, then the older event. A number shows both in full: `same?>` `s` opens `event>`
prefilled with the older event's heading, amount and date (same parsing, `OK? (y/n)`), then replaces the two with
`books::merged` (postings of both, a posting from the same source once, earlier noted day) via `place_waiting`;
`booked?>` `d` + `Delete it? (y/n)` deletes the waiting event, naming its postings not in the voucher
(`books::unbooked`). After either, matches are found again; none left: `No more possible matches.` and the waiting
list. Rejected pairs are not remembered. No closed-year restriction. Logged (business): made one waiting event of
two, deleted as already in the books as X n. No file format change. Manual: new "Finding matches", waiting key
table, log kinds. 399 tests pass, no warnings (Debug, trace DEBUG). Checked on The ITfied AB (backup restored
after): 3 matches (VAT payout 1630 + arrival 1920 fits; Loopia payment vs invoice D 6; VAT decision vs payout,
both on 1630), merged the VAT pair into one balanced event. In review.

Task 43: Hint of counter accounts (`core/books/hint`). `books::hint_for(books, account, debit, excluded)`: the vouchers
and waiting events of all years with a counted posting on the account on the same side (zero and up = debit), latest
first (by date, then the later in the books); counts the accounts on the other side in the latest `hint_counted` = 20
(most first, then the latest) and lists the latest `hint_listed` = 5 (constants in hint.hpp). `sides_in(postings)`:
each account and side once. `?` line at `post>`: hints for every account and side in the rows (no rows: asks for an
account); `? 1930`: that account on its side in the rows, both sides when not in the rows; not an account lists the
matches. `?` key at `postings>` (listed). The waiting event being posted is left out. Row help line mentions `?`.
Read only, nothing logged. Manual: new "The hint", rows table, postings> keys. 407 tests pass, no warnings (Debug,
trace DEBUG). Checked on The ITfied AB (Anthropic waiting event, dropped, nothing written): 1920 C gives 2440 (8),
6570 (4), ...; the latest 5 are mostly waiting events with only the 1920 posting. In review.

Task 44: Template list (`template?>`) and the hint's latest list show each voucher's or event's rows in the Debit and
Credit columns (`Repl::show_counted_rows`: `posting_heading` + `posting_row` with the account name, removed rows left
out, no Total), with the voucher's own amounts. `posting_brief` is kept for the match view's "Not in the voucher"
sentence only. Manual: The template, The hint. 407 tests pass, no warnings (Debug, trace DEBUG). In review.

Task 45: VAT return boxes (`core/skv/vat_return`, namespace `cratchit::skv::vat_return`), in memory only, no UI yet.
`resources/momsrapport/bas_to_momsrapport.csv` (`account;box;code`) is built into the binary like MANUAL.md
(`account_boxes_text()`, internal header `account_boxes.hpp`; `account_boxes(text)`: BOM/CR/blank lines skipped, code
not read, error names the line: account not four digits, box not two digits, account listed before).
`box_of(account)` -> optional `Box` (two-digit string). `Period` (first, last day), `quarter_of(year, 1-4)` calendar
quarters. `document_of(books, period)` -> `Document`: period, boxes by number (only those with postings), each with
its signed sum (debit +), its accounts (sum + `VoucherPosting`s: fiscal year start, series, number, date, amount).
Vouchers by date from every fiscal year (a quarter may span two); removed postings do not count; no rounding, no
box 49 calculation, every series counted (incl. M). Not logged (a broken built-in table logs a design insufficiency).
No manual change. 414 tests pass, no warnings (Debug, trace DEBUG). Not run on real books yet. In review.

Task 46: Numbered options, step 1 of 3 (refactor, no behaviour change). New `core/item_list` (`ItemList`): the page of a
list picked by number (1-9; over 9: 1-8, 0 up, 9 down). `on_key(key, count)` -> `Pick` (None / Scrolled / Item +
index), `show(count, out, item)` writes the up/down lines and calls `item(index, key)` per line, `reset`,
`keep_within(count)`. The eight copies in repl (projects, report years, waiting events, matches, vouchers, statement
files, statement entries, waiting events to add an entry to) use it; `max_keys`/`range_size` left repl.cpp. The year
menu (`<` `>`, keys 0-9 = SIE year numbers) is unchanged. No manual change. 417 tests pass, no warnings (Debug, trace
DEBUG). Only the project list's scrolling is covered by a REPL test; the others by the ItemList tests. In review.

Task 47: Numbered options, step 2 of 3. New `core/operations` (`Operations<Id>`, header only): operations at fixed
numbers 1-9; `add(number, label, id, offered)` (not offered = a gap; two may share a number when only one is offered),
`on_key`, `show`. `Repl::Op`, `Repl::operations()` (per prompt), `on_prompt_key` (the four prompts: `-`, `l`, then the
numbers; replaces `on_year_key` and the per-mode letter code), `on_operation`. `cratchit>`: 1 Workspace menu.
`workspace>`: 1 Projects. `project>`: 1 Fiscal years, 2 Import SIE, 3 Import statement, 4 Statement entries, 5 Reports.
Year: 1 Enter voucher, 2 Note event, 3 Waiting events, 4 View vouchers, 5 Import statement, 6 Statement entries,
7 Reports, 8 Close / Open. Workspace menu: 1 Open saved (gap when none), 2 Browse; hint "Press a number to choose".
Messages naming these keys follow ("Press 2 at project> to import an SIE file.", "Press 8 to open it.", "Press 3 to post
it.", "Press 2 to note one.", "Press 4/6 to book them.", "Press 3/5 to import a statement."). Kept: `-`, `l`, `h`, `q`,
Enter. Not yet: project menu (`n`, `a`), waiting list (`d`, `m`), `same?>`, `booked?>`, `D 4>`, `entry?>`, `postings>`,
`template?>` (step 3). Manual: workspace menu, workspace, project, fiscal years, year table, entry, noting, waiting
list, statement entries, SIE/statement import, reports, Using the prompt (new paragraph on fixed numbers). Tests
rewritten for the new keys, + 3 Operations tests. 420 tests pass, no warnings (Debug, trace DEBUG); checked with piped
input in a scratch folder. Not REPL-tested: the key list of a closed year (8 Open ...), covered by the Operations test.
In review.

Task 48: Numbered options, step 3 of 3. Lists only pick; a picked item shows its operations by number (`Operations`,
`Repl::show_operations`: the operations, `d` where it applies, then "Press a number ..."). Project menu (`project?>`):
1 Open a project (n listed; left out when none), 2 New, 3 Add existing; 1 lists them at `open?>` (new
`Mode::ProjectList`, `-` back to the menu). Waiting list (`waiting?>`) only picks; the event is shown at `waiting>`
(new `Mode::WaitingEvent`, `chosen_waiting_`): 1 Post it, `d` Delete it (`Delete it? (y/n)`; `n`/`-` back to the
event); `deleting_`, `delete?>` and `to_delete_` are gone. Finding matches moved to the year prompt as 9 (reads the
books first; `-` at `match?>` goes back to the year; none: message at the year). `same?>`: 1 Make them one event;
`booked?>`: `d` as before. `D 4>`: 1 Correct it, 2 Use it as a template. `entry?>`: 1 New waiting event, 2 Add to a
waiting event (left out when the year has none). `postings>`: 1 Save as X, 2 Flip (only with a template); `?`, `-`
kept. `template?>`: `0  None: type the rows yourself` (was `n`). Only `?` and `d` remain as letter/sign operations
(plus `-`, `q`, `h`, `l`, Enter, y/n). The match view's sentence now says "a statement entry is listed again".
Manual: project menu (+ `open?>`), template, rows/postings keys, flipping, saving, voucher view, correcting, waiting
list (+ `waiting>`), finding matches, statement entries, year table (9), Using the prompt (kept symbols, lists only
pick). 420 tests pass, no warnings (Debug, trace DEBUG and OFF); checked with piped input in a scratch folder. Test
names still use the old letters (e.g. `NKeyNotesAnEventAndWPostsIt`); not renamed. Not REPL-tested: help returning to
`open?>` / `waiting>`. In review.

Task 49: Only numbered choices are shown. Menus, lists, views and the key lists show only their numbered choices; the
"Press ..." hint lines (15 screens) and the `l`, `-`, `h`, `q`, `d`, `?` lines are gone. `Operations::other(key, word)`
names a non-number key and `one_liner()` gives "Other keys: Enter list, - back, h help, q quit."; `Repl::operations()`
adds them per mode (prompts + `l`; `waiting>` + `d delete`; `booked?>` + `d delete the waiting event ...`;
`postings>` + `? hint of counter accounts`, `- change the rows`; `amount?>` `- change the line`; `template?>`
`Enter use 1`). Any unknown key prints `Unknown key. <one-liner>` (`Repl::unknown_key`). Enter shows the screen
again in every single-key mode with numbered choices (`Repl::shows_on_enter`, `Repl::show_screen`, which also
replaces the help-return chain); at the four prompts it is `Keys at X:` + the numbers; at `postings>` the voucher and
its numbers. `?` stays the hint (counter accounts at `post>` / `postings>`). Year list (`year?>`): `ItemList` with
`Start::Zero` (keys 0-9 for up to ten years; more: first page 0-8 + `9 ...down`, later pages `0 ...up`, 1-8,
`9 ...down`; heading "(key 1 is year N)"); `<` `>` gone. Help browser: no key line; Enter shows the section again;
unknown key "Other keys: Enter this section again, - back, q leave help."; "No section 5 here." alone; no trailing
blank line. `event?>` heading is now "Add the entry to a waiting event in fiscal year ...". Kept: typing prompts,
y/n questions, `series>`/`series?>` letters and their lines, `--More--` (pager, log, help paging). Manual: screen
examples, year list, Using the prompt (new paragraph + example), Using help, Enter rows. 425 tests pass, no warnings
(Debug, trace DEBUG and OFF); checked with piped input. Not REPL-tested: the year list with more than ten years
(ItemList tests cover the paging). In review.

Task 50: `-` and `?` act at once on an empty typed line. Typing was already key by key in cratchit's own buffer
(`PathEditor`, terminal in single-key mode). `PathEditor::Result` gains `Back` (`-` on an empty buffer, any kind; echoed
`-`) and `Hint` (`?` on an empty row; stays in the buffer as `?`). `Repl::on_typing_key` maps `Back` to the existing
`Cancelled` path (same place as Enter on an empty line), except at `post>`, where `Back` removes the last row / drops
the voucher (was `-` + Enter; a typed `-` line is now an ordinary row error). `Hint` shows the rows' hint and
`start_rows(out, "?")` brings `post>` back with `?` on the line; `? 1930` + Enter hints that account, `?` alone + Enter
just clears it (no repeat). A prefilled line (path, guessed account, heading, correction/merge line) is not empty:
`-` is typed there (agreed a). On-screen "Enter on an empty ... goes back" texts now say "- or Enter ...". Manual:
typing a folder path (table + `-` row), creating a project, voucher line, rows table (`-`, `?`), The hint, waiting
event line, matches, statement path/account, account ledger, Using the prompt (new paragraph). 428 tests pass, no
warnings (Debug, trace DEBUG). In review.

Task 51: The key lists at `project>` (4) and the year prompt (6) show how many statement entries are not yet booked:
`Statement entries not yet booked (7)` / `... in this year (2)`. Counted only when the key list is shown (Enter), not
per key: `Repl::count_unbooked()` reads the statement stashes (and at `project>` the books) quietly, as the list does;
on a read error the count is left out. `Repl::unbooked_count_` holds it for `operations()`; `show_keys` is no longer
const. Manual: project and year key tables, key list example. 428 tests pass, no warnings (Debug, trace DEBUG). Only
the project count is REPL-tested with entries (the year count is tested at 0). In review.

Task 52: VAT return report, step 1. `project.txt` gets `vat_period = none | monthly | quarterly | yearly`
(`VatPeriod`, `vat_period_name` / `vat_period_of`, `Project::vat_period` optional, `save_project_vat_period`); any other
value is a read error. `create_project` writes `quarterly` (`default_vat_period`); opening a project without the entry
writes it (`Updated .cratchit/project.txt`, business log). No UI to edit it yet. `report?>` 4 VAT return (momsrapport):
quarterly lists `period?>` (new `Mode::VatPeriods`, ItemList, newest first) with `skv::vat_return::quarters_ending_in`
(the calendar quarters whose last day is in the fiscal year; a quarter may start in the year before); none: "No VAT
returns ..."; monthly/yearly: "not supported yet". A quarter shows `report_of(books, document_of(...), company)`
(`core/skv/vat_return/report`, `quarter_name`): each box with its amount, its accounts with sum and name, the voucher
postings; amounts signed as in the books (sign rules for the form later), box 49 not calculated, then the periods again
(`Repl::before_paging_`: a paged report goes back to the menu it came from). Manual: project.txt (`vat_period`), new
"The VAT return" under Reports, log kinds. 435 tests pass, no warnings (Debug, trace DEBUG); checked with piped input
in a scratch folder. In review.

Task 53: VAT steps per quarter: period, cleared, charged. Named constants: `skv::tax_account` = 1630
(`core/skv/accounts.hpp`), `vat_liability_account` = 2650, `vat_receivable_account` = 1650, `vat_boxes` (10-12,
30-32, 60-62, 48) in `vat_return/box.hpp` with `is_vat_account`, `is_vat_clearing_account`. `vat_return::steps_of(books,
periods)` (`core/skv/vat_return/steps`), by accounts, never by series: clearing voucher = counted rows on 2650/1650 and
on a VAT account (cleared = its 2650/1650 rows, in the quarter it is dated in); charge voucher = rows on 2650/1650 and
1630, none on a VAT account (charged = its 2650/1650 rows negated); period VAT = VAT-account rows of the other
vouchers. All signed as on the VAT accounts, so a done quarter shows the same amount three times. Charges in date
order go to the oldest quarter cleared by the same amount, starting on or before the charge, without a charge yet,
over all quarters of the books (`Steps::unmatched_charges` else). Payments 1630 <-> bank are no step. `period?>` list:
column heading + `Period Cleared Charged` per quarter + `cleared by M 1, charged by A 1`; unmatched charges dated in
the year listed under it. The quarter report ends with "The VAT of the period:" and the step vouchers. The boxes
still count every voucher (clearing and charge rows too). Manual: "The VAT return" rewritten. 440 tests pass, no
warnings (Debug, trace DEBUG). Not run on real books (the Fortnox samples are outside this session). In review.

## Next

Later: removing a single posting from a waiting event, the source reference in the saved voucher, typing postings
into a waiting event without making a voucher, several entries chosen at once.

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

- 2026-10-22: SIE import answers: imported vouchers are never changed; a differing voucher with the same series and
  number stops the whole import. Balances are calculated by cratchit: a year's opening balances are the previous year's
  closing balances in the books; the file's #IB/#UB/#RES are only checked, differences are noted to the user and logged
  with the vouchers that may explain them, and ignored as long as the vouchers do not conflict. Year -1 data in a file
  is check only. Account names etc. come from the newest fiscal year imported.

- 2026-10-22: Enter or an unknown key at a prompt lists the keys available there.

- 2026-10-22: Voucher entry (planned, not started). A new voucher goes into a series by its type. The series are
  Fortnox's defaults (no BAS or legal standard found; the law only requires numbered vouchers in order):
  A Redovisning, B Kundfakturor, C Inbetalningar från kunder, D Leverantörsfakturor, E Utbetalningar till
  leverantörer, F Kassa, G Avskrivning, H Periodisering, I Bokslut, J Kontantfaktura, K Kvitto, L Lön,
  M Momsrapport, N Lager. The imported books use A, B, C, D, E, I and M.
  Entry starts with heading, amount and date; then the series is chosen, with one guessed from the input as
  default. A date outside the open fiscal year is refused with a reason. No source-document reference
  and no correction help yet. Entered vouchers go into the same books as imported ones. Local vouchers are the
  truth (better sync rules with Fortnox later).

- 2026-10-22: Vouchers are entered in an opened fiscal year (`y` at `project>`). Years are numbered as SIE: 0 is the
  newest. The last opened year is remembered and reopened; `-` leaves it. Each year has an open/closed setting; new
  vouchers only in an open year. No "new year" operation yet: years come from SIE import. The year prompt is the
  fiscal year itself, e.g. `2026/27>`.

- 2026-10-23: A financial event has two parts: the Event (heading, amount, date: how it enters the books) and its
  Postings (amounts to BAS accounts). A voucher always carries its amount, without sign for now (negative amounts are
  new behaviour if needed). `Transaction` is called `Posting` in cratchit (SIE's #TRANS is not a transaction).
  Events waiting for postings and postings waiting for an event are kept inside the books, on their fiscal year.

- 2026-10-24: Template vouchers: after the event, a few earlier vouchers like it are offered; the chosen one guides the
  rows one at a time (Enter takes the suggestion), scaled to the new amount. Series step unchanged (default from the
  template; without one, guessed as before). Matching is on heading words; amount sign is a tie-breaker, ready for
  negative amounts (credit invoices).

- 2026-10-24: Vouchers are never edited or deleted; a wrong voucher is corrected with a new voucher reversing it, in
  the same series, optionally followed by the right one. Imported vouchers are not corrected in cratchit (for now).
  Whether a voucher is imported is derived from the stashed SIE files, not stored.

- 2026-10-24: Account statements are parsed into an in-memory document of valid postings (date, amount, text,
  optional saldo), found by cell content only. Files must have a UTF-8 BOM. A single posting row is accepted only with
  exactly one amount. The posting text joins the text cells with ` | ` (for the user to edit; no sample cell has `|`). Opening and closing saldo
  are kept as optional data when the saldo adds up. Using the document is designed later.

- 2026-10-24: Statements are imported on request (`s` at `project>` and at a year, the year limiting to its days), not
  from a watched folder; a folder path lists its files. Files are stashed in `.cratchit/statements` with a record;
  the postings are derived from the stashes, not stored in a file of their own. The BAS account is suggested by
  matching postings first (books, then earlier statements), then by saldo, and confirmed by the user. Key events of
  the import are logged.

- 2026-10-24: Words: a statement line is an *entry* (`statement::Entry`), an amount on a BAS account a *posting*
  (`books::Posting`, `postings.csv`, was `transactions.csv`). A waiting event may hold several postings, from entries of
  different statement files, and is completed into a voucher posting by posting. Storage reuses the voucher design:
  `events.csv` + `event_postings.csv`.

- 2026-10-25: Waiting events are listed by date, oldest first, ties in the order noted. The order is kept in the books
  (`place_waiting` is the only way in), so a future date edit re-places the event through the same function.

- 2026-10-25: Posting lists show the amount without sign in a Debit or a Credit column (zero as debit), with a heading
  and a Total line where a whole voucher or a waiting event's postings are shown; a removed posting stays in its column,
  marked, and does not count in the totals. Template lists and the hint's latest list show the rows in the columns too (heading, no Total, removed rows left
  out; was the one-line form, changed 2026-10-25). The one-line form `1930 C 1118.00` stays only inside a sentence (the
  match view's "Not in the voucher"). Labels Debit/Credit.
  Input stays signed; balances (trial balance, ledger balance) stay signed.

- 2026-10-25: With a template (also `t` and the reversed rows of a correction), the whole suggested voucher is shown
  after the series to save with `y`; `n` puts the rows in place at `post>` to change them from the end (`-` steps back
  to a suggestion). Only when the rows balance; else row by row as before.

- 2026-10-25: Matches of waiting events: same amount, at most 4 calendar days apart (a constant in code), same
  fiscal year. Kinds: two waiting events (made one, heading from the oldest event, editable) or a waiting event and
  a voucher (deleted as already in the books). Voucher matches are shown even without shared words. Rejected pairs
  are not remembered. No closed-year rule for now.

- 2026-10-25: Flip works on the template only (flip it, then apply it again), offered as `f` at the `postings>`
  prompt (which replaced `Save as ...? (y/n)`), not at `post>` and not on single postings. No special detection for
  corrections.

- 2026-10-25: Hint of counter accounts on request only (`?` at `post>` and `postings>`), from vouchers and waiting
  events with the account on the same side as in the rows; counts and list length are constants to tweak later.

- 2026-10-26: VAT return: first step only adds up postings on accounts into boxes by `bas_to_momsrapport.csv`;
  `momsrapport.csv` (box texts, sections) comes later. Named for the SKV domain (`skv::vat_return`, `box_of`,
  `Document`), not as a mapping. Calendar quarters. Document only, no UI. The Momsrapport voucher (series M) is
  counted like any other: it is meant to zero out what has been reported to SKV.

- 2026-10-26: Operations are picked by number, not letter. Kept as symbols: `-` back, `q` quit, `h` help, `l` log, Enter
  lists the keys, `y/n` confirm, `d` delete, `?` hint; series letters A-N (data, not operations). A screen either lists
  items or operations, never both: pick the item (repeat for sub-items), then its numbered operations. An operation's
  number is fixed; one shown only sometimes leaves a gap rather than renumbering the others. Steps: 1) `ItemList` for
  the lists, 2) the four main prompts and the workspace menu, 3) the other prompts. In the template list `0` is "no
  template" (lists start at 1, so 0 is free).
- 2026-10-26: Only numbered choices are shown in menus, lists and views. Other keys are named in one line by an
  unknown key; Enter shows the numbered choices again. `?` is the hint where a hint is needed (what it hints
  depends on the screen). The year list pages with 0/9 like the others (key = SIE year number on the first page).
  Help follows the same rule; typing prompts, y/n questions, series letters and `--More--` are kept as they are.
- 2026-10-26: At a typing prompt a key on an empty line may act at once: `-` goes back (at `post>`: removes the last
  row), `?` at `post>` shows the hint and stays on the line (an account typed after it hints on Enter). Only on an
  empty line; a line with text typed in for you is not empty. Digits, `h` and `q` stay text (lines can begin with them).

- 2026-10-27: The project's VAT period (`vat_period` in project.txt: none, monthly, quarterly, yearly = the fiscal
  year) says how often SKV expects a VAT return; default quarterly, written when missing; an edit UI later. A quarter
  belongs to the fiscal year its last day is in. The VAT return keeps the books' signs for now; rules for how they
  fill in the form come later.

- 2026-10-27: VAT steps, named in English: period VAT (on the VAT accounts), cleared (moved to 2650/1650, the books'
  record of the filed return), charged (SKV debits or credits the tax account 1630 against 2650/1650), paid (bank <->
  tax account; not a VAT step, it pays the tax account's balance; a later tax account report). Detected by accounts
  (named constants), never by voucher series.

## Open questions

- Should a template's rounding row (e.g. 3740 Öres- och kronutjämning) be scaled like the others? Today it takes the
  scaling remainder, so VAT ends up slightly off (Telia 1245.00: 2641 248.97, 3740 0.14).
- Should an SIE import into a closed fiscal year be refused (it is allowed now)?
- Should differing account data (name, SRU) from an older year's file be noted even when not taken?
- Should `#` start a comment line in `path = value` files? Currently not (such a line is an error unless it has `=`).
- Should the terminal encoding be saved (e.g. in runtime.txt) or only detected each start, as now?
- A quarter that spans two fiscal years is listed only under the year it ends in (agreed 2026-10-27; SKV accepts it).
  A split at the fiscal year end may come later. With a broken year the last months (e.g. April) are in a quarter
  of the next year, so they show only once that year is in the books.
- Should the box report (the form) leave out clearing and charge vouchers? Today a cleared quarter shows box 10 etc.
  as 0.00 and box 49 with the clearing, and a charge shows up as box 49 in the quarter it is dated in.
- Is a CI setup (e.g. GitHub Actions on macOS/Linux/Windows) wanted to verify cross-platform builds?

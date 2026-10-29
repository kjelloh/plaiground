# cratchit user manual

cratchit is a single-user app for Swedish bookkeeping. This manual describes
everything you can do with it. It grows as cratchit grows.

## Starting cratchit

Run `cratchit` from a terminal (see README.md for how to build it):

```sh
./build/Debug/cratchit
```

cratchit prints its version, then opens the workspace (see The workspace) and
the project you last opened in it (see Projects).
The first time, it asks you to choose a workspace folder. After that it opens
the same workspace each time and shows its prompt:

```
cratchit 0.1.0. Press h for help, q to quit.
Opened workspace /Users/me/Documents/cratchit
workspace> 
```

## Terminal encoding

When cratchit starts, it finds out which character encoding your terminal
uses for what you type (input) and for what cratchit prints (output). It keeps
these for the rest of the run. Inside, cratchit works in Unicode no matter what
the terminal uses.

How cratchit finds the encoding:

| Platform | Where it comes from                                                      |
|----------|--------------------------------------------------------------------------|
| macOS    | Always UTF-8.                                                            |
| Android  | Always UTF-8.                                                            |
| Linux    | Your locale: `LC_ALL`, `LC_CTYPE` or `LANG`, e.g. `sv_SE.UTF-8`.         |
| Windows  | The console's input and output code pages (see `chcp`), e.g. 850 or 65001. |

When cratchit cannot yet handle the encoding, or does not recognise it, it uses
UTF-8 and warns you before it does anything else, for example:

```
Warning: Terminal output encoding code page 850 (CP850) is not supported yet. Using UTF-8.
Warning: Terminal input encoding KOI8-R is unknown. Using UTF-8.
```

Input and output are checked separately, so you may get a warning for only
one of them. Today cratchit can read UTF-8 and CP437 (code page 437) and write UTF-8. To
avoid the warnings on Windows, run `chcp 65001` before starting cratchit. On
Linux, use a UTF-8 locale.

Every key you press is read in the terminal's input encoding and turned into
Unicode before cratchit acts on it. So you can type letters such as `å`, `ä`
and `ö` in a folder path, whichever of the supported encodings the terminal
uses. A key that does not make sense in that encoding is shown as `�`, treated
as an unknown key, and noted in the log.

On Windows with code page 437 or 850, the character `α` (437) or `Ó` (850)
cannot be typed: the console sends the same code for it as for the start of an
arrow key.

## The .cratchit folder

cratchit keeps its data in a folder named `.cratchit` in the folder you start
it from. Each set of books is a folder of its own: go to that folder and run
cratchit there.

### Fresh start

If `.cratchit` does not exist, cratchit creates it with two files,
`framework.txt` and `runtime.txt`, and reports each new file:

```
Initiated .cratchit/framework.txt in <folder>
Initiated .cratchit/runtime.txt in <folder>
```

### The files

Each file holds lines of `path = value`. Spaces around `=` do not matter, and
a value may contain spaces. Blank lines are ignored. A line without `=`, or a
path given twice, is an error.

`framework.txt` holds the date it was created and where the runtime is:

```
created = 2026-10-20
runtime = .
```

`runtime` names the folder whose `.cratchit` holds `runtime.txt`. `.` means
the same folder as `framework.txt`, which is the default and also what a
missing `runtime` line means. A relative folder is taken from the folder
holding `.cratchit`, for example `runtime = ../shared`. A full path such as
`runtime = /Users/me/shared` works too.

`runtime.txt` holds the date it was created and, once you have chosen one,
where the workspace is:

```
created = 2026-10-20
workspace = /Users/me/Documents/cratchit
```

`workspace` names the folder whose `.cratchit` holds `workspace.txt`.
cratchit writes it as a full path. If you write a relative folder yourself, it
is taken from the folder holding the runtime's `.cratchit`.

`workspace.txt` holds the date it was created and, once you have created or
added projects, the projects it lists and the one you last opened:

```
created = 2026-10-20
project.3f2b8c1e-9a4d-4e7f-b6a2-1c5d8e0f7a93 = /Users/me/Documents/cratchit/ITfied AB
project.0a1b2c3d-4e5f-4a6b-8c7d-8e9f0a1b2c3d = /Volumes/share/Föreningen
project = 3f2b8c1e-9a4d-4e7f-b6a2-1c5d8e0f7a93
```

Each `project.<uuid>` line lists a project: the path holds the project's uuid
and the value its folder. cratchit writes the folder as a full path. If you
write a relative folder yourself, it is taken from the workspace folder.
`project` names the uuid of the project you last opened.

`project.txt`, in a project folder's `.cratchit`, holds the date the project
was created, its uuid, the official name of the person, company or
organisation whose books it is, how often it reports VAT, after the first
import its organisation number, and once you have opened one, the start of the
fiscal year you last opened:

```
created = 2026-10-21
uuid = 3f2b8c1e-9a4d-4e7f-b6a2-1c5d8e0f7a93
name = The ITfied AB
vat_period = quarterly
orgnr = 556782-8172
year = 2026-05-01
```

The uuid is made by cratchit when it creates the project and never changes.
The first SIE import sets `orgnr` and `name` (see Importing an SIE file).
Opening a fiscal year sets `year` (see Fiscal years).

`vat_period` is how often SKV expects a VAT return (momsdeklaration) of the
project: `none` (not registered for VAT), `monthly`, `quarterly` or `yearly`
(per fiscal year). Any other value is an error when the project opens. A new
project gets `quarterly`. A project without the entry gets `vat_period =
quarterly` when it opens, shown as `Updated .cratchit/project.txt in <folder>`.
cratchit has no operation to change it yet; edit the file. Only `quarterly`
VAT returns can be shown so far (see Reports).

Booking the VAT clearing voucher of a quarter (or writing the file alone, to test
it) writes its eSKD file in the project folder itself (not in `.cratchit`), named `moms_<period>.eskd`, e.g.
`moms_202512.eskd`, for you to upload to Skatteverket (see Clearing the VAT of a
quarter). cratchit never overwrites one: when the name is taken it writes
`moms_202512_2.eskd`, then `_3` and so on. cratchit does not read these files.

Exporting a fiscal year (`2` at its prompt, see Exporting a fiscal year as an SIE
file) writes `sie_2026-27_all.se` or `sie_2026-27_diff.se` (`sie_2025_all.se`
for a calendar year), named by what it holds, in the
project folder in the same way, never overwriting an earlier export
(`sie_2026-27_all_2.se`, ...). An SIE import that makes the file the truth of a
year writes such an export of the year first (see Making the file the truth).

The project's books are in `.cratchit/books` in the project folder:

| File | What it holds |
|------|---------------|
| `books.txt` | `chart_type` (e.g. `EUBAS97`), `company_id`, `address.contact`, `address.street`, `address.postal`, `address.phone`, one `year.<start> = <end>` line per fiscal year, and a `closed.<start> = yes` line per year closed for new vouchers, dates as YYYY-MM-DD. |
| `accounts.csv` | The chart of accounts: `account,name,type,sru,unit`. |
| `dimensions.csv` | `dimension,name,super` (`super` only for a sub dimension). |
| `objects.csv` | `dimension,object,name`. |
| `<start>/opening.csv` | Per fiscal year, in a folder named by its start date: the opening balances the source reported, `account,amount`. |
| `<start>/vouchers.csv` | The year's vouchers, imported and entered: `series,number,date,text,amount,registered,signature`. The amount is without sign: what the debit rows and the credit rows each add up to. An imported voucher gets its debit total (SIE gives no amount). A voucher entered in cratchit is registered on the day it is saved. |
| `<start>/postings.csv` | Their rows (postings to BAS accounts): `series,number,kind,account,objects,amount,date,text,quantity,signature`. Books written before 2026-10-24 have it as `transactions.csv`; cratchit reads that and renames it on the next write (`Initiated ... postings.csv`, `Removed ... transactions.csv`). |
| `<start>/events.csv` | The year's waiting events, by date (oldest first, the same date in the order noted): `date,text,amount,noted` (see Waiting events). The amount is without sign; `noted` is the day it was noted. The file exists only while the year has waiting events: cratchit removes it when the last one is posted or deleted (`Removed ...`). |
| `<start>/event_postings.csv` | The postings the waiting events have so far (see Statement entries): `event,account,amount,date,text,source`. `event` is the event's row in `events.csv` (1 for the first), `date` and `text` those of the statement entry, `source` the stashed statement file and line it came from. The file exists only while some waiting event has postings. |

The `.csv` files are comma separated with a header line. A field holding a
comma, a quote or a line break is quoted, with quotes doubled. Amounts have a
point and two decimals, debit positive and credit negative, for example
`-6498.77`. `kind` is `normal`, `added` (a row added after the voucher was
registered) or `removed` (a row struck after it was registered; kept for the
history but not counted). `objects` holds dimension and object pairs as a comma
separated list, for example `1,100,6,P1`. `type` is `T`, `S`, `K` or `I`
(asset, liability, cost, income) when the source gave one.

`.cratchit/imports` in the project folder keeps an unchanged copy of every
imported file, named `<date>_<n>_<file name>`, and next to it a record
`<date>_<n>_<file name>.txt` of the import: `imported`, `source`, `encoding`,
`program`, `generated`, `company`, `orgnr`, `year`, `vouchers_added`,
`vouchers_changed`, `vouchers_known`, `notes` (the number of notes shown) and,
when the import made the file the truth of its year (see Making the file the
truth), the vouchers `renamed` (e.g. `D 5 as D 7`), `replaced`, `dropped` and
`demoted` (e.g. `A 12, D 4`) and `exported_before` (the export written just
before, e.g. `sie_2026-27_all.se`).

`.cratchit/statements` in the project folder keeps an unchanged copy of every
imported account statement file, named `<date>_<n>_<file name>`, and next to
it a record `<date>_<n>_<file name>.txt`: `imported`, `source`, `account` (the
BAS account the statement is for), `from` and `to` (the days whose entries
count), `entries` (in the file), `added`, `known` and `outside`. The
statement entries are kept nowhere else: cratchit reads them from these
copies when it needs them.

`log.txt`, next to `runtime.txt`, is the log (see The log). cratchit creates
it the first time it logs something and only ever adds lines to it.

### When something is missing

cratchit changes an existing file only when you ask it to: choosing a new
workspace sets the `workspace` line in `runtime.txt`, keeps the other lines,
and reports `Updated .cratchit/runtime.txt in <folder>`. Creating, adding or
opening a project lists it in `workspace.txt` and saves it as the last opened,
keeping the other lines, and reports `Updated .cratchit/workspace.txt in
<folder>` when the file changes. Importing an SIE file adds to the books and
reports each books file as `Initiated` or `Updated`.

cratchit adds a missing `runtime.txt` to its own `.cratchit` (when `runtime` is
`.`) and reports it as `Initiated`. It never creates a runtime in another
folder.

cratchit prints an error and stops when:

- `.cratchit` is a file rather than a folder, or lacks `framework.txt`
- a file has a line that is not `path = value`
- the `runtime` folder does not exist
- a `runtime` folder other than `.` has no `.cratchit/runtime.txt`
- cratchit cannot create a file it needs

To start over, remove or rename the `.cratchit` folder.

Problems with the workspace do not stop cratchit; see The workspace.

## The workspace

The workspace is the folder cratchit works in. Its `.cratchit` folder holds
`workspace.txt`.

### Opening the workspace

cratchit opens the workspace when it starts:

- If `runtime.txt` names a workspace folder, cratchit opens it and shows
  `Opened workspace <folder>`. A folder without `.cratchit` is a fresh
  location: cratchit creates `.cratchit/workspace.txt` there and reports it as
  `Initiated`.
- If no workspace is set, or its folder does not exist, cratchit asks you to
  choose one (see Choosing a workspace).
- If the folder has a `.cratchit` without `workspace.txt`, or a malformed
  `workspace.txt`, cratchit shows an error and stays at the `cratchit>` prompt.
  Press `1` and then `2` to choose another workspace (see The workspace menu).

### The workspace menu

Press `1` at the `cratchit>` prompt to open the workspace again or switch to
another one:

```
Workspace:
  1  Open /Users/me/Documents/cratchit
  2  Browse for a new workspace
```

| Key    | What it does                                              |
|--------|-----------------------------------------------------------|
| 1      | Opens the workspace saved in `runtime.txt`.               |
| 2      | Lets you choose another folder (see Choosing a workspace). |
| -      | Goes back to the `cratchit>` prompt.                      |
| h      | Opens this manual.                                        |
| q      | Quits cratchit.                                           |
| Ctrl-D | Quits cratchit.                                           |

So `1` `1` at `cratchit>` takes you back into the workspace. When no workspace
is saved, or its folder does not exist, only `2` is offered (it keeps its
number).

Choosing a folder with `2` switches the workspace: cratchit saves it in
`runtime.txt` and opens it from then on. The old workspace folder is left as it
is.

### Choosing a workspace

cratchit offers these folders, both on start when no workspace is set and
after `2` in the workspace menu:

```
Choose a workspace folder:
  1  /Users/me/books
  2  /Users/me/Documents/cratchit
  3  Type a folder path
```

| Key    | What it does                                                     |
|--------|------------------------------------------------------------------|
| 1      | Uses the folder you started cratchit in.                         |
| 2      | Uses `cratchit` in your Documents folder, creating it if needed. |
| 3      | Lets you type a folder path (see Typing a folder path).          |
| -      | Goes back: to the workspace menu after `2` there, else to the `cratchit>` prompt without a workspace. |
| h      | Opens this manual.                                               |
| q      | Quits cratchit.                                                  |
| Ctrl-D | Quits cratchit.                                                  |

The Documents folder is `~/Documents` on macOS. On Linux it is the one set in
`~/.config/user-dirs.dirs` (for example `~/Dokument`), else `~/Documents`. On
Windows it is your Documents folder, also when it has been moved, for example
to OneDrive. If cratchit cannot find it, option 2 is not shown.

When you choose a folder, cratchit adds `.cratchit/workspace.txt` to it if it
is missing, saves the folder in `runtime.txt` and opens the workspace. Choosing
the workspace that is already saved just opens it and leaves `runtime.txt` as
it is.

### Typing a folder path

For a workspace, the path starts at the root folder (`/`, or for example `C:\`
on Windows). For a project folder it starts at the workspace folder, and for
an SIE file to import at the folder cratchit started in. You type the rest,
one key at a time:

| Key       | What it does                                                  |
|-----------|---------------------------------------------------------------|
| Tab       | Completes the folder name you have started (see below). For an SIE file, file names too. |
| Backspace | Erases the last character.                                    |
| Enter     | Accepts the path. On an empty path, goes back to the choices or the project menu. |
| -         | On an empty path, goes back at once, as Enter does. Else it is part of the path. |
| Ctrl-D    | Quits cratchit.                                               |

Every other key, including `q` and `h`, is part of the path. Arrow keys
do nothing.

Tab works like in a shell. When one folder matches what you have typed, its
name is completed and a separator added. When several match, the name is
completed as far as they agree, and when they agree on nothing more, Tab
lists them. When none
match, the terminal beeps. Hidden folders (starting with `.`) match only when
you type the dot. When typing the path of a file, a unique file name is
completed without a separator after it.

A relative path is taken from the folder cratchit started in (for a workspace)
or from the workspace folder (for a project), and `..` goes up one folder.

When the folder exists, cratchit uses it. When it does not, cratchit asks
`Create it? (y/n)`. Press `y` to create it, or `n` to go back and change the
path. When adding an existing project, a missing folder is not created: cratchit
says so and lets you change the path.

### In the workspace

The prompt is `workspace>`.

| Key    | What it does                                 |
|--------|----------------------------------------------|
| 1      | Shows the project menu (see Projects).       |
| Enter  | Lists the numbered keys.                     |
| h      | Opens this manual.                           |
| l      | Shows the log (see The log).                 |
| -      | Goes back to the `cratchit>` prompt.         |
| q      | Quits cratchit.                              |
| Ctrl-D | Quits cratchit.                              |

When the workspace opens, cratchit also opens the project you last opened in
it, and the prompt is `project>`. Press `-` to get to `workspace>`.

## Projects

A project is the bookkeeping of one juridical person: a private person, a
company or an organisation, with one personal or organisation number. Each
project has a folder of its own, which can be anywhere. Its `.cratchit` holds
`project.txt` with the project's uuid and the official name (see The files).

The workspace lists its projects in `workspace.txt`. The same project can be
listed in more than one workspace.

### The project menu

Press `1` at the `workspace>` prompt:

```
Projects in /Users/me/Documents/cratchit:
  1  Open a project (2 listed)
  2  New project
  3  Add an existing project folder
```

The prompt is `project?>`.

| Key    | What it does                                                 |
|--------|--------------------------------------------------------------|
| 1      | Lists the projects to open one (below). Not shown when no project is listed. |
| 2      | Creates a new project (see Creating a project).              |
| 3      | Adds a project folder that already exists (see Adding an existing project). |
| -      | Goes back to the `workspace>` prompt.                        |
| h      | Opens this manual.                                           |
| q      | Quits cratchit.                                              |
| Ctrl-D | Quits cratchit.                                              |

`1` lists the projects. The prompt is `open?>`:

```
Projects in /Users/me/Documents/cratchit:
  1  ITfied AB  /Users/me/Documents/cratchit/ITfied AB
  2  (not found)  /Volumes/share/Föreningen
open?> 
```

Each project shows its name and folder. A folder that does not exist shows
`(not found)`, and a folder that cratchit cannot read as the listed project
shows `(error)`. Choosing either shows the error and leaves the list as it is.
Press the number of a project to open it; `-` goes back to the project menu.
With more than 9 projects the list shows 8 at a time. Key `9` shows `...down`
and moves to the next 8. Key `0` shows `...up` and moves back.

### Creating a project

Press `2` in the project menu and type the official name of the person,
company or organisation, then Enter. The prompt is `name>`. Backspace erases,
and `-` or Enter on an empty name goes back to the project menu. Every other
key, including `q` and `h` (and `-` once the name has begun), is part of the name.

Then choose the project folder:

```
Choose a folder for ITfied AB:
  1  /Users/me/Documents/cratchit/ITfied AB
  2  Type a folder path
```

| Key    | What it does                                                      |
|--------|-------------------------------------------------------------------|
| 1      | Uses a folder with the name in the workspace, creating it if needed. |
| 2      | Lets you type a folder path (see Typing a folder path).           |
| -      | Goes back to the project menu.                                    |
| h      | Opens this manual.                                                |
| q      | Quits cratchit.                                                   |

Option 1 is not shown when the name cannot be a folder name, for example when
it holds `/` or `:`.

cratchit creates `.cratchit/project.txt` in the folder with a new uuid, lists
the project in `workspace.txt` and opens it:

```
Created folder /Users/me/Documents/cratchit/ITfied AB
Initiated .cratchit/project.txt in /Users/me/Documents/cratchit/ITfied AB
Updated .cratchit/workspace.txt in /Users/me/Documents/cratchit
Opened project ITfied AB in /Users/me/Documents/cratchit/ITfied AB
project> 
```

A folder that already holds a `project.txt` is an error; add it with `3`
instead. The folder may hold other cratchit files, for example the workspace's
own `.cratchit`.

### Adding an existing project

Press `3` in the project menu and type the folder of a project, for example one
listed in another workspace (see Typing a folder path). The folder must hold
`.cratchit/project.txt`. cratchit lists the project in `workspace.txt` and
opens it.

A project whose uuid is already listed with another folder is an error, for
example a copy of a project folder. cratchit shows the error, logs it, and
lets you change the path. A copied project folder is not supported yet.

### In a project

The prompt is `project>`.

| Key    | What it does                                        |
|--------|-----------------------------------------------------|
| 1      | Lists the fiscal years to open one (see Fiscal years). |
| 2      | Imports an SIE file (see Importing an SIE file).    |
| 3      | Imports an account statement (see Importing an account statement). |
| 4      | Lists the statement entries not yet booked, to make waiting events of them (see Statement entries). The key list shows how many there are. |
| 5      | Shows reports of the books (see Reports).           |
| Enter  | Lists the numbered keys.                            |
| h      | Opens this manual.                                  |
| l      | Shows the log (see The log).                        |
| -      | Goes back to the `workspace>` prompt.               |
| q      | Quits cratchit.                                     |
| Ctrl-D | Quits cratchit.                                     |

The project you open is saved as the last opened in `workspace.txt`, so it
opens again the next time the workspace opens. If the last opened project is
not listed, or cannot be opened, cratchit shows the error and stays at
`workspace>`. When the project opens, the fiscal year you last opened in it
opens too (see Fiscal years).

### Fiscal years

Vouchers are entered in an open fiscal year. Press `1` at the `project>`
prompt. cratchit lists the fiscal years in the books, numbered as SIE numbers
them: 0 is the newest, 1 the year before, and so on. Each says whether it is
open or closed for new vouchers:

```
Fiscal years of The ITfied AB, 556782-8172 (0 is the newest):
  0  2026-05-01 - 2027-04-30  open
  1  2025-05-01 - 2026-04-30  open
  ...
  8  2018-05-01 - 2019-04-30  open
  9  ...down
year?> 
```

The prompt is `year?>`. Press the number of a year to open it. Up to ten years
are keys 0-9, the year numbers. With more, keys 0-8 show the nine newest and
`9` shows `...down`: the older years, eight at a time as keys 1-8, with `0`
for `...up` (newer) and `9` for `...down` (older). There the heading says
which year key 1 is, for example
`Fiscal years of The ITfied AB, 556782-8172 (key 1 is year 9):`.

Fiscal years come from SIE files (see Importing an SIE file). Without books,
cratchit says `No books yet. Press 2 at project> to import an SIE file.`

The year you open is saved as `year` in `project.txt`, so it opens again when
the project opens. If that year is not in the books, cratchit shows an error
and stays at `project>`.

#### In a fiscal year

The prompt is the fiscal year: `2026/27>` for a year from 2026 into 2027, or
`2025>` for a calendar year.

| Key    | What it does                                        |
|--------|-----------------------------------------------------|
| 1      | Booking: entering and noting, the two waiting lists and the vouchers (see Booking, below). The key list shows how many waiting events and statement entries not yet booked the year has, both always, e.g. `Booking (waiting: 1 event, 7 statement entries)`. |
| 2      | Exports the year as an SIE 4E file in the project folder: all of it, or the vouchers not in an SIE file you choose (see Exporting a fiscal year as an SIE file). Also for a closed year. |
| 3      | Imports an SIE file, as `2` at `project>` (see Importing an SIE file). Any fiscal year's file, not only this one's. You come back here. |
| 5      | Imports an account statement, only its entries in this year (see Importing an account statement). |
| 7      | Shows the reports of this year (see Reports). `-` there comes back here. |
| 8      | Closes the year for new vouchers when it is open, opens it when it is closed. The key list names the one offered. |
| 9      | Finds waiting events that may be the same event, or already in the books (see Finding matches). |
| Enter  | Lists the numbered keys.                            |
| h      | Opens this manual.                                  |
| l      | Shows the log (see The log).                        |
| -      | Goes back to the `project>` prompt.                 |
| q      | Quits cratchit.                                     |
| Ctrl-D | Quits cratchit.                                     |

Keys 4 and 6 are free: they were the booking operations before these
moved under 1, and the other keys kept their numbers.

A new year from an SIE import is open. Closing it stops new vouchers from being
entered in it until you open it again; `1` at `booking>` then says
`Fiscal year 2025-05-01 - 2026-04-30 is closed for new vouchers. Press 8 at the year prompt to open it.`
cratchit saves the setting in `books.txt` (see The files) and logs it. Closing
a year changes nothing else: its vouchers, balances and reports stay as they are,
and an SIE import into it still works.

#### Booking

`1` at the year prompt gathers what you do to book the year's events. The
prompt is `booking>`:

```
2026/27> 1
Keys at booking>:
  1  Enter a voucher
  2  Note an event to post later
  3  Waiting events (1)
  4  Statement entries not yet booked in this year (7)
  5  View vouchers (correct one, use it as a template, or delete the last of a series)
booking> 
```

| Key    | What it does                                        |
|--------|-----------------------------------------------------|
| 1      | Enters a voucher (see Entering a voucher). Only in an open year. |
| 2      | Notes an event to post later (see Waiting events). Only in an open year. |
| 3      | Lists the waiting events, to post or delete them (see Waiting events). The key list shows how many there are. |
| 4      | Lists the statement entries not yet booked in this year (see Statement entries). The key list shows how many there are. |
| 5      | Views the vouchers, to correct one, use it as a template, or delete the last of a series (see Viewing and correcting vouchers). |
| Enter  | Lists the numbered keys.                            |
| h      | Opens this manual.                                  |
| l      | Shows the log (see The log).                        |
| -      | Goes back to the year prompt.                       |
| q      | Quits cratchit.                                     |
| Ctrl-D | Quits cratchit.                                     |

When one of these is done (a voucher saved or dropped, an event noted, `-` from
a list), cratchit comes back to `booking>`.

### Entering a voucher

Press `1` at `booking>` in an open fiscal year (see Booking). A voucher
(verifikation) records one financial event. You type it in steps: what
happened, an earlier voucher to use as a template (when there is one), the
series, and the rows on the accounts. Nothing is saved until the rows balance
and you press `y`.

#### Heading, amount and date

Type them on one line at `voucher>`, in any order, and press Enter:

```
voucher> Telia faktura oktober 1 118,00 2026-10-22
  Heading  Telia faktura oktober
  Amount   1118.00
  Date     2026-10-22   (fiscal year 2026-05-01 - 2027-04-30)
OK? (y/n)
```

- The date is the last word that is a date, written `2026-10-22` or
  `20261022`. A voucher needs its date typed; the line above the prompt
  names today (`The date is needed (today is 2026-10-27).`). Without a date
  the line is kept for you to add it: `No date. Type the date, e.g. 2026-10-22.`
- The amount is the last other number: digits with an optional `-`, and a
  decimal comma or point, at most two decimals (`1118`, `1118,5`, `-1118.00`).
  A no-break space inside a number, as a bank may write it, is dropped.
- The rest is the heading, in the order typed. Numbers in it stay, as long as
  they are not last: in `Faktura 87 Payex 500` the heading is `Faktura 87 Payex`.

When number words come just before the amount, a space may separate thousands
(`1 118,00`) or a number in the heading from the amount (`Faktura 87 500`).
cratchit does not guess: it lists each reading and you choose one with its
number. The prompt is `amount?>`; `-` goes back to change the line.

```
voucher> Faktura 87 500
The amount is unclear: spaces may separate thousands. Which is it?
  1           500.00  Faktura 87
  2         87500.00  Faktura
amount?> 
```

A word only counts as a group of thousands when it has exactly three digits
and no sign, and the word before it has at most three digits and no decimals.
So `Faktura 8700 500`, `Faktura 87 -500` and `Faktura 87 50,00` are clear.
To avoid the question, type the amount without spaces.

Press `y` if the line is read right, or `n` (or `-`) to change it. When
something is missing or wrong, cratchit says so and keeps the line for you
to change, for example `No amount. Type it with digits, e.g. 1118,00`. A date
outside the open fiscal year is refused:

```
Refused: 2027-05-03 is outside fiscal year 2026-05-01 - 2027-04-30.
```

`-` or Enter on an empty line goes back to `booking>`.

#### The template

cratchit then looks for earlier vouchers like this one, to guide the rows.
A voucher is like it when its text shares words with your heading (case,
numbers and one-letter words do not count), it is in one of the series A-N,
and it has an amount and at least two rows. The most shared words come first,
then a voucher whose amount has the same sign as yours, then the latest.
Vouchers with rows on the same accounts, on the same sides (debit or credit),
show only once, as the best of them. At most five are listed, each with its
rows (removed rows left out) in the Debit and Credit columns, with the amounts
of that voucher:

```
Earlier vouchers like this one:
  1  D 9  2026-10-22       1118.00  Telia faktura oktober
       Account        Debit       Credit
       2440                      1118.00  Leverantörsskulder
       6212          894.40               Mobiltelefon
       2641          223.60               Debiterad ingående moms
  2  E 4  2026-07-01        987.00  Betalning Telia
       Account        Debit       Credit
       2440          987.00               Leverantörsskulder
       1930                       987.00  Företagskonto
  0  None: type the rows yourself
template?> 
```

| Key    | What it does |
|--------|--------------|
| 1-5    | Uses that voucher as the template. |
| Enter  | Uses the first one. |
| 0      | Uses no template: the series is guessed and the rows are typed as without one. |
| -      | Goes back to the line (for a waiting event, to the waiting list). |
| h      | Opens this manual. |
| q      | Quits cratchit. |

When no earlier voucher is like this one, cratchit goes straight to the
series. A template account that is no longer in the books is left out, with
`Account 6213 of D 9 is not in the books, so it is not suggested.`

#### The series

Each voucher goes in a series by its kind, as in Fortnox:

| Series | Name |
|--------|------|
| A | Redovisning |
| B | Kundfakturor |
| C | Inbetalningar från kunder |
| D | Leverantörsfakturor |
| E | Utbetalningar till leverantörer |
| F | Kassa |
| G | Avskrivning |
| H | Periodisering |
| I | Bokslut |
| J | Kontantfaktura |
| K | Kvitto |
| L | Lön |
| M | Momsrapport |
| N | Lager |

cratchit offers a series by default: the template's series. Without a
template, that of the voucher in the books whose text shares the most words
with your heading (case, numbers and one-letter words do not count; the
latest voucher wins a tie), or A without such a voucher. It shows which
voucher the default comes from:

```
Series (Enter = D, as D 3 2026-05-18 "Faktura: Telia"):
```

The prompt is `series>`. Press Enter to take the default, or the letter of
another series (upper or lower case). `-` goes back to the line. Here `h` is
series H, so help is not offered; `q` quits. When templates were listed, `-`
goes back to them instead.

The voucher gets the next number in its series in the fiscal year: the
highest number used plus one, or 1.

#### The rows

Type one row per line at `post>`: an account and an amount, debit positive,
credit negative, then Enter. The voucher balances when the debit rows add up
to the amount on the voucher line, and so do the credit rows. After each row
cratchit shows it and where the rows stand. Rows are shown with the amount
without sign, in a Debit or a Credit column (zero is shown as debit):

```
post> 1930 -1118
  1930                      1118.00  Företagskonto
  Remaining 1118.00 (debit 0.00, credit 1118.00, amount 1118.00).
post> 6212 894,40
  6212          894.40               Mobiltelefon
  Remaining 223.60 (debit 894.40, credit 1118.00, amount 1118.00).
post> 2641
  2641          223.60               Debiterad ingående moms
  Balanced. Enter saves.
```

With a template, cratchit suggests all its rows at once, right after the
series. Each amount is the template's, scaled by your amount over the
template's amount and rounded to the öre; the template's last row gets what
remains, so the rows balance. A negative amount on the voucher line flips the
suggested rows (debit becomes credit). The whole voucher is shown, ready to
save as it is:

```
Suggested from D 9, scaled to the amount:
Voucher D 10 2026-11-20 "Telia faktura november"
  Account        Debit       Credit
  2440                      1245.00  Leverantörsskulder
  6212          996.00               Mobiltelefon
  2641          249.00               Debiterad ingående moms
  Total        1245.00      1245.00

  1  Save as D 10
  2  Flip the template's rows (debit becomes credit)
postings> 
```

`1` saves it (see Saving). `2` flips the template (see Flipping the
template). `-` goes to `post>` with the suggested rows
in place, to change them from the end: `-` removes the last row, and that row
is then suggested again before `post>`. Enter on an empty line takes the
suggestion; a row you type takes its place instead. The next suggestion is
always the template row after the rows you have, and the template's last row
gets what remains. To change the middle row of three, press `-` twice, type
your row, then Enter for the last one:

```
The suggested rows are in place. - removes the last one and suggests it again: Enter takes the
suggestion, or type your own row instead.
  Balanced. Enter saves.
post> -
Removed  2641          249.00               Debiterad ingående moms
  Remaining 249.00 (debit 996.00, credit 1245.00, amount 1245.00).
  Suggested 2641          249.00               Debiterad ingående moms
post> -
Removed  6212          996.00               Mobiltelefon
  Remaining 1245.00 (debit 0.00, credit 1245.00, amount 1245.00).
  Suggested 6212          996.00               Mobiltelefon
post> 6212 1000
  6212         1000.00               Mobiltelefon
  Remaining 245.00 (debit 1000.00, credit 1245.00, amount 1245.00).
  Suggested 2641          245.00               Debiterad ingående moms
post> 
  2641          245.00               Debiterad ingående moms
  Balanced. Enter saves.
```

When the suggested rows do not balance, for example when a waiting event's
postings do not fit the template, the whole voucher is not offered:
cratchit goes to `post>` and suggests the rows one at a time, as above,
starting with `Rows from D 9 are suggested one at a time`.

#### Flipping the template

Sometimes the template's rows are on the wrong sides for this voucher, for
example when a credit note (kreditfaktura) is entered with an invoice as its
template. Press `2` at `postings>` to flip the template: each of its rows
changes side (debit becomes credit, credit becomes debit), and the rows are
suggested again from the flipped template. Rows you typed or changed after the
template's rows are replaced. The amount on the voucher line stays as it is.

```
postings> 2
Suggested from D 9, scaled to the amount and flipped:
Voucher D 10 2026-11-20 "Telia kreditfaktura"
  Account        Debit       Credit
  2440         1245.00               Leverantörsskulder
  6212                       996.00  Mobiltelefon
  2641                       249.00  Debiterad ingående moms
  Total        1245.00      1245.00
```

`2` again flips it back. When the flipped rows balance, the whole voucher is
shown at `postings>` again; else they are suggested one at a time at `post>`
(`Rows from D 9 are suggested one at a time, scaled to the amount and
flipped`). Only the template is flipped: the postings a waiting event already
has stay as they are (see The waiting list), and a single row cannot be
flipped. `2` is offered only when the voucher has a template (also when a
viewed voucher is used as the template and when correcting, see Viewing and
correcting vouchers); without one its number is left out.

Rows that add up to zero but not to the amount show
`Debit 223.60, credit 223.60, amount 1118.00: debit and credit must each reach the amount.`
More than the amount on either side shows `...: more than the amount.`

| You type          | What it does |
|-------------------|--------------|
| account amount    | Adds a row. The amount is written as on the voucher line; spaces in it are dropped. |
| account           | Adds a row with what remains. On the first row, the voucher's amount. When the rows add up to zero but not to the amount, cratchit asks you to type the amount. |
| a number prefix or a word | When it is not an account in the books, lists the accounts whose number starts with it or whose name contains it (case does not matter), at most 20. |
| -                 | On an empty row, at once: removes the last row. With no rows, drops the voucher: `Dropped the voucher. Nothing is saved.` |
| ?                 | On an empty row, at once: shows the hint for every account in the rows (see The hint). The `?` stays on the line; Enter alone clears it. |
| ? account         | Type an account after the `?`, then Enter: shows the hint for that account: on its side in the rows, or on both sides when it is not in the rows. Not an account: lists the matches, as above. |
| Enter (empty)     | When the voucher balances (at least two rows, debit and credit each adding up to the amount), shows it and asks to save it. Else adds the suggested row, when there is one. Else shows where the rows stand; the voucher cannot be saved. |

Only accounts in the books can be used. Backspace erases while you type.

#### The hint

Which accounts go against the ones you have? `?` at `post>` (or at
`postings>`) reminds you. For each account in the rows, on its side (debit or
credit), cratchit looks through the vouchers and waiting events of all fiscal
years with that account on the same side, the latest first. It counts the
accounts on the other side (the counter accounts) in the latest 20 of them,
the most used first, and lists the latest 5 with their rows in the Debit and
Credit columns:

```
post> ?
Hint for 1920 C PlusGiro, from vouchers and waiting events with 1920 on credit:
  Counter accounts in the latest 20:
    2440   D   8  Leverantörsskulder
    6570   D   4  Bankkostnader
  Latest:
    waiting  2026-09-28       1308.75  LOOPIA AB | Webhotell
      Account        Debit       Credit
      1920                      1308.75  PlusGiro
    E 4      2026-09-20       1245.00  Telia
      Account        Debit       Credit
      2440         1245.00               Leverantörsskulder
      1920                      1245.00  PlusGiro
```

The number after the side is in how many of the counted vouchers and waiting
events the counter account is. A waiting event is marked `waiting`; one with
only the hinted account's posting counts but adds no counter account. The
waiting event being posted is left out. `?` acts as soon as you press it on
an empty row, and `post>` comes back with the `?` still on the line: type an
account after it and Enter (`? 2440`) for the hint of that account, when it is
not in the rows for both its sides, or press Enter alone to clear the `?`.
With no rows, `?` asks for an account. The hint changes nothing: `post>`
comes back with the same rows and suggestion.

#### Saving

```
Voucher D 4 2026-10-22 "Telia faktura oktober"
  Account        Debit       Credit
  1930                      1118.00  Företagskonto
  6212          894.40               Mobiltelefon
  2641          223.60               Debiterad ingående moms
  Total        1118.00      1118.00

  1  Save as D 4
postings> 
```

The prompt is `postings>`:

| Key    | What it does |
|--------|--------------|
| 1      | Saves the voucher. |
| 2      | Flips the template's rows (see Flipping the template). Only listed when the voucher has a template. |
| ?      | Shows the hint for every account in the rows (see The hint). |
| -      | Goes back to the rows at `post>`. |
| Enter  | Shows the voucher and the numbered keys again. Any other key prints `Unknown key.` and names the other keys. |
| h      | Opens this manual. |
| q      | Quits cratchit. |

Press `1` to save. cratchit adds the
voucher to the fiscal year's files in the books (see The files), shows which
files it updated, says `Saved voucher D 4 into fiscal year 2026-05-01 -
2027-04-30.` and logs it (naming the template, if one was used, and whether it
was flipped). A saved
voucher is never changed or deleted. A wrong one is corrected with a new
voucher (see Viewing and correcting vouchers).

The vouchers you enter are the truth of the books until the next SIE import
of their fiscal year: the imported file is then the truth of that year. A
voucher you entered that the file lacks leaves the books and waits as a waiting
event, after you type `revert` (see Making the file the truth).

### Viewing and correcting vouchers

Press `5` at `booking>` (see Booking). cratchit lists the series used in
the year; press `0` to list the vouchers of all series, or a letter to list
that series (the prompt is `series?>`; here `h` is series H, so help is not
offered; `-` goes back to `booking>`):

```
Series in fiscal year 2026-05-01 - 2027-04-30:
  0  All series                         16 vouchers, last D 4 2026-10-24
  A  Redovisning                        12 vouchers, last A 12 2026-10-02
  D  Leverantörsfakturor                 4 vouchers, last D 4 2026-10-24

Press 0 to list all series, a letter to list that series, - to go back, q to quit.
series?> d
Vouchers D in fiscal year 2026-05-01 - 2027-04-30, newest first:
  1  D 4     2026-10-24       1245.00  Faktura Telia november  (entered)
  2  D 3     2026-05-18       2359.00  Faktura: Telia  (imported)
  ...
voucher?> 
```

In a series, newest first means the highest number first. `0` lists the
vouchers of every series in one list, the latest date first (on the same
date, the later registered first, then series and number from the highest):

```
series?> 0
Vouchers in fiscal year 2026-05-01 - 2027-04-30, all series, newest first:
  1  D 4     2026-10-24       1245.00  Faktura Telia november  (entered)
  2  A 12    2026-10-02         85.00  Bankavgift  (imported)
  ...
voucher?> 
```

With more than nine, keys 1-8
show eight at a time, `0` shows the ones before and `9` the ones after.
A voucher is *imported* when it is in one of the SIE files the project has
imported (the copies in `.cratchit/imports`, see The files); else it was
*entered* in cratchit.

A number shows the voucher, with its rows (`(added)` and `(removed)` mark rows
changed after it was registered, as in the voucher report). The prompt is
the voucher itself, such as `D 4>`:

```
Voucher D 4 2026-10-24 "Faktura Telia november" 1245.00
  Account        Debit       Credit
  2440                      1245.00  Leverantörsskulder
  6212          995.89               Mobiltelefon
  2641          248.97               Debiterad ingående moms
  3740            0.14               Öres- och kronutjämning
  Total        1245.00      1245.00
  Registered 2026-10-24, entered in cratchit

  1  Correct it
  2  Use it as a template
  3  Delete it (the last voucher of series D)
D 4> 
```

| Key    | What it does |
|--------|--------------|
| 1      | Corrects the voucher with a new one (below). Only for an entered voucher, in an open year. |
| 2      | Enters a new voucher with this one as the template: the voucher line, then the series and the rows, as in Entering a voucher, without the list of templates. Only in an open year. |
| 3      | Deletes the voucher (see Deleting the last voucher of a series, below). Shown only for the last voucher of its series (the highest number in the year), in an open year. |
| <      | Shows the voucher above this one in the list it was picked from (newer). |
| >      | Shows the voucher below this one in the list it was picked from (older). |
| -      | Goes back to the list. |
| h      | Opens this manual. |
| q      | Quits cratchit. |

`<` and `>` step through the list the voucher was picked from: a series, or
all series. At either end cratchit says `This is the newest voucher in the
list.` or `This is the oldest voucher in the list.`.

#### Correcting a voucher

A correction is a new voucher that reverses the wrong one: the same rows with
the opposite signs, so together they add up to nothing. The wrong voucher
stays in the books as it is. `1` starts the voucher line with the correction's
heading, amount and date, for you to change or accept. The date is the
corrected voucher's date:

```
voucher> Rättelse av D 4: Faktura Telia november 1245.00 2026-10-24
```

Then, as in Entering a voucher: `OK? (y/n)`, the series (by default the
corrected voucher's series), and the corrected voucher's rows reversed, shown
as the whole voucher to save (`The rows of D 4, reversed:`, then `postings>`:
`1` saves, `2` flips the rows, `-` puts the rows in place at `post>` to change
them, as in The rows).
When it is saved, cratchit says so and goes back to `booking>`:

```
Saved voucher D 5 into fiscal year 2026-05-01 - 2027-04-30.
D 5 reverses D 4.
```

An imported voucher is not corrected in cratchit:
`D 3 came with an SIE import, so cratchit does not correct it. Correct it in
the program it came from and import the file again.`

#### Deleting the last voucher of a series

The last voucher of a series can be deleted, as some bookkeeping programs
allow (Fortnox does), so the series keeps its numbers in an unbroken row. Use
it to remove a voucher that should not be there; a voucher with wrong amounts
is better corrected (above), which keeps it in the books. `3` at the voucher
asks `Delete D 4? (y/n)`; `n` or `-` keeps it. `y` deletes it from the books,
entered or imported:

```
Deleted voucher D 4 2026-10-24 "Faktura Telia november" 1245.00.
```

Then cratchit lists the series again (the series list when it has no voucher
left, `booking>` when the year has none). The next voucher entered in the
series gets the number again. The deleted voucher is logged in full: its date,
text, registration and rows (see The log).

An imported voucher is deleted only from cratchit's books: an SIE file that
still has it adds it again when imported. cratchit says so before it asks:

```
D 3> 3
D 3 came with an SIE import. Deleting it here changes only cratchit's books. Delete it in
the program it came from too, or the next import of a file that still has it adds it again.
Delete D 3? (y/n)
```

### Waiting events

An invoice or a receipt can be noted when it comes, and posted on the accounts
later. A noted event waits in its fiscal year until you post it or delete it.
It is not a voucher: it is in no series, has no number and is not in the
reports.

#### Noting an event

Press `2` at `booking>` in an open fiscal year. Type the heading, amount and
date at `event>` as at `voucher>` (see Entering a voucher): the same rules,
the same `amount?>` question for an unclear amount, and a date outside the
year is refused. Unlike a voucher, an event may leave out the date: then it is
today, when the year holds it (else `No date. Type the date, e.g. 2026-10-22.`).
Press `y` to note it:

```
2026/27> n
Type the heading, amount and date of the event to post later, e.g. Telia faktura oktober 1118,00 2026-10-22.
No date means today. Backspace erases, Enter accepts. - or Enter on an empty line goes back.
event> Telia faktura oktober 1118 2026-10-20
  Heading  Telia faktura oktober
  Amount   1118.00
  Date     2026-10-20   (fiscal year 2026-05-01 - 2027-04-30)
OK? (y/n) y
Initiated .cratchit/books/2026-05-01/events.csv in ...
Noted: 2026-10-20 Telia faktura oktober 1118.00 (1 waiting). Press 3 to post it.
```

The amount is kept without sign, as on a voucher. cratchit saves the event in
the year's `events.csv` (see The files) and logs it.

#### The waiting list

Press `3` at `booking>`. The prompt is `waiting?>`:

```
Waiting events in fiscal year 2026-05-01 - 2027-04-30:
  1  2026-10-20       1118.00  Telia faktura oktober
  2  2026-10-21        249.00  Kvitto Clas Ohlson
waiting?> 
```

They are listed by date, oldest first; events with the same date in the order
noted. With more than nine, keys 1-8 show eight
at a time, `0` shows the ones before and `9` the ones after. With none,
`3` says `No events are waiting in fiscal year ... Press 2 to note one.`
`-` goes back to `booking>`.

A number shows that event with its postings. The prompt is `waiting>`:

```
  2026-10-21        249.00  Kvitto Clas Ohlson

  1  Post it: enter its voucher
waiting> 
```

| Key    | What it does |
|--------|--------------|
| 1      | Posts the event: the template, the series, then the rows, as in Entering a voucher. Only in an open year. |
| d      | Deletes the event: `y` at `Delete it? (y/n)` deletes it, `n` or `-` goes back to the event. |
| -      | Goes back to the list. |
| h      | Opens this manual. |
| q      | Quits cratchit. |

An event made from statement entries (see Statement entries) shows its
postings under it, and where they stand:

```
  1  2026-09-28       1308.75  Websupport webhotell
       Account        Debit       Credit
       1920                      1308.75  2026-09-28  PlusGiro
       Total           0.00      1308.75
       Remaining 1308.75 (debit 0.00, credit 1308.75, amount 1308.75).
```

When you post an event, its heading, amount and date are the voucher line,
so cratchit goes straight to the template (or the series). `-` at
`template?>`, or at `series>` when no templates were listed, goes back to the
waiting list. The event's postings are the first rows; a template's suggestions
follow them, leaving out a row on an account the event already has. When the
event's postings and the suggestions balance together, the whole voucher is
shown to save as it is (see The rows); else they are at `post>`. `2` at
`postings>` flips only the template's rows, never the event's postings. An event
whose postings balance on their own is saved with Enter at `post>`. `-` at `post>` removes the last
row, the event's postings too; with no rows it drops the voucher, and the
event is still waiting as it was. When the voucher is saved, the event leaves
the list:

```
Saved voucher D 5 into fiscal year 2026-05-01 - 2027-04-30.
The event is no longer waiting (1 still waiting).
```

Deleting an event removes it from the list for good (it is logged). Neither
posting nor deleting changes any other waiting event. The statement entries
of a deleted event are not booked any more, so they are listed again (see
Statement entries).

#### Finding matches

One event can end up waiting twice: a receipt noted with `2` and the card
payment made into an event from a statement entry, or a transfer between two
of your own accounts, one entry on each statement. And an event noted to post
later may already be in the books, for example when a later SIE import
brought its voucher. Press `9` at the year prompt to find such matches. The
prompt is `match?>`:

```
2026/27> 9
Possible matches in fiscal year 2026-05-01 - 2027-04-30 (the same amount, at most 4 days apart):
  1  same   2026-07-21       2292.00  Utbetalning Moms
            2026-07-23       2292.00  Momsutbetalning
            2 days apart, the postings balance together on different accounts
  2  books  2026-09-01        311.25  Loopia överföring domäner
       D 6  2026-08-31        311.25  Loopia Faktura
            1 day apart, shared words: loopia
match?> 
```

A match is two events with the same amount, at most 4 calendar days apart
(a weekend and a bank day on each side; holidays are not counted), in the
same fiscal year:

| Kind  | What it pairs |
|-------|---------------|
| same  | Two waiting events. The older is listed first. |
| books | A waiting event and a voucher of the year, imported or entered (its series and number on the second line). |

The amount and the dates only make a pair a candidate: with the invoice
method (fakturametoden) an invoice and its payment have the same amount
within days, yet they are two events. You decide. The likely matches come
first: those whose postings fit (for `same`: both have postings, on
different accounts, and together they balance; for `books`: the waiting
event has postings and the voucher has them all), then those whose headings
share more words (ignoring case, numbers and one-letter words), then those
fewer days apart. A waiting event can be in several matches. With more than
nine, keys 1-8 show eight at a time, `0` the ones before and `9` the ones
after. With none, `9` says `No possible matches among the waiting events
(the same amount, at most 4 days apart).` A pair you leave is not
remembered: it is listed again the next time. `-` goes back to the year
prompt.

A number shows both events in full, with their postings (for `books`, the
voucher's rows). For `same` the prompt is `same?>`, with a line on where
the postings stand together. When both events post to the same account it
says so (`the postings balance, but both events post to 1630.`): such
postings cancel out, so the two are more likely two events in a row, such as
a tax decision and the payout from the tax account.

| Key    | What it does |
|--------|--------------|
| 1      | Makes them one event. The `event>` line starts with the older event's heading, amount and date for you to change (the same rules as at `voucher>`, see Entering a voucher), then `OK? (y/n)`. The one event has the postings of both (a posting from the same statement entry only once) and the earlier noted day. It takes the place of the two in the waiting list. `-` or Enter on an empty line goes back to `same?>`. |
| -      | Goes back to the matches. |
| h      | Opens this manual. |
| q      | Quits cratchit. |

```
Made one event: 2026-07-21 Utbetalning Moms 2292.00 (13 waiting). Its postings balance: post it from the waiting list.
```

For `books` the prompt is `booked?>`. The postings of the waiting event that
are not in the voucher (none of its postings has the same account, date and
amount) are named, as deleting the event drops them; a statement entry of
theirs is then listed again among the statement entries:

| Key    | What it does |
|--------|--------------|
| d      | Deletes the waiting event as already in the books: `y` at `Delete it? (y/n)` deletes it, `n` or `-` does not. The voucher is not changed. |
| -      | Goes back to the matches. |
| h      | Opens this manual. |
| q      | Quits cratchit. |

After making one event or deleting one, cratchit saves the year's
`events.csv` and `event_postings.csv` (see The files), logs it and finds the
matches again. When none are left it says `No more possible matches.` and
shows the waiting list.

#### Statement entries

The entries of the imported account statements (see Importing an account
statement) wait to be booked. Press `4` at `project>`, or `4` at `booking>`
in a fiscal year for only that year's entries. The prompt is `entries?>`:

```
Statement entries not yet booked in fiscal year 2026-05-01 - 2027-04-30:
  1  1920   2026-07-03         -3.70  AVGIFTER NORDEA
  2  1920   2026-07-23       2292.00  Skatt | 5050-1030 SK5567828172
  ...
  7  1920   2026-09-28      -1308.75  5343-2795 | LOOPIA AB (WEBSUPPORT) | 62600010249540 | Webhotell 12/10 - 11/1
entries?> 
```

An entry is booked when a posting on its account has its date and amount:
in a voucher, imported or entered, or in a waiting event. Each posting books
one entry, and an entry in several statement files is listed once. With more
than nine, keys 1-8 show eight at a time, `0` the ones before and `9` the
ones after.

A number shows the entry and where it is. The prompt is `entry?>`:

| Key    | What it does |
|--------|--------------|
| 1      | Makes a new waiting event with it. Type the heading at `heading>` (the entry's text is typed in for you to change). The event gets the entry's date and its amount without sign, and the entry as its posting on the statement's account. |
| 2      | Adds it to a waiting event of its fiscal year, chosen by number at `event?>` (listed as in the waiting list). The event's amount stays as it is. Not shown when the year has no waiting event. |
| -      | Goes back to the list. |
| h      | Opens this manual. |
| q      | Quits cratchit. |

Both are refused in a closed year. cratchit saves the event and its postings
in the year's `events.csv` and `event_postings.csv` (see The files), shows
where the postings stand, logs it and goes back to the list:

```
Noted: 2026-09-28 Websupport webhotell 1308.75 with 1920 -1308.75 (1 waiting). Remaining 1308.75 (debit 0.00, credit 1308.75, amount 1308.75).
```

Entries from different statements can meet in one event: a transfer from
PlusGiro to the savings account is one entry on each statement, and the two
make a balanced event, ready to be saved as a voucher (see The waiting
list). The voucher keeps the postings' dates and texts, but not which
statement line they came from.

### Importing an SIE file

An SIE file is the books exported from another bookkeeping program, such as
Fortnox. cratchit imports SIE type 4 files (4E, with vouchers), one fiscal year
per file: the year the file calls year 0. Export one file per fiscal year and
import them in any order.

Press `2` at the `project>` prompt, or `3` at a fiscal year's prompt, and type
the path of the file (see Typing a folder path). Either way the file may be of
any fiscal year, and you come back to the prompt you started from.

The path of a folder lists its SIE files at `sie?>`, each read and checked
against the books, so you can pick by fiscal year and export date:

```
SIE files in /Users/me/Downloads:
  1  2025-05-01 - 2026-04-30, generated 2026-10-19, The ITfied AB
       ITfied20261019.se
       116 vouchers: 4 new; 556782-8172; Fortnox 3.62.17
  2  2025-05-01 - 2026-04-30, generated 2026-06-01, The ITfied AB
       ITfied20260601.se
       98 vouchers: all in the books; 556782-8172; Fortnox 3.62.17
  3  2025-01-01 - 2025-12-31, generated 2026-10-19, Other AB
       Other AB 2025.se
       131 vouchers: not this project's company; 556111-2222; Fortnox 3.62.17
(2 other files left out)
sie?> 
```

The newest fiscal year comes first, and within a year the newest export
(`#GEN`); files alike in both are ordered by name. Each file shows its fiscal
year, export date and company on the numbered line; under it the file name;
last the number of vouchers, what importing it would do, the organisation
number and the program. What importing it would do is `4 new` (vouchers), `1 changed`, `2 renamed`, `20 out of the books (revert)`, or several of them, e.g. `4 new, 1 changed` (see What the import does), `all in the books`, `no new
vouchers, other data new` (accounts or company details), `2 errors`, or `not
this project's company` (another organisation number). Files that are not SIE
files are only counted. A number shows that file as below. With more than nine
files, keys 1-8 show eight at a time, `0` the ones before and `9` the ones
after. `-` goes back to the path; Enter lists the files again. A folder with no
SIE file says `No SIE files in <folder> (2 other files).` and stays at the path.

cratchit reads and checks the file and shows what importing it
would do, without changing anything:

```
SIE file /Users/me/Downloads/ITfied20261019.se
  Read as      CP437
  Program      Fortnox 3.62.17, generated 2026-10-19
  Company      The ITfied AB, 556782-8172
  Fiscal year  2025-05-01 - 2026-04-30
  Accounts     501: 501 new, 0 changed
  Vouchers     116: 116 new, 0 already in the books
Notes:
  - The project gets organisation number 556782-8172 (#ORGNR)
Import it? (y/n)
```

Press `y` to import, `n` or `-` not to. After any import, when a waiting event
may now be in the books (see Finding matches), cratchit adds `3 possible
matches of a waiting event and a voucher: press 9 at the year prompt to find
them.` When the file has errors, cratchit lists
them, says `Nothing is imported.` and asks nothing. When everything in the file
is already in the books, it says so.

On import cratchit adds to the books (see The files), stashes an unchanged copy
of the file with a record of the import in `.cratchit/imports`, and shows:

```
Stashed the file as .cratchit/imports/2026-10-22_1_ITfied20261019.se in /Users/me/Documents/cratchit/ITfied AB
Imported 116 vouchers into fiscal year 2025-05-01 - 2026-04-30 (0 already in the books).
```

#### The encoding

SIE files must be in PC8 (code page 437). cratchit reads them so, unless that
gives characters that do not belong in an SIE file, such as box drawing
characters. Then, if the file is UTF-8, cratchit reads it as UTF-8 and notes
it, for example `Not in PC8 (CP437) as SIE requires: read as CP437 it has '├'
(U+251C) on line 8. Read as UTF-8 instead.` Otherwise the encoding is unknown
and nothing is imported.

#### What the import does

- The first import sets the project's organisation number and name from the
  file (`#ORGNR`, `#FNAMN`) and updates `project.txt`. After that, a file for
  another organisation number or company name is an error.
- The file's fiscal year is added to the books. A year that overlaps a year in
  the books, or starts the same day but ends another, is an error.
- The file is the truth of its fiscal year (year 0). New vouchers are added.
  A voucher already in the books (same series and number) is skipped when it
  means the same: programs write a voucher differently, so a row dated as its
  voucher counts as a row without a date, and a quantity of `0` as none
  (Fortnox writes no row dates and quantity `0`). It is taken from the file when only history was added to it:
  its registration date and signature are the same (its date and text may have
  been edited), each of its rows in the books is in the file in the same place,
  unchanged or struck (`#BTRANS`), a struck row is still struck, and any
  further rows are added (`#RTRANS`, possibly struck since). It is then noted,
  e.g. `Voucher E 25 changes: date 2026-04-20 to 2026-04-22, 2 rows struck, 2
  added (signed Kjell-Olov Högdahl)` or `Voucher A 2 changes: text "Bank cost"
  to "Bank fee"`,
  and the summary says `Vouchers 116: 0 new, 1 changed, 115 already in the
  books`. A change to a voucher of a closed fiscal year is an error: open the
  year (8 at the year prompt) and import again.
- A row date the file is silent about is kept. When a row in the books has a
  date of its own (another day than its voucher, e.g. the bank's day from a
  statement entry) and the file's row with the same account and amount has no
  date, the voucher keeps the books' date, noted as `Voucher A 11: kept 1 row
  date the file lacks`. So statement entries stay booked after a round trip. A
  date the file gives wins, and a row dated as its voucher follows the
  voucher's date in the file.
- Any other difference between the year's vouchers in the books and in the
  file (a voucher that differs otherwise, or one the file lacks) makes the file
  the truth only when you type `revert` (see Making the file the truth).
- A waiting event the file brings as a voucher (the same date, and the same
  rows: account and amount) is taken off the waiting list. Each voucher the
  file puts into the books takes at most one. The summary says so before you
  answer, `Waiting      18 events in the file as vouchers: taken off the
  waiting list`, and after the import `Took 18 waiting events off the waiting
  list: in the file as vouchers.`
- Rows added or struck in a voucher after it was registered (`#RTRANS`,
  `#BTRANS`) are kept with the voucher and marked. Struck rows do not count.
- New accounts, dimensions and objects are added. Changed names, types, SRU
  codes and company details are taken, and noted, only from a file whose year is
  the latest in the books, so an older file does not undo newer names.

#### Errors

Nothing is imported when the file:

- is not SIE type 4, lacks `#FNAMN`, `#ORGNR` or a year 0 (`#RAR 0`), or is in
  another currency than SEK (`#VALUTA`)
- has a line cratchit cannot read, or a record with a missing or invalid field
  (for example a date that is not YYYYMMDD, or an amount with a comma)
- has a voucher without series or number, given twice, dated outside the
  fiscal year, using an account the file does not define, or not balancing
- has a checksum (`#KSUMMA`) that does not match its content
- does not match the project or the books, as above

#### Making the file the truth

When the year's vouchers in the books differ from the file's other than by
added history, the file is the truth, and cratchit shows what that does to the
vouchers in the books before anything is changed. Each voucher of the year in
the books, in this order:

1. **The same voucher under another name.** It has the same date and the same
   rows (account and amount, struck rows not counted) as a voucher of the file
   with another series or number, for example after the other program
   numbered it again. The file's voucher takes its place. When several match,
   the one with the same text comes first, then the lowest series and number.
2. **Replaced.** The file has a voucher with its series and number that
   differs otherwise (for example another registration date or other rows).
   The file's voucher takes its place.
3. **Dropped.** Two vouchers left, with the same date, where the rows of one
   are the rows of the other turned (debit for credit): a correction and the
   voucher it corrects. Both leave the books; together they added up to
   nothing. A credit note on another day than its invoice is not dropped.
4. **Out of the books, now a waiting event.** Any other voucher the file
   lacks. It becomes a waiting event of the year (see The waiting list) with
   its heading, amount, date and rows, noted on the day it was registered. Its
   rows balance, so posting it again takes only a template choice (`0`), the
   series and Enter. Statement entries it booked stay booked.

The file's other vouchers are added. For example, a file exported from Fortnox
before vouchers were entered in cratchit:

```
The file becomes the truth of fiscal year 2026-05-01 - 2027-04-30:
  Dropped, a correction and what it corrects (2):
    A 11, A 20
  Out of the books, now waiting events (18):
    D 4, D 5, D 6, D 7, D 8, A 12, A 13, A 14, A 15, A 16
    E 4, E 5, A 17, A 18, A 19, E 6, D 9, D 10
  The file was made 2026-10-19; 20 vouchers leaving the books were registered after that.
  Before: the year is exported as sie_2026-27_all.se in the project folder.
Type revert and Enter to make the file the truth of this year, or Enter alone to import nothing.
revert> 
```

The line on when the file was made shows when vouchers that leave the books
(replaced, dropped or now waiting) were registered after the file's export
date (`#GEN`): the file may be older than the books.

Type `revert` and Enter: cratchit first exports the year as it is (see
Exporting a fiscal year as an SIE file), so nothing is lost, then imports the
file and says what left the books:

```
Wrote sie_2026-27_all.se in /Users/me/Documents/cratchit/The ITfied AB: fiscal year 2026-05-01 - 2027-04-30, 37 vouchers (SIE 4E).
...
Imported 0 vouchers into fiscal year 2026-05-01 - 2027-04-30 (17 already in the books).
Out of the books: 2 dropped, 18 now waiting events.
```

Anything else than `revert`, Enter alone or `-` imports nothing.
If the export cannot be written, nothing is imported.

In a closed fiscal year this is an error (`The file would change 20 vouchers in
the books against the history rule, but fiscal year ... is closed. Open the
year to make the file the truth`).

The import's record lists the vouchers (see The files), and the log has each
one in full (see The log). To go back, import the export written before: its
vouchers come back (as imported), and take the waiting events made from them
off the waiting list.

#### Balances and notes

cratchit keeps its own balances: a fiscal year directly after another in the
books opens with that year's closing balances, for the balance sheet accounts
(BAS classes 1-2, or type T and S). The result accounts (3-8, or K and I) open
at zero. The first year in the books, or one after a gap, opens with the
opening balances its file reported.

The balances in the file (`#IB`, `#UB`, `#RES`) are only checked. Each
difference is shown as a note and logged, but does not stop the import:

- the file's own year does not add up (opening balance + transactions against
  the closing balance, or transactions against the result)
- the file's opening balance differs from its closing balance of the year before
- the file's opening balance differs from what cratchit carries from its own
  closing balance of the year before; cratchit uses its own
- the file's balances of an earlier year (year -1 and so on) differ from that
  year in the books
- the import changes the opening balances of a later year in the books, now
  carried from the imported year

Where one or more vouchers in the books have exactly the difference on the
account, the note names them as the possible cause, for example
`Possibly from voucher M 5 2026-04-30 "Momsrapport"`.

Unknown records in the file are ignored and noted, as SIE requires.

### Exporting a fiscal year as an SIE file

`2` at a fiscal year's prompt exports the year as an SIE 4E file (the format
Fortnox and other programs import) in the project folder. It asks what to
export (the prompt is `export?>`; `-` goes back to the year):

```
2026/27> 2
Export fiscal year 2026-05-01 - 2027-04-30 as an SIE file:
  1  All: every voucher (17), with balances, as sie_2026-27_all.se
  2  Diff: the vouchers not in an SIE file you choose, without balances
export?> 1
Wrote sie_2026-27_all.se in /Users/me/Documents/cratchit/The ITfied AB: fiscal year 2026-05-01 - 2027-04-30, 17 vouchers (SIE 4E).
```

| Key | What it does |
|-----|--------------|
| 1   | All: the whole year with balances, as `sie_2026-27_all.se` (below). For a backup, or for a program that does not have the year yet. |
| 2   | Diff: only the vouchers the books have and an SIE file of the same year lacks, as `sie_2026-27_diff.se` (see Exporting the diff). For a program that already has the year, such as Fortnox. |

Fortnox imports every voucher in an SIE file, also those it already has. So
send Fortnox a diff, never all of a year it has: that books every voucher
twice.

The file has, as Fortnox's own export does:

| Part | What it is |
|------|------------|
| Identification | cratchit as the program and the day of the export; the project's name and organisation number; the company id, address and chart type the books took from the SIE import; the year as year 0, and the year before as -1 when it is in the books. |
| Accounts | Every account in the books with its name, type, SRU code and unit (the newest ones, see Importing an SIE file), every dimension and object. |
| Balances | `#IB` and `#UB` (opening and closing) of each balance account where either is not zero, and `#RES` (the year's result) of each other account that is not zero, for year 0 and -1. They are cratchit's own figures (see Reports): a year opens with the closing balances of the year before when that year is in the books, so they may differ from what Fortnox reported. |
| Vouchers | The year's vouchers, imported and entered, with every row (rows added or struck after registration as `#RTRANS` and `#BTRANS`). |

Waiting events are not in the file: they are not vouchers yet. Nor are period
balances (`#PSALDO`) or a checksum (`#KSUMMA`).

The file is in CP437 (PC8), as SIE requires. A character CP437 does not have,
such as `€`, does not belong in an SIE file; cratchit writes it as its Unicode
code point, e.g. `<U+20AC>`, says how many there are and logs it:

```
1 character SIE (CP437) does not have is written as its code point, e.g. <U+20AC>. Search the
file for <U+ to find them, and correct the texts in the books.
```

A second export of the same year is written next to the first
(`sie_2026-27_all_2.se`); cratchit never overwrites a file. cratchit can import its
own export again: it finds nothing new.

#### Exporting the diff

`2` at `export?>` asks what to compare with (the prompt is `diff?>`; `-` goes
back to `export?>`):

```
Export the vouchers of the books that are not in an SIE file of the same fiscal year, e.g. the
other program's latest export. Compare with:
  1  The last import of the year: 2026-10-29_1_TheITfiedAB20261029_111636.se
  2  Choose an SIE file
diff?>
```

`1` takes the copy of the year's last SIE import (see The files); it is left
out when the year has none. `2` asks for the path of an SIE file, typed from
the start folder as for an import; a folder lists its SIE files to pick one.
The file must be for the project's organisation number and the same fiscal
year (year 0).

A voucher of the books is in the file when the file has a voucher with the same
date and the same rows (account and amount, struck rows not counted), whatever
its series and number: the other program may have numbered it again. cratchit
lists both differences:

```
Compared with TheITfiedAB20261029_111636.se (Fortnox 3.62.17, generated 2026-10-29):
  In the books, not in the file (1):
    D 4     2026-08-07        923.00  Fortnox Faktura
  In the file, not in the books (0):
Write sie_2026-27_diff.se? (y/n) y
Wrote sie_2026-27_diff.se in /Users/me/Documents/cratchit/The ITfied AB: fiscal year 2026-05-01 - 2027-04-30, 1 voucher, no balances (SIE 4E).
```

The diff file has the identification, the accounts and those vouchers, but no
balances (`#IB`, `#UB`, `#RES`), so the other program cannot take balances from
a part of the year. When every voucher is in the file: `Every voucher of fiscal
year ... is in the file. Nothing to export.`

When the file has vouchers the books lack, the books are behind the other
program, and a diff from them would be stale. Nothing is exported:
`Nothing is exported: the file has vouchers the books lack, so the books are
behind it. Import the file first (3 at the year prompt), then export again.`

A round trip with Fortnox: export the diff against Fortnox's latest export (or
the last import, when nothing was booked in Fortnox since), import it in
Fortnox, then export the year from Fortnox and import that into cratchit. The
vouchers come back under Fortnox's numbers (see Making the file the truth).

### Importing an account statement

An account statement is a file from the bank or from Skatteverket listing the
entries of one account: Nordea's CSV files and Skatteverket's "Bokförda
transaktioner" (CSV or .skv). Press `3` at `project>`, or `5` at a fiscal
year's prompt to import only the entries in that year. The entries wait to be
booked (see Statement entries); nothing is put on the accounts yet.

Each line of a statement is an *entry*. A *posting* is an amount on a BAS
account, in a voucher (or an event): an entry becomes a posting when it is put
on the account the statement is for.

#### Choosing the file

Type the path of the file at `path>`, from the folder cratchit was started in.
Tab completes folder and file names. The path of a folder lists its files,
each read to show what it holds, at `statement?>`:

```
Files in /Users/me/Downloads:
  1  PLUSGIROKONTO FTG 51 86 87-9 - 2026-10-20 11.05.39.csv
       7 entries, 2026-07-03 - 2026-09-28, new
  2  bokf_trans_165567828172.skv
       The file does not start with a UTF-8 byte order mark (BOM), so its encoding is unknown. Save it as UTF-8 with a program that writes a BOM.
statement?> 
```

A file shows `new`, `3 already imported` or `already imported` (by the
statements imported so far). With more than nine files, keys 1-8 show eight
at a time, `0` the ones before and `9` the ones after. `-` goes back to the
path. `-` or Enter on an empty path goes back to the prompt you came from.

#### How cratchit reads a statement

cratchit finds the entries by what the cells hold, not by the column names:

- The file must start with a UTF-8 byte order mark (BOM). Without one its
  encoding cannot be told, and cratchit asks you to save it as UTF-8 with a
  program that writes a BOM.
- The cells are separated by `;`, tab or `,`, whichever gives the most lines
  with the same number of cells. Cells may be in double quotes.
- An entry line has exactly one date (`2026-09-28` or `2026/09/28`, alone in
  its cell) and at least one amount (`-1308,75`, `2 292`, `1997`, alone in its
  cell). A date or amount inside other text does not count.
- The entries are the longest run of such lines in a row, with the same
  number of cells and the date in the same place. Lines before and after (a
  header, "Ingående saldo ...") are not entries.
- When a column can be the saldo (the balance after each entry), it must
  add up line by line: the saldo before plus the amounts since. A line without
  a saldo is fine (Nordea shows one per day). This also tells whether the file
  lists the newest or the oldest entry first. Without a saldo, the dates tell.
- A file with a single entry is read only when that line has a single
  amount: one line cannot tell the amount from the saldo.
- The text of an entry is the line's other cells joined with ` | `, without
  empty cells, a cell that repeats the one before, or cells the same on every
  line (such as `SEK`).

When cratchit cannot tell (no saldo adds up, two runs of the same length,
dates in no order), it says why, with the lines, and imports nothing.

#### The account

The statement is shown, then cratchit asks which BAS account it is for, at
`account>`. It suggests one, typed in for you:

```
Statement file /Users/me/Downloads/PLUSGIROKONTO FTG 51 86 87-9 - 2026-04-12 17.38.43.csv
  Entries   15, 2026-01-07 - 2026-03-31 (lines 2-16, newest first)
  Saldo     9911.33 before, 6011.55 after; it adds up on every line that has one
Notes:
  - Line 1 is not an entry.
Suggested account 1920 PlusGiro: 15 of 15 entries match postings on 1920 in the books.
Type the BAS account the statement is for, e.g. 1930. A number prefix or a word of the name lists
accounts. Enter accepts; - or Enter on an empty line imports nothing.
account> 1920
```

The suggestion is the account with the most postings in the books' vouchers
that match an entry (same day, same amount). Without such postings, the
account whose imported statements hold the most of these entries. Without that, the
account whose latest imported statement ends with the saldo this one starts
with. A tie suggests nothing. Press Enter to take it, or change it. A number
prefix or a word that is not an account lists the matching accounts.

#### Importing

```
  Account   1920 PlusGiro
  New       15
  Known     0 (in statements already imported)
Import it? (y/n) y
Stashed the file as .cratchit/statements/2026-10-24_1_PLUSGIROKONTO FTG 51 86 87-9 - 2026-04-12 17.38.43.csv in ...
Imported 15 entries for account 1920 (0 already known, 0 outside). Press 4 to book them.
```

At a fiscal year's prompt the last line says `Press 1 and then 4 to book them.`

An entry already in a statement imported for the account (same date,
amount, saldo and text) is known, and not imported again; files that overlap
are fine. Entries outside every fiscal year in the books, or at a year's
prompt outside that year, are listed as `Outside` and not imported:

```
  Outside   1 (outside fiscal year 2025-01-01 - 2025-12-31), not imported:
    2026-01-02         -1.85  AVGIFTER
```

With nothing new, cratchit says `Nothing to import: no entry is new.` (or
`... no entry is new inside the books.` when some are outside). `y`
stashes an unchanged copy of the file with a record (see The files); `n`
imports nothing. Reading the file, the account chosen and the import are
logged.

### Reports

Press `5` at the `project>` prompt. cratchit lists the fiscal years in the
books, newest first; press a number to choose one (with more than 9, keys 0
and 9 scroll as in the project menu). Then choose a report. In a fiscal year,
`7` shows the reports of that year at once, and `-` goes back to its prompt:

```
Reports for 2025-05-01 - 2026-04-30:
  1  Trial balance (saldobalans)
  2  Vouchers
  3  Account ledger
  4  VAT return (momsrapport)
```

The prompt is `report?>`.

| Key    | What it does                                                    |
|--------|-----------------------------------------------------------------|
| 1      | Trial balance: each balance sheet account with opening balance, change and closing balance, each result account with its result, and totals. It says where the opening balances come from, and lists those that differ from what the source reported. |
| 2      | Every voucher of the year with its rows in Debit and Credit columns, and the totals. Rows added or struck after registration are marked `(added)` or `(removed)`, with the signature when there is one; a removed row does not count in the totals. |
| 3      | Account ledger: type an account number at `account>`, then Enter. Shows the opening balance, every row on the account by date in Debit and Credit columns with the balance after it, and the closing balance. `-` or Enter on an empty number goes back. |
| 4      | VAT return (momsrapport): choose a period (see below). |
| -      | Goes back: from the reports to the years (or to the fiscal year prompt they were opened from), from the years to `project>`. |
| h      | Opens this manual.                                              |
| q      | Quits cratchit.                                                 |

Amounts have two decimals. Rows show debit and credit in columns of their own,
without sign; balances are debit positive and credit negative. A report longer
than the terminal window is shown a page at a time with `--More--`: space or
Enter shows the next page, `-` or `q` stops. After a report the report menu is
shown again.

#### The VAT return

`4` lists the VAT return periods of the year by the project's `vat_period` (see
The files). A quarterly VAT return covers a calendar quarter; the year lists
the quarters whose last day is in the year. A quarter that starts in the year
before still belongs to the year it ends in, and counts the vouchers of both
years. Only the quarters that have ended are numbered, newest first from key
0. The last quarter that ended before today is marked `to report next`: the VAT
is cleared after the quarter has ended. The quarter running today, when it ends
in the year, is shown on top without a key and marked `so far (+ to view)`;
press `+` to see its VAT return so far. Each quarter shows its VAT in three steps:

```
VAT return quarters ending in 2026-01-01 - 2026-12-31 (today 2026-10-27):
                  Period     Cleared     Charged
     2026 Q4      -25.00        0.00        0.00  so far (+ to view)
  0  2026 Q3     -250.00        0.00        0.00  to report next  2 checks
  1  2026 Q2     1120.00     1120.00     1120.00  cleared by M 2, charged by A 3
  2  2026 Q1     -250.08     -250.00     -250.00  cleared by M 1, charged by A 1
Charged, not matched to a quarter: A 4  2026-08-13  -9.99
```

`2 checks` says the checks of the quarter (see Checks of a quarter) found two
things to look at; a quarter where they find nothing has no mark.

With more than ten ended quarters, keys 0-8 show the newest and `9 ...down`
the older ones, as in the year list. A fiscal year none of whose quarters has
started yet has no VAT returns to show.

| Step    | What it is                                                         |
|---------|--------------------------------------------------------------------|
| Period  | The VAT the period's vouchers post on the VAT accounts: those that report in boxes 10-12, 30-32, 60-62 (output VAT) and 48 (input VAT). Clearing vouchers are left out. |
| Cleared | What clearing vouchers dated in the period move from the VAT accounts to 2650 (Redovisningskonto för moms) or 1650 (Momsfordran). A clearing voucher is any voucher with rows on 2650 or 1650 and on a VAT account, whatever its series. The amount is the rows on 2650 and 1650. |
| Charged | What SKV charged or credited the tax account (1630, Skattekonto) for the period. A charge voucher has rows on 2650 or 1650 and on 1630, and none on a VAT account. The amount is the rows on 2650 and 1650, with the sign turned. |

The figures keep the sign of the VAT accounts: VAT to pay is negative (credit),
VAT to get back positive (debit). A quarter that is cleared and charged shows
about the same amount three times; Cleared and Charged are in whole kronor, so
they may differ from Period by the rounding.

A charge is usually dated after its quarter, so it is not placed by its date:
taking the charges in date order, each goes to the oldest quarter that is
cleared by the same amount, starts on or before the charge's date and has no
charge yet, in any fiscal year of the books. A charge dated in the year that no
quarter takes is listed under the quarters as `Charged, not matched to a
quarter`. Payments between the tax account and the bank (1630 and e.g. 1930)
are not a step: they pay the tax account's balance, not one quarter.

The prompt is `period?>`. Press a number to see that quarter's VAT return and
then its clearing voucher (see Clearing the VAT of a quarter), or
`+` for the quarter running today so far (the VAT return only) (its heading ends `so far (today
...)`); `-` goes back to the reports. To see the quarters of another fiscal
year, open that year (see Fiscal years). With `vat_period = none` cratchit says there are no
VAT returns; `monthly` and `yearly` are not supported yet.

The VAT return lists each box of the form that the period's postings report in,
in the form's order, under the heading of the form's section it is in, with its
amount and its title on the form, and under it each account with its sum and
name and the vouchers behind it with their text, then box 49, then the three
steps with their vouchers:

```
VAT return (momsrapport) 2025-01-01 - 2025-03-31
Test AB, 556000-0001

Box  Account  Voucher  Date                 Amount  Text

Momspliktig försäljning eller uttag
05                                         1000.00  Momspliktig försäljning som inte ingår i r...
     3001                                 -1000.00  Sales 25%
              B 1      2025-02-10         -1000.00  Sale

10                                            0.00  Utgående moms 25%
     2611                                     0.00  Output VAT 25%
              B 1      2025-02-10          -250.00  Sale
              M 1      2025-03-31           250.00  Momsrapport

Beräkning av moms
49                                            0.00  10 + 11 + 12 + 30 + 31 + 32 + 60 + 61 + 62 - 48

The VAT of the period:
  Period            -250.00
  Cleared           -250.00  M 1  2025-03-31
  Charged           -250.00  A 1  2025-05-12

Box amounts are as on the form; accounts and vouchers as in the books: debit positive, credit negative.
Box 49 on the form: 0.00, nothing to pay or get back.
```

The sections and titles are SKV's words on the form, from the table built into
cratchit (`momsrapport.csv`), in Swedish as on the form; a title longer than 45
characters is cut and ends with `...`. Box 48 has two titles: `Ingående moms att
dra av`, or `Tidigare avdragen moms att betala tillbaka` when it is negative.
The order is the form's: sales and output VAT (05-12), purchases with reverse
charge (20-24, 30-32), import (50, 60-62), sales exempt from VAT (35-42), then
the VAT (48, 49).

Which box an account reports in comes from the table built into cratchit
(`bas_to_momsrapport.csv`). Only boxes that add up postings on accounts are
listed with accounts: the table names 2650 and 1650 as sources of box 49, not as
accounts that report in it. The boxes count every voucher dated in the period,
of every series, including clearing and charge vouchers. Removed rows do not
count.

A box's amount is the figure you give on SKV's form. Sales (boxes 05-08, 35,
36, 38-42) and output VAT (10-12, 30-32, 60-62) are credit in the books, so the
form takes their sum with the sign turned: sales of 1000.00 are 1000 in box 05.
Purchase bases (20-24, 37, 50) and input VAT (48) are debit in the books and keep
their sign. A box is negative only when its postings go the other way, e.g. credit
notes larger than the input VAT in box 48. The accounts and vouchers under a box
keep the sign they have in the books (debit positive, credit negative).

A box's amount is in whole kronor, as on the form: the öre are dropped (rounded
towards zero, so 17.50 becomes 17.00 and -2.50 becomes -2.00). The accounts and
vouchers under it keep their öre, so their sum may be up to 0.99 larger (sign
aside) than the box. The steps (Period, Cleared, Charged) are not rounded.

Box 49 (Moms att betala eller få tillbaka) is added up as the form does it:
10 + 11 + 12 + 30 + 31 + 32 + 60 + 61 + 62 - 48, from the box amounts. The box
49 row shows the form's figure: positive is VAT to pay,
negative VAT to get back (with only input VAT, box 49 is minus box 48). The
last line says it in words. A period without postings in any box shows no box 49.
After the VAT return of the quarter running today (`+`) the periods are shown again.

#### Checks of a quarter

The quarter's screen (below) starts with checks of the books behind its VAT
return: what could make the return wrong. They only warn; nothing is stopped.

```
Checks of 2026 Q3 (they only warn):
  ! 13 waiting events dated in the period, not yet in the books (the first 2026-07-18 "Momsbeslut Q2")
  ! A 48 2026-03-31 "Kortköp ... LAMYSHOP": posts 2650 or 1650 together with 1920, 6110; only a VAT clearing or a charge voucher posts them
```

When they find nothing: `Checks of 2026 Q3: nothing found.` The vouchers and
events are those dated in the quarter, from every fiscal year; struck rows do
not count.

| Check | What it finds |
|-------|---------------|
| Not fully booked | Waiting events and statement entries not yet booked (see Statement entries) dated in the quarter. |
| 2650 and 1650 | A voucher posting 2650 (Redovisningskonto för moms) or 1650 (Momsfordran) with rows on other accounts than VAT accounts (boxes 10-12, 30-32, 60-62, 48), 1630 (Skattekonto), 3740 (Öres- och kronutjämning) and memo accounts (9000-9999). Only a clearing or a charge voucher posts them; a purchase with its VAT against 2650 leaves the VAT out of the boxes. |
| Reverse charge and import | Per voucher: a purchase base (boxes 20-24, or 50) without output VAT (boxes 30-32, or 60-62), or output VAT without a base; with one output VAT box, its VAT must be the base times the box's rate (25 % for 30 and 60, 12 % for 31 and 61, 6 % for 32 and 62), rounded towards zero to whole öre; and box 48 must deduct that output VAT in full. |
| Registered after the clearing | A voucher on an account of a box, dated in the quarter, registered after the quarter's clearing voucher: the VAT return made then lacks it. |
| VAT accounts outside the return | Rows on accounts 2600-2699 (2650 aside) that report in no box. |

#### Clearing the VAT of a quarter

After the VAT return of an ended quarter, cratchit shows its checks (above) and
proposes the voucher that clears it, at a prompt named after the quarter (e.g.
`2025 Q4>`):

```
Checks of 2025 Q4: nothing found.

VAT clearing voucher for 2025 Q4 (proposed):
Voucher M 2 2025-12-31 "Momsrapport 2025-10-01 - 2025-12-31"
  Account        Debit       Credit
  2611          500.00               Output VAT 25%
  2650                       500.00  VAT clearing
  Total         500.00       500.00

eSKD file moms_202512.eskd:
  ForsMomsEjAnnan               2000
  MomsUtgHog                     500
  MomsBetala                     500

  1  Book it as M 2
  2  Show the report again
  3  Write the eSKD file only (no voucher, e.g. to test it)
  4  Review the boxes against SKV's descriptions
```

The voucher is made from the VAT return:

| Row  | What it is                                                         |
|------|--------------------------------------------------------------------|
| VAT accounts | Each account of a VAT box (10-12, 30-32, 60-62, 48) gets minus what the quarter's vouchers posted on it, öre and all, so the quarter leaves it at zero. Other boxes (such as 05) are not touched. |
| 2650 or 1650 | Box 49 as on the form (whole kronor): VAT to pay is credited to 2650 (Redovisningskonto för moms), VAT to get back debited to 1650 (Momsfordran). No row when box 49 is 0. |
| 3740 | Öres- och kronutjämning: what is left, because the boxes drop their öre, so that debit and credit add up. No row when nothing is left. |

It is dated the quarter's last day, headed `Momsrapport <first day> - <last
day>`, in series M (Momsrapport) with the next number of the year.

The eSKD file is the VAT return as Skatteverket's e-service "Lämna
momsdeklaration" takes it as a file (eSKDUpload 6.0, XML). It is made from the
same VAT return as the voucher, and the screen lists what it will hold, in file
order: each box with postings under its tag (e.g. `MomsUtgHog` for box 10),
with the box's amount in whole kronor, then `MomsBetala` (box 49), always. The
file has the organisation number (`orgnr` in `.cratchit/project.txt`) and the
period as the year and the quarter's last month, e.g. `202512`. Which tag each
box goes in comes from the table built into cratchit
(`momsrapport_box_to_eskd.csv`). For example:

```
<?xml version="1.0" encoding="ISO-8859-1"?>
<eSKDUpload Version="6.0">
  <OrgNr>556000-0001</OrgNr>
  <Moms>
    <Period>202512</Period>
    <ForsMomsEjAnnan>2000</ForsMomsEjAnnan>
    <MomsUtgHog>500</MomsUtgHog>
    <MomsBetala>500</MomsBetala>
  </Moms>
</eSKDUpload>
```

A negative amount has `-` straight before it (box 49 to get back, box 48 when
credit notes take it below zero).

| Key   | What it does                                                         |
|-------|----------------------------------------------------------------------|
| 1     | Book the voucher as it is shown (it cannot be changed here) and write the eSKD file in the project folder: `Wrote moms_202512.eskd in <folder>. Upload it in Skatteverket's e-service Lämna momsdeklaration.` Then the quarters are listed again, the quarter now with its Cleared amount. If the file cannot be written, the voucher is still saved and cratchit says so. |
| 2     | Show the quarter's VAT return again, then this screen.                 |
| 3     | Write the eSKD file only, without booking the voucher: `Wrote moms_202512.eskd in <folder>. No voucher is booked: the VAT of 2025 Q4 is not cleared in the books.` You stay at the quarter. Use it to try the file in Skatteverket's e-service before you book. Each press writes a new file (`_2`, `_3`, ...); the name on the screen is the next one free (Enter shows it again). Also in a closed year, as the books do not change. |
| 4     | Review the boxes against SKV's descriptions (see Reviewing the boxes against SKV's descriptions). Offered when the quarter has postings in any box. |
| Enter | Show this screen again.                                              |
| -     | Back to the quarters (`period?>`).                                   |

When the VAT accounts already add up to zero in the quarter, for example
because it is cleared, cratchit says `Nothing to clear: the VAT accounts add up
to 0.00 in 2025 Q4.` and only 2 (and 4) is offered. In a fiscal year closed for new
vouchers the voucher is shown, with a line saying it cannot be booked, and 1 is
left out (3 is still offered). A project without an organisation number (`orgnr`,
set by the first SIE import) gets neither 1 nor 3, as the eSKD file needs it.

Once booked, the quarter adds up to zero, so cratchit cannot write its eSKD file
again from the books. Keep the file you uploaded; to try the file first, use 3
before you book.

#### Reviewing the boxes against SKV's descriptions

`4` at the quarter lists the boxes to review (the prompt is `review?>`): every
box of each section of the form that has a box with postings, in the form's
order, with its title (cut to 45 characters) and how many postings it has, or
`no postings`. The count, not the amount, as a cleared quarter adds up to 0.00 in
its VAT boxes. So a box that should have postings but has none shows next to the
ones that do: here box 30 (output VAT on the purchase) is missing next to box 20:

```
Boxes of 2026 Q1 to review against SKV's descriptions (every box of a section with postings):
  1  20  Inköp av varor från annat EU-land                 1 posting
  2  21  Inköp av tjänster från annat EU-land enlig...   no postings
  ...
  6  30  Utgående moms 25%                               no postings
  ...
  9  48  Ingående moms att dra av                         2 postings
review?>
```

Box 49 is not listed: the form adds it up. With more than nine boxes, keys 1-8
show eight at a time, `0` the ones before and `9` the ones after. `-` goes back
to the quarter.

A number shows the box (the prompt is the box, e.g. `box 20>`): its full title
and section; its amount on the form, or that it has no postings; its accounts
with their sums and names and the vouchers behind them with their texts (as in
the books: debit positive, credit negative); the other accounts that report in
the box (from `bas_to_momsrapport.csv`), without postings, one a line with its
name in the books' chart of accounts (`(not in the chart of accounts)` when the
books lack it); then SKV's description
of the box (from `momsrapport.csv`), wrapped to 80 characters. A long box is
paged with `--More--`.

```
Box 20  Inköp av varor från annat EU-land
Section: Momspliktiga inköp vid omvänd betalningsskyldighet
On the form: 124.00
Accounts and vouchers as in the books (debit positive, credit negative):
  9021           124.00  Varuvärde Inlöp annat EG-land (Momsrapport ruta 20)
        A 48    2026-03-31         124.00  Kortköp 260330 SP LAMYSHOP
Other accounts that report in box 20, without postings:
  4515  Inköp av varor från annat EU-land, 25 %
  4516  (not in the chart of accounts)
  4517  (not in the chart of accounts)

SKV's description of box 20:
Inköp av varor från annat EU-land
Fyll i det här fältet om du har köpt varor från ett annat EU-land som du själv
ska redovisa den utgående momsen för.  ...

  1  Show box 20 again
box 20>
```

| Key   | What it does                                       |
|-------|----------------------------------------------------|
| 1     | Show the box again.                                |
| >     | The next box in the list.                          |
| <     | The box before in the list.                        |
| Enter | Show the numbered choice again.                    |
| -     | Back to the list of boxes.                         |

At either end of the list cratchit says `This is the first box in the list.` or
`This is the last box in the list.`. Nothing is changed or logged: the review is
for reading.

## Using the prompt

At the prompt, each key acts as soon as you press it. You don't need to press Enter.
Space and tab are ignored, except while you type a folder path, a name, an
account number, a voucher or event line, or a row (see Typing a folder path,
Creating a project, Entering a voucher and Waiting events).

At the `cratchit>`, `workspace>`, `project>`, fiscal year (such as
`2026/27>`) and `booking>` prompts, Enter lists the numbered keys you can press there, for
example:

```
project> 
Keys at project>:
  1  Fiscal years
  2  Import an SIE file
  3  Import an account statement
  4  Statement entries not yet booked (7)
  5  Reports
```

Only the numbered keys are listed. The other keys (`-` back, `l` the log, `h`
help, `q` quit, and at some screens `d` delete or `?` a hint) are not shown;
any other key prints `Unknown key.` and names them in one line:

```
project> x
Unknown key. Other keys: Enter list, - back, l log, h help, q quit.
```

The menus, lists and views (such as the workspace menu, the project menu,
the waiting list and a voucher) work the same way: they show only their
numbered choices, Enter shows them again, and an unknown key names the other
keys of that screen.

What you can do at these prompts is picked by number. Each number stays with
its choice: when a choice is not offered (such as opening the saved workspace
when there is none), its number is left out of the list and the other
choices keep theirs. Letters and signs are kept for what works the same
everywhere: `-` back, `q` quit, `h` help, `l` the log, Enter the key list,
`d` delete, `?` the hint, and `y`/`n` to answer a question.

A list (of projects, waiting events, vouchers, statement entries ...) only
picks: a number shows that item, and what you can do with it is then listed
by number.

Where you type a line (a path, a name, an account, a voucher or event line,
a heading or a row), cratchit keeps what you type in its own buffer. While the
line is empty, `-` goes back at once, as Enter on an empty line does (at
`post>` it removes the last row). Once you have typed something, `-` is part
of the line; to begin a line with a minus, type a space first. A line that
starts with text typed in for you (a path, a guessed account, a heading) is
not empty: erase it first to go back. At `post>`, `?` on an empty row shows
the hint at once (see The hint).

At the `cratchit>` prompt:

| Key    | What it does                        |
|--------|-------------------------------------|
| h      | Opens this manual (see Using help). |
| 1      | Shows the workspace menu.           |
| l      | Shows the log (see The log).        |
| Enter  | Lists the numbered keys.            |
| q      | Quits cratchit.                     |
| Ctrl-D | Quits cratchit (same as q).         |

Ctrl-C stops cratchit at once.

## The log

cratchit logs what happens while it runs, as extra information for you. The
log is not part of your books. Nothing from the log is shown on screen unless
you ask for it.

### Where the log is

All runs log to the same file, `log.txt` in the runtime's `.cratchit` folder
(next to `runtime.txt`, see The .cratchit folder). Each run adds to the end.
What happens at start, before cratchit has found the runtime, is written as
soon as it has. When cratchit stops with an error at start, nothing is logged.

If the log can't be written, cratchit warns once at start and goes on without
it:

```
Warning: Cannot open log <folder>/.cratchit/log.txt. Nothing is logged.
```

### What a log line holds

Each event is one line: the local time, the kind of event, where in
cratchit's code it was logged, and what happened:

```
2026-10-21T14:03:12 BUSINESS repl.cpp:285 Opened workspace /Users/me/Documents/cratchit
```

| Kind                 | What it means                                          |
|----------------------|--------------------------------------------------------|
| BUSINESS             | Something cratchit did, such as opening a workspace, setting a project's missing `vat_period` to the default, reading, checking or importing an SIE file, a note of an import, each reason an import was refused, an import that made the file the truth of a year (and each voucher it renamed, replaced, dropped or made a waiting event, in full), each waiting event an import took off the waiting list, a deleted voucher (in full), opening, closing or reopening a fiscal year, saving an entered voucher (naming its template, or the voucher it corrects, and whether it was flipped), booking the VAT clearing voucher of a quarter, writing its eSKD file, exporting a fiscal year as an SIE file (all, or the diff with a file), a diff export refused as the file has vouchers the books lack, noting or deleting a waiting event, making one waiting event of two, deleting a waiting event as already in the books, making a waiting event of a statement entry or adding one to it, or reading, choosing the account of and importing an account statement. |
| DESIGN_INSUFFICIENCY | Something cratchit is not designed for yet, such as a terminal encoding it can't handle, an SIE file in an unknown encoding, an account statement it cannot read, an eSKD or SIE file it could not write, or an SIE export with characters CP437 does not have. |
| DEVELOPMENT_TRACE    | Details for developing cratchit. Only Debug builds log these (see README.md). |

### Viewing the log

Press `l` at the `cratchit>`, `workspace>` or `project>` prompt. cratchit shows the log
newest event first, so you read backwards in time. When it does not fit in the
terminal window, it shows one page and then `--More--`:

| Key         | What it does                         |
|-------------|--------------------------------------|
| space/Enter | Shows the next page of older events. |
| -           | Stops and goes back to the prompt.   |
| q           | Stops and goes back to the prompt.   |
| Ctrl-D      | Quits cratchit.                      |

After the oldest event cratchit shows `(start of log)` and returns to the
prompt. When the output is not a terminal, the whole log is shown at once.

## Using help

Press `h` at the prompt to read this manual inside cratchit. It shows the
manual one section at a time, as plain text. The prompt changes to `help>`.

### Moving between sections

A section with subsections lists them with a number in front. Press the number
to open that section. Keep pressing numbers until you reach the text you want.
The prompt shows where you are, for example `help 2.1>` for the first
subsection of section 2.

| Key    | What it does                                             |
|--------|----------------------------------------------------------|
| 1-9    | Opens the section with that number.                      |
| -      | Goes back to the section above. At the top, leaves help. |
| Enter  | Shows the current section again.                         |
| h      | Shows the current section again.                         |
| q      | Leaves help and returns to the prompt you came from.     |
| Ctrl-D | Quits cratchit.                                          |

A section shows only its numbered subsections. Any other key prints
`Unknown key. Other keys: Enter this section again, - back, q leave help.`

The Escape key does nothing, because arrow keys send the same code first.

### Many sections

When a section has more than 9 subsections, the list shows 8 of them at a time.
Key `9` shows `...down` and moves to the next 8. Key `0` shows `...up` and moves
back to the previous 8.

### Long sections

When a section does not fit in the terminal window, help shows one page and
then `--More--`, like the `more` command. Press space or Enter to see the next
page. Press `-` to stop and go back, or `q` to leave help. When the output is
not a terminal, everything is shown at once.

## Running with piped input

When input does not come from a terminal, cratchit reads keys from it one by one.
This is useful for scripts:

```sh
printf '1q' | ./build/Debug/cratchit
```

In a fresh folder this creates `.cratchit` (see The .cratchit folder), chooses
the current folder as the workspace (key `1`) and quits. Once a workspace is
set, `printf 'q'` just opens it and quits.

cratchit stops at `q` or when the input ends.

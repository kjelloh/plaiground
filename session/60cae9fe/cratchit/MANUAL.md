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
organisation whose books it is, after the first import its organisation
number, and once you have opened one, the start of the fiscal year you last
opened:

```
created = 2026-10-21
uuid = 3f2b8c1e-9a4d-4e7f-b6a2-1c5d8e0f7a93
name = The ITfied AB
orgnr = 556782-8172
year = 2026-05-01
```

The uuid is made by cratchit when it creates the project and never changes.
The first SIE import sets `orgnr` and `name` (see Importing an SIE file).
Opening a fiscal year sets `year` (see Fiscal years).

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
`vouchers_known` and `notes` (the number of notes shown).

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
  Press `w` and browse to choose another workspace (see The workspace menu).

### The workspace menu

Press `w` at the `cratchit>` prompt to open the workspace again or switch to
another one:

```
Workspace:
  w  Open /Users/me/Documents/cratchit
  b  Browse for a new workspace
```

| Key    | What it does                                              |
|--------|-----------------------------------------------------------|
| w      | Opens the workspace saved in `runtime.txt`.               |
| b      | Lets you choose another folder (see Choosing a workspace). |
| -      | Goes back to the `cratchit>` prompt.                      |
| h      | Opens this manual.                                        |
| q      | Quits cratchit.                                           |
| Ctrl-D | Quits cratchit.                                           |

So `w` `w` at `cratchit>` takes you back into the workspace. When no workspace
is saved, or its folder does not exist, only `b` is offered.

Choosing a folder with `b` switches the workspace: cratchit saves it in
`runtime.txt` and opens it from then on. The old workspace folder is left as it
is.

### Choosing a workspace

cratchit offers these folders, both on start when no workspace is set and
after `b` in the workspace menu:

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
| -      | Goes back: to the workspace menu after `b`, else to the `cratchit>` prompt without a workspace. |
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
| Ctrl-D    | Quits cratchit.                                               |

Every other key, including `q`, `h` and `-`, is part of the path. Arrow keys
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
| p      | Shows the project menu (see Projects).       |
| Enter  | Lists these keys.                            |
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

Press `p` at the `workspace>` prompt:

```
Projects in /Users/me/Documents/cratchit:
  1  ITfied AB  /Users/me/Documents/cratchit/ITfied AB
  2  (not found)  /Volumes/share/Föreningen
  n  New project
  a  Add an existing project folder
```

The prompt is `project?>`. Each project shows its name and folder. A folder
that does not exist shows `(not found)`, and a folder that cratchit cannot read
as the listed project shows `(error)`. Choosing either shows the error and
leaves the list as it is.

| Key    | What it does                                                 |
|--------|--------------------------------------------------------------|
| 1-9    | Opens the project with that number.                          |
| n      | Creates a new project (see Creating a project).              |
| a      | Adds a project folder that already exists (see Adding an existing project). |
| -      | Goes back to the `workspace>` prompt.                        |
| h      | Opens this manual.                                           |
| q      | Quits cratchit.                                              |
| Ctrl-D | Quits cratchit.                                              |

With more than 9 projects the list shows 8 at a time. Key `9` shows `...down`
and moves to the next 8. Key `0` shows `...up` and moves back.

### Creating a project

Press `n` in the project menu and type the official name of the person,
company or organisation, then Enter. The prompt is `name>`. Backspace erases,
and Enter on an empty name goes back to the project menu. Every other key,
including `q`, `h` and `-`, is part of the name.

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

A folder that already holds a `project.txt` is an error; add it with `a`
instead. The folder may hold other cratchit files, for example the workspace's
own `.cratchit`.

### Adding an existing project

Press `a` in the project menu and type the folder of a project, for example one
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
| y      | Lists the fiscal years to open one (see Fiscal years). |
| i      | Imports an SIE file (see Importing an SIE file).    |
| s      | Imports an account statement (see Importing an account statement). |
| a      | Lists the statement entries not yet booked, to make waiting events of them (see Statement entries). |
| r      | Shows reports of the books (see Reports).           |
| Enter  | Lists these keys.                                   |
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

Vouchers are entered in an open fiscal year. Press `y` at the `project>`
prompt. cratchit lists the fiscal years in the books, numbered as SIE numbers
them: 0 is the newest, 1 the year before, and so on. Each says whether it is
open or closed for new vouchers:

```
Fiscal years of The ITfied AB, 556782-8172 (0 is the newest):
  0  2026-05-01 - 2027-04-30  open
  1  2025-05-01 - 2026-04-30  open
  ...
  9  2017-05-01 - 2018-04-30  open
  >  ...older

Press a number to open a year, - to go back, h for help, q to quit.
year?> 
```

The prompt is `year?>`. Press the number of a year to open it. Ten years are
shown at a time: `>` shows older years and `<` newer ones. On the older pages
the keys start at 0 again, and the heading says which year key 0 is, for
example `Fiscal years 10 to 10 ... (key 0 is year 10)`.

Fiscal years come from SIE files (see Importing an SIE file). Without books,
cratchit says `No books yet. Press i to import an SIE file.`

The year you open is saved as `year` in `project.txt`, so it opens again when
the project opens. If that year is not in the books, cratchit shows an error
and stays at `project>`.

#### In a fiscal year

The prompt is the fiscal year: `2026/27>` for a year from 2026 into 2027, or
`2025>` for a calendar year.

| Key    | What it does                                        |
|--------|-----------------------------------------------------|
| e      | Enters a voucher (see Entering a voucher). Only in an open year. |
| n      | Notes an event to post later (see Waiting events). Only in an open year. |
| w      | Lists the waiting events, to post or delete them (see Waiting events). The key list shows how many there are. |
| v      | Views the vouchers, to correct one or use it as a template (see Viewing and correcting vouchers). |
| s      | Imports an account statement, only its entries in this year (see Importing an account statement). |
| a      | Lists the statement entries not yet booked in this year (see Statement entries). |
| r      | Shows the reports of this year (see Reports). `-` there comes back here. |
| c      | Closes the year for new vouchers (shown when the year is open). |
| o      | Opens the year for new vouchers (shown when it is closed). |
| Enter  | Lists these keys.                                   |
| h      | Opens this manual.                                  |
| l      | Shows the log (see The log).                        |
| -      | Goes back to the `project>` prompt.                 |
| q      | Quits cratchit.                                     |
| Ctrl-D | Quits cratchit.                                     |

A new year from an SIE import is open. Closing it stops new vouchers from being
entered in it until you open it again; `e` then says
`Fiscal year 2025-05-01 - 2026-04-30 is closed for new vouchers. Press o to open it.`
cratchit saves the setting in `books.txt` (see The files) and logs it. Closing
a year changes nothing else: its vouchers, balances and reports stay as they are,
and an SIE import into it still works.

### Entering a voucher

Press `e` at the prompt of an open fiscal year (see Fiscal years). A voucher
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
  `20261022`. Without a date, today is used when it is in the fiscal year;
  else cratchit asks for it:
  `No date, and today is outside the fiscal year. Type the date.`
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

Press a number to choose, - to change the line, h for help, q to quit.
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

Enter on an empty line goes back to the year prompt.

#### The template

cratchit then looks for earlier vouchers like this one, to guide the rows.
A voucher is like it when its text shares words with your heading (case,
numbers and one-letter words do not count), it is in one of the series A-N,
and it has an amount and at least two rows. The most shared words come first,
then a voucher whose amount has the same sign as yours, then the latest.
Vouchers with rows on the same accounts, on the same sides (debit or credit),
show only once, as the best of them. At most five are listed, each with its
rows on one line: the account, `D` (debit) or `C` (credit) and the amount:

```
Earlier vouchers like this one:
  1  D 9  2026-10-22       1118.00  Telia faktura oktober
      2440 C 1118.00  6212 D 894.40  2641 D 223.60
  2  E 4  2026-07-01        987.00  Betalning Telia
      2440 D 987.00  1930 C 987.00

Press a number or Enter (= 1) to use it as a template, n for none, - to go back, h for help, q to quit.
template?> 
```

| Key    | What it does |
|--------|--------------|
| 1-5    | Uses that voucher as the template. |
| Enter  | Uses the first one. |
| n      | Uses no template: the series is guessed and the rows are typed as without one. |
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

  s  Save as D 10
  f  Flip the template's rows (debit becomes credit)
  -  Change the rows
  h  Help (the manual)
  q  Quit
postings> 
```

`s` saves it (see Saving). `f` flips the template (see Flipping the
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
template. Press `f` at `postings>` to flip the template: each of its rows
changes side (debit becomes credit, credit becomes debit), and the rows are
suggested again from the flipped template. Rows you typed or changed after the
template's rows are replaced. The amount on the voucher line stays as it is.

```
postings> f
Suggested from D 9, scaled to the amount and flipped:
Voucher D 10 2026-11-20 "Telia kreditfaktura"
  Account        Debit       Credit
  2440         1245.00               Leverantörsskulder
  6212                       996.00  Mobiltelefon
  2641                       249.00  Debiterad ingående moms
  Total        1245.00      1245.00
```

`f` again flips it back. When the flipped rows balance, the whole voucher is
shown at `postings>` again; else they are suggested one at a time at `post>`
(`Rows from D 9 are suggested one at a time, scaled to the amount and
flipped`). Only the template is flipped: the postings a waiting event already
has stay as they are (see The waiting list), and a single row cannot be
flipped. `f` is offered only when the voucher has a template (also with `t`
and when correcting, see Viewing and correcting vouchers).

Rows that add up to zero but not to the amount show
`Debit 223.60, credit 223.60, amount 1118.00: debit and credit must each reach the amount.`
More than the amount on either side shows `...: more than the amount.`

| You type          | What it does |
|-------------------|--------------|
| account amount    | Adds a row. The amount is written as on the voucher line; spaces in it are dropped. |
| account           | Adds a row with what remains. On the first row, the voucher's amount. When the rows add up to zero but not to the amount, cratchit asks you to type the amount. |
| a number prefix or a word | When it is not an account in the books, lists the accounts whose number starts with it or whose name contains it (case does not matter), at most 20. |
| -                 | Removes the last row. With no rows, drops the voucher: `Dropped the voucher. Nothing is saved.` |
| Enter (empty)     | When the voucher balances (at least two rows, debit and credit each adding up to the amount), shows it and asks to save it. Else adds the suggested row, when there is one. Else shows where the rows stand; the voucher cannot be saved. |

Only accounts in the books can be used. Backspace erases while you type.

#### Saving

```
Voucher D 4 2026-10-22 "Telia faktura oktober"
  Account        Debit       Credit
  1930                      1118.00  Företagskonto
  6212          894.40               Mobiltelefon
  2641          223.60               Debiterad ingående moms
  Total        1118.00      1118.00

  s  Save as D 4
  -  Change the rows
  h  Help (the manual)
  q  Quit
postings> 
```

The prompt is `postings>`:

| Key    | What it does |
|--------|--------------|
| s      | Saves the voucher. |
| f      | Flips the template's rows (see Flipping the template). Only listed when the voucher has a template. |
| -      | Goes back to the rows at `post>`. |
| Enter  | Lists these keys. Any other key prints `Unknown key.` and the list. |
| h      | Opens this manual. |
| q      | Quits cratchit. |

Press `s` to save. cratchit adds the
voucher to the fiscal year's files in the books (see The files), shows which
files it updated, says `Saved voucher D 4 into fiscal year 2026-05-01 -
2027-04-30.` and logs it (naming the template, if one was used, and whether it
was flipped). A saved
voucher is never changed or deleted. A wrong one is corrected with a new
voucher (see Viewing and correcting vouchers).

The vouchers you enter are the truth of the books. A later SIE import keeps
them: a voucher in the books that the file lacks is noted, and a voucher in
the file with the same series and number but other content is an error, so
nothing is imported. Keep booking a fiscal year in one program only, cratchit
or the other, until cratchit can sync them.

### Viewing and correcting vouchers

Press `v` at the prompt of a fiscal year. cratchit lists the series used in
the year; press a letter to list that series (the prompt is `series?>`; here
`h` is series H, so help is not offered; `-` goes back to the year):

```
Series in fiscal year 2026-05-01 - 2027-04-30:
  A  Redovisning                        12 vouchers, last A 12 2026-10-02
  D  Leverantörsfakturor                 4 vouchers, last D 4 2026-10-24

Press a letter to list that series, - to go back, q to quit.
series?> d
Vouchers D in fiscal year 2026-05-01 - 2027-04-30, newest first:
  1  D 4     2026-10-24       1245.00  Faktura Telia november  (entered)
  2  D 3     2026-05-18       2359.00  Faktura: Telia  (imported)
  ...

Press a number to view it, - to go back, h for help, q to quit.
voucher?> 
```

Newest first means the highest number first. With more than nine, keys 1-8
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

Press c to correct it, t to use it as a template, - to go back, h for help, q to quit.
D 4> 
```

| Key    | What it does |
|--------|--------------|
| c      | Corrects the voucher with a new one (below). Only for an entered voucher, in an open year. |
| t      | Enters a new voucher with this one as the template: the voucher line, then the series and the rows, as in Entering a voucher, without the list of templates. Only in an open year. |
| -      | Goes back to the list. |
| h      | Opens this manual. |
| q      | Quits cratchit. |

#### Correcting a voucher

A correction is a new voucher that reverses the wrong one: the same rows with
the opposite signs, so together they add up to nothing. The wrong voucher
stays in the books as it is. `c` starts the voucher line with the correction's
heading and amount, for you to change or accept; without a date it is dated
today (asked for when today is outside the year):

```
voucher> Rättelse av D 4: Faktura Telia november 1245.00
```

Then, as in Entering a voucher: `OK? (y/n)`, the series (by default the
corrected voucher's series), and the corrected voucher's rows reversed, shown
as the whole voucher to save (`The rows of D 4, reversed:`, then `postings>`:
`s` saves, `f` flips the rows, `-` puts the rows in place at `post>` to change
them, as in The rows).
When it is saved, cratchit offers to enter the right voucher:

```
Saved voucher D 5 into fiscal year 2026-05-01 - 2027-04-30.
D 5 reverses D 4. The right voucher can follow, with D 4 as the template.
Enter the right voucher now? (y/n) 
```

`y` starts the voucher line with the corrected voucher's heading and amount,
and its rows are the template. `n` goes back to the year prompt.

An imported voucher is not corrected in cratchit:
`D 3 came with an SIE import, so cratchit does not correct it. Correct it in
the program it came from and import the file again.`

### Waiting events

An invoice or a receipt can be noted when it comes, and posted on the accounts
later. A noted event waits in its fiscal year until you post it or delete it.
It is not a voucher: it is in no series, has no number and is not in the
reports.

#### Noting an event

Press `n` at the prompt of an open fiscal year. Type the heading, amount and
date at `event>` as at `voucher>` (see Entering a voucher): the same rules,
the same `amount?>` question for an unclear amount, and a date outside the
year is refused. Press `y` to note it:

```
2026/27> n
Type the heading, amount and date of the event to post later, e.g. Telia faktura oktober 1118,00 2026-10-22.
No date means today. Backspace erases, Enter accepts. Enter on an empty line goes back.
event> Telia faktura oktober 1118 2026-10-20
  Heading  Telia faktura oktober
  Amount   1118.00
  Date     2026-10-20   (fiscal year 2026-05-01 - 2027-04-30)
OK? (y/n) y
Initiated .cratchit/books/2026-05-01/events.csv in ...
Noted: 2026-10-20 Telia faktura oktober 1118.00 (1 waiting). Press w to post it.
```

The amount is kept without sign, as on a voucher. cratchit saves the event in
the year's `events.csv` (see The files) and logs it.

#### The waiting list

Press `w` at the year prompt. The prompt is `waiting?>`:

```
Waiting events in fiscal year 2026-05-01 - 2027-04-30:
  1  2026-10-20       1118.00  Telia faktura oktober
  2  2026-10-21        249.00  Kvitto Clas Ohlson

Press a number to post it, d to delete one, - to go back, h for help, q to quit.
waiting?> 
```

They are listed by date, oldest first; events with the same date in the order
noted. With more than nine, keys 1-8 show eight
at a time, `0` shows the ones before and `9` the ones after. With none,
`w` says `No events are waiting in fiscal year ... Press n to note one.`

| Key    | What it does |
|--------|--------------|
| 1-9    | Posts that event: the series, then the rows, as in Entering a voucher. Only in an open year. |
| d      | Deletes one: the prompt becomes `delete?>`; press the number of the event, then `y` at `Delete it? (y/n)`. `-` at `delete?>` or `n` deletes nothing. |
| -      | Goes back to the year prompt. |
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
list. The event's postings are the first rows; a template's suggestions
follow them, leaving out a row on an account the event already has. When the
event's postings and the suggestions balance together, the whole voucher is
shown to save as it is (see The rows); else they are at `post>`. `f` at
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
of a deleted event are not booked any more, so they are listed by `a` again.

#### Statement entries

The entries of the imported account statements (see Importing an account
statement) wait to be booked. Press `a` at `project>`, or at a fiscal year's
prompt for only that year's entries. The prompt is `entries?>`:

```
Statement entries not yet booked in fiscal year 2026-05-01 - 2027-04-30:
  1  1920   2026-07-03         -3.70  AVGIFTER NORDEA
  2  1920   2026-07-23       2292.00  Skatt | 5050-1030 SK5567828172
  ...
  7  1920   2026-09-28      -1308.75  5343-2795 | LOOPIA AB (WEBSUPPORT) | 62600010249540 | Webhotell 12/10 - 11/1

Press a number to choose an entry, - to go back, h for help, q to quit.
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
| n      | Makes a new waiting event with it. Type the heading at `heading>` (the entry's text is typed in for you to change). The event gets the entry's date and its amount without sign, and the entry as its posting on the statement's account. |
| w      | Adds it to a waiting event of its fiscal year, chosen by number at `event?>` (listed as in the waiting list). The event's amount stays as it is. |
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

Press `i` at the `project>` prompt and type the path of the file (see Typing a
folder path). cratchit reads and checks the file and shows what importing it
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

Press `y` to import, `n` or `-` not to. When the file has errors, cratchit lists
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
- Vouchers (financial events) are never changed once in the books. A voucher
  already in the books (same series and number) is skipped when it is
  identical, and is an error when it differs. New vouchers are added. Vouchers
  in the books that the file lacks are noted and kept.
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

### Importing an account statement

An account statement is a file from the bank or from Skatteverket listing the
entries of one account: Nordea's CSV files and Skatteverket's "Bokförda
transaktioner" (CSV or .skv). Press `s` at `project>`, or at a fiscal year's
prompt to import only the entries in that year. The entries wait to be
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

Press a number to import that file, - to go back, h for help, q to quit.
statement?> 
```

A file shows `new`, `3 already imported` or `already imported` (by the
statements imported so far). With more than nine files, keys 1-8 show eight
at a time, `0` the ones before and `9` the ones after. `-` goes back to the
path. Enter on an empty path goes back to the prompt you came from.

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
accounts. Enter accepts; Enter on an empty line imports nothing.
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
Imported 15 entries for account 1920 (0 already known, 0 outside). Press a to book them.
```

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

Press `r` at the `project>` prompt. cratchit lists the fiscal years in the
books, newest first; press a number to choose one (with more than 9, keys 0
and 9 scroll as in the project menu). Then choose a report. In a fiscal year,
`r` shows the reports of that year at once, and `-` goes back to its prompt:

```
Reports for 2025-05-01 - 2026-04-30:
  1  Trial balance (saldobalans)
  2  Vouchers
  3  Account ledger
```

The prompt is `report?>`.

| Key    | What it does                                                    |
|--------|-----------------------------------------------------------------|
| 1      | Trial balance: each balance sheet account with opening balance, change and closing balance, each result account with its result, and totals. It says where the opening balances come from, and lists those that differ from what the source reported. |
| 2      | Every voucher of the year with its rows in Debit and Credit columns, and the totals. Rows added or struck after registration are marked `(added)` or `(removed)`, with the signature when there is one; a removed row does not count in the totals. |
| 3      | Account ledger: type an account number at `account>`, then Enter. Shows the opening balance, every row on the account by date in Debit and Credit columns with the balance after it, and the closing balance. Enter on an empty number goes back. |
| -      | Goes back: from the reports to the years (or to the fiscal year prompt they were opened from), from the years to `project>`. |
| h      | Opens this manual.                                              |
| q      | Quits cratchit.                                                 |

Amounts have two decimals. Rows show debit and credit in columns of their own,
without sign; balances are debit positive and credit negative. A report longer
than the terminal window is shown a page at a time with `--More--`: space or
Enter shows the next page, `-` or `q` stops. After a report the report menu is
shown again.

## Using the prompt

At the prompt, each key acts as soon as you press it. You don't need to press Enter.
Space and tab are ignored, except while you type a folder path, a name, an
account number, a voucher or event line, or a row (see Typing a folder path,
Creating a project, Entering a voucher and Waiting events).

At the `cratchit>`, `workspace>`, `project>`, fiscal year (such as
`2026/27>`) and `postings>` prompts, Enter lists the keys you can press there,
for example:

```
project> 
Keys at project>:
  y  Fiscal years
  i  Import an SIE file
  r  Reports
  l  Show the log
  -  Back to workspace>
  h  Help (the manual)
  q  Quit
```

Any other key prints `Unknown key.` and the same list. In the menus (such as
the workspace menu, the project menu and the reports), the menu itself shows
the keys.

At the `cratchit>` prompt:

| Key    | What it does                        |
|--------|-------------------------------------|
| h      | Opens this manual (see Using help). |
| l      | Shows the log (see The log).        |
| w      | Shows the workspace menu.           |
| Enter  | Lists these keys.                   |
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
| BUSINESS             | Something cratchit did, such as opening a workspace, reading, checking or importing an SIE file, a note of an import, opening, closing or reopening a fiscal year, saving an entered voucher (naming its template, or the voucher it corrects, and whether it was flipped), noting or deleting a waiting event, making a waiting event of a statement entry or adding one to it, or reading, choosing the account of and importing an account statement. |
| DESIGN_INSUFFICIENCY | Something cratchit is not designed for yet, such as a terminal encoding it can't handle, an SIE file in an unknown encoding, or an account statement it cannot read. |
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
| h      | Shows the current section again.                         |
| q      | Leaves help and returns to the prompt you came from.     |
| Ctrl-D | Quits cratchit.                                          |

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

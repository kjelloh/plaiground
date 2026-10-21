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
was created, its uuid and the official name of the person, company or
organisation whose books it is:

```
created = 2026-10-21
uuid = 3f2b8c1e-9a4d-4e7f-b6a2-1c5d8e0f7a93
name = ITfied AB
```

The uuid is made by cratchit when it creates the project and never changes.

`log.txt`, next to `runtime.txt`, is the log (see The log). cratchit creates
it the first time it logs something and only ever adds lines to it.

### When something is missing

cratchit changes an existing file only when you ask it to: choosing a new
workspace sets the `workspace` line in `runtime.txt`, keeps the other lines,
and reports `Updated .cratchit/runtime.txt in <folder>`. Creating, adding or
opening a project lists it in `workspace.txt` and saves it as the last opened,
keeping the other lines, and reports `Updated .cratchit/workspace.txt in
<folder>` when the file changes.

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
on Windows). For a project folder it starts at the workspace folder. You type
the rest, one key at a time:

| Key       | What it does                                                  |
|-----------|---------------------------------------------------------------|
| Tab       | Completes the folder name you have started (see below).       |
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
you type the dot.

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

| Key    | What it does                          |
|--------|---------------------------------------|
| h      | Opens this manual.                    |
| l      | Shows the log (see The log).          |
| -      | Goes back to the `workspace>` prompt. |
| q      | Quits cratchit.                       |
| Ctrl-D | Quits cratchit.                       |

The project you open is saved as the last opened in `workspace.txt`, so it
opens again the next time the workspace opens. If the last opened project is
not listed, or cannot be opened, cratchit shows the error and stays at
`workspace>`.

## Using the prompt

At the prompt, each key acts as soon as you press it. You don't need to press Enter.
Enter, space and tab are ignored, except while you type a folder path or a
name (see Typing a folder path and Creating a project).

| Key    | What it does                        |
|--------|-------------------------------------|
| h      | Opens this manual (see Using help). |
| l      | Shows the log (see The log).        |
| w      | Shows the workspace menu.           |
| q      | Quits cratchit.                     |
| Ctrl-D | Quits cratchit (same as q).         |

Any other key prints `Unknown key. Press h for help.`

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
| BUSINESS             | Something cratchit did, such as opening a workspace.   |
| DESIGN_INSUFFICIENCY | Something cratchit is not designed for yet, such as a terminal encoding it can't handle. |
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

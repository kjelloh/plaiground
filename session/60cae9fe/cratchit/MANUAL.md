# cratchit user manual

cratchit is a single-user app for Swedish bookkeeping. This manual describes
everything you can do with it. It grows as cratchit grows.

## Starting cratchit

Run `cratchit` from a terminal (see README.md for how to build it):

```sh
./build/Debug/cratchit
```

cratchit prints its version, then opens the workspace (see The workspace).
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

`workspace.txt` holds the date it was created:

```
created = 2026-10-20
```

### When something is missing

cratchit changes an existing file only when you ask it to: choosing a
workspace sets the `workspace` line in `runtime.txt`, keeps the other lines,
and reports `Updated .cratchit/runtime.txt in <folder>`.

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

cratchit opens the workspace when it starts, and when you press `w` at the
`cratchit>` prompt:

- If `runtime.txt` names a workspace folder, cratchit opens it and shows
  `Opened workspace <folder>`. A folder without `.cratchit` is a fresh
  location: cratchit creates `.cratchit/workspace.txt` there and reports it as
  `Initiated`.
- If no workspace is set, or its folder does not exist, cratchit asks you to
  choose one (see Choosing a workspace).
- If the folder has a `.cratchit` without `workspace.txt`, or a malformed
  `workspace.txt`, cratchit shows an error and stays at the `cratchit>` prompt.

### Choosing a workspace

cratchit offers these folders:

```
Choose a workspace folder:
  1  /Users/me/Documents/cratchit
  2  /Users/me/books
  3  Type a folder path
```

| Key    | What it does                                                     |
|--------|------------------------------------------------------------------|
| 1      | Uses `cratchit` in your Documents folder, creating it if needed. |
| 2      | Uses the folder you started cratchit in.                         |
| 3      | Lets you type a folder path (see Typing a folder path).          |
| -      | Goes back to the `cratchit>` prompt without a workspace.         |
| h      | Opens this manual.                                               |
| q      | Quits cratchit.                                                  |
| Ctrl-D | Quits cratchit.                                                  |

The Documents folder is `~/Documents` on macOS. On Linux it is the one set in
`~/.config/user-dirs.dirs` (for example `~/Dokument`), else `~/Documents`. On
Windows it is your Documents folder, also when it has been moved, for example
to OneDrive. If cratchit cannot find it, option 1 is not shown.

When you choose a folder, cratchit adds `.cratchit/workspace.txt` to it if it
is missing, saves the folder in `runtime.txt` and opens the workspace.

### Typing a folder path

The path starts at the root folder (`/`, or for example `C:\` on Windows) and
you type the rest, one key at a time:

| Key       | What it does                                                  |
|-----------|---------------------------------------------------------------|
| Tab       | Completes the folder name you have started (see below).       |
| Backspace | Erases the last character.                                    |
| Enter     | Accepts the path. On an empty path, goes back to the choices. |
| Ctrl-D    | Quits cratchit.                                               |

Every other key, including `q`, `h` and `-`, is part of the path. Arrow keys
do nothing.

Tab works like in a shell. When one folder matches what you have typed, its
name is completed and a separator added. When several match, the name is
completed as far as they agree, and when they agree on nothing more, Tab
lists them. When none
match, the terminal beeps. Hidden folders (starting with `.`) match only when
you type the dot.

When the folder exists, cratchit uses it. When it does not, cratchit asks
`Create it? (y/n)`. Press `y` to create it, or `n` to go back and change the
path.

### In the workspace

The prompt is `workspace>`.

| Key    | What it does                         |
|--------|--------------------------------------|
| h      | Opens this manual.                   |
| -      | Goes back to the `cratchit>` prompt. |
| q      | Quits cratchit.                      |
| Ctrl-D | Quits cratchit.                      |

## Using the prompt

At the prompt, each key acts as soon as you press it. You don't need to press Enter.
Enter, space and tab are ignored, except while you type a folder path (see
Typing a folder path).

| Key    | What it does                        |
|--------|-------------------------------------|
| h      | Opens this manual (see Using help). |
| w      | Opens the workspace.                |
| q      | Quits cratchit.                     |
| Ctrl-D | Quits cratchit (same as q).         |

Any other key prints `Unknown key. Press h for help.`

Ctrl-C stops cratchit at once.

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
printf '2q' | ./build/Debug/cratchit
```

In a fresh folder this creates `.cratchit` (see The .cratchit folder), chooses
the current folder as the workspace (key `2`) and quits. Once a workspace is
set, `printf 'q'` just opens it and quits.

cratchit stops at `q` or when the input ends.

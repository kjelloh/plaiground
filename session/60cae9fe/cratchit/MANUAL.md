# cratchit user manual

cratchit is a single-user app for Swedish bookkeeping. This manual describes
everything you can do with it. It grows as cratchit grows.

## Starting cratchit

Run `cratchit` from a terminal (see README.md for how to build it):

```sh
./build/Debug/cratchit
```

cratchit prints its version and shows the prompt:

```
cratchit 0.1.0. Press h for help, q to quit.
cratchit> 
```

## Using the prompt

At the prompt, each key acts as soon as you press it. You don't need to press Enter.
Enter, space and tab are ignored.

| Key    | What it does                        |
|--------|-------------------------------------|
| h      | Opens this manual (see Using help). |
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
| q      | Leaves help and returns to the `cratchit>` prompt.       |
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
printf 'hq' | ./build/Debug/cratchit
```

cratchit stops at `q` or when the input ends.

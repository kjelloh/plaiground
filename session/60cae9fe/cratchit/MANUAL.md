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

| Key    | What it does                 |
|--------|------------------------------|
| h      | Shows the list of keys.      |
| q      | Quits cratchit.              |
| Ctrl-D | Quits cratchit (same as q).  |

Any other key prints `Unknown key. Press h for help.`

Ctrl-C stops cratchit at once.

## Running with piped input

When input does not come from a terminal, cratchit reads keys from it one by one.
This is useful for scripts:

```sh
printf 'hq' | ./build/Debug/cratchit
```

cratchit stops at `q` or when the input ends.

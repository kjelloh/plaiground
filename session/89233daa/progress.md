# pyroceph progress

Working rules: small steps, the user tests each one and makes all commits.
All files stay inside `session/89233daa/`.

## Current status

Step 1 is built and waiting for the user's test.

## Steps

| # | Step | Status |
|---|------|--------|
| 1 | CMake/Conan skeleton: raw keys to Unicode code points, `q` quits | Waiting for user test |

## Decisions

- Enter posts the buffer to the current view state.
- `-` changes to the previous state. Undo restores the whole Model. Both update the view.
- Each drill-down digit is one stack level.
- The client designs the DAG so that non-list options fit 0..9.
- The clear key is picked when we reach that step.
- Transcoding code is copied into this session (only the files needed), as walk_pdf did from cratchit.
  Step 1 copies `charset/unicode.hpp` and `charset/utf8.*` from `9fe36d28/walk_pdf/src`.
- Raw key reading is plain code (termios / conio), adapted from cratchit's `RawTerminal`, with no Conan package.

## Known limits of step 1

- Input is always decoded as UTF-8. Detecting the platform's encoding is a later step
  (see `60cae9fe/cratchit/src/core/terminal_encoding.*`).
- The Windows branch has not been compiled or run. Windows output and non-UTF-8 code pages are not handled yet.

## Open questions

- Do `q`, `-` and digits act even while the buffer holds text? This must be settled before the buffer step.

# pyroceph progress

Working rules: small steps, the user tests each one and makes all commits.
All files stay inside `session/89233daa/`.

## Current status

Step 1 is built and waiting for the user's test.

## Steps

| # | Step | Status |
|---|------|--------|
| 1 | CMake/Conan skeleton: raw keys to Unicode code points, `q` quits | Waiting for user test |

## Specification

Agreed behaviour and open questions now live in `specification.md`.

## Known limits of step 1

- Input is always decoded as UTF-8. Detecting the platform's encoding is a later step
  (see `60cae9fe/cratchit/src/core/terminal_encoding.*`).
- The Windows branch has not been compiled or run. Windows output and non-UTF-8 code pages are not handled yet.

# Progress

## Now

Task 1: CMake + Conan scaffold in `cratchit/`. Builds without warnings, tests pass. In review.

## Next

To be given in chat.

## Agreed in chat

- 2026-10-19: Tasks are given in chat, not in files. Conventions, architecture and domain knowledge live in the code the user approves, not in separate docs.
- 2026-10-19: Implement natively, or consume from Conan when appropriate.
- 2026-10-19: Targets: `cratchit_core` library (all logic) + thin `cratchit` executable + `cratchit_tests` (GoogleTest via Conan).
- 2026-10-19: Classic headers (`#pragma once`), no C++20 modules.

## Open questions

- Should warnings be errors (`-Werror` / `/WX`)? Currently not.
- Is a CI setup (e.g. GitHub Actions on macOS/Linux/Windows) wanted to verify cross-platform builds?

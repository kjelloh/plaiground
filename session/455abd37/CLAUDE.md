# CLAUDE.md: cratchit take 2 (session 455abd37)

The repo-level sandbox rule applies: write only inside `session/455abd37/`.
Read `session.md`, then `README.md` and `docs/interfaces.md`.

## The one rule of this take

An interface is written down in `specs/` before code uses it, and code reads the spec instead of
hard-coding it. Where code can't read the spec, `src/app/spec_check.cpp` must check that they agree.
After any change, run `cratchit check-specs` and the tests.

- Spec format: `docs/cspec_format.md`. Every spec has the `spec.*` header and names its sources.
- Mark uncertainty in the spec (`….status = unverified`, `….note`). Don't hide it in code.
- Prefer free official sources: bas.se, sie.se, skatteverket.se, bolagsverket.se, bfn.se.

## Code conventions (as in take 1)

- C++23, CMake, Conan, GoogleTest. Zero warnings.
- snake_case functions and variables, CamelCase types, `m_` prefix on all members.
- Money is `Amount` (integer öre). Errors use `std::expected`.
- Small files, one domain idea each.

## Data and privacy

- Real data stays outside git: tests read take 1's `../62c6984e/_sie_in` and `_eskd_irl` and skip
  when they are absent. Never copy real org numbers, names or amounts into `testdata/` or `specs/`.
- `testdata/` holds fictional examples and SKV's public example files only.

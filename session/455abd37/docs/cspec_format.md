# The cratchit spec format (`.cspec`)

cratchit pins down every interface in plain text files of `path = value` lines.
The same format is used for interface specs, mappings, legal requirements, settings,
and for some of cratchit's own data (such as financial event listings).

## Example

```text
# Comment lines start with '#'
spec.id = vat.boxes
spec.kind = mapping
spec.version = 1
box.05.element = ForsMomsEjAnnan
box.05.title = "Momspliktig försäljning som inte ingår i ruta 06, 07 eller 08"
csv.thousands_separator = " "
```

## Rules

1. The file is UTF-8 text. LF or CRLF line endings.
2. Blank lines are ignored. A line whose first non-blank character is `#` is a comment.
   There are no end-of-line comments: a `#` after the `=` is part of the value.
3. Every other line is `path = value`. The line is split at the **first** `=`.
   Path and value are trimmed of surrounding blanks.
4. A path is one or more segments joined by `.`. A segment uses only `A–Z a–z 0–9 _ -`.
5. A value is the rest of the line. If it starts and ends with `"`, it is a quoted string:
   the quotes are removed and `\"`, `\\`, `\n` and `\t` are unescaped. Use quotes for
   empty values, or for values with blanks at either end.
6. A path may appear only once in a file.
7. A path may have both a value and children. This is used for annotations:
   `rounding.box = truncate` together with `rounding.box.note = …` and `rounding.box.status = …`.
8. Children keep the order of their first appearance in the file. Lists are written as
   numbered children: `field.01 = …`, `field.02 = …`.

What a value means (text, integer, date, account range…) is defined by the spec that
reads it, not by the format. Shared value conventions are listed below.

## Why no sections, includes or nesting syntax?

Each line stands alone. You can `grep 'box.05.'` in any file and see the whole truth.
Diffs show exactly what changed. A file can be produced by a generator with plain `printf`.

## Required header for spec files under `specs/`

| Path | Meaning |
|---|---|
| `spec.id` | Unique id, e.g. `vat.eskd_6p0`. The interface registry refers to specs by file path, so the id is for humans and provenance records |
| `spec.kind` | `format` (a file or API layout), `mapping` (from one set of codes to another), `rules` (calculation rules), `registry` (a list of things), `legal`, `settings` or `schema` |
| `spec.version` | Increase it when the content changes meaning |
| `spec.status` | `draft` (written from memory or notes), `checked` (compared to the source), `verified` (proven by real data, such as reproducing filed returns) |
| `spec.title` | One line |
| `spec.source.NN.title` / `.url` / `.local` / `.retrieved` | Where the content comes from. `local` is a path to a downloaded copy |
| `spec.derived_by` | `hand` or the name of the generator tool and its version |

Uncertain entries get a sibling `….status = unverified` and/or a `….note = …` explaining the doubt.

## Shared value conventions

| Kind | Syntax | Example |
|---|---|---|
| boolean | `yes` / `no` | `file.include_doctype = yes` |
| integer | decimal digits, optional `-` | `max_length = 300` |
| date | `YYYYMMDD` (as in SIE) | `20240501` |
| month day | `MMDD` | `0501` |
| amount | decimal with `.`, optional `-` | `-250.50` |
| account list | comma-separated BAS accounts or `from-to` ranges | `2610-2613, 2617` |
| list of words | comma-separated | `10, 11, 12` |
| reference | path relative to `specs/` | `vat/vat_boxes.cspec` |

## Code

- Parser and in-memory tree: `src/cspec/spec_tree.hpp` (`SpecTree::parse`, `load`, `value`, `require`, `children`)
- Writing values: `cspec::quote` (used for event listings and provenance records)
- Every file under `specs/` is parsed and validated by `tests/specs_test.cpp` and by
  `cratchit check-specs`.

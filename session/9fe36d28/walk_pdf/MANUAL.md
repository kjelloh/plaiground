# walk_pdf

walk_pdf walks the structure of a PDF file and shows each part it visits as a
*path* (where in the file it is) and *content* (what is there). The main aim is
the text of the PDF, so that it can be turned into plain text or Markdown.

> Status: early. walk_pdf finds the file's revisions and their top-level
> elements. It does not yet look inside objects or extract text.

## Usage

```
walk_pdf <file.pdf>          list the whole file
walk_pdf <file.pdf> <path>   list the element at path and what is inside it
walk_pdf [options]
```

`walk_pdf <file.pdf>` prints one line per element:

```
    offset       size  path                     preview
         0       1633  /rev[0]
         0          8  /rev[0]/header           %PDF-1.7
        12        171  /rev[0]/obj[73867]       73867 0 obj
       187        416  /rev[0]/xref[0]          xref
       603        259  /rev[0]/trailer[0]       trailer
      1614         12  /rev[0]/startxref[0]     startxref
      1628          5  /rev[0]/eof[0]           %%EOF
      1637   16878001  /rev[1]
      ...
```

(The heading line is shown here for explanation only; walk_pdf does not print it.)

- *offset* is the element's byte offset in the file, and *size* its length in bytes.
- *path* says which element it is (see Paths below).
- *preview* is the start of the element's first line. Bytes outside printable
  ASCII are shown as `.`. Revisions have no preview.

`walk_pdf <file.pdf> <path>` prints the same kind of lines, for the element at
`path` and everything inside it. Quote the path in the shell, as `[` and `]`
mean something to most shells:

```
walk_pdf book.pdf '/rev[1]/trailer[0]'
```

## Paths

A path names one element of the file, from the top down, like a file system
path. It is plain text: the same path names the same element every time
walk_pdf is run on the same file. `/` is the whole file.

A PDF file is its original part followed by any updates appended to it later.
Each of these is a *revision*, and runs up to and including its `%%EOF` line.
An update may store a new version of an object under the same object number,
so the revision is part of the path: `/rev[0]/obj[5]` and `/rev[1]/obj[5]`
are two versions of object 5.

| Path segment     | Element                                              |
|------------------|------------------------------------------------------|
| `rev[i]`         | Revision i of the file, counted from 0               |
| `header`         | `%PDF-x.y`, the version line at the start of the file |
| `comment[i]`     | A comment line, e.g. the line of binary marker bytes |
| `obj[n]`         | Indirect object number n: `n g obj ... endobj`       |
| `xref[i]`        | A classic cross-reference table                      |
| `trailer[i]`     | `trailer << ... >>`                                  |
| `startxref[i]`   | `startxref` and the offset of its cross-reference    |
| `eof[i]`         | `%%EOF`                                              |
| `unknown[i]`     | Bytes walk_pdf could not recognise                   |

An object is named by its object number alone: `12 0 obj` is `obj[12]`. The
second number in the file, the generation, is left out: each revision has at
most one object with a given number, so the revision and the number are enough.
(The generation is 0 for nearly every object; it only goes up when an update
reuses the number of a deleted object.) Path strings contain no blanks.

Elements other than objects are numbered in file order from 0 within their
revision, per kind.

Files made for fast web viewing ("linearized") have two revisions: a small
first part for the first page, then the rest.

PDF 1.5 and later may keep the cross-reference in a stream object
(`/Type /XRef`) instead of an `xref` table. That is an ordinary `obj[n]`, and
its revision has no `xref` or `trailer`.

## Options

| Option         | What it does                  |
|----------------|-------------------------------|
| `-h`, `--help` | Print this manual and exit.   |

Running walk_pdf without arguments prints its name and version.

## Exit codes

| Code | Meaning                                                   |
|------|-----------------------------------------------------------|
| 0    | Success.                                                  |
| 1    | The file could not be read, or has no element at the path. |
| 2    | Invalid command line arguments, or an invalid path.       |

## Building

walk_pdf is built with CMake, and its packages come from Conan 2. It needs C++23.

```sh
conan install . --build=missing -s compiler.cppstd=23 -s build_type=Debug
conan install . --build=missing -s compiler.cppstd=23 -s build_type=Release

cmake --preset conan-debug   && cmake --build --preset conan-debug
cmake --preset conan-release && cmake --build --preset conan-release
```

The programs end up in `build/Debug/walk_pdf` and `build/Release/walk_pdf`.
Run the unit tests with `ctest --preset conan-debug` (or `conan-release`).

Warnings are treated as errors. To build once with warnings allowed, configure
with `cmake --compile-no-warning-as-error ...`.

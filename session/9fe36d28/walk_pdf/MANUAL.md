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
    offset       size  path                     peek
         0      16575  /rev[0]                  19 elements: 13 objects, xref, trailer
         0          8  /rev[0]/header           version 1.3
         9         12  /rev[0]/comment[0]       <binary marker, 11 bytes>
        22        488  /rev[0]/obj[3]           << /Filter /FlateDecode... + stream <417 bytes, FlateDecode>
       511        113  /rev[0]/obj[1]           << /Type /Page /Parent 2 0 R /Resources 4 0 R /Contents 3...
       625        107  /rev[0]/obj[4]           << /ProcSet [ /PDF /Text ] /ColorSpace << /Cs1 5 0 R /Cs2...
       ...
     16137        290  /rev[0]/xref[0]          14 entries, objects 0-13
     16427        126  /rev[0]/trailer[0]       << /Size 14 /Root 10 0 R /Info 13 0 R /ID [ <d261359c6942...
     16554         15  /rev[0]/startxref[0]     16137
     16570          5  /rev[0]/eof[0]
```

(The heading line is shown here for explanation only; walk_pdf does not print it.)

- *offset* is the element's byte offset in the file, and *size* its length in bytes.
- *path* says which element it is (see Paths below).
- *peek* is a short look at what the element holds (see Peek below).

`walk_pdf <file.pdf> <path>` prints the same kind of lines, for the element at
`path` and everything inside it. Quote the path in the shell, as `[` and `]`
mean something to most shells:

```
walk_pdf book.pdf '/rev[1]/trailer[0]'
```

## Peek

The peek column shows what an element holds, without the keyword that opens it
(`12 0 obj`, `trailer`, ...), since the path already says what it is:

| Element    | Peek                                                              |
|------------|-------------------------------------------------------------------|
| revision   | How many elements it has, e.g. `19 elements: 13 objects, xref, trailer` |
| header     | The PDF version, e.g. `version 1.3`                               |
| comment    | Its text, or `<binary marker, n bytes>` for the line that tells tools the file holds binary data |
| object     | Its body, e.g. `<< /Type /Page ... >>`. If it holds a stream, its dictionary followed by `+ stream <n bytes, filters>`, where the filters (e.g. `FlateDecode`) say how the data is compressed or encoded |
| xref       | How many entries it has, and for which object numbers             |
| trailer    | Its dictionary                                                    |
| startxref  | The byte offset of the cross-reference                            |
| eof        | Nothing                                                           |
| unknown    | Its text, or `<n bytes, binary>`                                  |

Runs of whitespace are shown as one space, and other bytes outside printable
ASCII as `.`. Text that is too long is cut and ends with `...`. A peek is at
most 60 characters, except that a stream's description is always shown in
full, so an object with a stream can be a little wider.

Stream data is never shown, only described. Compressed streams, which hold
the text of most pages, can not be read yet. Nor can the strings and streams
of an encrypted PDF (one whose trailer has `/Encrypt`): they show as `.`.

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

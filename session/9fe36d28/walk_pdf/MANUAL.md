# walk_pdf

walk_pdf walks the structure of a PDF file and shows each part it visits as a
*path* (where in the file it is) and *content* (what is there). The main aim is
the text of the PDF, so that it can be turned into plain text or Markdown.

> Status: early. walk_pdf lists only the first layer of a PDF, its top-level
> elements. It does not yet look inside objects or extract text.

## Usage

```
walk_pdf <file.pdf>
walk_pdf [options]
```

`walk_pdf <file.pdf>` prints one line per top-level element of the file:

```
    offset       size  path                 preview
         0          8  /header              %PDF-1.7
        12        171  /obj[73867 0]        73867 0 obj
       187        416  /xref[0]             xref
       603        259  /trailer[0]          trailer
      1614         12  /startxref[0]        startxref
      1628          5  /eof[0]              %%EOF
```

(The heading line is shown here for explanation only; walk_pdf does not print it.)

- *offset* is the element's byte offset in the file, and *size* its length in bytes.
- *path* says which element it is. Objects are named by object number and
  generation, `obj[12 0]`. Elements that can repeat are numbered in file order
  from 0, `xref[1]`. A file with incremental updates has several `xref`,
  `trailer`, `startxref` and `%%EOF` elements.
- *preview* is the start of the element's first line. Bytes outside printable
  ASCII are shown as `.`.

The paths at this layer are:

| Path             | Element                                              |
|------------------|------------------------------------------------------|
| `/header`        | `%PDF-x.y`, the version line                         |
| `/comment[i]`    | A comment line, e.g. the line of binary marker bytes |
| `/obj[n g]`      | An indirect object, `n g obj ... endobj`             |
| `/xref[i]`       | A classic cross-reference table                      |
| `/trailer[i]`    | `trailer << ... >>`                                  |
| `/startxref[i]`  | `startxref` and the offset of its cross-reference    |
| `/eof[i]`        | `%%EOF`                                              |
| `/unknown[i]`    | Bytes walk_pdf could not recognise                   |

PDF 1.5 and later may keep the cross-reference in a stream object
(`/Type /XRef`) instead of an `xref` table. At this layer that is an ordinary
`/obj[n g]`, and the file has no `/xref` or `/trailer`.

## Options

| Option         | What it does                  |
|----------------|-------------------------------|
| `-h`, `--help` | Print this manual and exit.   |

Running walk_pdf without arguments prints its name and version.

## Exit codes

| Code | Meaning                         |
|------|---------------------------------|
| 0    | Success.                        |
| 1    | The file could not be read.     |
| 2    | Invalid command line arguments. |

## Building

walk_pdf is built with CMake, and its packages come from Conan 2. It needs C++23.

```sh
conan install . --build=missing -s compiler.cppstd=23 -s build_type=Debug
conan install . --build=missing -s compiler.cppstd=23 -s build_type=Release

cmake --preset conan-debug   && cmake --build --preset conan-debug
cmake --preset conan-release && cmake --build --preset conan-release
```

The programs end up in `build/Debug/walk_pdf` and `build/Release/walk_pdf`.

Warnings are treated as errors. To build once with warnings allowed, configure
with `cmake --compile-no-warning-as-error ...`.

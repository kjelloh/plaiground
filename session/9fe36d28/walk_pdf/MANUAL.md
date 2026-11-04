# walk_pdf

walk_pdf walks the structure of a PDF file and shows each part it visits as a
*path* (where in the file it is) and *content* (what is there). The main aim is
the text of the PDF, so that it can be turned into plain text or Markdown.

> Status: early. walk_pdf finds the file's revisions, their top-level
> elements, and what is inside objects and trailers: dictionary entries, array
> items and stream data. It decrypts files that open without a password,
> decompresses streams, and splits page content into text blocks and their
> operators. It does not yet turn the text operators into readable text.

## Usage

```
walk_pdf <file.pdf> [<path>] [--depth N]
walk_pdf [options]
```

walk_pdf lists the element at `path` and what is inside it, `N` levels down,
depth first: each element is followed by its children before its next
sibling. Without a path it starts at `/`, the whole file. Without `--depth` it
goes 3 levels down.

`walk_pdf <file.pdf>` prints one line per element:

```
    offset       size  path                             peek
         0     489247  /                                1 revision
         0     489246  /rev[0]                          53 elements: 48 objects, xref, trailer
         0          8  /rev[0]/header                   version 1.4
         9        167  /rev[0]/obj[1]                   << /Title "Invoice" /Creator "wkhtmltopdf 0.12.4" /Produc...
        27         18  /rev[0]/obj[1]/Title             "Invoice"
        55         40  /rev[0]/obj[1]/Creator           "wkhtmltopdf 0.12.4"
       ...
     55726        120  /rev[0]/obj[5]                   << /Type /Page /Parent 2 0 R /Contents 35 0 R /Resources ...
     55743          5  /rev[0]/obj[5]/Type              /Page
     55757          5  /rev[0]/obj[5]/Parent            2 0 R
       ...
     56069     423950  /rev[0]/obj[35]                  << /Length 36 0 R /F... + stream <423875 bytes, FlateDecode>
     56089          6  /rev[0]/obj[35]/Length           36 0 R
     56104         12  /rev[0]/obj[35]/Filter           /FlateDecode
     56127     423875  /rev[0]/obj[35]/stream           <423875 bytes>
```

(The heading line is shown here for explanation only; walk_pdf does not print it.)

- *offset* is the element's byte offset in the file, and *size* its length in bytes.
  Inside a decoded stream, which is not in the file, the offset is in the
  decoded data and starts with `+`, e.g. `+0`.
- *path* says which element it is (see Paths below).
- *peek* is a short look at what the element holds (see Peek below).

`walk_pdf <file.pdf> <path>` starts at the element at `path`. Quote the path in
the shell, as `[` and `]` mean something to most shells:

```
walk_pdf book.pdf '/rev[1]/trailer[0]'
walk_pdf book.pdf '/rev[1]' --depth 0     # only the revision itself
```

## Peek

The peek column shows what an element holds, without the keyword that opens it
(`12 0 obj`, `trailer`, ...), since the path already says what it is:

| Element    | Peek                                                              |
|------------|-------------------------------------------------------------------|
| `/`        | How many revisions the file has, and if it is encrypted, how and which permissions it does not grant (see Encrypted files below) |
| revision   | How many elements it has, e.g. `19 elements: 13 objects, xref, trailer` |
| header     | The PDF version, e.g. `version 1.3`                               |
| comment    | Its text, or `<binary marker, n bytes>` for the line that tells tools the file holds binary data |
| object     | Its body, e.g. `<< /Type /Page ... >>`. If it holds a stream, its dictionary followed by `+ stream <n bytes, filters>`, where the filters (e.g. `FlateDecode`) say how the data is compressed or encoded |
| xref       | How many entries it has, and for which object numbers             |
| trailer    | Its dictionary                                                    |
| startxref  | The byte offset of the cross-reference                            |
| eof        | Nothing                                                           |
| unknown    | Its text, or `<n bytes, binary>`                                  |
| a value inside an object: dictionary, array, name, number, reference (`12 0 R`), boolean, null | Its text |
| string     | Text strings marked as UTF-16 or UTF-8 are shown decoded, in quotes: `"Invoice"`. Other strings as they are in the file: `(D:20261012)` |
| stream     | The stream's data if it is text (an uncompressed page, say), otherwise `<n bytes, filters>`, e.g. `<423875 bytes, FlateDecode>` |
| text block | The block, e.g. `BT /F8 16 Tf 1 0 0 -1 0 0 Tm 0 -775 Td <0001> Tj ET` |
| operation  | Its operands and operator, e.g. `<0001> Tj`, or `<inline image, n bytes>` |
| decoded    | The stream's decoded data if it is text (e.g. a page's drawing instructions), otherwise `<n bytes>`. If decoding stopped early, why, first: `(FlateDecode: data ends early, ...)` |

Runs of whitespace are shown as one space, and other bytes outside printable
ASCII as `.`. Text that is too long is cut and ends with `...`. A peek is at
most 60 characters, except that a stream's description is always shown in
full, so an object with a stream can be a little wider.

Dictionaries and arrays are shown rebuilt from their entries and items, so that
the strings in them are decoded too: `<< /Title "Invoice" /Creator "wkhtmltopdf 0.12.4" ... >>`.

A PDF text string is decoded when it starts with a mark saying how it is
encoded: `FE FF` for UTF-16 (big-endian) and `EF BB BF` for UTF-8. Bytes that
do not decode are shown as `�` (U+FFFD). Strings without a mark use
PDFDocEncoding, which is not decoded yet.

A stream's data in the file is usually compressed, and in an encrypted file
also encrypted. Its `decoded` child is the data decrypted and with its filters
applied: `FlateDecode` (zlib, with PNG and TIFF predictors), `ASCIIHexDecode`
and `ASCII85Decode`. Streams with other filters, such as images (`DCTDecode`),
have no `decoded` child.

## Encrypted files

A PDF can be encrypted (its trailer has `/Encrypt`). Most such files open
without a password: the key is computed from values in the file itself, which
is how a browser shows them. walk_pdf does the same, for the Standard security
handler with RC4 (40 to 128 bits). It then shows strings and stream data
decrypted in the peek column. Strings that are not marked as UTF-16 or UTF-8
are shown decrypted in parentheses: `(Bilagor)`.

The `/` row says how the file is encrypted, and which permissions its owner
does not grant, e.g.

```
2 revisions; encrypted: RC4 128-bit, no password needed; not permitted: modify, copy/extract, annotate, extract for accessibility
```

The peek is a look at the structure, cut to one line per element. It is not a
way to extract a file's text: walk_pdf will honour "copy/extract" when it gets
a text output.

Not decrypted: files that need a password, and files encrypted with AES
(which shows as `AES-128: not supported yet` or `AES-256: not supported yet`).
Their strings and streams are shown as they are in the file.

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
| *key*            | The value of a dictionary entry, e.g. `Type`, `Resources` |
| `[i]`            | Item i of an array, counted from 0                   |
| `stream`         | The data of a stream object, as it is in the file    |
| `decoded`        | A stream's data decrypted and decompressed (not in the file) |
| `BT[i]`          | Text block i (`BT ... ET`) of a content stream, counted from 0 |
| `op[i]`          | Operation i of a text block: operands and an operator, e.g. `/F8 16 Tf` |

What is inside what:

- A revision holds its header, comments, objects, xref, trailer, startxref and eof.
- An object holding a dictionary has the dictionary's entries as children,
  followed by `stream` if it is a stream object. An object holding an array
  has its items. A trailer has its dictionary's entries.
- A dictionary value has its entries, and an array value its items. Other
  values have no children.
- A stream has `decoded` as its child, when walk_pdf can apply its filters.
- A decoded *content stream*, the drawing instructions of a page or form, has
  its text blocks `BT[i]` as children. A text block has its operations `op[i]`
  (without its `BT` and `ET`), and an operation has its operands `[i]`. Drawing
  instructions outside text blocks are not listed.

walk_pdf does not yet follow a page to its `/Contents`, so it tells content
streams from other streams by their dictionary and data: a stream is taken
as a content stream unless its `/Type` or `/Subtype` says otherwise (an image,
font, object stream and so on), it is a font program (`/Length1`), its data
is a CMap (`begincmap`, e.g. a font's `/ToUnicode`), or its data is binary.

For example, `/rev[0]/obj[35]/stream/decoded/BT[0]/op[3]` is the fourth
operation of the first text block of the page drawn by object 35, and
`.../op[3]/[0]` its first operand.

For example, `/rev[0]/obj[5]/Resources/Font/F8` is the `/F8` entry of the
`/Font` dictionary of the `/Resources` dictionary of object 5, and
`/rev[0]/obj[5]/MediaBox/[2]` is the third number of its `/MediaBox` array.

A key is written as it is, except that a byte which would clash with the path
syntax (whitespace, `/`, `[`, `]` and other PDF delimiters, `#`, or a byte
outside printable ASCII) is written `#` and two hex digits, as in PDF names:
the key `A B` is `A#20B`. Keys that are spelled like the reserved segments
`header`, `stream` and `decoded` get their first letter written that way: `#68eader`.
The empty key is `#`.

An object is named by its object number alone: `12 0 obj` is `obj[12]`. The
second number in the file, the generation, is left out: each revision has at
most one object with a given number, so the revision and the number are enough.
(The generation is 0 for nearly every object; it only goes up when an update
reuses the number of a deleted object.) Path strings contain no blanks.

Elements of a revision other than objects are numbered in file order from 0
within their revision, per kind.

Files made for fast web viewing ("linearized") have two revisions: a small
first part for the first page, then the rest.

PDF 1.5 and later may keep the cross-reference in a stream object
(`/Type /XRef`) instead of an `xref` table. That is an ordinary `obj[n]`, and
its revision has no `xref` or `trailer`.

## Options

| Option         | What it does                                                    |
|----------------|-----------------------------------------------------------------|
| `--depth N`    | How many levels below the path to list. 0 lists only the element at the path, 1 adds its children, and so on. Default 3. |
| `-h`, `--help` | Print this manual and exit.                                     |

With the default depth of 3, walk_pdf lists revisions, their elements, and the
entries and items of objects and trailers, but not what is nested deeper.
`--depth 4` also lists each stream's decoded data, and `--depth 7` everything
down to the operands of text operators. To look into one page's text, start at
its decoded stream: `walk_pdf file.pdf '/rev[0]/obj[35]/stream/decoded' --depth 2`. Give
a path and a depth to look further into one part, e.g.
`walk_pdf file.pdf '/rev[0]/obj[5]' --depth 3`.

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

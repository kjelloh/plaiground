# walk_pdf progress

Progress of the `walk_pdf` C++ console app in [walk_pdf/](walk_pdf/).
See [session.md](session.md) for what the session is about, and
[walk_pdf/MANUAL.md](walk_pdf/MANUAL.md) for how the app is used.

## Current state (2026-11-04)

- `walk_pdf <file.pdf> [<path>] [--depth N]` walks a PDF depth first and prints one
  line per element: offset, size, path and a peek into its content. Default depth 3.
- The walker understands: revisions, the top-level elements of a revision
  (header, comments, objects, xref, trailer, startxref, eof), and what is inside
  objects and trailers (dictionary entries, array items, stream data).
- 54 unit tests pass in Debug and Release (`ctest --preset conan-debug`).
- Tested on the three PDFs in `_Resources/` (two e-books, one voucher).

## Design decisions

| Decision | Why |
|---|---|
| CMake + Conan 2, C++23, warnings are errors (`CMAKE_COMPILE_WARNING_AS_ERROR`) | Cross platform, as asked. Same setup as the cratchit sessions |
| Debug and Release builds via `conan install -s build_type=...` | Asked for both |
| `MANUAL.md` built into the app, printed by `--help` | Same mechanism as cratchit |
| Our own PDF walker, no PDF library | To see what we can do ourselves |
| No coroutines | Apple clang 16 has coroutines but not `std::generator`; decided to skip |
| A path is a pure value (segments), not tied to memory | Paths can be printed, parsed back, compared and kept |
| Path string has no blanks, e.g. `/rev[0]/obj[5]/Resources/Font/F8` | Asked for |
| Revision segment `rev[i]` | An object number can occur once per revision; the revision makes every path unique |
| Objects are `obj[n]`, without the generation | Unique within a revision, so the generation is redundant |
| Keys bare, array items `[i]`, stream data `stream`; awkward key bytes as `#xx` | One string form per path; reserved words `header`/`stream` escaped |
| Document API acts on paths: `find(path)` and `children(path)` | The client (main) does its own recursive DFS with a depth counter |
| Internal path index, a bounded cache (65,536 entries) | Without it a path-only walk is quadratic; unbounded it used 1.4 GB |
| Peek column: content without its opening keyword; binary data described | The old preview only repeated what the path said |

## Done

1. Build scaffolding: CMake, Conan, Debug/Release, `.gitignore`, `MANUAL.md` with `--help`.
2. First layer: revision-less listing of top-level elements (callback walker).
3. Redesign: pure-value `Path`, `Document` with `find(path)`, iterable `Content`, revisions.
4. Fixed a quadratic keyword search (22 s → 0.12 s on the 17 MB e-book).
5. Object segments simplified from `obj[n g]` via `obj[g.n]` to `obj[n]`.
6. Peek column with descriptions of binary data and streams.
7. Path-only API (`find`, `children`) with an internal index; client-side DFS in main with `--depth`, default 3.
8. Layer 3: tokenizer (`src/syntax.*`) for PDF values; dictionary entries, array items and stream data as children.
9. Bounded index (1.4 GB → 177 MB on the 54 MB e-book), wider path column.

## Findings about the test PDFs

- Both e-books are linearized (two revisions) and encrypted: Standard security
  handler, 128-bit RC4 (`/V 2 /R 3`). Their strings and streams are encrypted.
- The voucher's page text is in a `FlateDecode` stream (`/rev[0]/obj[35]/stream`),
  as text operators (`BT ... Tj ... ET`) showing glyph ids (`<0001>`), which need the
  font's `/ToUnicode` map to become characters.
- Some PDFs keep objects inside object streams (`/Type /ObjStm`); those objects are
  not visible until object streams are unpacked.
- Strings may be UTF-16 (the voucher's `/Title`), shown today as `(.. I n v o i c e)`.

## Next

1. Decompress `FlateDecode` stream data (zlib from Conan), so `.../stream` gets children.
2. Tokenize content streams: text blocks (`BT ... ET`) and text operators (`Tj`, `TJ`, ...).
3. Follow references (`12 0 R`) via the cross-reference table; logical walk of pages,
   with a visited set against cycles.
4. Map glyphs to characters through fonts' `/ToUnicode` maps.
5. Decrypt the e-books (RC4, Standard security handler).
6. Unpack object streams (`/Type /ObjStm`).
7. Decode UTF-16 strings in the peek.

## Open questions

- Default `--depth` once streams have children: 3 lists all entries of every
  object, which is already 7.4 million lines for the 54 MB e-book.

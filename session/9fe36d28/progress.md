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
- 116 unit tests pass in Debug and Release (`ctest --preset conan-debug`).
- A `charset` library for text transcoding (Unicode, UTF-8, UTF-16, CP437).
- PDF text strings marked as UTF-16BE or UTF-8 are shown decoded in the peek,
  e.g. `/Title "Invoice"`.
- Encrypted files that open without a password (Standard security handler, RC4)
  are decrypted for the peek; the `/` row shows the encryption and the denied
  permissions. Both e-books now show readable outline titles.
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
| Text transcoding in a separate `charset` library (namespace `charset::`), seeded from cratchit | Reuse the user's own transcoding code; keep it apart from the PDF code |
| Every `push` in `charset` gives at most one code point (`std::optional`); a broken sequence and the unit that broke it give one U+FFFD | One API and one failure rule across UTF-8 and UTF-16. Cost: a character right after a lone UTF-16 high surrogate is lost. No surrogate pairs or broken UTF-16 in the test PDFs. If lossless is needed later: hold the extra code point and return it on the next push |
| Decoded strings shown in quotes `"..."`, undecoded ones as in the file `(...)` | You can tell which strings were decoded |
| Decrypt with the empty user password (Standard handler, RC4); own MD5 and RC4 in a `crypto` library | What viewers do; self-contained, no OpenSSL needed. AES not yet |
| The peek shows decrypted content; a future text output honours the "copy/extract" permission (`/P` bit 5) | The user's decision (2026-11-04): peeking for structure inspection is not extraction. Agreed that actual text extraction respects the publisher's flag |

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
10. Brought in the `charset` seed from `session/60cae9fe/cratchit/src/core/charset/`
    (unicode, utf8, cp437, detect) and its tests, as `walk_pdf/src/charset/`, a CMake
    library target `charset` linked by `walk_pdf_core`. Only include paths changed.
11. UTF-16 in `charset::utf16` (big- and little-endian, surrogate pairs, U+FFFD for
    broken input; at most one code point per push, as in utf8). PDF strings decoded (`src/text_string.*`: literal escapes, hex
    strings, UTF-16BE/UTF-8 text marks). The peek rebuilds dictionaries and arrays
    from their tokens, shows decoded strings in quotes, and cuts UTF-8 between characters.
12. UTF-16 `push` changed to give at most one code point, as utf8 (a broken sequence and
    the unit that broke it give one U+FFFD).
13. Decryption: `crypto` library (MD5, RC4; tested against RFC 1321 and known RC4 vectors),
    `src/security.*` (Algorithms 1, 2, 4, 5 of PDF 32000-1), `Document::security()` and
    `Document::decrypt(path, bytes)`. The peek decrypts strings and stream starts, and the
    `/` row shows the encryption and denied permissions. Tested with small encrypted PDFs
    whose expected values were computed independently (Python hashlib).

## Findings about the test PDFs

- Both e-books are linearized (two revisions) and encrypted: Standard security
  handler, 128-bit RC4 (`/V 2 /R 3`), empty user password (they open without one).
  Permissions `/P -572`: printing allowed; modify, copy/extract, annotate and
  extract for accessibility not permitted. Decrypted, their outline titles read
  e.g. "Innehållsförteckning", "Bilaga 1 - Exempel på nollkoder i klass 0".
- The voucher's page text is in a `FlateDecode` stream (`/rev[0]/obj[35]/stream`),
  as text operators (`BT ... Tj ... ET`) showing glyph ids (`<0001>`), which need the
  font's `/ToUnicode` map to become characters.
- Some PDFs keep objects inside object streams (`/Type /ObjStm`); those objects are
  not visible until object streams are unpacked.
- Strings may be UTF-16 (the voucher's `/Title`, outline titles such as
  `"249,00 SEK"`); these are now decoded. None of them has a character above
  U+FFFF (no surrogate pairs).
- The e-book string that decoded as garbage before decryption
  (`/rev[1]/obj[71129]/A/[0]/Headers/[0]`) decrypts to `(node00004768)`.

## Next

1. Decompress `FlateDecode` stream data (zlib from Conan), so `.../stream` gets children.
2. Tokenize content streams: text blocks (`BT ... ET`) and text operators (`Tj`, `TJ`, ...).
3. Follow references (`12 0 R`) via the cross-reference table; logical walk of pages,
   with a visited set against cycles.
4. Map glyphs to characters through fonts' `/ToUnicode` maps.
5. AES decryption (`/V 4` with `/AESV2`, `/V 5`), if a test PDF needs it.
6. Unpack object streams (`/Type /ObjStm`).
7. Decode PDFDocEncoding text strings (those without a `FE FF`/`EF BB BF` mark),
   a new decoder in `charset`.

## Open questions

- Default `--depth` once streams have children: 3 lists all entries of every
  object, which is already 7.4 million lines for the 54 MB e-book.

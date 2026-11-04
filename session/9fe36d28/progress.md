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
- 147 unit tests pass in Debug and Release (`ctest --preset conan-debug`).
- A `charset` library for text transcoding (Unicode, UTF-8, UTF-16, CP437).
- PDF text strings marked as UTF-16BE or UTF-8 are shown decoded in the peek,
  e.g. `/Title "Invoice"`.
- Encrypted files that open without a password (Standard security handler, RC4)
  are decrypted for the peek; the `/` row shows the encryption and the denied
  permissions. Both e-books now show readable outline titles.
- Streams are decompressed: `.../stream/decoded` is the data decrypted and with
  its filters applied (FlateDecode with predictors, ASCIIHex, ASCII85). Pages'
  drawing instructions, with their text operators (`BT ... Tj ... ET`), are visible.
- Content streams are split into text blocks `BT[i]`, their operations `op[i]` and
  operands `[i]`, e.g. `/rev[0]/obj[35]/stream/decoded/BT[0]/op[3]` = `<0001> Tj`.
- References are resolved: `37 0 R -> /rev[0]/obj[37]`; a `Tf` leads to its font
  through the page's (inherited) or form's resources: `/F8 16 Tf -> /rev[0]/obj[8]`.
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
| `charset` stages are named `Decoder` (encoded bytes → one code point per push) and `Encoder` (one code point → its encoded bytes), not `…Buffer` | The library provides byte-by-byte pipe stages only, never Unicode buffers; the names should not suggest otherwise |
| Decrypt with the empty user password (Standard handler, RC4); own MD5 and RC4 in a `crypto` library | What viewers do; self-contained, no OpenSSL needed. AES not yet |
| zlib from Conan (`zlib/1.3.1`) for FlateDecode; own code for predictors, ASCIIHex, ASCII85 | zlib is the standard; the rest is small |
| Segment `decoded` under `stream`; `Content` can view a decoded stream's bytes (shared, from a cache bounded to 64 MB) instead of the file's; offsets inside shown as `+n` | Decoded bytes are not in the file. Paths stay pure values; `stream` stays the raw file bytes |
| Content stream children: text blocks `BT[i]` → operations `op[i]` → operands `[i]`; drawing outside text blocks not listed | The goal is text; keeps listings manageable |
| Content streams told apart by dictionary and data (not image/font/CMap/object stream, not binary) | References (a page's `/Contents`) are not followed yet; to be replaced when they are |
| References resolved to paths (pure values), not followed as tree children | The file-layout tree stays a tree; references can form cycles (/Parent, /Kids) |
| Object table (revision, number) -> byte range, built when the file is opened | Resolving 100,000+ references and looking up objects without rescanning revisions |
| A reference leads to the latest revision that has the object; generation not checked | Like a reader using the newest cross-reference section |
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
14. Renamed `ToUnicodeBuffer` → `Decoder` and `FromUnicodeBuffer` → `Encoder` in
    `charset::utf8`, `utf16` and `cp437`. Audit: the library holds no Unicode buffers
    (only one character's pending encoded bytes per stage, and CP437's constant table).
    Kept for now: `utf8::Decoder::failed_bytes()` (a growing vector of failed input
    bytes) and `detect::is_utf8(span)` (a whole-buffer function), both from cratchit.
15. Stream decoding: zlib from Conan, `src/filters.*` (FlateDecode with PNG/TIFF
    predictors, ASCIIHexDecode, ASCII85Decode), `Document::filters(path)`,
    `Document::decode(path)` (decrypt, then filters; cached), segment and kind `decoded`,
    buffer-backed `Content` (`in_file()`, `+offset` in the listing). Zero bytes count as
    binary in the peek. 16 new tests (filters against zlib round trips, hand-checked
    predictor rows, Python-made ASCII85 vectors; decrypt-then-decode).
16. Content streams: `next_operation` tokenizer (operands + operator, inline images as one
    operation), kinds/segments `BT[i]` and `op[i]`, operands as `[i]`,
    `Document::is_content_stream`. 5 new tests.
17. References: object table made when the file is opened, `Document::object(n)`,
    `resolve(path)`, `follow(path)`, `resources(stream)` (page /Resources, inherited
    through /Parent with a cycle guard; a form's own) and `font(path, name)`. Peek shows
    where references and `Tf` lead. 10 new tests (`test/test_references.cpp`).

## Findings about the test PDFs

- Both e-books are linearized (two revisions) and encrypted: Standard security
  handler, 128-bit RC4 (`/V 2 /R 3`), empty user password (they open without one).
  Permissions `/P -572`: printing allowed; modify, copy/extract, annotate and
  extract for accessibility not permitted. Decrypted, their outline titles read
  e.g. "Innehållsförteckning", "Bilaga 1 - Exempel på nollkoder i klass 0".
- The voucher's page text is in a `FlateDecode` stream (`/rev[0]/obj[35]/stream`),
  as text operators (`BT ... Tj ... ET`) showing glyph ids (`<0001>`), which need the
  font's `/ToUnicode` map to become characters.
- All streams in the three test PDFs decode without problems (voucher: 6 of 8,
  the other 2 are JPEG images; e-books: 4183 of 4201 and 603 of 608, the rest images).
- The voucher's fonts have `/ToUnicode` maps (uncompressed, `obj[42]` and `obj[47]`:
  `/CIDInit ... beginbfchar`), needed to turn its glyph ids into characters.
- Text in the test PDFs (at `--depth 6`): voucher 2 text blocks, 80 `Tj` with 2-byte
  glyph ids (`<0001>`); e-book "bokforing" 34,166 text blocks, hex glyph ids
  (needs `/ToUnicode`); e-book "bokslutsb" 31,232 text blocks with single-byte
  literal strings, nearly readable already (`(Civilr\344ttslig bed...)`, `\344` = ä in
  the font's encoding).
- All `Tf` in the test PDFs lead to a font (voucher 2 fonts, "bokforing" 8,
  "bokslutsb" 36), and all 289,000+ references resolve. The voucher's text font
  is `/rev[0]/obj[8]`: Type0, DejaVuSans, `/Encoding /Identity-H` (2-byte codes),
  `/ToUnicode -> /rev[0]/obj[47]`.
- A full `--depth 6` walk of "bokslutsb" went from 12.9 s to 19.9 s, mostly from
  finding every page's resources once; fine for now.
- Some PDFs keep objects inside object streams (`/Type /ObjStm`); those objects are
  not visible until object streams are unpacked.
- Strings may be UTF-16 (the voucher's `/Title`, outline titles such as
  `"249,00 SEK"`); these are now decoded. None of them has a character above
  U+FFFF (no surrogate pairs).
- The e-book string that decoded as garbage before decryption
  (`/rev[1]/obj[71129]/A/[0]/Headers/[0]`) decrypts to `(node00004768)`.

## Next

1. Logical walk of pages (catalog -> page tree -> pages in reading order),
   with a visited set against cycles.
2. Map glyphs to characters: fonts' `/ToUnicode` maps (voucher, e-book "bokforing"),
   and single-byte encodings such as WinAnsi/MacRoman with `/Differences` (e-book "bokslutsb").
3. AES decryption (`/V 4` with `/AESV2`, `/V 5`), if a test PDF needs it.
4. Unpack object streams (`/Type /ObjStm`).
5. Decode PDFDocEncoding text strings (those without a `FE FF`/`EF BB BF` mark),
   a new decoder in `charset`.

## Open questions

- Default `--depth` once streams have children: 3 lists all entries of every
  object, which is already 7.4 million lines for the 54 MB e-book.

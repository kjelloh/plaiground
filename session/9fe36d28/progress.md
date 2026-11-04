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
- 186 unit tests pass in Debug and Release (`ctest --preset conan-debug`).
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
- **Text is readable**: text blocks and text-showing operators show their text
  mapped through their fonts (/ToUnicode, or WinAnsi/MacRoman/Standard with
  /Differences), e.g. `"Inköp av råvaror och material i Sverige,"`.
- Display order: `/pages/page[i]/op[j]` walks pages in page tree order, all
  their drawing operations in order, and forms drawn by `Do`.
- `walk_pdf --text <file>`: the document's text as plain text, laid out by where
  it is drawn (lines by baseline, words joined by glyph widths). Refused when the
  permissions do not allow copying (the e-books).
- Tested on the PDFs in `_Resources/`: two e-books and (since 2026-11-04) 30 vouchers.

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
| Codes -> Unicode: /ToUnicode first, then a simple font's /Encoding; Type0 codes by Identity-H/V (2 bytes) or an embedded CMap's code space | The order the PDF standard recommends for extracting text |
| WinAnsi, MacRoman and Adobe Standard as `charset` decoders; tables made from Python's cp1252 and mac_roman codecs | They are single-byte encodings, like cp437; the codecs are an independent source |
| Glyph names: a generated subset (ASCII, Latin-1, Windows-1252, Mac OS Roman, `uniXXXX`/`uXXXX`) | No Adobe Glyph List available offline; enough for Latin text; none of the test fonts needs more |
| Text block text: strings in content-stream order; new lines and TJ gaps of -250 or more become spaces | Simple and good on the test files; text placement (reading order) comes later |
| Display order under `/pages` (not listed under `/`); `page[i]` -> `op[j]` across all content streams; a form's operations as children of its `Do` | A second, logical tree beside the file layout; keeps the default listing small |
| The font in effect in display order is tracked as the resolved font object, carried into forms | Tf selects a font object by name in the resources of the moment; a form may not have that name |
| Plain text document = `PlainText { pages of lines }`, strings only; all knowledge in the builder (interpreter + layout) | The user's design: a simple document type, a visitor that knows where text goes. Markdown later gets its own builder and document |
| Layout: lines by baseline (half a font size), space when a gap > 1/5 of the size, empty line when > 1.5 lines, direction rounded to quarter turns | Simple rules, good on the vouchers; columns side by side are a known limit |
| /ActualText of marked content replaces the glyphs' text | A voucher draws a hyphen with a glyph that maps to a space, and says "-" in /ActualText |
| `plain_text` checks the copy/extract permission (bit 5) | As agreed: text extraction honours the publisher's flag |
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
18. Codes to text: `CMap` (codespace ranges, bfchar, bfrange in both forms; targets
    UTF-16BE decoded through the `charset` pipe), `charset::winansi`, `macroman`,
    `adobe_standard` decoders, glyph names (`src/glyph_names.*`), `Font` (code splitting
    and mapping), `Document::load_font`, `font_name_at` (last Tf, with q/Q), and
    `Document::text(path)` for operations and text blocks. Peek shows the text.
    15 new tests.
19. Display order: `Kind::pages`/`page`, `Document::pages()` (page tree, cycle guard),
    `trailer_entry(key)`, `inherited(node, key)`, operations of a page's content streams
    and of forms drawn by `Do` (8 levels at most), `display_resources`, `display_font`.
    Index entries can keep their decoded buffer. 8 new tests (`test/test_display.cpp`).
20. Plain text: `Font::glyphs` with widths (/Widths, /FirstChar, /MissingWidth; Type0
    /W, /DW; Type3 /FontMatrix), `src/plain_text.*` (interpreter of graphics and text
    state, forms with /Matrix, /ActualText; line layout; `PlainText`, permission check),
    `walk_pdf --text`. 16 new tests (`test/test_plain_text.cpp`).

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
- Text through fonts: both e-books' text blocks (34,166 and 31,232) map with no
  U+FFFD, e.g. "Sålda handelsvaror positiv VMB 25 %", "(4 kap. 1 § första stycket".
- The voucher has only two text blocks: a lone space, and the OCR payment line in
  font /OCRB, "H # 62600012236149 # 311 25 8 > 53432795#41#" (reference, amount,
  bankgiro). Its other visible text is drawn as shapes (no characters); only the
  outline titles ("249,00 SEK", "5343-2795") carry it as text.
- Display order: "bokforing" 830 pages, 253,503 text operations; "bokslutsb" 510
  pages, 34,879 (27,679 operations inside forms). "bokforing" splits words over
  several Tj ("BAS-intr" "essent" "er"): joining them needs positions.
- Vouchers added to `_Resources/_vouchers` (30 in all): `4816_13`, `20578563254`
  and the `Kvittens_*` files keep their pages in object streams (no pages found
  yet). `FAKTURA_903006266022` has a Type0/Identity-H font without /ToUnicode
  (/AllAndNone2): its glyph ids can not be mapped without the embedded font
  program's own tables (42 operations show U+FFFD).
- `--text` on the 30 vouchers (1 s in all): 23 give their text, e.g. the Fortnox and
  Anthropic invoices read line by line with whole words and Swedish letters; 7 give
  none (object streams). Columns side by side are joined on one line
  ("Anthropic, PBC Bill to").
- Some PDFs keep objects inside object streams (`/Type /ObjStm`); those objects are
  not visible until object streams are unpacked.
- Strings may be UTF-16 (the voucher's `/Title`, outline titles such as
  `"249,00 SEK"`); these are now decoded. None of them has a character above
  U+FFFF (no surrogate pairs).
- The e-book string that decoded as garbage before decryption
  (`/rev[1]/obj[71129]/A/[0]/Headers/[0]`) decrypts to `(node00004768)`.

## Next

1. Unpack object streams (`/Type /ObjStm`): 7 of the 30 vouchers keep their
   pages in them and give no text yet.
2. Columns: split a page's lines into columns before joining them.
3. Markdown: a second builder over the same interpreter, keeping font sizes and
   styles and marked content tags (headings, paragraphs, lists).
4. AES decryption (`/V 4` with `/AESV2`, `/V 5`), if a test PDF needs it.
5. Decode PDFDocEncoding text strings (those without a `FE FF`/`EF BB BF` mark),
   a new decoder in `charset`.

## Open questions

- Default `--depth` once streams have children: 3 lists all entries of every
  object, which is already 7.4 million lines for the 54 MB e-book.

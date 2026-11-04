# walk_pdf API

The text of a PDF, as plain text, for programs that process it further, such
as a bookkeeping application reading vouchers (invoices, receipts).

> Status: the API is defined and used by walk_pdf's own `--text`. How another
> program builds walk_pdf's sources into its own is not settled yet; for now,
> add the files of `src/` (and the zlib package) to its build.

## The call

```cpp
#include "walk_pdf.hpp"

auto const result = walk_pdf::extract_text(std::filesystem::path{"Faktura48293618.pdf"});
if (!result) {
  // result.error().kind says why, result.error().message says it in words
  return;
}
for (auto const& page : result->text.pages) {
  for (auto const& line : page) {
    // e.g. "Fakturadatum: 2025-05-08", "Belopp att betala ... 615,00"
  }
}
if (result->report.unmapped_glyphs > 0) {
  // some characters could not be read: check the text before trusting it
}
```

There are two ways in:

```cpp
// A PDF file
std::expected<ExtractedText, TextError> extract_text(std::filesystem::path const& file);

// A PDF held in memory, e.g. an e-mail attachment
std::expected<ExtractedText, TextError> extract_text(std::string pdf_bytes);
```

## What comes back

```cpp
struct ExtractedText {
  PlainText text;     // the text
  TextReport report;  // what to know before trusting it
};

struct PlainText {
  std::vector<std::vector<std::string>> pages;  // each page's lines, top to bottom
  std::string to_string() const;                // all of it, each page after "--- page n ---"
};

struct TextReport {
  std::size_t pages = 0;            // pages found
  std::size_t unmapped_glyphs = 0;  // characters that could not be read (shown as U+FFFD)
  bool encrypted = false;           // the file was encrypted, and walk_pdf decrypted it
};
```

- All text is **UTF-8**.
- A line is the text along one baseline, left to right, with a space where
  there is a gap. Lines are top to bottom; an empty line marks a wider gap.
  Lines have no spaces at their ends.
- `report.pages == 0` usually means the file keeps its pages in object streams,
  which walk_pdf does not read yet: there is no text, though it is a PDF.

## When there is no text

```cpp
struct TextError {
  enum class Kind {
    cannot_read,             // the file could not be read
    not_a_pdf,               // no "%PDF-" header in the first 1024 bytes
    needs_password,          // encrypted, and opening it needs a password
    unsupported_encryption,  // encrypted with a method walk_pdf can not decrypt (AES)
    not_permitted,           // its permissions do not allow copying or extracting its text
  };
  Kind kind;
  std::string message;       // for people, in English
};
```

`not_permitted` is about the file, not about walk_pdf: its owner has marked it
"no copying or extracting" (the `/P` entry of an encrypted file). walk_pdf
honours that.

## Guarantees

- **No output, no files.** The API writes nothing to the console and creates
  no files. (The app's `--text` prints what it returns.)
- **No exceptions** of its own; only `std::bad_alloc` if memory runs out.
- **Each call is independent.** It reads the PDF, extracts the text and keeps
  nothing. Calls may run at the same time in different threads.
- **The PDF is only read.** A damaged file gives what walk_pdf could make of it,
  not a crash.

## How the text is found

For each page, in the order the page tree gives:

1. The page's drawing instructions are followed in drawing order, including
   the forms they draw, to find each piece of text, where it is drawn, and in
   which font.
2. Each piece of text is turned into characters through its font: the font's
   `/ToUnicode` map, or its encoding (WinAnsi, MacRoman, Standard, with
   `/Differences`). Marked content with `/ActualText` gives that text instead.
3. The pieces are laid out in lines by where they are on the page, and joined
   with spaces where there are gaps.

Encrypted files that open without a password (RC4) are decrypted. Compressed
streams (FlateDecode, ASCIIHex, ASCII85) are decompressed.

## Limits

- **Columns side by side** come out joined on the same lines: a left and a
  right address block become one line each, the left text then the right.
- **Pages in object streams** are not found yet (`report.pages == 0`).
- **Fonts without a map** to Unicode give U+FFFD for their characters, counted
  in `report.unmapped_glyphs`.
- **Text drawn as shapes**, as some PDF makers do with all text, has no
  characters and gives no text. The outline (bookmarks) of such a file may
  still name key figures, but the API does not read outlines yet.
- **AES-encrypted** files and files that need a password give an error.

## Example: amounts on a voucher

```cpp
#include "walk_pdf.hpp"

#include <print>
#include <regex>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    return 2;
  }
  auto const result = walk_pdf::extract_text(std::filesystem::path{argv[1]});
  if (!result) {
    std::println(stderr, "{}: {}", argv[1], result.error().message);
    return 1;
  }
  std::regex const amount{R"(\d{1,3}(?:[ .]\d{3})*,\d{2})"}; // e.g. 1 234,50 or 615,00
  for (auto const& page : result->text.pages) {
    for (auto const& line : page) {
      if (std::regex_search(line, amount)) {
        std::println("{}", line);
      }
    }
  }
}
```

On `Faktura48293618.pdf` this prints lines such as
`101030 1 st Bokföring à 164 sek/mån 3 Mån 164,00 492,00` and
`0,00 0,00 492,00 123,00 0,00 615,00`.

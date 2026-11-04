#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace walk_pdf {

// Where in the PDF an element is, as a list of segments from the top.
// The first layer has one segment, e.g. "header", "obj[12 0]", "xref[1]".
class Path {
public:
  Path() = default;
  explicit Path(std::vector<std::string> segments) : segments_(std::move(segments)) {}

  std::vector<std::string> const& segments() const { return segments_; }
  Path child(std::string segment) const;

  // "/obj[12 0]" for the path {"obj[12 0]"}
  std::string to_string() const;

private:
  std::vector<std::string> segments_;
};

// What kind of top-level element the walker found
enum class Kind {
  header,    // %PDF-x.y
  comment,   // % ... up to end of line
  object,    // n g obj ... endobj
  xref,      // xref ... (classic cross-reference table)
  trailer,   // trailer << ... >>
  startxref, // startxref n
  eof,       // %%EOF
  unknown,   // bytes the walker could not make sense of
};

std::string_view to_string(Kind kind);

// The bytes of an element and where they are in the file
struct Content {
  Kind kind;
  std::size_t offset;    // byte offset in the file
  std::string_view bytes; // the element's raw bytes (may be binary)
};

using Client = std::function<void(Path const&, Content const&)>;

// Walks the raw bytes of a PDF file and reports each element to a client.
// Breadth first: for now only the first layer (the file's top-level elements).
class Walker {
public:
  explicit Walker(std::string_view file_bytes) : bytes_(file_bytes) {}

  void walk(Client const& client) const;

private:
  std::string_view bytes_;
};

} // namespace walk_pdf

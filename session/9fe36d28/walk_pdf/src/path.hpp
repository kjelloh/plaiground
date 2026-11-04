#pragma once

#include <compare>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace walk_pdf {

// The kinds of element the walker knows about
enum class Kind {
  document,  // the whole file (the root, path "/")
  revision,  // the original file or one appended update, up to and including its %%EOF
  header,    // %PDF-x.y at the start of the file
  comment,   // % ... up to end of line
  object,    // n g obj ... endobj
  xref,      // xref ... (classic cross-reference table)
  trailer,   // trailer << ... >>
  startxref, // startxref n
  eof,       // %%EOF
  unknown,   // bytes the walker could not make sense of
};

std::string_view to_string(Kind kind);

// One step in a path: which child of its parent an element is.
// An object is named by its object number and generation, the header by its
// kind alone, and anything else by its ordinal among siblings of the same kind.
struct Segment {
  Kind kind = Kind::document;
  std::uint64_t number = 0;     // object number, or ordinal
  std::uint32_t generation = 0; // objects only

  static Segment revision(std::uint64_t ordinal) { return {Kind::revision, ordinal, 0}; }
  static Segment header() { return {Kind::header, 0, 0}; }
  static Segment object(std::uint64_t number, std::uint32_t generation) {
    return {Kind::object, number, generation};
  }
  static Segment ordinal(Kind kind, std::uint64_t ordinal) { return {kind, ordinal, 0}; }

  // "rev[1]", "header", "obj[12 0]", "xref[0]"
  std::string to_string() const;

  auto operator<=>(Segment const&) const = default;
};

// Where an element is in the PDF, as a pure value: the segments from the root down.
// It holds no pointers or offsets; the Document looks up what a path refers to.
// The root (the whole document) is the empty path, "/".
class Path {
public:
  Path() = default;
  explicit Path(std::vector<Segment> segments) : segments_(std::move(segments)) {}

  std::vector<Segment> const& segments() const { return segments_; }
  bool is_root() const { return segments_.empty(); }
  Path child(Segment segment) const;
  Path parent() const;

  // "/rev[0]/obj[12 0]"; the root is "/"
  std::string to_string() const;

  // Reads the string form back; parse(p.to_string()) == p for every path p
  static std::expected<Path, std::string> parse(std::string_view text);

  auto operator<=>(Path const&) const = default;

private:
  std::vector<Segment> segments_;
};

} // namespace walk_pdf

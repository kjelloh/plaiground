#include "walker.hpp"

#include <charconv>
#include <format>
#include <map>
#include <optional>

namespace walk_pdf {

Path Path::child(std::string segment) const {
  auto segments = segments_;
  segments.push_back(std::move(segment));
  return Path{std::move(segments)};
}

std::string Path::to_string() const {
  if (segments_.empty()) {
    return "/";
  }
  std::string result;
  for (auto const& segment : segments_) {
    result += '/';
    result += segment;
  }
  return result;
}

std::string_view to_string(Kind kind) {
  switch (kind) {
  case Kind::header: return "header";
  case Kind::comment: return "comment";
  case Kind::object: return "object";
  case Kind::xref: return "xref";
  case Kind::trailer: return "trailer";
  case Kind::startxref: return "startxref";
  case Kind::eof: return "eof";
  case Kind::unknown: return "unknown";
  }
  return "?";
}

namespace {

// PDF 32000-1:2008, 7.2.2 Character set
bool is_whitespace(char c) {
  return c == '\0' || c == '\t' || c == '\n' || c == '\f' || c == '\r' || c == ' ';
}

bool is_delimiter(char c) {
  return c == '(' || c == ')' || c == '<' || c == '>' || c == '[' || c == ']' || c == '{' ||
         c == '}' || c == '/' || c == '%';
}

bool is_regular(char c) { return !is_whitespace(c) && !is_delimiter(c); }

bool is_digit(char c) { return c >= '0' && c <= '9'; }

// Reads the raw bytes of a PDF file, one top-level element at a time
class Scanner {
public:
  explicit Scanner(std::string_view bytes) : bytes_(bytes) {}

  bool at_end() const { return pos_ >= bytes_.size(); }
  std::size_t pos() const { return pos_; }
  void seek(std::size_t pos) { pos_ = std::min(pos, bytes_.size()); }
  std::string_view slice(std::size_t from, std::size_t to) const {
    return bytes_.substr(from, to - from);
  }

  void skip_whitespace() {
    while (!at_end() && is_whitespace(bytes_[pos_])) {
      ++pos_;
    }
  }

  // Position just after the end of the current line (after its EOL marker)
  std::size_t end_of_line(std::size_t from) const {
    auto pos = from;
    while (pos < bytes_.size() && bytes_[pos] != '\n' && bytes_[pos] != '\r') {
      ++pos;
    }
    // The EOL marker is CR, LF or CR LF
    if (pos < bytes_.size() && bytes_[pos] == '\r') {
      ++pos;
    }
    if (pos < bytes_.size() && bytes_[pos] == '\n') {
      ++pos;
    }
    return pos;
  }

  // Position just after the end of the line's text, before its EOL marker
  std::size_t end_of_text(std::size_t from) const {
    auto pos = from;
    while (pos < bytes_.size() && bytes_[pos] != '\n' && bytes_[pos] != '\r') {
      ++pos;
    }
    return pos;
  }

  // Is the keyword at pos and delimited on both sides?
  bool keyword_at(std::size_t pos, std::string_view keyword) const {
    if (bytes_.substr(pos, keyword.size()) != keyword) {
      return false;
    }
    if (pos > 0 && is_regular(bytes_[pos - 1])) {
      return false;
    }
    auto const after = pos + keyword.size();
    return after >= bytes_.size() || !is_regular(bytes_[after]);
  }

  // Position of the next delimited keyword at or after from
  std::optional<std::size_t> find_keyword(std::size_t from, std::string_view keyword) const {
    while (true) {
      auto const pos = bytes_.find(keyword, from);
      if (pos == std::string_view::npos) {
        return std::nullopt;
      }
      if (keyword_at(pos, keyword)) {
        return pos;
      }
      from = pos + 1;
    }
  }

  // An unsigned integer at pos, and the position after it
  std::optional<std::pair<unsigned long long, std::size_t>> integer_at(std::size_t pos) const {
    unsigned long long value = 0;
    auto const* first = bytes_.data() + pos;
    auto const* last = bytes_.data() + bytes_.size();
    auto const [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr == first || (ptr != last && is_regular(*ptr))) {
      return std::nullopt;
    }
    return std::pair{value, pos + static_cast<std::size_t>(ptr - first)};
  }

  std::size_t skip_whitespace_from(std::size_t pos) const {
    while (pos < bytes_.size() && is_whitespace(bytes_[pos])) {
      ++pos;
    }
    return pos;
  }

  char at(std::size_t pos) const { return pos < bytes_.size() ? bytes_[pos] : '\0'; }
  std::size_t size() const { return bytes_.size(); }

private:
  std::string_view bytes_;
  std::size_t pos_ = 0;
};

// "n g obj" at pos: the object number, generation and position after "obj"
struct ObjectStart {
  unsigned long long number;
  unsigned long long generation;
  std::size_t body;
};

std::optional<ObjectStart> object_start_at(Scanner const& scanner, std::size_t pos) {
  auto const number = scanner.integer_at(pos);
  if (!number) {
    return std::nullopt;
  }
  auto const generation = scanner.integer_at(scanner.skip_whitespace_from(number->second));
  if (!generation) {
    return std::nullopt;
  }
  auto const obj = scanner.skip_whitespace_from(generation->second);
  if (!scanner.keyword_at(obj, "obj")) {
    return std::nullopt;
  }
  return ObjectStart{number->first, generation->first, obj + 3};
}

// Position after "endobj" of the object whose body starts at from.
// A stream's data may hold any bytes, so skip it via "endstream".
// Not yet handled: "endobj" or "stream" inside a string in the object.
std::size_t end_of_object(Scanner const& scanner, std::size_t from) {
  auto pos = from;
  while (true) {
    auto const endobj = scanner.find_keyword(pos, "endobj");
    auto const stream = scanner.find_keyword(pos, "stream");
    if (stream && (!endobj || *stream < *endobj)) {
      auto const endstream = scanner.find_keyword(*stream + 6, "endstream");
      if (!endstream) {
        return scanner.size(); // Damaged file: the object runs to the end
      }
      pos = *endstream + 9;
      continue;
    }
    return endobj ? *endobj + 6 : scanner.size();
  }
}

// Position after the ">>" that closes the dictionary starting at from ("<<").
// Not yet handled: "<<" or ">>" inside a string in the dictionary.
std::size_t end_of_dictionary(Scanner const& scanner, std::size_t from) {
  int depth = 0;
  auto pos = from;
  while (pos + 1 < scanner.size()) {
    if (scanner.at(pos) == '<' && scanner.at(pos + 1) == '<') {
      ++depth;
      pos += 2;
    } else if (scanner.at(pos) == '>' && scanner.at(pos + 1) == '>') {
      --depth;
      pos += 2;
      if (depth == 0) {
        return pos;
      }
    } else {
      ++pos;
    }
  }
  return scanner.size();
}

} // namespace

void Walker::walk(Client const& client) const {
  Scanner scanner{bytes_};
  Path const root;
  std::map<Kind, int> counts; // Ordinal of repeated elements, e.g. xref[1]

  auto ordinal = [&](Kind kind) { return counts[kind]++; };
  auto report = [&](std::string segment, Kind kind, std::size_t from, std::size_t to) {
    client(root.child(std::move(segment)), Content{kind, from, scanner.slice(from, to)});
  };

  // Unrecognised bytes are gathered into one element until something is recognised
  std::optional<std::size_t> unknown_from;
  auto flush_unknown = [&](std::size_t to) {
    if (unknown_from) {
      report(std::format("unknown[{}]", ordinal(Kind::unknown)), Kind::unknown, *unknown_from, to);
      unknown_from.reset();
    }
  };

  while (true) {
    scanner.skip_whitespace();
    if (scanner.at_end()) {
      break;
    }
    auto const from = scanner.pos();
    auto const c = scanner.at(from);

    if (c == '%') {
      flush_unknown(from);
      auto const to = scanner.end_of_text(from);
      auto const text = scanner.slice(from, to);
      if (text.starts_with("%PDF-")) {
        report("header", Kind::header, from, to);
      } else if (text.starts_with("%%EOF")) {
        report(std::format("eof[{}]", ordinal(Kind::eof)), Kind::eof, from, to);
      } else {
        report(std::format("comment[{}]", ordinal(Kind::comment)), Kind::comment, from, to);
      }
      scanner.seek(to);
    } else if (auto const object = is_digit(c) ? object_start_at(scanner, from) : std::nullopt) {
      flush_unknown(from);
      auto const to = end_of_object(scanner, object->body);
      report(std::format("obj[{} {}]", object->number, object->generation), Kind::object, from, to);
      scanner.seek(to);
    } else if (scanner.keyword_at(from, "xref")) {
      flush_unknown(from);
      // The table runs until its trailer
      auto const to = scanner.find_keyword(from + 4, "trailer").value_or(scanner.size());
      report(std::format("xref[{}]", ordinal(Kind::xref)), Kind::xref, from, to);
      scanner.seek(to);
    } else if (scanner.keyword_at(from, "trailer")) {
      flush_unknown(from);
      auto const dictionary = scanner.skip_whitespace_from(from + 7);
      auto const to = end_of_dictionary(scanner, dictionary);
      report(std::format("trailer[{}]", ordinal(Kind::trailer)), Kind::trailer, from, to);
      scanner.seek(to);
    } else if (scanner.keyword_at(from, "startxref")) {
      flush_unknown(from);
      auto to = from + 9;
      if (auto const offset = scanner.integer_at(scanner.skip_whitespace_from(to))) {
        to = offset->second;
      }
      report(std::format("startxref[{}]", ordinal(Kind::startxref)), Kind::startxref, from, to);
      scanner.seek(to);
    } else {
      if (!unknown_from) {
        unknown_from = from;
      }
      scanner.seek(scanner.end_of_line(from));
    }
  }
  flush_unknown(scanner.size());
}

} // namespace walk_pdf

#include "path.hpp"

#include <array>
#include <charconv>
#include <format>
#include <optional>
#include <utility>

namespace walk_pdf {

std::string_view to_string(Kind kind) {
  switch (kind) {
  case Kind::document: return "document";
  case Kind::revision: return "revision";
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

// How each kind is named in a path segment. The document (root) has no segment.
constexpr std::array<std::pair<Kind, std::string_view>, 9> segment_names{{
    {Kind::revision, "rev"},
    {Kind::header, "header"},
    {Kind::comment, "comment"},
    {Kind::object, "obj"},
    {Kind::xref, "xref"},
    {Kind::trailer, "trailer"},
    {Kind::startxref, "startxref"},
    {Kind::eof, "eof"},
    {Kind::unknown, "unknown"},
}};

std::string_view segment_name(Kind kind) {
  for (auto const& [k, name] : segment_names) {
    if (k == kind) {
      return name;
    }
  }
  return "?";
}

std::optional<Kind> segment_kind(std::string_view name) {
  for (auto const& [kind, n] : segment_names) {
    if (n == name) {
      return kind;
    }
  }
  return std::nullopt;
}

// An unsigned decimal number that is all of text
template <typename T> std::optional<T> parse_number(std::string_view text) {
  T value{};
  auto const* last = text.data() + text.size();
  auto const [ptr, ec] = std::from_chars(text.data(), last, value);
  if (text.empty() || ec != std::errc{} || ptr != last) {
    return std::nullopt;
  }
  return value;
}

std::expected<Segment, std::string> parse_segment(std::string_view text) {
  auto const open = text.find('[');
  auto const name = text.substr(0, open);
  auto const kind = segment_kind(name);
  if (!kind) {
    return std::unexpected(std::format("unknown segment '{}'", text));
  }
  if (*kind == Kind::header) {
    if (open != std::string_view::npos) {
      return std::unexpected(std::format("'header' takes no [...] in '{}'", text));
    }
    return Segment::header();
  }
  if (open == std::string_view::npos || !text.ends_with(']')) {
    return std::unexpected(std::format("expected '{}[...]' but got '{}'", name, text));
  }
  auto const inside = text.substr(open + 1, text.size() - open - 2);
  if (*kind == Kind::object) {
    auto const space = inside.find(' ');
    auto const number = parse_number<std::uint64_t>(inside.substr(0, space));
    auto const generation = space == std::string_view::npos
                                ? std::nullopt
                                : parse_number<std::uint32_t>(inside.substr(space + 1));
    if (!number || !generation) {
      return std::unexpected(std::format("expected 'obj[number generation]' but got '{}'", text));
    }
    return Segment::object(*number, *generation);
  }
  auto const ordinal = parse_number<std::uint64_t>(inside);
  if (!ordinal) {
    return std::unexpected(std::format("expected '{}[number]' but got '{}'", name, text));
  }
  return Segment::ordinal(*kind, *ordinal);
}

} // namespace

std::string Segment::to_string() const {
  switch (kind) {
  case Kind::header: return std::string{segment_name(kind)};
  case Kind::object: return std::format("obj[{} {}]", number, generation);
  default: return std::format("{}[{}]", segment_name(kind), number);
  }
}

Path Path::child(Segment segment) const {
  auto segments = segments_;
  segments.push_back(segment);
  return Path{std::move(segments)};
}

Path Path::parent() const {
  auto segments = segments_;
  if (!segments.empty()) {
    segments.pop_back();
  }
  return Path{std::move(segments)};
}

std::string Path::to_string() const {
  if (segments_.empty()) {
    return "/";
  }
  std::string result;
  for (auto const& segment : segments_) {
    result += '/';
    result += segment.to_string();
  }
  return result;
}

std::expected<Path, std::string> Path::parse(std::string_view text) {
  if (!text.starts_with('/')) {
    return std::unexpected(std::format("a path starts with '/' but got '{}'", text));
  }
  std::vector<Segment> segments;
  auto rest = text.substr(1);
  while (!rest.empty()) {
    auto const slash = rest.find('/');
    auto const part = rest.substr(0, slash);
    auto segment = parse_segment(part);
    if (!segment) {
      return std::unexpected(segment.error());
    }
    segments.push_back(*segment);
    if (slash == std::string_view::npos) {
      break;
    }
    rest = rest.substr(slash + 1);
    if (rest.empty()) {
      return std::unexpected(std::format("a path does not end with '/' but got '{}'", text));
    }
  }
  return Path{std::move(segments)};
}

} // namespace walk_pdf

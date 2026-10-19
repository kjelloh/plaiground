#pragma once

// A parsed cspec file: ordered 'path = value' entries (see docs/cspec_format.md)

#include <cstddef>
#include <expected>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::cspec {

struct SpecEntry {
  std::string m_path;
  std::string m_value;
  int m_line{};
};

class SpecTree {
public:
  // source_name is used in error messages and provenance records.
  static std::expected<SpecTree, std::string> parse(std::string_view text, std::string source_name);
  static std::expected<SpecTree, std::string> load(std::filesystem::path const& path);

  std::optional<std::string> value(std::string_view path) const;
  std::string value_or(std::string_view path, std::string_view fallback) const;
  std::expected<std::string, std::string> require(std::string_view path) const;

  // True if the path has a value or children.
  bool contains(std::string_view path) const;

  // Names of the direct child segments of 'path', in order of first appearance.
  // An empty path gives the top-level segments.
  std::vector<std::string> children(std::string_view path) const;

  std::vector<SpecEntry> const& entries() const { return m_entries; }
  std::string const& source() const { return m_source; }

  // "<source>:<line>" for the entry at 'path', or just "<source>".
  std::string where(std::string_view path) const;

private:
  std::vector<SpecEntry> m_entries;
  std::map<std::string, std::size_t, std::less<>> m_index;
  std::string m_source;
};

bool is_valid_path(std::string_view path);

// A value as a quoted cspec string: "…" with " and \ escaped. For writing cspec files.
std::string quote(std::string_view text);

} // namespace cratchit::cspec

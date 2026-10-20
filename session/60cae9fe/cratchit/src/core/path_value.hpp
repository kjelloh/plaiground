#pragma once

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// One `path = value` line of a cratchit data file
struct PathValue {
  std::string path;
  std::string value;
};

using PathValues = std::vector<PathValue>;

// Parse `path = value` lines, in file order. Both sides are trimmed, so a value
// may contain spaces. Blank lines are skipped. Error text for a line without
// `=`, an empty path, or a path given twice.
std::expected<PathValues, std::string> parse_path_values(std::string_view text);

// The value of a path, if present
std::optional<std::string> find_value(PathValues const& entries, std::string_view path);

} // namespace cratchit

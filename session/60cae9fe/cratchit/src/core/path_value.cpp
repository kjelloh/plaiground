#include "core/path_value.hpp"

#include <algorithm>
#include <format>

namespace cratchit {

namespace {

std::string_view trim(std::string_view text) {
  auto const first = text.find_first_not_of(" \t\r");
  if (first == std::string_view::npos) {
    return {};
  }
  auto const last = text.find_last_not_of(" \t\r");
  return text.substr(first, last - first + 1);
}

} // namespace

std::expected<PathValues, std::string> parse_path_values(std::string_view text) {
  PathValues entries;
  std::size_t line_number = 0;
  while (!text.empty()) {
    ++line_number;
    auto const end = text.find('\n');
    auto const line = trim(text.substr(0, end));
    text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
    if (line.empty()) {
      continue;
    }
    auto const equals = line.find('=');
    if (equals == std::string_view::npos) {
      return std::unexpected(std::format("line {}: expected 'path = value'", line_number));
    }
    auto const path = trim(line.substr(0, equals));
    if (path.empty()) {
      return std::unexpected(std::format("line {}: no path before '='", line_number));
    }
    if (find_value(entries, path)) {
      return std::unexpected(std::format("line {}: '{}' is given twice", line_number, path));
    }
    entries.push_back({std::string(path), std::string(trim(line.substr(equals + 1)))});
  }
  return entries;
}

std::optional<std::string> find_value(PathValues const& entries, std::string_view path) {
  auto const it = std::ranges::find(entries, path, &PathValue::path);
  if (it == entries.end()) {
    return std::nullopt;
  }
  return it->value;
}

} // namespace cratchit

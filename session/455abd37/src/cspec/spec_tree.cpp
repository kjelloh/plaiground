#include "cspec/spec_tree.hpp"

#include "core/text.hpp"

#include <fstream>
#include <set>
#include <sstream>

namespace cratchit::cspec {

namespace {

bool is_segment_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
}

std::expected<std::string, std::string> unquote(std::string_view value) {
  if (value.size() < 2 || value.front() != '"' || value.back() != '"') return std::string{value};
  std::string out;
  auto const inner = value.substr(1, value.size() - 2);
  for (std::size_t i = 0; i < inner.size(); ++i) {
    char const c = inner[i];
    if (c != '\\') {
      if (c == '"') return std::unexpected("unescaped '\"' inside quoted value");
      out += c;
      continue;
    }
    if (++i == inner.size()) return std::unexpected("quoted value ends with '\\'");
    switch (inner[i]) {
      case '"': out += '"'; break;
      case '\\': out += '\\'; break;
      case 'n': out += '\n'; break;
      case 't': out += '\t'; break;
      default: return std::unexpected(std::string{"unknown escape '\\"} + inner[i] + "'");
    }
  }
  return out;
}

} // namespace

bool is_valid_path(std::string_view path) {
  if (path.empty() || path.front() == '.' || path.back() == '.') return false;
  char previous = '.';
  for (char c : path) {
    if (c == '.') {
      if (previous == '.') return false;
    } else if (!is_segment_char(c)) {
      return false;
    }
    previous = c;
  }
  return true;
}

std::string quote(std::string_view text) {
  std::string out = "\"";
  for (char c : text) {
    if (c == '"' || c == '\\') out += '\\';
    if (c == '\n') {
      out += "\\n";
      continue;
    }
    out += c;
  }
  return out + "\"";
}

std::expected<SpecTree, std::string> SpecTree::parse(std::string_view text, std::string source_name) {
  SpecTree tree;
  tree.m_source = std::move(source_name);
  int line_number = 0;
  for (auto line : text::split_lines(text)) {
    ++line_number;
    auto const trimmed = text::trim(line);
    if (trimmed.empty() || trimmed.front() == '#') continue;
    auto const fail = [&](std::string const& message) {
      return std::unexpected(tree.m_source + ":" + std::to_string(line_number) + ": " + message);
    };
    auto const equals = trimmed.find('=');
    if (equals == std::string_view::npos) return fail("expected 'path = value'");
    auto const path = std::string{text::trim(trimmed.substr(0, equals))};
    if (!is_valid_path(path)) return fail("invalid path \"" + path + "\"");
    auto value = unquote(text::trim(trimmed.substr(equals + 1)));
    if (!value) return fail(value.error());
    if (tree.m_index.contains(path)) {
      return fail("duplicate path \"" + path + "\" (first at line " +
                  std::to_string(tree.m_entries[tree.m_index.find(path)->second].m_line) + ")");
    }
    tree.m_index.emplace(path, tree.m_entries.size());
    tree.m_entries.push_back({path, std::move(*value), line_number});
  }
  return tree;
}

std::expected<SpecTree, std::string> SpecTree::load(std::filesystem::path const& path) {
  std::ifstream file{path, std::ios::binary};
  if (!file) return std::unexpected("cannot open " + path.string());
  std::ostringstream content;
  content << file.rdbuf();
  return parse(content.str(), path.string());
}

std::optional<std::string> SpecTree::value(std::string_view path) const {
  if (auto const it = m_index.find(path); it != m_index.end()) return m_entries[it->second].m_value;
  return std::nullopt;
}

std::string SpecTree::value_or(std::string_view path, std::string_view fallback) const {
  if (auto v = value(path)) return *v;
  return std::string{fallback};
}

std::expected<std::string, std::string> SpecTree::require(std::string_view path) const {
  if (auto v = value(path)) return *v;
  return std::unexpected(m_source + ": missing \"" + std::string{path} + "\"");
}

bool SpecTree::contains(std::string_view path) const {
  if (m_index.contains(path)) return true;
  auto const prefix = std::string{path} + ".";
  auto const it = m_index.lower_bound(prefix);
  return it != m_index.end() && it->first.starts_with(prefix);
}

std::vector<std::string> SpecTree::children(std::string_view path) const {
  auto const prefix = path.empty() ? std::string{} : std::string{path} + ".";
  std::vector<std::string> result;
  std::set<std::string, std::less<>> seen;
  for (auto const& entry : m_entries) {
    if (!entry.m_path.starts_with(prefix)) continue;
    auto const rest = std::string_view{entry.m_path}.substr(prefix.size());
    auto const segment = std::string{rest.substr(0, rest.find('.'))};
    if (seen.insert(segment).second) result.push_back(segment);
  }
  return result;
}

std::string SpecTree::where(std::string_view path) const {
  if (auto const it = m_index.find(path); it != m_index.end()) {
    return m_source + ":" + std::to_string(m_entries[it->second].m_line);
  }
  return m_source;
}

} // namespace cratchit::cspec

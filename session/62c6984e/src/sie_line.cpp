#include "sie_line.hpp"

#include <cstddef>

namespace cratchit {

namespace {

bool is_space(char c) { return c == ' ' || c == '\t'; }

} // namespace

std::expected<std::vector<SieField>, std::string> split_sie_line(std::string_view line) {
  std::vector<SieField> fields;
  std::size_t pos = 0;

  while (true) {
    while (pos < line.size() && is_space(line[pos])) ++pos;
    if (pos == line.size()) break;

    SieField field;
    if (line[pos] == '"') {
      ++pos;
      bool closed = false;
      while (pos < line.size()) {
        char const c = line[pos++];
        if (c == '"') {
          closed = true;
          break;
        }
        if (c == '\\' && pos < line.size()) {
          field.m_text += line[pos++];
        } else {
          field.m_text += c;
        }
      }
      if (!closed) return std::unexpected("Unterminated quoted string");
    } else if (line[pos] == '{') {
      field.m_is_object_list = true;
      ++pos;
      bool in_quotes = false;
      bool closed = false;
      while (pos < line.size()) {
        char const c = line[pos++];
        if (c == '\\' && in_quotes && pos < line.size()) {
          field.m_text += c;
          field.m_text += line[pos++];
          continue;
        }
        if (c == '"') in_quotes = !in_quotes;
        if (c == '}' && !in_quotes) {
          closed = true;
          break;
        }
        field.m_text += c;
      }
      if (!closed) return std::unexpected("Unterminated object list");
    } else {
      while (pos < line.size() && !is_space(line[pos])) field.m_text += line[pos++];
    }
    fields.push_back(std::move(field));
  }
  return fields;
}

} // namespace cratchit

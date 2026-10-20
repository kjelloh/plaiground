#include "core/manual.hpp"

#include <algorithm>
#include <cstddef>
#include <regex>

namespace cratchit {

namespace {

std::vector<std::string_view> split_lines(std::string_view text) {
  std::vector<std::string_view> lines;
  while (!text.empty()) {
    auto end = text.find('\n');
    auto line = text.substr(0, end);
    if (line.ends_with('\r')) {
      line.remove_suffix(1);
    }
    lines.push_back(line);
    if (end == std::string_view::npos) {
      break;
    }
    text.remove_prefix(end + 1);
  }
  return lines;
}

std::string_view trim(std::string_view s) {
  auto first = s.find_first_not_of(" \t");
  if (first == std::string_view::npos) {
    return {};
  }
  auto last = s.find_last_not_of(" \t");
  return s.substr(first, last - first + 1);
}

bool is_blank(std::string_view line) {
  return trim(line).empty();
}

bool is_fence(std::string_view line) {
  auto t = trim(line);
  return t.starts_with("```") || t.starts_with("~~~");
}

// The number of leading #s of a heading line, or 0 if the line is not a heading
int heading_level(std::string_view line) {
  auto level = line.find_first_not_of('#');
  if (level == 0 || level > 6) {
    return 0;
  }
  if (level != std::string_view::npos && line[level] != ' ') {
    return 0;
  }
  return level == std::string_view::npos ? static_cast<int>(line.size()) : static_cast<int>(level);
}

// Remove emphasis and link markup from markdown outside code spans
std::string strip_emphasis_and_links(std::string const& s) {
  static std::regex const link(R"(\[([^\]]+)\]\([^)]*\))");
  static std::regex const bold(R"(\*\*([^*]+)\*\*)");
  static std::regex const em(R"(\*([^*\s]([^*]*[^*\s])?)\*)");
  auto result = std::regex_replace(s, link, "$1");
  result = std::regex_replace(result, bold, "$1");
  return std::regex_replace(result, em, "$1");
}

// Remove inline markup, keeping the content of `code` spans as is
std::string strip_inline(std::string_view line) {
  std::string result;
  while (!line.empty()) {
    auto open = line.find('`');
    auto close = open == std::string_view::npos ? open : line.find('`', open + 1);
    if (close == std::string_view::npos) {
      result += strip_emphasis_and_links(std::string(line));
      break;
    }
    result += strip_emphasis_and_links(std::string(line.substr(0, open)));
    result += line.substr(open + 1, close - open - 1);
    line.remove_prefix(close + 1);
  }
  return result;
}

// The number of characters shown, counting each UTF-8 sequence once
std::size_t display_width(std::string_view s) {
  return static_cast<std::size_t>(
      std::ranges::count_if(s, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

bool is_table_line(std::string_view line) {
  return trim(line).starts_with('|');
}

std::vector<std::string> table_cells(std::string_view line) {
  auto t = trim(line);
  t.remove_prefix(1);
  if (t.ends_with('|') && !t.ends_with("\\|")) {
    t.remove_suffix(1);
  }
  std::vector<std::string> cells;
  std::string cell;
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] == '\\' && i + 1 < t.size() && t[i + 1] == '|') {
      cell += '|';
      ++i;
    } else if (t[i] == '|') {
      cells.push_back(strip_inline(trim(cell)));
      cell.clear();
    } else {
      cell += t[i];
    }
  }
  cells.push_back(strip_inline(trim(cell)));
  return cells;
}

// The |---|:---:| row between the header and the body of a table
bool is_separator_row(std::vector<std::string> const& cells) {
  return std::ranges::all_of(cells, [](std::string const& cell) {
    return !cell.empty() && cell.find_first_not_of("-: ") == std::string::npos;
  });
}

// Lay out table rows as columns separated by two spaces
void append_table(std::vector<std::vector<std::string>> const& rows, std::vector<std::string>& out) {
  std::vector<std::size_t> widths;
  for (auto const& row : rows) {
    widths.resize(std::max(widths.size(), row.size()));
    for (std::size_t i = 0; i < row.size(); ++i) {
      widths[i] = std::max(widths[i], display_width(row[i]));
    }
  }
  for (auto const& row : rows) {
    std::string line = " ";
    for (std::size_t i = 0; i < row.size(); ++i) {
      line += ' ' + row[i] + std::string(widths[i] - display_width(row[i]) + 1, ' ');
    }
    line.erase(line.find_last_not_of(' ') + 1);
    out.push_back(std::move(line));
  }
}

std::string plain_line(std::string_view line) {
  auto indent = line.substr(0, line.find_first_not_of(" \t"));
  auto body = line.substr(indent.size());
  if (body.starts_with("> ")) {
    body.remove_prefix(2);
  }
  if (body.starts_with("* ") || body.starts_with("+ ")) {
    return std::string(indent) + "- " + strip_inline(body.substr(2));
  }
  return std::string(indent) + strip_inline(body);
}

// A heading (or the text before the first heading) and the raw markdown under it
struct FlatSection {
  int level;
  std::string title;
  std::string markdown;
};

Section build_tree(std::vector<FlatSection> const& flat, std::size_t& i) {
  auto const& f = flat[i++];
  Section section{f.title, to_plain_text(f.markdown), {}};
  while (i < flat.size() && flat[i].level > f.level) {
    section.children.push_back(build_tree(flat, i));
  }
  return section;
}

} // namespace

std::string to_plain_text(std::string_view markdown) {
  std::vector<std::string> out;
  std::vector<std::vector<std::string>> table;
  bool in_code = false;
  auto flush_table = [&] {
    if (!table.empty()) {
      append_table(table, out);
      table.clear();
    }
  };
  for (auto line : split_lines(markdown)) {
    if (is_fence(line)) {
      flush_table();
      in_code = !in_code;
    } else if (in_code) {
      out.push_back(is_blank(line) ? "" : "    " + std::string(line));
    } else if (is_table_line(line)) {
      auto cells = table_cells(line);
      if (!is_separator_row(cells)) {
        table.push_back(std::move(cells));
      }
    } else {
      flush_table();
      out.push_back(is_blank(line) ? "" : plain_line(line));
    }
  }
  flush_table();

  // Join the lines, keeping at most one blank line in a row and none at the ends
  std::string text;
  bool blank_pending = false;
  for (auto const& line : out) {
    if (line.empty()) {
      blank_pending = !text.empty();
      continue;
    }
    if (blank_pending) {
      text += '\n';
      blank_pending = false;
    }
    text += line + '\n';
  }
  return text;
}

Section parse_manual(std::string_view markdown) {
  std::vector<FlatSection> flat{{0, "", ""}};
  bool in_code = false;
  for (auto line : split_lines(markdown)) {
    if (is_fence(line)) {
      in_code = !in_code;
    }
    int level = in_code ? 0 : heading_level(line);
    if (level > 0) {
      flat.push_back({level, strip_inline(trim(line.substr(static_cast<std::size_t>(level)))), ""});
    } else {
      flat.back().markdown += std::string(line) + '\n';
    }
  }
  std::size_t i = 0;
  auto root = build_tree(flat, i);
  if (root.text.empty() && root.children.size() == 1) {
    return std::move(root.children.front());
  }
  return root;
}

} // namespace cratchit

#include "core/help.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <iterator>
#include <print>

namespace cratchit {

namespace {

// With more children than keys 1-9, keys 1-8 show a range, 0 scrolls up and 9 down
constexpr std::size_t max_keys = 9;
constexpr std::size_t range_size = 8;

std::vector<std::string> lines_of(std::string const& text) {
  std::vector<std::string> lines;
  std::size_t start = 0;
  while (start < text.size()) {
    auto end = text.find('\n', start);
    if (end == std::string::npos) {
      end = text.size();
    }
    lines.push_back(text.substr(start, end - start));
    start = end + 1;
  }
  return lines;
}

std::string hint(Section const& section) {
  return section.children.empty() ? "Press - to go back, q to leave help."
                                  : "Press a number to open a section, - to go back, q to leave help.";
}

} // namespace

HelpBrowser::HelpBrowser(Section const& manual, std::size_t page_lines)
    : manual_(manual), page_lines_(page_lines) {}

void HelpBrowser::open(std::ostream& out) {
  path_ = {{&manual_, 0, 0}};
  waiting_lines_.clear();
  show_section(out);
}

HelpBrowser::Result HelpBrowser::on_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'q') {
    path_.clear();
    waiting_lines_.clear();
    return Result::Leave;
  }
  if (key == U'-') {
    back(out);
    return path_.empty() ? Result::Leave : Result::Stay;
  }
  if (is_paging()) {
    if (key == U' ' || key == U'\n' || key == U'\r') {
      show_next_page(out);
    } else {
      std::println(out, "Press space for more, - to go back, q to leave help.");
    }
    return Result::Stay;
  }
  if (key == U'h') {
    show_section(out);
  } else if (key >= U'0' && key <= U'9') {
    select(static_cast<char>(key), out);
  } else {
    std::println(out, "Unknown key. {}", hint(*path_.back().section));
  }
  return Result::Stay;
}

std::string HelpBrowser::prompt() const {
  if (is_paging()) {
    return "--More-- ";
  }
  std::string result = "help";
  for (std::size_t i = 1; i < path_.size(); ++i) {
    result += std::format("{}{}", i == 1 ? " " : ".", path_[i].number);
  }
  return result + "> ";
}

bool HelpBrowser::is_paging() const {
  return !waiting_lines_.empty();
}

void HelpBrowser::show_section(std::ostream& out) {
  auto const& section = *path_.back().section;
  std::vector<std::string> lines;
  if (!section.title.empty()) {
    lines.push_back(section.title);
    lines.push_back("");
  }
  if (!section.text.empty()) {
    std::ranges::move(lines_of(section.text), std::back_inserter(lines));
    lines.push_back("");
  }
  if (!section.children.empty()) {
    std::ranges::move(menu_lines(), std::back_inserter(lines));
    lines.push_back("");
  }
  lines.push_back(hint(section));
  show(std::move(lines), out);
}

void HelpBrowser::show_menu(std::ostream& out) {
  auto const& section = *path_.back().section;
  std::vector<std::string> lines;
  if (!section.title.empty()) {
    lines.push_back(section.title);
    lines.push_back("");
  }
  std::ranges::move(menu_lines(), std::back_inserter(lines));
  lines.push_back("");
  lines.push_back(hint(section));
  show(std::move(lines), out);
}

void HelpBrowser::back(std::ostream& out) {
  waiting_lines_.clear();
  path_.pop_back();
  if (!path_.empty()) {
    show_menu(out);
  }
}

void HelpBrowser::select(char digit, std::ostream& out) {
  auto& level = path_.back();
  auto const& children = level.section->children;
  auto const n = children.size();
  auto const k = static_cast<std::size_t>(digit - '0');
  std::size_t index = n; // not found
  if (n <= max_keys) {
    if (k >= 1) {
      index = k - 1;
    }
  } else if (k == 0 && level.first_shown > 0) {
    level.first_shown -= range_size;
    show_menu(out);
    return;
  } else if (k == 9 && level.first_shown + range_size < n) {
    level.first_shown += range_size;
    show_menu(out);
    return;
  } else if (k >= 1 && k <= range_size) {
    index = level.first_shown + k - 1;
  }
  if (index >= n) {
    std::println(out, "No section {} here. {}", digit, hint(*level.section));
    return;
  }
  path_.push_back({&children[index], index + 1, 0});
  show_section(out);
}

std::vector<std::string> HelpBrowser::menu_lines() const {
  auto const& level = path_.back();
  auto const& children = level.section->children;
  auto const n = children.size();
  std::vector<std::string> lines;
  if (n <= max_keys) {
    for (std::size_t i = 0; i < n; ++i) {
      lines.push_back(std::format("  {}  {}", i + 1, children[i].title));
    }
    return lines;
  }
  if (level.first_shown > 0) {
    lines.push_back("  0  ...up");
  }
  auto const last = std::min(level.first_shown + range_size, n);
  for (std::size_t i = level.first_shown; i < last; ++i) {
    lines.push_back(std::format("  {}  {}", i - level.first_shown + 1, children[i].title));
  }
  if (last < n) {
    lines.push_back("  9  ...down");
  }
  return lines;
}

void HelpBrowser::show(std::vector<std::string> lines, std::ostream& out) {
  waiting_lines_ = std::move(lines);
  show_next_page(out);
}

void HelpBrowser::show_next_page(std::ostream& out) {
  auto count = page_lines_ == 0 ? waiting_lines_.size() : std::min(page_lines_, waiting_lines_.size());
  for (std::size_t i = 0; i < count; ++i) {
    std::println(out, "{}", waiting_lines_[i]);
  }
  waiting_lines_.erase(waiting_lines_.begin(), waiting_lines_.begin() + static_cast<std::ptrdiff_t>(count));
}

} // namespace cratchit

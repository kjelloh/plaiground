#include "core/log_view.hpp"

#include <algorithm>
#include <fstream>
#include <print>

namespace cratchit {

LogView::LogView(std::size_t page_lines) : page_lines_(page_lines) {}

LogView::Result LogView::open(std::filesystem::path const& file, std::ostream& out) {
  waiting_lines_ = lines_newest_first(file);
  if (waiting_lines_.empty()) {
    std::println(out, "The log {} is empty.", file.string());
    return Result::Leave;
  }
  std::println(out, "Log {}, newest first:", file.string());
  return show_next_page(out);
}

LogView::Result LogView::on_key(char key, std::ostream& out) {
  if (key == '-' || key == 'q') {
    waiting_lines_.clear();
    return Result::Leave;
  }
  if (key == ' ' || key == '\n' || key == '\r') {
    return show_next_page(out);
  }
  std::println(out, "Press space for older events, - or q to leave the log.");
  return Result::Stay;
}

std::string LogView::prompt() const {
  return "--More-- ";
}

LogView::Result LogView::show_next_page(std::ostream& out) {
  auto const count = page_lines_ == 0 ? waiting_lines_.size() : std::min(page_lines_, waiting_lines_.size());
  for (std::size_t i = 0; i < count; ++i) {
    std::println(out, "{}", waiting_lines_[i]);
  }
  waiting_lines_.erase(waiting_lines_.begin(), waiting_lines_.begin() + static_cast<std::ptrdiff_t>(count));
  if (!waiting_lines_.empty()) {
    return Result::Stay;
  }
  std::println(out, "(start of log)");
  return Result::Leave;
}

std::vector<std::string> lines_newest_first(std::filesystem::path const& file) {
  std::vector<std::string> lines;
  std::ifstream in(file);
  for (std::string line; std::getline(in, line);) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    lines.push_back(std::move(line));
  }
  std::ranges::reverse(lines);
  return lines;
}

} // namespace cratchit

#include "core/pager.hpp"

#include <algorithm>
#include <print>

namespace cratchit {

Pager::Pager(std::size_t page_lines) : page_lines_(page_lines) {}

Pager::Result Pager::open(std::vector<std::string> lines, std::ostream& out) {
  lines_ = std::move(lines);
  shown_ = 0;
  return show_next_page(out);
}

Pager::Result Pager::on_key(charset::unicode::CodePoint key, std::ostream& out) {
  if (key == U'-' || key == U'q') {
    lines_.clear();
    return Result::Leave;
  }
  if (key == U' ' || key == U'\n' || key == U'\r') {
    return show_next_page(out);
  }
  std::println(out, "Press space for more, - or q to stop.");
  return Result::Stay;
}

std::string Pager::prompt() const {
  return "--More-- ";
}

Pager::Result Pager::show_next_page(std::ostream& out) {
  auto const left = lines_.size() - shown_;
  auto const count = page_lines_ == 0 ? left : std::min(page_lines_, left);
  for (std::size_t i = 0; i < count; ++i) {
    std::println(out, "{}", lines_[shown_ + i]);
  }
  shown_ += count;
  if (shown_ < lines_.size()) {
    return Result::Stay;
  }
  lines_.clear();
  return Result::Leave;
}

} // namespace cratchit

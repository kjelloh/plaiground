#pragma once

#include "core/charset/unicode.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace cratchit {

// Shows lines a page at a time, like the 'more' command
class Pager {
public:
  enum class Result { Stay, Leave };

  // page_lines is the number of lines per page; 0 shows everything at once
  explicit Pager(std::size_t page_lines);

  // Show the first page. Leave when it all fit.
  Result open(std::vector<std::string> lines, std::ostream& out);

  // Space or Enter shows the next page; - or q leaves
  Result on_key(charset::unicode::CodePoint key, std::ostream& out);

  std::string prompt() const;

private:
  std::size_t page_lines_;
  std::vector<std::string> lines_;
  std::size_t shown_ = 0;

  Result show_next_page(std::ostream& out);
};

} // namespace cratchit

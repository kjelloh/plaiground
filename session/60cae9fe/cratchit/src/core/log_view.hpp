#pragma once

#include "core/charset/unicode.hpp"

#include <cstddef>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

namespace cratchit {

// Shows the log newest event first, a page at a time like the 'more' command
class LogView {
public:
  enum class Result { Stay, Leave };

  // page_lines is the number of lines per page; 0 shows everything at once
  explicit LogView(std::size_t page_lines);

  // Show the first page of file, newest event first. Leave when it all fit.
  Result open(std::filesystem::path const& file, std::ostream& out);

  // Space or Enter shows the next (older) page; - or q leaves
  Result on_key(charset::unicode::CodePoint key, std::ostream& out);

  std::string prompt() const;

private:
  std::size_t page_lines_;
  std::vector<std::string> waiting_lines_;

  Result show_next_page(std::ostream& out);
};

// The lines of file, last line first; none when it is missing
std::vector<std::string> lines_newest_first(std::filesystem::path const& file);

} // namespace cratchit

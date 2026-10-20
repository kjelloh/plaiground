#pragma once

#include "core/manual.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace cratchit {

// Browses the manual one section at a time: number keys open a section, - goes back.
// Long output is shown a page at a time, like the 'more' command.
class HelpBrowser {
public:
  enum class Result { Stay, Leave };

  // page_lines is the number of lines per page; 0 shows everything at once
  HelpBrowser(Section const& manual, std::size_t page_lines);

  // Show the top of the manual
  void open(std::ostream& out);

  Result on_key(char key, std::ostream& out);

  // "help> ", "help 2.1> ", or the more prompt while a page is waiting
  std::string prompt() const;

  // True while the rest of a long output waits for space or Enter
  bool is_paging() const;

private:
  struct Level {
    Section const* section;
    std::size_t number;      // 1-based position in the parent; 0 for the top
    std::size_t first_shown; // index of the first child in the shown range
  };

  Section const& manual_;
  std::size_t page_lines_;
  std::vector<Level> path_;
  std::vector<std::string> waiting_lines_;

  void show_section(std::ostream& out);
  void show_menu(std::ostream& out);
  void back(std::ostream& out);
  void select(char key, std::ostream& out);
  std::vector<std::string> menu_lines() const;
  void show(std::vector<std::string> lines, std::ostream& out);
  void show_next_page(std::ostream& out);
};

} // namespace cratchit

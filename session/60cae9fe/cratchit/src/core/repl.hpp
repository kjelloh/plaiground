#pragma once

#include "core/help.hpp"
#include "core/manual.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <ostream>
#include <string>

namespace cratchit {

// What the REPL loop should do after a key has been handled
enum class Step { Continue, Quit };

// The state of the app between key presses: the main prompt, or browsing help
class Repl {
public:
  // page_lines is the number of help lines per page; 0 shows everything at once
  Repl(Section const& manual, std::size_t page_lines);

  // Handle one key press, writing any response to out
  Step on_key(char key, std::ostream& out);

  // True for keys that do nothing in the current mode, so are not even echoed
  bool ignores(char key) const;

  std::string prompt() const;

private:
  HelpBrowser help_;
  bool in_help_ = false;
};

// Supplies the next key press, or nothing at end of input
using KeySource = std::function<std::optional<char>()>;

// Prompt, read a key, handle it, until quit or end of input
void run_repl(KeySource const& next_key, std::ostream& out, Section const& manual, std::size_t page_lines);

} // namespace cratchit

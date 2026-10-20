#include "core/repl.hpp"

#include <print>
#include <string_view>

namespace cratchit {

namespace {

constexpr char ctrl_d = '\x04';
// Esc does nothing: arrow keys also start with it, so the two can't be told apart
constexpr char escape = '\x1b';

bool is_whitespace(char key) {
  return key == '\n' || key == '\r' || key == ' ' || key == '\t';
}

// How a key press is shown after the prompt, as echo is off in raw mode
std::string_view echo_of(char const& key) {
  if (key == ctrl_d) {
    return "^D";
  }
  if (is_whitespace(key)) {
    return "";
  }
  return {&key, 1};
}

} // namespace

Repl::Repl(Section const& manual, std::size_t page_lines) : help_(manual, page_lines) {}

Step Repl::on_key(char key, std::ostream& out) {
  if (key == ctrl_d) {
    return Step::Quit;
  }
  if (in_help_) {
    in_help_ = help_.on_key(key, out) == HelpBrowser::Result::Stay;
    return Step::Continue;
  }
  switch (key) {
  case 'h':
    in_help_ = true;
    help_.open(out);
    return Step::Continue;
  case 'q':
    return Step::Quit;
  default:
    std::println(out, "Unknown key. Press h for help.");
    return Step::Continue;
  }
}

bool Repl::ignores(char key) const {
  if (key == escape) {
    return true;
  }
  // Space and Enter show the next page of help
  if (in_help_ && help_.is_paging()) {
    return key == '\t';
  }
  return is_whitespace(key);
}

std::string Repl::prompt() const {
  return in_help_ ? help_.prompt() : "cratchit> ";
}

void run_repl(KeySource const& next_key, std::ostream& out, Section const& manual, std::size_t page_lines) {
  Repl repl(manual, page_lines);
  while (true) {
    std::print(out, "{}", repl.prompt());
    out.flush();
    std::optional<char> key;
    do {
      key = next_key();
    } while (key && repl.ignores(*key));
    if (!key) {
      std::println(out, "");
      return;
    }
    std::println(out, "{}", echo_of(*key));
    if (repl.on_key(*key, out) == Step::Quit) {
      return;
    }
  }
}

} // namespace cratchit

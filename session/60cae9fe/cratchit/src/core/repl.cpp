#include "core/repl.hpp"

#include <print>
#include <string_view>

namespace cratchit {

namespace {

constexpr char ctrl_d = '\x04';

void print_help(std::ostream& out) {
  std::println(out, "Keys:");
  std::println(out, "  h  show this help");
  std::println(out, "  q  quit");
}

bool is_ignored(char key) {
  return key == '\n' || key == '\r' || key == ' ' || key == '\t';
}

} // namespace

Step on_key(char key, std::ostream& out) {
  switch (key) {
  case 'h':
    print_help(out);
    return Step::Continue;
  case 'q':
  case ctrl_d:
    return Step::Quit;
  default:
    std::println(out, "Unknown key. Press h for help.");
    return Step::Continue;
  }
}

void run_repl(KeySource const& next_key, std::ostream& out) {
  while (true) {
    std::print(out, "cratchit> ");
    out.flush();
    std::optional<char> key;
    do {
      key = next_key();
    } while (key && is_ignored(*key));
    if (!key) {
      std::println(out, "");
      return;
    }
    // Echo is off in raw mode, so show the key that was pressed
    std::println(out, "{}", *key == ctrl_d ? std::string_view("^D") : std::string_view(&*key, 1));
    if (on_key(*key, out) == Step::Quit) {
      return;
    }
  }
}

} // namespace cratchit

#include "core/terminal_encoding.hpp"

#include <print>

int main() {
  auto const terminal_encoding = cratchit::choose_terminal_encoding(cratchit::report_terminal_encoding());
  for (auto const& warning : terminal_encoding.warnings) {
    std::println(stderr, "{}", warning);
  }
}

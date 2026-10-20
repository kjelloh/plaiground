#include "core/manual.hpp"
#include "core/repl.hpp"
#include "core/terminal.hpp"
#include "core/version.hpp"

#include <iostream>
#include <print>

int main() {
  std::println("cratchit {}. Press h for help, q to quit.", cratchit::version());
  cratchit::RawTerminal terminal;
  auto const manual = cratchit::parse_manual(cratchit::manual_text());
  cratchit::run_repl([&terminal] { return terminal.read_key(); }, std::cout, manual, cratchit::page_lines());
  return 0;
}

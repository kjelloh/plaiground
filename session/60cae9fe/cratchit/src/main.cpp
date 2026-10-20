#include "core/data_folder.hpp"
#include "core/manual.hpp"
#include "core/repl.hpp"
#include "core/terminal.hpp"
#include "core/version.hpp"

#include <filesystem>
#include <iostream>
#include <print>

int main() {
  std::println("cratchit {}. Press h for help, q to quit.", cratchit::version());
  auto const cwd = std::filesystem::current_path();
  auto const folder = cratchit::open_data_folder(cwd, cratchit::today_iso());
  if (!folder) {
    std::println(stderr, "Error: {}", folder.error());
    return 1;
  }
  for (auto const& entry : folder->initiated) {
    std::println("Initiated {} in {}", entry.file.generic_string(), entry.folder.string());
  }
  cratchit::RawTerminal terminal;
  auto const manual = cratchit::parse_manual(cratchit::manual_text());
  cratchit::run_repl([&terminal] { return terminal.read_key(); }, std::cout, manual, cratchit::page_lines());
  return 0;
}

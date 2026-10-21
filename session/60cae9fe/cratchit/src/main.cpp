#include "core/data_folder.hpp"
#include "core/manual.hpp"
#include "core/repl.hpp"
#include "core/terminal.hpp"
#include "core/terminal_encoding.hpp"
#include "core/version.hpp"
#include "core/workspace.hpp"

#include <filesystem>
#include <iostream>
#include <print>

int main() {
  std::println("cratchit {}. Press h for help, q to quit.", cratchit::version());
  // Kept for the run: the encodings of the bytes cratchit reads from and writes to the terminal
  auto const terminal_encoding = cratchit::choose_terminal_encoding(cratchit::report_terminal_encoding());
  for (auto const& warning : terminal_encoding.warnings) {
    std::println(stderr, "{}", warning);
  }
  auto const cwd = std::filesystem::current_path();
  auto const today = cratchit::today_iso();
  auto const folder = cratchit::open_data_folder(cwd, today);
  if (!folder) {
    std::println(stderr, "Error: {}", folder.error());
    return 1;
  }
  for (auto const& entry : folder->initiated) {
    std::println("Initiated {} in {}", entry.file.generic_string(), entry.folder.string());
  }
  cratchit::RawTerminal terminal;
  auto const manual = cratchit::parse_manual(cratchit::manual_text());
  cratchit::Repl repl(manual, cratchit::page_lines(),
                      {folder->runtime_file, cwd, cratchit::default_workspace_folder(), today});
  repl.open_workspace(std::cout);
  cratchit::run_repl(repl, [&terminal] { return terminal.read_key(); }, std::cout);
  return 0;
}

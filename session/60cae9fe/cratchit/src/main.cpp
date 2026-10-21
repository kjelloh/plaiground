#include "core/data_folder.hpp"
#include "core/key_input.hpp"
#include "core/log.hpp"
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
  auto const cwd = std::filesystem::current_path();
  LOG_BUSINESS("cratchit {} started in {}", cratchit::version(), cwd.string());
  // Kept for the run: the encodings of the bytes cratchit reads from and writes to the terminal
  auto const terminal_encoding = cratchit::choose_terminal_encoding(cratchit::report_terminal_encoding());
  for (auto const& warning : terminal_encoding.warnings) {
    std::println(stderr, "{}", warning);
    LOG_DESIGN_INSUFFICIENCY("{}", warning);
  }
  auto const today = cratchit::today_iso();
  auto const folder = cratchit::open_data_folder(cwd, today);
  if (!folder) {
    std::println(stderr, "Error: {}", folder.error());
    return 1;
  }
  // One log for every run, next to runtime.txt
  auto const log_file = folder->runtime_file.parent_path() / "log.txt";
  if (auto const opened = cratchit::log::open(log_file); !opened) {
    std::println(stderr, "Warning: {}. Nothing is logged.", opened.error());
  }
  for (auto const& entry : folder->initiated) {
    std::println("Initiated {} in {}", entry.file.generic_string(), entry.folder.string());
    LOG_BUSINESS("Initiated {} in {}", entry.file.generic_string(), entry.folder.string());
  }
  cratchit::RawTerminal terminal;
  auto const manual = cratchit::parse_manual(cratchit::manual_text());
  LOG_DEVELOPMENT_TRACE("Page lines {}", cratchit::page_lines());
  cratchit::Repl repl(manual, cratchit::page_lines(),
                      {folder->runtime_file, cwd, cratchit::default_workspace_folder(), today}, log_file);
  repl.open_workspace(std::cout);
  // Keys are decoded from the terminal's input encoding to Unicode before the REPL acts on them
  cratchit::KeyReader keys(terminal_encoding.in, [&terminal] { return terminal.read_key(); });
  cratchit::run_repl(repl, [&keys] { return keys.next(); }, std::cout);
  LOG_BUSINESS("cratchit quit");
  return 0;
}

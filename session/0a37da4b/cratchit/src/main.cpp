#include "core/dot_folder.hpp"
#include "core/log.hpp"
#include "core/terminal_encoding.hpp"

#include <cstdio>
#include <filesystem>
#include <print>

int main() {
  auto const cwd = std::filesystem::current_path();
  LOG_BUSINESS("cratchit startade i {}", cwd.string());
  auto const terminal_encoding = cratchit::choose_terminal_encoding(cratchit::report_terminal_encoding());
  for (auto const& warning : terminal_encoding.warnings) {
    std::println(stderr, "{}", warning);
    LOG_DESIGN_INSUFFICIENCY("{}", warning);
  }
  auto const root = cratchit::dot_folder_root();
  if (!root) {
    std::println(stderr, "Fel: {}", root.error());
    return 1;
  }
  auto const dot_folder = cratchit::open_dot_folder(*root, cratchit::today_iso());
  if (!dot_folder) {
    std::println(stderr, "Fel: {}", dot_folder.error());
    return 1;
  }
  // One log for every run, next to runtime.txt
  auto const log_file = dot_folder->runtime_file.parent_path() / "log.txt";
  if (auto const opened = cratchit::log::open(log_file); !opened) {
    std::println(stderr, "Varning: {}. Inget loggas.", opened.error());
  }
  for (auto const& entry : dot_folder->initiated) {
    std::println("Skapade {} i {}", entry.file.generic_string(), entry.folder.string());
    LOG_BUSINESS("Skapade {} i {}", entry.file.generic_string(), entry.folder.string());
  }
  LOG_BUSINESS("cratchit avslutades");
}

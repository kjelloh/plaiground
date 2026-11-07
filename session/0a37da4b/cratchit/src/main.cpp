#include "core/dot_folder.hpp"
#include "core/log.hpp"
#include "core/terminal_encoding.hpp"
#include "pyroceph/tui/key_reader.hpp"
#include "pyroceph/tui/prompt_loop.hpp"
#include "pyroceph/tui/raw_mode.hpp"

#include <cstdio>
#include <filesystem>
#include <optional>
#include <print>

namespace {

// The charset encoding to read terminal input with. choose_terminal_encoding
// only gives charsets cratchit can read; ASCII is read as its superset UTF-8.
charset::Encoding encoding_of(cratchit::Charset charset) {
  return charset == cratchit::Charset::Cp437 ? charset::Encoding::cp437 : charset::Encoding::utf8;
}

// No key is mapped to a choice yet
std::optional<pyroceph::tui::Choice> lookup_choice(pyroceph::tui::CodePoint) { return std::nullopt; }

} // namespace

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
  {
    pyroceph::tui::RawMode raw_mode;
    auto keys = pyroceph::tui::terminal_key_reader(encoding_of(terminal_encoding.in));
    std::println("Tryck en tangent i taget. Ctrl-C, Ctrl-D eller Ctrl-Z avslutar.");
    LOG_BUSINESS("Promptloopen startade");
    pyroceph::tui::run_prompt_loop(keys, lookup_choice, stdout);
    if (keys.failure_count() > 0) LOG_DESIGN_INSUFFICIENCY("{} tangenter kunde inte avkodas", keys.failure_count());
  }
  LOG_BUSINESS("cratchit avslutades");
}

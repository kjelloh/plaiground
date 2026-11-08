#include "core/dot_folder.hpp"
#include "core/log.hpp"
#include "core/terminal_encoding.hpp"
#include "pyroceph/tui/key_reader.hpp"
#include "pyroceph/tui/prompt_loop.hpp"
#include "pyroceph/tui/raw_mode.hpp"
#include "pyroceph/tui/state.hpp"
#include "ui/root_menu.hpp"

#include <cstdio>
#include <filesystem>
#include <string>
#include <print>

namespace {

// The charset encoding to read terminal input with. choose_terminal_encoding
// only gives charsets cratchit can read; ASCII is read as its superset UTF-8.
charset::Encoding encoding_of(cratchit::Charset charset) {
  return charset == cratchit::Charset::Cp437 ? charset::Encoding::cp437 : charset::Encoding::utf8;
}

} // namespace

int main() {
  auto const cwd = std::filesystem::current_path();
  LOG_BUSINESS("cratchit startade i {}", cwd.string());
  auto const terminal_encoding = cratchit::choose_terminal_encoding(cratchit::report_terminal_encoding());
  for (auto const& warning : terminal_encoding.warnings) {
    std::println(stderr, "{}", warning);
    LOG_DESIGN_INSUFFICIENCY("{}", warning);
  }
  // The framework comes with the installation, next to the program
  auto const program_folder = cratchit::program_folder();
  if (!program_folder) {
    std::println(stderr, "Fel: {}", program_folder.error());
    return 1;
  }
  auto const framework = cratchit::open_framework(*program_folder);
  if (!framework) {
    std::println(stderr, "Fel: {}", framework.error());
    return 1;
  }
  // The runtime is the user's own, in the home folder
  auto const home = cratchit::user_home();
  if (!home) {
    std::println(stderr, "Fel: {}", home.error());
    return 1;
  }
  auto const runtime = cratchit::open_runtime(*home, cratchit::today_iso());
  if (!runtime) {
    std::println(stderr, "Fel: {}", runtime.error());
    return 1;
  }
  // One log for every run, next to runtime.txt
  auto const log_file = runtime->file.parent_path() / "log.txt";
  if (auto const opened = cratchit::log::open(log_file); !opened) {
    std::println(stderr, "Varning: {}. Inget loggas.", opened.error());
  }
  LOG_BUSINESS("Framework {} från {}", framework->version, framework->file.string());
  for (auto const& entry : runtime->initiated) {
    std::println("Skapade {} i {}", entry.file.generic_string(), entry.folder.string());
    LOG_BUSINESS("Skapade {} i {}", entry.file.generic_string(), entry.folder.string());
  }
  {
    pyroceph::tui::RawMode raw_mode;
    auto keys = pyroceph::tui::terminal_key_reader(encoding_of(terminal_encoding.in));
    // The workspace is opened when the user chooses it (0 in the root)
    pyroceph::tui::Navigator navigator(cratchit::ui::make_root_menu(cratchit::default_workspace_folder()));
    std::println("Tryck ? för gemensamma val.");
    LOG_BUSINESS("Promptloopen startade");
    pyroceph::tui::run_prompt_loop(keys, navigator, stdout);
    if (keys.failure_count() > 0) LOG_DESIGN_INSUFFICIENCY("{} tangenter kunde inte avkodas", keys.failure_count());
  }
  LOG_BUSINESS("cratchit avslutades");
}

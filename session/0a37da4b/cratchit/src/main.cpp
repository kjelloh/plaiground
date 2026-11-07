#include "core/dot_folder.hpp"
#include "core/log.hpp"
#include "core/terminal_encoding.hpp"
#include "pyroceph/tui/key_reader.hpp"
#include "pyroceph/tui/prompt_loop.hpp"
#include "pyroceph/tui/raw_mode.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstdio>
#include <algorithm>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <print>

namespace {

// The charset encoding to read terminal input with. choose_terminal_encoding
// only gives charsets cratchit can read; ASCII is read as its superset UTF-8.
charset::Encoding encoding_of(cratchit::Charset charset) {
  return charset == cratchit::Charset::Cp437 ? charset::Encoding::cp437 : charset::Encoding::utf8;
}

// Demo: Bokslut takes a year of four digits
pyroceph::tui::InputResult bokslut_year(std::u32string_view text) {
  auto const is_digit = [](char32_t c) { return c >= U'0' && c <= U'9'; };
  if (text.size() != 4 || !std::ranges::all_of(text, is_digit)) {
    return std::unexpected(std::string("Ange ett år med fyra siffror, till exempel 2025."));
  }
  std::string year;
  for (auto const c : text) year.push_back(static_cast<char>(c));
  return std::format("Bokslut för {} noterat.", year);
}

// A first tree of states to navigate. The choices only move between states.
pyroceph::tui::State make_menu() {
  using pyroceph::tui::State;
  return State{"Huvudmeny",
               {
                   {U'1', "Projekt", State{"Projekt", {{U'1', "Öppna", State{"Öppna projekt"}}}}},
                   {U'2', "Bokslut", State{"Bokslut", {}, bokslut_year}},
               }};
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
    auto const menu = make_menu();
    pyroceph::tui::Navigator navigator(menu);
    std::println("Tryck ? för gemensamma val.");
    LOG_BUSINESS("Promptloopen startade");
    pyroceph::tui::run_prompt_loop(keys, navigator, stdout);
    if (keys.failure_count() > 0) LOG_DESIGN_INSUFFICIENCY("{} tangenter kunde inte avkodas", keys.failure_count());
  }
  LOG_BUSINESS("cratchit avslutades");
}

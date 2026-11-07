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
#include <memory>
#include <string>
#include <string_view>
#include <print>

namespace {

// The charset encoding to read terminal input with. choose_terminal_encoding
// only gives charsets cratchit can read; ASCII is read as its superset UTF-8.
charset::Encoding encoding_of(cratchit::Charset charset) {
  return charset == cratchit::Charset::Cp437 ? charset::Encoding::cp437 : charset::Encoding::utf8;
}

using pyroceph::tui::CodePoint;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;

// Demo states. Each state creates the next one when the user goes there.

class OppnaProjektState : public State {
public:
  std::string name() const override { return "Öppna projekt"; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
};

class ProjektState : public State {
public:
  std::string name() const override { return "Projekt"; }
  OptionsList options() const override { return {{U'1', "Öppna"}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return std::make_unique<OppnaProjektState>();
    return nullptr;
  }
};

// Takes a year of four digits
class BokslutState : public State {
public:
  std::string name() const override { return "Bokslut"; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  InputResult on_input(std::u32string_view text) override {
    auto const is_digit = [](char32_t c) { return c >= U'0' && c <= U'9'; };
    if (text.size() != 4 || !std::ranges::all_of(text, is_digit)) {
      return std::unexpected(std::string("Ange ett år med fyra siffror, till exempel 2025."));
    }
    std::string year;
    for (auto const c : text) year.push_back(static_cast<char>(c));
    return std::format("Bokslut för {} noterat.", year);
  }
};

class HuvudmenyState : public State {
public:
  std::string name() const override { return "Huvudmeny"; }
  OptionsList options() const override { return {{U'1', "Projekt"}, {U'2', "Bokslut"}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return std::make_unique<ProjektState>();
    if (key == U'2') return std::make_unique<BokslutState>();
    return nullptr;
  }
};

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
    pyroceph::tui::Navigator navigator(std::make_unique<HuvudmenyState>());
    std::println("Tryck ? för gemensamma val.");
    LOG_BUSINESS("Promptloopen startade");
    pyroceph::tui::run_prompt_loop(keys, navigator, stdout);
    if (keys.failure_count() > 0) LOG_DESIGN_INSUFFICIENCY("{} tangenter kunde inte avkodas", keys.failure_count());
  }
  LOG_BUSINESS("cratchit avslutades");
}

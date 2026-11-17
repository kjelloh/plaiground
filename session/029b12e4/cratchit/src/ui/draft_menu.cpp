#include "ui/draft_menu.hpp"

#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/log.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/design_insufficiency.hpp"
#include "ui/help_topics.hpp"
#include "ui/year_view.hpp"

#include <expected>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

namespace cratchit::ui {

namespace {

namespace fs = std::filesystem;

using pyroceph::tui::Accepted;
using pyroceph::tui::CodePoint;
using pyroceph::tui::Completion;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

class ReadStatementState : public State {
public:
  ReadStatementState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) {}
  std::string name() const override { return "Läs kontoutdrag"; }
  View view() const override {
    return {"Skriv sökvägen till kontoutdraget och tryck Enter. Tab fyller i mappar och filer."};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  // The folder of the file chosen last, so that going back offers its neighbours
  std::u32string start_input() const override {
    return pyroceph::tui::path_input_of(chosen_folder_.value_or(project_.folder));
  }
  // All folders and files: cratchit does not judge what a statement may be
  Completion complete(std::u32string_view text) const override { return pyroceph::tui::complete_path(text, true); }
  InputResult on_input(std::u32string_view text) override {
    auto const file = pyroceph::tui::path_of(text);
    if (file.empty()) return std::unexpected(std::string("Skriv sökvägen till kontoutdraget."));
    std::error_code ec;
    if (fs::is_directory(file, ec)) {
      return std::unexpected(std::format("{} är en mapp. Välj en fil i den med Tab.", file.string()));
    }
    if (!fs::is_regular_file(file, ec)) return std::unexpected(std::format("Det finns ingen fil {}.", file.string()));
    chosen_folder_ = file.parent_path();
    LOG_BUSINESS("Kontoutdrag valt för {} i {}: {}", label_of(year_), project_.folder.string(), file.string());
    return Accepted(make_design_insufficiency_state("Läs kontoutdrag", "att läsa in kontoutdrag"));
  }
  std::string help_topic() const override { return std::string(help_topic::read_statement); }

private:
  Project project_;
  FiscalYear year_;
  std::optional<fs::path> chosen_folder_;
};

class DraftState : public State {
public:
  DraftState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) { read(); }
  std::string name() const override { return "Utkast"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    if (count_ == 0) return {std::format("Utkast {}: inga utkast.", label_of(year_))};
    return {std::format("Utkast {}: {} utkast.", label_of(year_), count_)};
  }
  OptionsList options() const override { return {{U'0', "Läs kontoutdrag"}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'0') return make_read_statement_menu(project_, year_);
    return nullptr;
  }
  void on_return() override { read(); }
  std::string help_topic() const override { return std::string(help_topic::drafts); }

private:
  void read() {
    error_.clear();
    count_ = 0;
    auto const books = books::read_books(project_.folder);
    if (!books) {
      LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project_.folder.string(), books.error());
      error_ = books.error();
      return;
    }
    auto const start = parse_iso_date(year_.start);
    auto const* year = start ? books->year_starting(*start) : nullptr;
    if (year != nullptr) count_ = year->drafts.size();
  }

  Project project_;
  FiscalYear year_;
  std::size_t count_ = 0;
  std::string error_;
};

} // namespace

std::unique_ptr<State> make_draft_menu(Project project, FiscalYear year) {
  return std::make_unique<DraftState>(std::move(project), std::move(year));
}

std::unique_ptr<State> make_read_statement_menu(Project project, FiscalYear year) {
  return std::make_unique<ReadStatementState>(std::move(project), std::move(year));
}

} // namespace cratchit::ui

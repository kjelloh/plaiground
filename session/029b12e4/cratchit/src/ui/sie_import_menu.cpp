#include "ui/sie_import_menu.hpp"

#include "core/date.hpp"
#include "core/log.hpp"
#include "core/sie_import.hpp"
#include "core/sie_project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/help_topics.hpp"

#include <algorithm>
#include <format>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::Accepted;
using pyroceph::tui::CodePoint;
using pyroceph::tui::Completion;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

using Report = std::function<void(std::string, FiscalYear)>;

// "SIE-fil: <file>", then "(<program>, skapad <day>)" with what the file tells
std::string file_line(SieImportPlan const& plan) {
  auto line = std::format("SIE-fil: {}", plan.sie_file.string());
  if (!plan.program.empty() && !plan.generated.empty()) return line + std::format(" ({}, skapad {})", plan.program, plan.generated);
  if (!plan.program.empty()) return line + std::format(" ({})", plan.program);
  if (!plan.generated.empty()) return line + std::format(" (skapad {})", plan.generated);
  return line;
}

// "1 ny verifikation", "2 nya verifikationer"
std::string new_vouchers_text(std::size_t count) {
  return count == 1 ? "1 ny verifikation" : std::format("{} nya verifikationer", count);
}

class ConfirmImportState : public State {
public:
  ConfirmImportState(Project project, SieImportPlan plan, State const* origin, Report report)
      : project_(std::move(project)), plan_(std::move(plan)), origin_(origin), report_(std::move(report)) {}
  std::string name() const override { return "Bekräfta"; }
  View view() const override {
    if (imported_) return {"cratchit har importerat SIE-filen."};
    auto const& import = plan_.import;
    View lines;
    lines.push_back(file_line(plan_));
    if (!plan_.alert.empty()) lines.push_back(plan_.alert);
    if (plan_.expected) {
      lines.push_back(std::format("Räkenskapsår: {} – {} (cratchit räknade med {} – {})", format_iso_date(import.year_start),
                                  format_iso_date(import.year_end), plan_.expected->start, plan_.expected->end));
      lines.push_back("cratchit kontrollerar inte Skatteverkets godkännande av en omläggning. Du ansvarar för att "
                      "räkenskapsåret är korrekt.");
    } else {
      lines.push_back(std::format("Räkenskapsår: {} – {}", format_iso_date(import.year_start), format_iso_date(import.year_end)));
    }
    lines.push_back(std::format("Verifikationer: {} {}, {} {}, {} finns redan", import.vouchers_added,
                                import.vouchers_added == 1 ? "ny" : "nya", plan_.changed.size(),
                                plan_.changed.size() == 1 ? "ändrad" : "ändrade", import.vouchers_known));
    if (!plan_.changed.empty()) {
      lines.push_back("Ändrade (strukna och tillagda rader):");
      for (std::size_t i = 0; i < shown(); ++i) lines.push_back("  " + change_text(plan_.changed[i]));
      if (shown() < plan_.changed.size()) {
        lines.push_back(std::format("  … och {} till. Tryck 2 för fler.", plan_.changed.size() - shown()));
      }
    }
    lines.push_back(std::format("Konton: {} nya, {} med ändrat namn eller ändrade uppgifter", import.accounts_added,
                                import.accounts_changed));
    if (import.identity_changed) lines.push_back(std::format("Bokföringen får organisationsnummer {}.", import.identity.orgnr));
    lines.push_back(std::format("cratchit flyttar SIE-filen till {}.", (project_.folder / imported_sie_folder_name).string()));
    return lines;
  }
  OptionsList options() const override {
    if (imported_) return {};
    OptionsList result{{U'1', "Importera"}};
    if (shown() < plan_.changed.size()) result.push_back({more_key, "Mer…"});
    return result;
  }
  // Mer… shows one page more of the changed vouchers
  bool on_key_in_place(CodePoint key) override {
    if (imported_ || key != more_key || shown() >= plan_.changed.size()) return false;
    pages_ += 1;
    return true;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (imported_ || key != U'1') return nullptr;
    auto const imported = import_sie(project_, plan_);
    if (!imported) {
      LOG_BUSINESS("Kunde inte importera {} i {}: {}", plan_.sie_file.string(), project_.folder.string(), imported.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte importera SIE-filen: {}", imported.error()));
    }
    imported_ = true;
    LOG_BUSINESS("Importerade {} i projektet {} ({}): räkenskapsåret {} – {}, {} nya verifikationer, {} nya konton; filen "
                 "flyttad till {}",
                 plan_.sie_file.string(), project_.name, project_.uuid, format_iso_date(plan_.import.year_start),
                 format_iso_date(plan_.import.year_end), plan_.import.vouchers_added, plan_.import.accounts_added,
                 imported->moved_to.string());
    for (auto const& changed : plan_.changed) LOG_BUSINESS("SIE-import ändrade verifikationen {}", change_text(changed));
    for (auto const& note : plan_.import.notes) LOG_BUSINESS("SIE-import: {}", note);
    if (report_) {
      FiscalYear const year{format_iso_date(plan_.import.year_start), format_iso_date(plan_.import.year_end)};
      report_(std::format("cratchit importerade {}: {} och {} {} i räkenskapsåret {} – {}. Filen ligger nu i {}.",
                          plan_.sie_file.filename().string(), new_vouchers_text(plan_.import.vouchers_added),
                          plan_.changed.size(), plan_.changed.size() == 1 ? "ändrad" : "ändrade", year.start, year.end,
                          imported_sie_folder_name),
              year);
    }
    return nullptr; // back to the fiscal year (back_to)
  }
  State const* back_to() const override { return imported_ ? origin_ : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::confirm_import); }

private:
  // The changed vouchers are listed a page at a time; Mer… shows one page more
  static constexpr std::size_t page = 10;
  static constexpr CodePoint more_key = U'2';
  std::size_t shown() const { return std::min(plan_.changed.size(), page * pages_); }

  Project project_;
  SieImportPlan plan_;
  State const* origin_;
  Report report_;
  std::size_t pages_ = 1;
  bool imported_ = false;
};

class ImportSieState : public State {
public:
  ImportSieState(Project project, FiscalYear year, State const* origin, Report report)
      : project_(std::move(project)), year_(std::move(year)), origin_(origin), report_(std::move(report)) {}
  std::string name() const override { return "Importera från SIE"; }
  View view() const override { return {"Skriv sökvägen till SIE-filen och tryck Enter. Tab fyller i mappar och SIE-filer."}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::u32string start_input() const override { return pyroceph::tui::path_input_of(project_.folder); }
  Completion complete(std::u32string_view text) const override {
    return pyroceph::tui::complete_path(text, true, [](std::filesystem::path const& file) { return is_sie_file_name(file); });
  }
  InputResult on_input(std::u32string_view text) override {
    auto const file = pyroceph::tui::path_of(text);
    auto plan = plan_sie_import(project_, file, year_);
    if (!plan) {
      LOG_BUSINESS("Kan inte importera {} i {}: {}", file.string(), project_.folder.string(), plan.error());
      return std::unexpected(std::format("cratchit kan inte importera filen: {}", plan.error()));
    }
    return Accepted(std::make_unique<ConfirmImportState>(project_, std::move(*plan), origin_, report_));
  }
  std::string help_topic() const override { return std::string(help_topic::import_sie); }

private:
  Project project_;
  FiscalYear year_;
  State const* origin_;
  Report report_;
};

} // namespace

std::unique_ptr<State> make_sie_import_menu(Project project, FiscalYear year, State const* origin, Report report) {
  return std::make_unique<ImportSieState>(std::move(project), std::move(year), origin, std::move(report));
}

} // namespace cratchit::ui

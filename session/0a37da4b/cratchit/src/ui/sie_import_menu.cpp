#include "ui/sie_import_menu.hpp"

#include "core/date.hpp"
#include "core/log.hpp"
#include "core/sie_import.hpp"
#include "core/sie_project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/help_topics.hpp"

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

using Report = std::function<void(std::string)>;

class ConfirmImportState : public State {
public:
  ConfirmImportState(Project project, SieImportPlan plan, State const* origin, Report report)
      : project_(std::move(project)), plan_(std::move(plan)), origin_(origin), report_(std::move(report)) {}
  std::string name() const override { return "Bekräfta"; }
  View view() const override {
    if (imported_) return {"cratchit har importerat SIE-filen."};
    auto const& import = plan_.import;
    View lines;
    lines.push_back(plan_.generated.empty()
                        ? std::format("SIE-fil: {} ({})", plan_.sie_file.string(), plan_.program)
                        : std::format("SIE-fil: {} ({}, skapad {})", plan_.sie_file.string(), plan_.program, plan_.generated));
    if (!plan_.alert.empty()) lines.push_back(plan_.alert);
    lines.push_back(std::format("Räkenskapsår: {} – {} ({})", format_iso_date(import.year_start), format_iso_date(import.year_end),
                                plan_.new_year ? "nytt i bokföringen" : "finns i bokföringen"));
    lines.push_back(std::format("Verifikationer: {} nya, {} finns redan", import.vouchers_added, import.vouchers_known));
    lines.push_back(std::format("Konton: {} nya, {} med ändrat namn eller ändrade uppgifter", import.accounts_added,
                                import.accounts_changed));
    if (import.identity_changed) lines.push_back(std::format("Bokföringen får organisationsnummer {}.", import.identity.orgnr));
    lines.push_back(std::format("cratchit flyttar SIE-filen till {}.", (project_.folder / imported_sie_folder_name).string()));
    return lines;
  }
  OptionsList options() const override {
    if (imported_) return {};
    return {{U'1', "Importera"}};
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
    for (auto const& note : plan_.import.notes) LOG_BUSINESS("SIE-import: {}", note);
    if (report_) {
      report_(std::format("cratchit importerade {}: {} nya verifikationer i räkenskapsåret {} – {}. Filen ligger nu i {}.",
                          plan_.sie_file.filename().string(), plan_.import.vouchers_added,
                          format_iso_date(plan_.import.year_start), format_iso_date(plan_.import.year_end),
                          imported_sie_folder_name));
    }
    return nullptr; // back to the project (back_to)
  }
  State const* back_to() const override { return imported_ ? origin_ : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::confirm_import); }

private:
  Project project_;
  SieImportPlan plan_;
  State const* origin_;
  Report report_;
  bool imported_ = false;
};

class ImportSieState : public State {
public:
  ImportSieState(Project project, State const* origin, Report report)
      : project_(std::move(project)), origin_(origin), report_(std::move(report)) {}
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
    auto plan = plan_sie_import(project_, file);
    if (!plan) {
      LOG_BUSINESS("Kan inte importera {} i {}: {}", file.string(), project_.folder.string(), plan.error());
      return std::unexpected(std::format("cratchit kan inte importera filen: {}", plan.error()));
    }
    return Accepted(std::make_unique<ConfirmImportState>(project_, std::move(*plan), origin_, report_));
  }
  std::string help_topic() const override { return std::string(help_topic::import_sie); }

private:
  Project project_;
  State const* origin_;
  Report report_;
};

} // namespace

std::unique_ptr<State> make_sie_import_menu(Project project, State const* origin, Report report) {
  return std::make_unique<ImportSieState>(std::move(project), origin, std::move(report));
}

} // namespace cratchit::ui

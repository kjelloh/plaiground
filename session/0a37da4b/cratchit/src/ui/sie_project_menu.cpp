#include "ui/sie_project_menu.hpp"

#include "core/log.hpp"
#include "core/project.hpp"
#include "core/sie_project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/help_topics.hpp"

#include <format>
#include <optional>
#include <string>
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

std::string date_text(Date const& date) { return format_iso_date(date); }

class ConfirmSieProjectState : public State {
public:
  ConfirmSieProjectState(Workspace workspace, SieProjectPlan plan, State const* origin)
      : workspace_(std::move(workspace)), plan_(std::move(plan)), origin_(origin) {}
  std::string name() const override { return "Bekräfta"; }
  View view() const override {
    if (created_) return {std::format("cratchit har skapat bokföringen i {}.", plan_.folder.string())};
    View lines;
    lines.push_back(std::format("SIE-fil: {}", plan_.sie_file.string()));
    if (!plan_.alert.empty()) lines.push_back(plan_.alert);
    lines.push_back(plan_.orgnr.empty() ? std::format("Företag: {}", plan_.name)
                                        : std::format("Företag: {} ({})", plan_.name, plan_.orgnr));
    lines.push_back(std::format("Räkenskapsår: {} – {}", date_text(plan_.import.year_start), date_text(plan_.import.year_end)));
    lines.push_back(std::format("Verifikationer: {}", plan_.import.vouchers_added));
    lines.push_back(std::format("Konton: {}", plan_.import.books.accounts.size()));
    lines.push_back(std::format("cratchit skapar bokföringen i {} och flyttar dit SIE-filen.", plan_.folder.string()));
    return lines;
  }
  OptionsList options() const override {
    if (created_) return {};
    return {{U'1', "Skapa bokföringen"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (created_ || key != U'1') return nullptr;
    auto project = create_project_from_sie(plan_, today_iso());
    if (!project) {
      LOG_BUSINESS("Kunde inte skapa bokföringen från {}: {}", plan_.sie_file.string(), project.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte skapa bokföringen: {}", project.error()));
    }
    created_ = true;
    LOG_BUSINESS("Skapade projektet {} i {} från {} ({} verifikationer)", project->name, project->folder.string(),
                 plan_.sie_file.string(), plan_.import.vouchers_added);
    for (auto const& note : plan_.import.notes) LOG_BUSINESS("SIE-import: {}", note);
    if (auto const remembered = remember_project(workspace_, *project); !remembered) {
      LOG_BUSINESS("Samlingsmappen kunde inte komma ihåg projektet: {}", remembered.error());
      return pyroceph::tui::make_error_state(
          std::format("cratchit skapade bokföringen men kunde inte skriva in den i samlingsmappen: {}", remembered.error()));
    }
    return nullptr; // back to where the user started (back_to)
  }
  State const* back_to() const override { return created_ ? origin_ : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::confirm_sie_project); }

private:
  Workspace workspace_;
  SieProjectPlan plan_;
  State const* origin_; // where the user started, gone back to once created
  bool created_ = false;
};

class FromSieState : public State {
public:
  FromSieState(Workspace workspace, std::filesystem::path start_folder, State const* origin)
      : workspace_(std::move(workspace)), start_folder_(std::move(start_folder)), origin_(origin) {}
  std::string name() const override { return "Från SIE"; }
  View view() const override {
    return {"Skriv sökvägen till SIE-filen och tryck Enter. Tab fyller i mappar och SIE-filer."};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::u32string start_input() const override { return pyroceph::tui::path_input_of(start_folder_); }
  Completion complete(std::u32string_view text) const override {
    return pyroceph::tui::complete_path(text, true, [](std::filesystem::path const& file) { return is_sie_file_name(file); });
  }
  InputResult on_input(std::u32string_view text) override {
    auto const file = pyroceph::tui::path_of(text);
    auto plan = plan_project_from_sie(file);
    if (!plan) {
      LOG_BUSINESS("Kan inte skapa bokföring från {}: {}", file.string(), plan.error());
      return std::unexpected(std::format("cratchit kan inte skapa bokföringen: {}", plan.error()));
    }
    return Accepted(std::make_unique<ConfirmSieProjectState>(workspace_, std::move(*plan), origin_));
  }
  std::string help_topic() const override { return std::string(help_topic::from_sie); }

private:
  Workspace workspace_;
  std::filesystem::path start_folder_;
  State const* origin_;
};

} // namespace

std::unique_ptr<State> make_project_from_sie(Workspace workspace, std::filesystem::path start_folder,
                                             State const* origin) {
  return std::make_unique<FromSieState>(std::move(workspace), std::move(start_folder), origin);
}

} // namespace cratchit::ui

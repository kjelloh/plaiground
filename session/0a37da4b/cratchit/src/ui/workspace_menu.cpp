#include "ui/workspace_menu.hpp"

#include "ui/help_topics.hpp"
#include "core/log.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "ui/add_project_menu.hpp"
#include "ui/project_menu.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

class WorkspaceState : public State {
public:
  WorkspaceState(Workspace workspace, std::vector<ListedProject> projects)
      : workspace_(std::move(workspace)), projects_(std::move(projects)) {}
  std::string name() const override { return "Samlingsmapp"; }
  View view() const override {
    View lines;
    if (!workspace_.initiated.empty()) lines.push_back("cratchit skapade en ny samlingsmapp.");
    lines.push_back(workspace_.folder.string());
    if (projects_.size() > max_listed_projects) lines.push_back("Samlingsmappen har fler bokföringar än cratchit visar här.");
    return lines;
  }
  OptionsList options() const override {
    OptionsList result;
    for (std::size_t i = 0; i < listed(); ++i) result.push_back({key_of(i), projects_[i].caption});
    result.push_back({add_key, "Lägg till bokföring"});
    return result;
  }
  OnKeyResult on_key(CodePoint key) override {
    for (std::size_t i = 0; i < listed(); ++i) {
      if (key == key_of(i)) return open(projects_[i]);
    }
    if (key == add_key) return make_add_project_menu(workspace_, workspace_.folder, this);
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::workspace); }

  // A project may have been added on the way back here: read the list again
  void on_return() override {
    auto projects = list_projects(workspace_);
    if (!projects) {
      LOG_BUSINESS("cratchit kunde inte läsa om samlingsmappen: {}", projects.error());
      return;
    }
    projects_ = std::move(*projects);
  }

private:
  std::size_t listed() const { return std::min(projects_.size(), max_listed_projects); }
  static CodePoint key_of(std::size_t index) { return static_cast<CodePoint>(U'1' + index); }

  static OnKeyResult open(ListedProject const& listed) {
    auto project = open_project(listed.folder, listed.uuid);
    if (!project) {
      LOG_BUSINESS("Kunde inte öppna projektet i {}: {}", listed.folder.string(), project.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte öppna bokföringen: {}", project.error()));
    }
    LOG_BUSINESS("Öppnade projektet {} i {}", project->name, project->folder.string());
    return make_project_menu(std::move(*project));
  }

  Workspace workspace_;
  std::vector<ListedProject> projects_;
};

} // namespace

std::unique_ptr<State> make_workspace_menu(Workspace workspace, std::vector<ListedProject> projects) {
  return std::make_unique<WorkspaceState>(std::move(workspace), std::move(projects));
}

} // namespace cratchit::ui

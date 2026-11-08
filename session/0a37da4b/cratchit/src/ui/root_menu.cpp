#include "ui/root_menu.hpp"

#include "core/dot_folder.hpp"
#include "core/log.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "ui/test_menu.hpp"
#include "ui/workspace_menu.hpp"

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

OnKeyResult cannot_open_workspace(std::string const& reason) {
  LOG_BUSINESS("cratchit kunde inte öppna samlingsmappen: {}", reason);
  return pyroceph::tui::make_error_state(std::format("cratchit kunde inte öppna samlingsmappen: {}", reason));
}

class RootState : public State {
public:
  explicit RootState(WorkspaceFolder workspace_folder) : workspace_folder_(std::move(workspace_folder)) {}
  std::string name() const override { return "Cratchit"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'0', "Samlingsmapp"}, {U'9', "Testmeny"}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'0') return open();
    if (key == U'9') return make_test_menu();
    return nullptr;
  }

private:
  OnKeyResult open() const {
    if (!workspace_folder_) return cannot_open_workspace(workspace_folder_.error());
    auto workspace = open_workspace(*workspace_folder_, today_iso());
    if (!workspace) return cannot_open_workspace(workspace.error());
    for (auto const& entry : workspace->initiated) {
      LOG_BUSINESS("Skapade {} i {}", entry.file.generic_string(), entry.folder.string());
    }
    auto projects = list_projects(*workspace);
    if (!projects) return cannot_open_workspace(projects.error());
    LOG_BUSINESS("Öppnade samlingsmappen {} med {} projekt", workspace->folder.string(), projects->size());
    return make_workspace_menu(std::move(*workspace), std::move(*projects));
  }

  WorkspaceFolder workspace_folder_;
};

} // namespace

std::unique_ptr<State> make_root_menu(WorkspaceFolder workspace_folder) {
  return std::make_unique<RootState>(std::move(workspace_folder));
}

} // namespace cratchit::ui

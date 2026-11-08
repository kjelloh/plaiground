#include "ui/workspace_menu.hpp"

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
  explicit WorkspaceState(Workspace workspace) : workspace_(std::move(workspace)) {}
  std::string name() const override { return "Samlingsmapp"; }
  View view() const override {
    View lines;
    if (!workspace_.initiated.empty()) lines.push_back("Ny samlingsmapp skapad.");
    lines.push_back(workspace_.folder.string());
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }

private:
  Workspace workspace_;
};

} // namespace

std::unique_ptr<State> make_workspace_menu(Workspace workspace) {
  return std::make_unique<WorkspaceState>(std::move(workspace));
}

} // namespace cratchit::ui

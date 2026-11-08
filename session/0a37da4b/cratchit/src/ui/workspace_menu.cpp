#include "ui/workspace_menu.hpp"

#include <string>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

class WorkspaceState : public State {
public:
  std::string name() const override { return "Samlingsmapp"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
};

} // namespace

std::unique_ptr<State> make_workspace_menu() { return std::make_unique<WorkspaceState>(); }

} // namespace cratchit::ui

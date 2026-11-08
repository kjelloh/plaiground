#include "ui/add_project_menu.hpp"

#include <string>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

// Opens an existing project the workspace does not list yet. Empty for now.
class OpenExistingProjectState : public State {
public:
  std::string name() const override { return "Öppna befintlig"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
};

// Creates a new project in the workspace. Empty for now.
class NewProjectState : public State {
public:
  std::string name() const override { return "Ny bokföring"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
};

class AddProjectState : public State {
public:
  std::string name() const override { return "Lägg till"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'1', "Öppna befintlig"}, {U'2', "Ny bokföring"}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return std::make_unique<OpenExistingProjectState>();
    if (key == U'2') return std::make_unique<NewProjectState>();
    return nullptr;
  }
};

} // namespace

std::unique_ptr<State> make_add_project_menu() { return std::make_unique<AddProjectState>(); }

} // namespace cratchit::ui

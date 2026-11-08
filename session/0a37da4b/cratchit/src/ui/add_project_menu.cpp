#include "ui/add_project_menu.hpp"

#include "ui/design_insufficiency.hpp"

#include <string>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

// The captions, used both in the menu and for the states they lead to
constexpr char const* open_existing_caption = "Öppna befintlig";
constexpr char const* new_project_caption = "Ny bokföring";

class AddProjectState : public State {
public:
  std::string name() const override { return "Lägg till"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'1', open_existing_caption}, {U'2', new_project_caption}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') {
      return make_design_insufficiency_state(open_existing_caption, "att öppna en bokföring som ligger utanför listan");
    }
    if (key == U'2') return make_design_insufficiency_state(new_project_caption, "att skapa en ny bokföring");
    return nullptr;
  }
};

} // namespace

std::unique_ptr<State> make_add_project_menu() { return std::make_unique<AddProjectState>(); }

} // namespace cratchit::ui

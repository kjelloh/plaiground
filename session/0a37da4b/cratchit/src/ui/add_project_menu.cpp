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
constexpr char const* from_sie_caption = "Från SIE";
constexpr char const* from_details_caption = "Från uppgifter";

// Creates a new project, from an SIE file or from the details the user enters
// (name, organisation number and fiscal year). Neither is there yet.
class NewProjectState : public State {
public:
  std::string name() const override { return new_project_caption; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'1', from_sie_caption}, {U'2', from_details_caption}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return make_design_insufficiency_state(from_sie_caption, "att skapa en ny bokföring från en SIE-fil");
    if (key == U'2') {
      return make_design_insufficiency_state(
          from_details_caption, "att skapa en ny bokföring från namn, organisationsnummer och räkenskapsår");
    }
    return nullptr;
  }
};

class AddProjectState : public State {
public:
  std::string name() const override { return "Lägg till"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'1', open_existing_caption}, {U'2', new_project_caption}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') {
      return make_design_insufficiency_state(open_existing_caption, "att öppna en bokföring som ligger utanför listan");
    }
    if (key == U'2') return std::make_unique<NewProjectState>();
    return nullptr;
  }
};

} // namespace

std::unique_ptr<State> make_add_project_menu() { return std::make_unique<AddProjectState>(); }

} // namespace cratchit::ui

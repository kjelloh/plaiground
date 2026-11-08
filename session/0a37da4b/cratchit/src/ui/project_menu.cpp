#include "ui/project_menu.hpp"

#include "ui/design_insufficiency.hpp"
#include "ui/help_topics.hpp"

#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

// The captions, used both in the menu and for the states they lead to
constexpr char const* fiscal_years_caption = "Räkenskapsår";
constexpr char const* quarters_caption = "Kvartal";

class ProjectState : public State {
public:
  explicit ProjectState(Project project) : project_(std::move(project)) {}
  std::string name() const override { return project_.name; }
  View view() const override { return {project_.folder.string()}; }
  OptionsList options() const override { return {{U'1', fiscal_years_caption}, {U'2', quarters_caption}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return make_design_insufficiency_state(fiscal_years_caption, "att välja räkenskapsår i bokföringen");
    if (key == U'2') return make_design_insufficiency_state(quarters_caption, "att välja kvartal i bokföringen");
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::project); }

private:
  Project project_;
};

} // namespace

std::unique_ptr<State> make_project_menu(Project project) {
  return std::make_unique<ProjectState>(std::move(project));
}

} // namespace cratchit::ui

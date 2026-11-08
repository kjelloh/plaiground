#include "ui/project_menu.hpp"

#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

class ProjectState : public State {
public:
  explicit ProjectState(Project project) : project_(std::move(project)) {}
  std::string name() const override { return project_.name; }
  View view() const override { return {project_.folder.string()}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }

private:
  Project project_;
};

} // namespace

std::unique_ptr<State> make_project_menu(Project project) {
  return std::make_unique<ProjectState>(std::move(project));
}

} // namespace cratchit::ui

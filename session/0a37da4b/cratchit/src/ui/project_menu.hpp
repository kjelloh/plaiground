#pragma once

#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// An opened project. Its name is the project's name, so the path reads e.g.
// "Cratchit > Samlingsmapp > The ITfied AB". Its view shows the folder. Its
// options are 1 "Räkenskapsår" and 2 "Kvartal", separate since a quarter may
// span two fiscal years. Neither is there yet: they lead to the design
// insufficiency state (see design_insufficiency.hpp). No input. Reached by a
// digit from the workspace.
std::unique_ptr<pyroceph::tui::State> make_project_menu(Project project);

} // namespace cratchit::ui

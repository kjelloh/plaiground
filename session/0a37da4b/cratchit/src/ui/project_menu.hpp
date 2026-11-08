#pragma once

#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// An opened project. Its name is the project's name, so the path reads e.g.
// "Cratchit > Samlingsmapp > The ITfied AB". Its view shows the folder. No
// options and no input yet, so only the common keys work there and - goes back.
// Reached by a digit from the workspace.
std::unique_ptr<pyroceph::tui::State> make_project_menu(Project project);

} // namespace cratchit::ui

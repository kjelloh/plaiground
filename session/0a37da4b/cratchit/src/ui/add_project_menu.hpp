#pragma once

#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// The key that adds to the list a state shows, e.g. a project to the workspace
inline constexpr pyroceph::tui::CodePoint add_key = U'+';

// Adds a project to the workspace, named "Lägg till" in the path: 1 opens an
// existing project that the workspace does not list yet ("Öppna befintlig"),
// 2 creates a new one ("Ny bokföring"). Both lead to empty states for now. No
// input. Reached by + from the workspace.
std::unique_ptr<pyroceph::tui::State> make_add_project_menu();

} // namespace cratchit::ui

#pragma once

#include "core/dot_folder.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>
#include <vector>

namespace cratchit::ui {

// The most projects the workspace offers, as the keys 1..9
inline constexpr std::size_t max_listed_projects = 9;

// The opened workspace, named "Samlingsmapp" in the UI. Its view shows the
// folder (and that cratchit created it, when it did). Its options are the projects
// it remembers (see list_projects), 1..9 in the order given, and last
// "+ Lägg till bokföring" (see add_project_menu.hpp). With more than 9
// projects the first 9 are offered and the view says so. A digit opens its project (see project_menu.hpp), or leads to the
// error state when it cannot be opened. No input. Reached by 0 from the root.
std::unique_ptr<pyroceph::tui::State> make_workspace_menu(Workspace workspace, std::vector<ListedProject> projects);

} // namespace cratchit::ui

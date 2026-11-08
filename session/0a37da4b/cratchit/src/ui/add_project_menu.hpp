#pragma once

#include "core/dot_folder.hpp"
#include "pyroceph/tui/state.hpp"

#include <filesystem>
#include <memory>

namespace cratchit::ui {

// The key that adds to the list a state shows, e.g. a project to the workspace
inline constexpr pyroceph::tui::CodePoint add_key = U'+';

// Adds a project to the workspace, named "Lägg till" in the path:
// * 1 "Öppna befintlig" opens an existing project that the workspace does not
//   list yet (see make_open_existing_project).
// * 2 "Ny bokföring" creates a new one, 1 from an SIE file ("Från SIE") or 2
//   from name, organisation number and fiscal year ("Från uppgifter"). Neither
//   is there yet: they lead to the design insufficiency state (see design_insufficiency.hpp).
// No input. Reached by + from the workspace. start_folder is where the user
// starts typing the path of an existing project.
std::unique_ptr<pyroceph::tui::State> make_add_project_menu(Workspace workspace, std::filesystem::path start_folder);

// Opens an existing project, named "Öppna befintlig" in the path. The user
// types the path of the project folder, starting from start_folder, and Tab
// completes folder names. Enter opens the project there (see open_project),
// makes the workspace remember it (see remember_project) and leads to the
// project (see project_menu.hpp). When it cannot be opened the reason is shown
// and the path is kept, to be corrected. No options.
std::unique_ptr<pyroceph::tui::State> make_open_existing_project(Workspace workspace, std::filesystem::path start_folder);

} // namespace cratchit::ui

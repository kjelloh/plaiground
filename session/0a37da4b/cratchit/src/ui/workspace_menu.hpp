#pragma once

#include "core/dot_folder.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>
#include <vector>

namespace cratchit::ui {

// The most projects the workspace offers, as the keys 1..9
inline constexpr std::size_t max_listed_projects = 9;

// The opened workspace, named "Samlingsmapp" in the UI. It knows only where its
// projects are (projects, see list_projects); everything shown about them is
// read from each project when shown (see describe_project).
//
// Its view shows the folder (and that cratchit created it, when it did), and
// above the projects a header "Namn Org.nr Skapad Mapp". Its options are the
// projects, 1..9 ordered by name, in columns: name, organisation number, the
// day created and the folder (see short_folder_text), marked "(finns inte
// längre)" when the folder is gone and "(går inte att öppna)" when it is no
// project with the remembered uuid. Last "+ Lägg till bokföring" (see
// add_project_menu.hpp). With more than 9 projects the first 9 are offered and
// the view says so. A digit opens its project (see project_menu.hpp), or leads
// to the error state when it cannot be opened. It reads its list again when the
// user comes back to it, so that an added project shows. No input. Reached by
// 0 from the root.
std::unique_ptr<pyroceph::tui::State> make_workspace_menu(Workspace workspace, std::vector<ListedProject> projects);

} // namespace cratchit::ui

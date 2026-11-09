#pragma once

#include "core/dot_folder.hpp"
#include "pyroceph/tui/state.hpp"

#include <filesystem>
#include <memory>

namespace cratchit::ui {

// Creates a new project from an SIE file, named "Från SIE" in the path. The
// user types the path of the SIE file, starting from start_folder; Tab
// completes folders and SIE files (.se, .si, .sie). Enter reads and plans the
// project (see plan_project_from_sie) and leads to "Bekräfta", which shows the
// plan and offers 1 "Skapa bokföringen". When the file cannot be used the reason
// is shown and the path is kept, to be corrected. No options.
//
// "Bekräfta" 1 creates the project (see create_project_from_sie): a folder
// named after the company next to the SIE file, with the whole file imported,
// and the file moved into it. The workspace remembers the project, and the TUI
// goes back to origin (State::back_to), where the user started, e.g. the
// workspace that now lists the project. Without origin "Bekräfta" stays and says
// that the project was created, offering nothing more. A failure leads to the
// error state.
std::unique_ptr<pyroceph::tui::State> make_project_from_sie(Workspace workspace, std::filesystem::path start_folder,
                                                            pyroceph::tui::State const* origin = nullptr);

} // namespace cratchit::ui

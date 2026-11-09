#pragma once

#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <functional>
#include <memory>
#include <string>

namespace cratchit::ui {

// Imports an SIE file into the project, named "Importera från SIE" in the path.
// The user types the path of the SIE file, starting in the project folder; Tab
// completes folders and SIE files. Enter plans the import (see plan_sie_import)
// and leads to "Bekräfta", which shows the file, its fiscal year (new in the
// books or already there) and what it adds, and offers 1 "Importera". When the
// file cannot be imported the reason is shown and the path is kept.
//
// "Bekräfta" 1 imports (see import_sie: the books, and the SIE file moved to
// importerade_sie_filer), gives report what was done, and the TUI goes back to
// origin (State::back_to), the project. A failure leads to the error state.
std::unique_ptr<pyroceph::tui::State> make_sie_import_menu(Project project, pyroceph::tui::State const* origin,
                                                           std::function<void(std::string)> report);

} // namespace cratchit::ui

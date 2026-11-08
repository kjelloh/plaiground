#pragma once

#include "core/dot_folder.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// The opened workspace: where the user's projects will be listed. Named
// "Samlingsmapp" in the UI. Its view shows the folder (and that it was created,
// when it was). No options and no input yet, so only the common keys work
// there and - goes back. Reached by 0 from the root.
std::unique_ptr<pyroceph::tui::State> make_workspace_menu(Workspace workspace);

} // namespace cratchit::ui

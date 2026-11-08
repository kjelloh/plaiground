#pragma once

#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// The workspace: where the user's projects are listed. Named "Samlingsmapp"
// in the UI. Empty for now: no options and no input, so only the common keys
// work there and - goes back. Reached by 0 from the root.
std::unique_ptr<pyroceph::tui::State> make_workspace_menu();

} // namespace cratchit::ui

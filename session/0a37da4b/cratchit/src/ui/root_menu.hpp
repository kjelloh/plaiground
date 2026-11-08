#pragma once

#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// The root of cratchit's UI, named "Cratchit" in the prompt. 0 leads to the
// workspace (see workspace_menu.hpp), 9 to the test menu (see test_menu.hpp).
// More real menus come here as 1, 2, ...
std::unique_ptr<pyroceph::tui::State> make_root_menu();

} // namespace cratchit::ui

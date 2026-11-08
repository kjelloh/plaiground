#pragma once

#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// The root of cratchit's UI, named "Cratchit" in the prompt. The real menus
// come here as 1, 2, ...; 9 leads to the test menu (see test_menu.hpp).
std::unique_ptr<pyroceph::tui::State> make_root_menu();

} // namespace cratchit::ui

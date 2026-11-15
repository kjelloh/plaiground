#pragma once

#include "pyroceph/tui/state.hpp"

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

namespace cratchit::ui {

// Where 0 opens the workspace, or why that place is not known (shown as the
// error when the user presses 0)
using WorkspaceFolder = std::expected<std::filesystem::path, std::string>;

// The root of cratchit's UI, named "Cratchit" in the prompt. 0 opens the
// workspace in workspace_folder (see workspace_menu.hpp), or leads to the error
// state when it cannot be opened. 9 leads to the test menu (see test_menu.hpp).
// More real menus come here as 1, 2, ...
std::unique_ptr<pyroceph::tui::State> make_root_menu(WorkspaceFolder workspace_folder);

} // namespace cratchit::ui

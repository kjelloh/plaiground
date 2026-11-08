#pragma once

#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstddef>
#include <memory>

namespace cratchit::ui {

// The most fiscal years a project offers, as the keys 1..9
inline constexpr std::size_t max_listed_fiscal_years = 9;

// An opened project. Its name is the project's name, so the path reads e.g.
// "Cratchit > Samlingsmapp > The ITfied AB". Its view shows the folder (and
// when there are no fiscal years, more than are offered, or they cannot be
// read). Its options are the fiscal years (see list_fiscal_years), the latest
// as 1, up to 9, each leading to its year (see fiscal_year_menu.hpp). No
// input. Reached by a digit from the workspace.
std::unique_ptr<pyroceph::tui::State> make_project_menu(Project project);

} // namespace cratchit::ui

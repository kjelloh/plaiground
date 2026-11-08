#pragma once

#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// A fiscal year of an opened project. Its name is the year's label (see
// label_of), so the path reads e.g. "Cratchit > Samlingsmapp > The ITfied AB > 2025/26".
// Its view shows the period. No options and no input yet. Reached by a digit
// from the project.
std::unique_ptr<pyroceph::tui::State> make_fiscal_year_menu(Project project, FiscalYear year);

} // namespace cratchit::ui

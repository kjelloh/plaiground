#pragma once

#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// Utkast: the fiscal year's drafts, the events on their way into the books.
// For now the view only says how many drafts the year has ("Utkast <year>:
// inga utkast" or "Utkast <year>: 3 utkast"), read again when the user comes
// back. 0 Läs kontoutdrag leads to make_read_statement_menu. Opened with 0 in
// Årsflöde. No input.
std::unique_ptr<pyroceph::tui::State> make_draft_menu(Project project, FiscalYear year);

// Läs kontoutdrag: the user types the path of an account statement, starting
// in the project folder; Tab completes folders and all files (no filter on
// what a statement may be). Enter on a file leads to Inte än, as reading a
// statement is not there yet. Enter on anything else (nothing typed, a folder,
// a path that does not exist) is refused with the reason, and the path is
// kept. Coming back, the path starts in the folder of the file chosen last.
std::unique_ptr<pyroceph::tui::State> make_read_statement_menu(Project project, FiscalYear year);

} // namespace cratchit::ui

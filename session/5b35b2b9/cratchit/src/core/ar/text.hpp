#pragma once

#include "core/ar/equity.hpp"
#include "core/ar/notes.hpp"
#include "core/ar/statement.hpp"
#include "core/ar/trace.hpp"
#include "core/sie/document.hpp"

#include <string>
#include <vector>

namespace cratchit::ar {

// Kronor with a space between thousands, e.g. "-84 193"
std::string format_kronor(Kronor kronor);

// Förändringar i eget kapital as plain text: a column per post and the total,
// a row per change, each amount rounded to kronor on its own
std::string format_equity_changes(EquityChanges const& changes);

// The resultaträkning and the balansräkning as plain text, one column per
// year (index 0 first): the shown lines, indented by level, long texts wrapped
std::string format_statements(Statement const& statement, std::vector<sie::FiscalYear> const& years);

// The notes as plain text, for fiscal year 0 (years.front()): each note's
// sections with their rows, then its sum
std::string format_notes(Notes const& notes, std::vector<sie::FiscalYear> const& years);

// The trace of the årsredovisning as plain text, in the order of the report:
// Förändringar i eget kapital (accounts of the first and last row, vouchers of
// the changes), the resultaträkning and balansräkning (accounts and year 0
// vouchers under each shown item), then the notes (accounts of the first and
// last row of each section, vouchers of its changes)
std::string format_trace(EquityChanges const& equity, Statement const& statement, Notes const& notes,
                         sie::Document const& document, std::vector<sie::FiscalYear> const& years);

} // namespace cratchit::ar

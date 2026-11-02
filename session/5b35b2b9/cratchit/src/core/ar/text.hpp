#pragma once

#include "core/ar/statement.hpp"
#include "core/sie/document.hpp"

#include <string>
#include <vector>

namespace cratchit::ar {

// Kronor with a space between thousands, e.g. "-84 193"
std::string format_kronor(Kronor kronor);

// The resultaträkning and the balansräkning as plain text, one column per
// year (index 0 first): the shown lines, indented by level, long texts wrapped
std::string format_statements(Statement const& statement, std::vector<sie::FiscalYear> const& years);

// Each shown item with the accounts in it and their amounts as booked, to
// check the mapping by
std::string format_accounts(Statement const& statement, std::vector<sie::FiscalYear> const& years);

} // namespace cratchit::ar

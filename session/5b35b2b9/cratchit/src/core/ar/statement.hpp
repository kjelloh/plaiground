#pragma once

#include "core/amount.hpp"
#include "core/ar/layout.hpp"
#include "core/sie/document.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace cratchit::ar {

// Whole kronor, as the årsredovisning shows them
using Kronor = std::int64_t;

// An amount in öre to whole kronor, half away from zero
Kronor to_kronor(Amount amount);

// What a fiscal year ends with, by account: the closing balance (#UB) of
// balance accounts and the result (#RES) of result accounts
using Balances = std::map<std::string, Amount>;

Balances year_balances(sie::Document const& document, int year);

// A line of the årsredovisning with its amounts, one per year given
struct StatementLine {
  int level = 0;
  LineKind kind = LineKind::item;
  std::string text;
  std::vector<Amount> exact;   // in öre, as the årsredovisning shows it
  std::vector<Kronor> amounts; // exact, rounded
  // For items: the accounts in it with their exact amounts as booked, per year
  std::vector<Balances> accounts;
  bool shown = false; // items with an amount in any year, and the headings and sums over them
};

struct Statement {
  std::vector<StatementLine> lines;   // the layout's lines but outside
  std::vector<std::string> errors;    // accounts with an amount that are on no line, or on two
};

// The årsredovisning of years (index 0 first) by layout. Each item is the sum
// of its accounts, shown as the årsredovisning does: class 1 accounts
// (tillgångar) as booked, all others negated, so that income, equity and debt
// are positive and costs negative. A sum adds the items after the last heading
// at its level or above. Each line is rounded to kronor from its exact amount,
// so a sum may differ by a krona from the shown items it adds.
Statement build_statement(std::vector<Line> const& layout, std::vector<Balances> const& years);

// The exact amounts of the first line with this text, or nothing
std::vector<Amount> const* amounts_of(Statement const& statement, std::string_view text);

// What does not add up: the balansräkning not balancing, or the year's result
// differing between resultaträkning and balansräkning, for any year, to the öre
std::vector<std::string> check_statement(Statement const& statement);

} // namespace cratchit::ar

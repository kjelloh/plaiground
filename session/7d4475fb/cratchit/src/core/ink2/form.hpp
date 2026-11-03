#pragma once

#include "core/amount.hpp"
#include "core/balances.hpp"
#include "core/ink2/mapping.hpp"

#include <map>
#include <string>
#include <vector>

namespace cratchit::ink2 {

// The amount of a field of INK2R
struct FieldAmount {
  bool used = false; // some account with an amount is in it; then shown even when 0
  Amount exact = 0;  // in öre, as the field shows it (see Field::sign)
  Kronor kronor = 0; // exact, rounded half away from zero
  Balances accounts; // the accounts in it with their amounts as booked
};

// INK2R (räkenskapsschema) of one fiscal year
struct Form {
  std::map<std::string, FieldAmount> fields; // by SRU code, every field of the mapping
  // Accounts of class 1-8 with an amount that go to no field, or to more than one
  std::vector<std::string> errors;
  // Accounts that are not of class 1-8 (not on INK2R), and what does not add up in whole kronor
  std::vector<std::string> notes;
};

// The fields of lines for the balances of a fiscal year. Each account with an amount goes to
// the fields with a group of accounts that holds it, whose condition the group meets: "Om
// netto +" (and the profit of "+ 899x") when the group of the account nets to a credit (or a
// debit for 899x) or zero, "Om netto -" (the loss of "– 899x") when to the other side.
Form build_form(std::vector<Line> const& lines, Balances const& balances);

// The sums of the form in whole kronor, as the fields show them
struct Sums {
  Kronor assets = 0;            // 72xx
  Kronor equity_and_debt = 0;   // 73xx
  Kronor result = 0;            // 74xx less 75xx, but 7450 and 7550: income less costs
  Kronor year_result = 0;       // 7450 less 7550, as 3.26 and 3.27 give it
};

Sums sums_of(Form const& form);

} // namespace cratchit::ink2

#pragma once

#include "core/amount.hpp"
#include "core/books/books.hpp"

#include <string>
#include <string_view>

// Postings as shown: the amount without sign in a debit column (zero and up) or a credit column (below zero)
namespace cratchit::books {

// The amount without sign on its side, the other side empty
struct Sides {
  std::string debit;
  std::string credit;
};
Sides sides_of(Amount amount);

// The column heading of posting rows: Account, Debit, Credit
std::string posting_heading();

// One posting row: the account, the amount on its side, then rest (e.g. the account name) when not empty
std::string posting_row(std::string_view account, Amount amount, std::string_view rest);

// The debit and credit totals of postings in the columns of posting_row (removed postings not counted)
std::string posting_total(Postings const& postings);

// One posting in a line of several: the account, D or C and the amount without sign, e.g. 1930 C 1118.00
std::string posting_brief(Posting const& posting);

} // namespace cratchit::books

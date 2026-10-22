#pragma once

#include "core/books/books.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::books {

// Report lines for one fiscal year of the books (index into books.years),
// headed by the company and the year

// Trial balance (saldobalans): every account with a balance or transactions.
// Balance accounts with opening balance, change and closing balance; result
// accounts with the year's change; totals. Says where the opening balances come
// from, and lists where carried ones differ from those the source reported.
std::vector<std::string> trial_balance(Books const& books, std::size_t year, std::string_view company);

// Every voucher in book order with its rows; added and removed rows marked
std::vector<std::string> voucher_list(Books const& books, std::size_t year, std::string_view company);

// Account ledger (huvudbok for one account): opening balance, every row on the
// account by date with the running balance, closing balance. Nothing for an
// account the books neither define nor use in the year.
std::optional<std::vector<std::string>> account_ledger(Books const& books, std::size_t year, std::string_view company,
                                                       std::string_view account);

} // namespace cratchit::books

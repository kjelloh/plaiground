#pragma once

#include "core/books/books.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Hints while typing rows: the counter accounts used before with an account on the same side
namespace cratchit::books {

// How many of the latest vouchers and waiting events a hint lists
inline constexpr std::size_t hint_listed = 5;

// How many of the latest vouchers and waiting events a hint counts the counter accounts in
inline constexpr std::size_t hint_counted = 20;

// A voucher or a waiting event in the books
struct HintSource {
  std::size_t year = 0;  // an index into books.years
  std::size_t index = 0; // an index into the year's vouchers, or its waiting events
  bool waiting = false;

  bool operator==(HintSource const&) const = default;
};

// An account on the other side than the hinted one, in the counted sources
struct CounterAccount {
  std::string account;
  std::size_t count = 0; // in how many of the counted sources
  HintSource latest;     // the latest of them

  bool operator==(CounterAccount const&) const = default;
};

// The vouchers and waiting events with a counted posting on account on one side
// (debit: zero and up, as sides_of), the latest first (by date, then the later in the books)
struct Hint {
  std::string account;
  bool debit = true;
  std::size_t counted = 0;              // the latest sources counted, at most the counted given
  std::vector<CounterAccount> counters; // in the most sources first, then the latest
  std::vector<HintSource> latest;       // at most the listed given

  bool operator==(Hint const&) const = default;
};

// The hint for account on one side, over every fiscal year in books; excluded
// (e.g. the waiting event being posted) is left out
Hint hint_for(Books const& books, std::string_view account, bool debit, std::optional<HintSource> excluded = {},
              std::size_t counted = hint_counted, std::size_t listed = hint_listed);

// An account on one side
struct AccountSide {
  std::string account;
  bool debit = true;

  bool operator==(AccountSide const&) const = default;
};

// The account and side of each counted posting, each once, in the order of the postings
std::vector<AccountSide> sides_in(Postings const& postings);

} // namespace cratchit::books

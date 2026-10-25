#include "core/books/hint.hpp"

#include <algorithm>

namespace cratchit::books {

namespace {

bool is_debit(Amount amount) {
  return amount >= 0;
}

bool counts(Posting const& posting) {
  return posting.kind != Posting::Kind::removed;
}

Postings const& postings_of(Books const& books, HintSource const& source) {
  auto const& year = books.years[source.year];
  return source.waiting ? year.waiting[source.index].postings : year.vouchers[source.index].postings;
}

Date date_of(Books const& books, HintSource const& source) {
  auto const& year = books.years[source.year];
  return source.waiting ? year.waiting[source.index].event.date : year.vouchers[source.index].event.date;
}

} // namespace

Hint hint_for(Books const& books, std::string_view account, bool debit, std::optional<HintSource> excluded,
              std::size_t counted, std::size_t listed) {
  Hint hint{std::string{account}, debit, 0, {}, {}};
  // In the order of the books: each year's vouchers, then its waiting events
  std::vector<HintSource> found;
  auto const has = [&](Postings const& postings) {
    return std::ranges::any_of(postings, [&](Posting const& posting) {
      return counts(posting) && posting.account == account && is_debit(posting.amount) == debit;
    });
  };
  for (std::size_t y = 0; y < books.years.size(); ++y) {
    auto const& year = books.years[y];
    for (std::size_t v = 0; v < year.vouchers.size(); ++v) {
      if (has(year.vouchers[v].postings)) {
        found.push_back({y, v, false});
      }
    }
    for (std::size_t w = 0; w < year.waiting.size(); ++w) {
      HintSource const source{y, w, true};
      if (source != excluded && has(year.waiting[w].postings)) {
        found.push_back(source);
      }
    }
  }
  // The latest first; of the same date, the later in the books first
  std::ranges::reverse(found);
  std::ranges::stable_sort(found, [&](HintSource const& a, HintSource const& b) {
    return date_of(books, a) > date_of(books, b);
  });
  hint.counted = std::min(found.size(), counted);
  for (std::size_t i = 0; i < hint.counted; ++i) {
    std::vector<std::string> seen;
    for (auto const& posting : postings_of(books, found[i])) {
      if (!counts(posting) || is_debit(posting.amount) == debit || std::ranges::contains(seen, posting.account)) {
        continue;
      }
      seen.push_back(posting.account);
      auto const counter = std::ranges::find(hint.counters, posting.account, &CounterAccount::account);
      if (counter == hint.counters.end()) {
        hint.counters.push_back({posting.account, 1, found[i]});
      } else {
        ++counter->count;
      }
    }
  }
  // Already in the order of their latest source; the most sources first
  std::ranges::stable_sort(hint.counters, std::ranges::greater{}, &CounterAccount::count);
  found.resize(std::min(found.size(), listed));
  hint.latest = std::move(found);
  return hint;
}

std::vector<AccountSide> sides_in(Postings const& postings) {
  std::vector<AccountSide> sides;
  for (auto const& posting : postings) {
    AccountSide side{posting.account, is_debit(posting.amount)};
    if (counts(posting) && !std::ranges::contains(sides, side)) {
      sides.push_back(std::move(side));
    }
  }
  return sides;
}

} // namespace cratchit::books

#include "core/books/match.hpp"

#include "core/books/entry.hpp"

#include <algorithm>
#include <chrono>
#include <map>
#include <tuple>

namespace cratchit::books {

namespace {

int days_between(Date a, Date b) {
  auto const days = (std::chrono::sys_days{b} - std::chrono::sys_days{a}).count();
  return static_cast<int>(days < 0 ? -days : days);
}

// Both have postings, on accounts the other has none on, and together they balance
bool balance_together(WaitingEvent const& a, WaitingEvent const& b) {
  if (a.postings.empty() || b.postings.empty()) {
    return false;
  }
  for (auto const& posting : a.postings) {
    if (std::ranges::any_of(b.postings, [&](Posting const& other) { return other.account == posting.account; })) {
      return false;
    }
  }
  return Draft{a.event, 0, {}, merged(a, b, a.event).postings}.is_balanced();
}

} // namespace

std::vector<Match> find_matches(FiscalYear const& year, int window_days) {
  std::vector<Match> matches;
  for (std::size_t i = 0; i < year.waiting.size(); ++i) {
    auto const& waiting = year.waiting[i];
    for (std::size_t j = i + 1; j < year.waiting.size(); ++j) {
      auto const& other = year.waiting[j];
      auto const days = days_between(waiting.event.date, other.event.date);
      if (other.event.amount == waiting.event.amount && days <= window_days) {
        matches.push_back({Match::Kind::same, i, j, days, shared_words(waiting.event.heading, other.event.heading),
                           balance_together(waiting, other)});
      }
    }
    for (std::size_t v = 0; v < year.vouchers.size(); ++v) {
      auto const& voucher = year.vouchers[v];
      auto const days = days_between(waiting.event.date, voucher.event.date);
      if (voucher.event.amount == waiting.event.amount && days <= window_days) {
        matches.push_back({Match::Kind::books, i, v, days, shared_words(waiting.event.heading, voucher.event.heading),
                           !waiting.postings.empty() && unbooked(waiting, voucher).empty()});
      }
    }
  }
  std::ranges::stable_sort(matches, [](Match const& a, Match const& b) {
    if (a.fits != b.fits) {
      return a.fits;
    }
    if (a.words.size() != b.words.size()) {
      return a.words.size() > b.words.size();
    }
    return std::tie(a.days, a.waiting) < std::tie(b.days, b.waiting);
  });
  return matches;
}

Postings unbooked(WaitingEvent const& waiting, Voucher const& voucher) {
  std::map<std::tuple<std::string, Date, Amount>, std::size_t> booked;
  for (auto const& posting : voucher.postings) {
    if (posting.kind != Posting::Kind::removed) {
      ++booked[{posting.account, posting.date.value_or(voucher.event.date), posting.amount}];
    }
  }
  Postings left;
  for (auto const& posting : waiting.postings) {
    auto const it = booked.find({posting.account, posting.date.value_or(waiting.event.date), posting.amount});
    if (it != booked.end() && it->second > 0) {
      --it->second;
    } else {
      left.push_back(posting);
    }
  }
  return left;
}

WaitingEvent merged(WaitingEvent const& a, WaitingEvent const& b, Event event) {
  WaitingEvent result{std::move(event), std::min(a.noted, b.noted), a.postings};
  for (auto const& posting : b.postings) {
    bool const same_entry = !posting.source.empty() && std::ranges::any_of(a.postings, [&](Posting const& kept) {
      return kept.source == posting.source;
    });
    if (!same_entry) {
      result.postings.push_back(posting);
    }
  }
  return result;
}

} // namespace cratchit::books

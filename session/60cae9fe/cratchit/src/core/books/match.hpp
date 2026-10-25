#pragma once

#include "core/books/books.hpp"

#include <cstddef>
#include <string>
#include <vector>

// Waiting events that may be the same event as another waiting event, or as a voucher already in the books
namespace cratchit::books {

// How many days apart two events may be and still be the same event: a weekend
// and a bank day on each side (Thursday to Monday). Calendar days, no holidays.
inline constexpr int match_window_days = 4;

// Two events that may be the same: the same amount, at most the window apart
struct Match {
  enum class Kind {
    same,  // two waiting events
    books, // a waiting event and a voucher in the books
  };

  Kind kind = Kind::same;
  std::size_t waiting = 0; // the waiting event, an index into the year's waiting (the older of two)
  std::size_t other = 0;   // the other waiting event (same), or the voucher (books), an index into the year's
  int days = 0;            // the days between their dates
  std::vector<std::string> words; // the heading words they share (see shared_words)
  // same: both have postings, on different accounts, and together they balance; books: the waiting event has postings, all in the voucher
  bool fits = false;

  bool operator==(Match const&) const = default;
};

// The possible matches in year, the likely ones first: those that fit, then
// more shared words, then fewer days apart, then the older waiting event.
// A pair of waiting events is listed once; a waiting event may be in several matches.
std::vector<Match> find_matches(FiscalYear const& year, int window_days = match_window_days);

// The postings of waiting not booked by voucher: none of the voucher's postings
// (not removed) has its account, date and amount. Each voucher posting books one.
Postings unbooked(WaitingEvent const& waiting, Voucher const& voucher);

// One event of two waiting events: event, the earlier noted day, and the
// postings of both (a posting of b from the same statement entry as one of a's left out)
WaitingEvent merged(WaitingEvent const& a, WaitingEvent const& b, Event event);

} // namespace cratchit::books

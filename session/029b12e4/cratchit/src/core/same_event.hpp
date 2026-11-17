#pragma once

#include "core/books/books.hpp"
#include "core/date.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// Drafts that may be the same financial event as another draft, or as a
// voucher already in the books, as take 3 (60cae9fe) found matches among its
// waiting events (beslut 2026-11-17). The user decides; cratchit only lists
// the candidates and makes two drafts one.
namespace cratchit {

// How many days apart two events may be and still be the same event: a weekend
// and a bank day on each side (Thursday to Monday). Calendar days, no holidays.
inline constexpr int same_event_window_days = 4;

// Two that may be the same event: the same amount without sign, at most the window apart
struct SameEvent {
  enum class Kind {
    drafts, // two drafts
    booked, // a draft and a voucher in the books
  };

  Kind kind = Kind::drafts;
  std::size_t draft = 0; // the draft, an index into the year's drafts (the older of two)
  std::size_t other = 0; // the other draft (drafts), or the voucher (booked), an index into the year's
  int days = 0;          // the days between their dates
  std::vector<std::string> words; // the words their texts share (see shared_words)
  // drafts: one has just the event and the other just postings, or both have postings,
  // on different accounts, that balance together; booked: the draft has postings, all in the voucher
  bool fits = false;

  bool operator==(SameEvent const&) const = default;
};

// What a draft amounts to without sign: its event's amount, else what its postings move
Amount amount_of(books::Draft const& draft);

// The text a draft is compared by: its heading, else the texts of its postings
std::string compared_text(books::Draft const& draft);

// The words two texts share, in alphabetical order: letters and digits (and any
// non-ASCII character), case folded, without numbers and one-letter words
std::vector<std::string> shared_words(std::string_view a, std::string_view b);

// The possible pairs in year, the likely ones first: those that fit, then more
// shared words, then fewer days apart, then the older draft. A pair of drafts is
// listed once; a draft may be in several pairs. An amount of zero pairs with nothing.
std::vector<SameEvent> find_same_events(books::FiscalYear const& year, int window_days = same_event_window_days);

// The postings of draft not booked by voucher: none of the voucher's postings
// (not removed) has its account, date and amount. Each voucher posting books one.
books::Postings unbooked(books::Draft const& draft, books::Voucher const& voucher);

// One draft of two: event, the earlier noted day, and the postings of both (a
// posting of b from the same statement entry as one of a's left out)
books::Draft merged(books::Draft const& a, books::Draft const& b, books::Event event);

// Makes the drafts at index a and b (a != b) of the fiscal year starting on
// year_start one (see merged, with event) and writes the books. Only when the
// drafts there are still the expected ones (the ones the user saw), as drafts
// are numbered by their place and may have changed. Gives the index of the one
// draft. Error text when the books cannot be read or written, the year is not
// in them, or a draft is not there any more. Does not log.
std::expected<std::size_t, std::string> merge_drafts(std::filesystem::path const& project_folder, Date year_start,
                                                     std::size_t a, books::Draft const& expected_a, std::size_t b,
                                                     books::Draft const& expected_b, books::Event const& event);

} // namespace cratchit

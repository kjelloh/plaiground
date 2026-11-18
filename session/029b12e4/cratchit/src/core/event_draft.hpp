#pragma once

#include "core/amount.hpp"
#include "core/books/books.hpp"
#include "core/date.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// A financial event described on one line, as in take 3 (60cae9fe), made a
// draft: the event alone, without postings (stage händelse).
namespace cratchit {

// The event typed on one line: heading, amount and date in any order,
// separated by spaces. The date is the last word that is a date (YYYY-MM-DD or
// YYYYMMDD) and must be there. The amount is the last other number: digits
// with an optional + and a decimal comma or point, at most two decimals, e.g.
// 1118,00; a no-break space inside it is dropped. The rest, in the order
// typed, is the heading. Number words just before the amount may be groups of
// thousands (87 500 may be 500 or 87500), so each reading is a choice: the last
// word alone first, then with one more word before it, and so on; a reading
// without heading is left out. A single choice is clear. The amount may be
// below zero: a draft's amount keeps its sign, minus for an event that goes the
// other way than usual (a credit note, a refund; beslut 2026-11-17). Error
// text says what is missing or wrong. Does not log.
std::expected<std::vector<books::Event>, std::string> parse_event(std::string_view text);

// Adds the event as a draft of the fiscal year starting on year_start, noted
// today, without postings, and writes the books. Gives the draft's index among
// the year's drafts (oldest first; see place_draft). Error text when the books
// cannot be read or written, the year is not in them or the date is outside it.
// Does not log.
std::expected<std::size_t, std::string> add_event_draft(std::filesystem::path const& project_folder, Date year_start,
                                                        books::Event const& event, Date today);

// Forgets (removes) the draft at index among the drafts of the fiscal year
// starting on year_start, and writes the books. Only when the draft there is
// still expected (the one the user saw), as the drafts are numbered by their
// place and may have changed. Error text when the books cannot be read or
// written, the year is not in them, or the draft is not there any more.
// Does not log.
std::expected<void, std::string> forget_draft(std::filesystem::path const& project_folder, Date year_start,
                                              std::size_t index, books::Draft const& expected);

// Replaces the draft at index of the fiscal year starting on year_start with
// changed (e.g. a posting added, its event changed) and writes the books. Only
// when the draft there is still expected (the one the user saw). Gives the
// index of the changed draft, which moves when its date changed (see
// place_draft). Error text when the books cannot be read or written, the year
// is not in them, the draft is not there any more, or the changed date is
// outside the year. Does not log.
std::expected<std::size_t, std::string> replace_draft(std::filesystem::path const& project_folder, Date year_start,
                                                      std::size_t index, books::Draft const& expected,
                                                      books::Draft changed);

// An amount of a posting as typed: an optional sign (+ or -), digits, and a
// decimal comma or point with at most two decimals, e.g. -3,70 or 1118. Spaces
// (also no-break spaces) inside it do not count. Nothing for anything else.
std::optional<Amount> parse_typed_amount(std::string_view text);

} // namespace cratchit

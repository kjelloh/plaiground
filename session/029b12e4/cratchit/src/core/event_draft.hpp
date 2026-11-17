#pragma once

#include "core/books/books.hpp"
#include "core/date.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
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
// without heading is left out. A single choice is clear. An amount below zero
// is refused for now (beslut 2026-11-17). Error text says what is missing or
// wrong. Does not log.
std::expected<std::vector<books::Event>, std::string> parse_event(std::string_view text);

// Adds the event as a draft of the fiscal year starting on year_start, noted
// today, without postings, and writes the books. Gives the draft's index among
// the year's drafts (oldest first; see place_draft). Error text when the books
// cannot be read or written, the year is not in them or the date is outside it.
// Does not log.
std::expected<std::size_t, std::string> add_event_draft(std::filesystem::path const& project_folder, Date year_start,
                                                        books::Event const& event, Date today);

} // namespace cratchit

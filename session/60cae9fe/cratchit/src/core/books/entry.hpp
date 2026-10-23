#pragma once

#include "core/amount.hpp"
#include "core/books/books.hpp"
#include "core/date.hpp"

#include <cstddef>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Entering a voucher by hand: the event line, the series, the postings
namespace cratchit::books {

// The event typed first (see Event): heading, amount and date in any order, separated by spaces. The date is the
// last word that is a date (YYYY-MM-DD or YYYYMMDD); without one, today, if given. The
// amount is the last other number: digits with an optional sign and a decimal
// comma or point, e.g. -1118,00; a no-break space inside it is dropped. The
// rest, in the order typed, is the heading. Number words just before the amount
// may be groups of thousands (87 500 may be 500 or 87500), so each reading is a
// choice: the last word alone first, then with one more word before it, and so
// on. A single choice is clear. Error text says what is missing or wrong.
std::expected<std::vector<Event>, std::string> parse_event(std::string_view text, std::optional<Date> today);

// A voucher series a voucher can be entered in
struct SeriesName {
  std::string_view letter;
  std::string_view name;
};

// A to N, the series of Fortnox
std::span<SeriesName const> series_names();

// The series name of letter (upper case), if it is one of series_names
std::optional<SeriesName> series_name(std::string_view letter);

// The series offered by default for a heading
struct Guess {
  std::string series = "A";
  Voucher const* like = nullptr; // the voucher the guess comes from, if any
};

// The series of the voucher in the books whose text shares the most words with
// heading (ignoring case, numbers and one-letter words); the latest such voucher
// on a tie. Only vouchers in a series of series_names count. Else A.
Guess guess_series(Books const& books, std::string_view heading);

// The highest number in series in year, plus one; 1 for a series not used yet
std::string next_number(FiscalYear const& year, std::string_view series);

// A row typed at post>: an account and, optionally, an amount
struct Row {
  std::string account;
  std::optional<Amount> amount;

  bool operator==(Row const&) const = default;
};

// "<account> [amount]"; the amount as in parse_event, spaces in it dropped
std::expected<Row, std::string> parse_row(std::string_view text);

// Accounts whose number starts with query, or whose name contains it, ignoring case
std::vector<Account const*> find_accounts(Books const& books, std::string_view query);

// A voucher being entered: its event and the postings typed so far
struct Draft {
  Event event;
  std::size_t year = 0; // index in books.years
  std::string series;
  Postings postings;

  // What the next posting needs to make the postings add up to zero (minus their sum)
  Amount remaining() const;

  // The amount on the voucher line, without sign: what debit and credit must each add up to
  Amount target() const;

  // At least two postings, debit and credit each adding up to the amount
  bool is_balanced() const;

  // Where the postings stand, e.g. "Remaining 223.60 (debit 1118.00, credit 894.40, amount 1118.00)."
  std::string status() const;

  Voucher voucher(std::string number, Date registered) const;
};

} // namespace cratchit::books

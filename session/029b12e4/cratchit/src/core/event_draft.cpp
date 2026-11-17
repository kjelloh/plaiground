#include "core/event_draft.hpp"

#include "core/amount.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <utility>

namespace cratchit {

namespace {

bool is_digit(char c) { return c >= '0' && c <= '9'; }

bool all_digits(std::string_view text) { return !text.empty() && std::ranges::all_of(text, is_digit); }

// Words separated by spaces and tabs
std::vector<std::string> split(std::string_view text) {
  std::vector<std::string> words;
  std::string word;
  for (char const c : text) {
    if (c == ' ' || c == '\t') {
      if (!word.empty()) {
        words.push_back(std::move(word));
        word.clear();
      }
    } else {
      word += c;
    }
  }
  if (!word.empty()) words.push_back(std::move(word));
  return words;
}

// Without no-break spaces (U+00A0, U+202F), as a bank may write 1 118,00
std::string without_no_break_spaces(std::string text) {
  for (std::string_view const space : {std::string_view("\xC2\xA0"), std::string_view("\xE2\x80\xAF")}) {
    for (auto at = text.find(space); at != std::string::npos; at = text.find(space)) text.erase(at, space.size());
  }
  return text;
}

// The parts of a number: an optional sign, digits, and optionally a decimal comma or point and digits
struct Number {
  std::string_view sign;
  std::string_view whole;
  std::string_view decimals; // empty when there is no decimal separator
};

std::optional<Number> number_of(std::string_view text) {
  Number number;
  if (text.starts_with('-') || text.starts_with('+')) {
    number.sign = text.substr(0, 1);
    text.remove_prefix(1);
  }
  auto const separator = text.find_first_of(",.");
  number.whole = text.substr(0, separator);
  if (separator != std::string_view::npos) {
    number.decimals = text.substr(separator + 1);
    if (!all_digits(number.decimals)) return std::nullopt;
  }
  if (!all_digits(number.whole)) return std::nullopt;
  return number;
}

std::expected<Amount, std::string> amount_of(std::string const& text) {
  auto const number = number_of(text);
  if (!number) return std::unexpected(std::format("{} är inget belopp.", text));
  auto const plain = std::format("{}{}{}{}", number->sign == "-" ? "-" : "", number->whole,
                                 number->decimals.empty() ? "" : ".", number->decimals);
  auto const amount = parse_amount(plain);
  if (!amount) return std::unexpected(std::format("{} är inget belopp (högst två decimaler).", text));
  return *amount;
}

std::optional<Date> date_of(std::string_view word) {
  if (auto const date = parse_iso_date(word)) return date;
  return parse_compact_date(word);
}

std::string joined(std::vector<std::string> const& words) {
  std::string result;
  for (auto const& word : words) result += (result.empty() ? "" : " ") + word;
  return result;
}

} // namespace

std::expected<std::vector<books::Event>, std::string> parse_event(std::string_view text) {
  auto const words = split(text);
  std::optional<std::size_t> date_at;
  for (std::size_t i = 0; i < words.size(); ++i) {
    if (date_of(words[i])) date_at = i;
  }
  std::vector<std::string> plain;
  for (auto const& word : words) plain.push_back(without_no_break_spaces(word));
  auto const is_amount_word = [&](std::size_t i) { return i != date_at && number_of(plain[i]).has_value(); };
  std::optional<std::size_t> last;
  for (std::size_t i = 0; i < words.size(); ++i) {
    if (is_amount_word(i)) last = i;
  }
  if (!last) return std::unexpected("Inget belopp. Skriv det med siffror, till exempel 1118,00.");
  if (!date_at) return std::unexpected("Inget datum. Skriv datumet, till exempel 2026-10-22.");
  // Each number word just before it may be a group of thousands: 87 500 may be 500 or 87500.
  // Every reading is a choice, the last word alone first.
  auto const has_decimals = [&](std::size_t i) { return plain[i].find_first_of(",.") != std::string::npos; };
  auto const is_group = [&](std::size_t i) {
    return number_of(plain[i])->sign.empty() && number_of(plain[i])->whole.size() == 3 &&
           (i == *last || !has_decimals(i));
  };
  auto const date = *date_of(words[*date_at]);
  std::vector<books::Event> choices;
  for (auto first = *last;; --first) {
    std::string amount_text;
    for (auto i = first; i <= *last; ++i) amount_text += plain[i];
    auto const amount = amount_of(amount_text);
    std::vector<std::string> heading;
    for (std::size_t i = 0; i < words.size(); ++i) {
      if (i != date_at && (i < first || i > *last)) heading.push_back(words[i]);
    }
    if (first == *last) {
      if (!amount) return std::unexpected(amount.error());
      if (*amount == 0) return std::unexpected("Beloppet är noll.");
      if (heading.empty()) return std::unexpected("Ingen rubrik. Skriv vad som hände, till exempel Telia faktura oktober.");
    }
    if (amount && *amount != 0 && !heading.empty()) choices.push_back({joined(heading), *amount, date});
    if (first == 0 || !is_group(first) || !is_amount_word(first - 1) || has_decimals(first - 1) ||
        number_of(plain[first - 1])->whole.size() > 3) {
      break;
    }
  }
  if (std::ranges::any_of(choices, [](books::Event const& e) { return e.amount < 0; })) {
    return std::unexpected("Skriv beloppet utan minustecken.");
  }
  return choices;
}

std::expected<std::size_t, std::string> add_event_draft(std::filesystem::path const& project_folder, Date year_start,
                                                        books::Event const& event, Date today) {
  auto books = books::read_books(project_folder);
  if (!books) return std::unexpected(books.error());
  auto const year = std::ranges::find_if(books->years, [&](books::FiscalYear const& y) { return y.start == year_start; });
  if (year == books->years.end()) {
    return std::unexpected(std::format("Räkenskapsåret som börjar {} finns inte i bokföringen", format_iso_date(year_start)));
  }
  if (!year->holds(event.date)) {
    return std::unexpected(std::format("{} ligger utanför räkenskapsåret {} – {}", format_iso_date(event.date),
                                       format_iso_date(year->start), format_iso_date(year->end)));
  }
  books::Draft draft;
  draft.event = event;
  draft.noted = today;
  auto const index = year->place_draft(std::move(draft));
  if (auto written = books::write_books(project_folder, *books); !written) return std::unexpected(written.error());
  return index;
}

std::expected<void, std::string> forget_draft(std::filesystem::path const& project_folder, Date year_start,
                                              std::size_t index, books::Draft const& expected) {
  auto books = books::read_books(project_folder);
  if (!books) return std::unexpected(books.error());
  auto const year = std::ranges::find_if(books->years, [&](books::FiscalYear const& y) { return y.start == year_start; });
  if (year == books->years.end()) {
    return std::unexpected(std::format("Räkenskapsåret som börjar {} finns inte i bokföringen", format_iso_date(year_start)));
  }
  if (index >= year->drafts.size() || year->drafts[index] != expected) {
    return std::unexpected("Utkastet finns inte längre på sin plats i bokföringen. Öppna det igen från Utkast.");
  }
  year->drafts.erase(year->drafts.begin() + static_cast<std::ptrdiff_t>(index));
  if (auto written = books::write_books(project_folder, *books); !written) return std::unexpected(written.error());
  return {};
}

} // namespace cratchit

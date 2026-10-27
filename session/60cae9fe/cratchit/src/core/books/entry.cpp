#include "core/books/entry.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <set>
#include <utility>

namespace cratchit::books {

namespace {

constexpr std::array<SeriesName, 14> all_series{{
    {"A", "Redovisning"},
    {"B", "Kundfakturor"},
    {"C", "Inbetalningar från kunder"},
    {"D", "Leverantörsfakturor"},
    {"E", "Utbetalningar till leverantörer"},
    {"F", "Kassa"},
    {"G", "Avskrivning"},
    {"H", "Periodisering"},
    {"I", "Bokslut"},
    {"J", "Kontantfaktura"},
    {"K", "Kvitto"},
    {"L", "Lön"},
    {"M", "Momsrapport"},
    {"N", "Lager"},
}};

bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

bool all_digits(std::string_view text) {
  return !text.empty() && std::ranges::all_of(text, is_digit);
}

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
  if (!word.empty()) {
    words.push_back(std::move(word));
  }
  return words;
}

// Without no-break spaces (U+00A0, U+202F), as a bank may write 1 118,00
std::string without_no_break_spaces(std::string text) {
  for (std::string_view const space : {std::string_view("\xC2\xA0"), std::string_view("\xE2\x80\xAF")}) {
    for (auto at = text.find(space); at != std::string::npos; at = text.find(space)) {
      text.erase(at, space.size());
    }
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
    if (!all_digits(number.decimals)) {
      return std::nullopt;
    }
  }
  if (!all_digits(number.whole)) {
    return std::nullopt;
  }
  return number;
}

std::expected<Amount, std::string> amount_of(std::string const& text) {
  auto const number = number_of(text);
  if (!number) {
    return std::unexpected(std::format("Not an amount: {}", text));
  }
  auto const sie = std::format("{}{}{}{}", number->sign == "-" ? "-" : "", number->whole,
                               number->decimals.empty() ? "" : ".", number->decimals);
  auto const amount = parse_amount(sie);
  if (!amount) {
    return std::unexpected(std::format("Not an amount: {} (at most two decimals)", text));
  }
  return *amount;
}

std::optional<Date> date_of(std::string_view word) {
  if (auto const date = parse_iso_date(word)) {
    return date;
  }
  return parse_compact_date(word);
}

// Upper case ASCII and Latin-1 letters (such as Å, Ä, Ö, É in UTF-8) to lower case
std::string folded(std::string_view text) {
  std::string result(text);
  for (std::size_t i = 0; i < result.size(); ++i) {
    auto const c = static_cast<unsigned char>(result[i]);
    if (c >= 'A' && c <= 'Z') {
      result[i] = static_cast<char>(c - 'A' + 'a');
    } else if (c == 0xC3 && i + 1 < result.size()) {
      auto const next = static_cast<unsigned char>(result[i + 1]);
      if (next >= 0x80 && next <= 0x9E && next != 0x97) {
        result[i + 1] = static_cast<char>(next + 0x20);
      }
      ++i;
    }
  }
  return result;
}

// The words of a text to compare headings by: letters and digits (and any non-ASCII
// character), folded, without numbers and one-letter words
std::set<std::string> words_of(std::string_view text) {
  std::set<std::string> words;
  std::string word;
  auto const add = [&] {
    if (word.size() > 1 && !all_digits(word)) {
      words.insert(folded(word));
    }
    word.clear();
  };
  for (char const c : text) {
    auto const u = static_cast<unsigned char>(c);
    if (u >= 0x80 || is_digit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      word += c;
    } else {
      add();
    }
  }
  add();
  return words;
}

std::string joined(std::vector<std::string> const& words) {
  std::string result;
  for (auto const& word : words) {
    result += result.empty() ? "" : " ";
    result += word;
  }
  return result;
}

} // namespace

std::expected<std::vector<Event>, std::string> parse_event(std::string_view text, std::optional<Date> today) {
  auto const words = split(text);
  std::optional<std::size_t> date_at;
  for (std::size_t i = 0; i < words.size(); ++i) {
    if (date_of(words[i])) {
      date_at = i;
    }
  }
  std::vector<std::string> plain;
  for (auto const& word : words) {
    plain.push_back(without_no_break_spaces(word));
  }
  auto const is_amount_word = [&](std::size_t i) { return i != date_at && number_of(plain[i]).has_value(); };
  std::optional<std::size_t> last;
  for (std::size_t i = 0; i < words.size(); ++i) {
    if (is_amount_word(i)) {
      last = i;
    }
  }
  if (!last) {
    return std::unexpected("No amount. Type it with digits, e.g. 1118,00");
  }
  // Each number word just before it may be a group of thousands: 87 500 may be 500 or 87500.
  // Every reading is a choice, the last word alone first.
  auto const has_decimals = [&](std::size_t i) { return plain[i].find_first_of(",.") != std::string::npos; };
  auto const is_group = [&](std::size_t i) {
    return number_of(plain[i])->sign.empty() && number_of(plain[i])->whole.size() == 3 &&
           (i == *last || !has_decimals(i));
  };
  if (!date_at && !today) {
    return std::unexpected("No date. Type the date, e.g. 2026-10-22.");
  }
  auto const date = date_at ? *date_of(words[*date_at]) : *today;
  std::vector<Event> choices;
  for (auto first = *last;; --first) {
    std::string amount_text;
    for (auto i = first; i <= *last; ++i) {
      amount_text += plain[i];
    }
    auto const amount = amount_of(amount_text);
    std::vector<std::string> heading;
    for (std::size_t i = 0; i < words.size(); ++i) {
      if (i != date_at && (i < first || i > *last)) {
        heading.push_back(words[i]);
      }
    }
    if (first == *last) {
      if (!amount) {
        return std::unexpected(amount.error());
      }
      if (*amount == 0) {
        return std::unexpected("The amount is zero.");
      }
      if (heading.empty()) {
        return std::unexpected("No heading. Type what happened, e.g. Telia faktura oktober");
      }
    }
    if (amount && *amount != 0 && !heading.empty()) {
      choices.push_back({joined(heading), *amount, date});
    }
    if (first == 0 || !is_group(first) || !is_amount_word(first - 1) || has_decimals(first - 1) ||
        number_of(plain[first - 1])->whole.size() > 3) {
      break;
    }
  }
  return choices;
}

std::span<SeriesName const> series_names() {
  return all_series;
}

std::optional<SeriesName> series_name(std::string_view letter) {
  auto const it = std::ranges::find(all_series, letter, &SeriesName::letter);
  return it == all_series.end() ? std::nullopt : std::optional(*it);
}

std::vector<std::string> shared_words(std::string_view a, std::string_view b) {
  auto const in_b = words_of(b);
  std::vector<std::string> shared;
  for (auto const& word : words_of(a)) {
    if (in_b.contains(word)) {
      shared.push_back(word);
    }
  }
  return shared;
}

Guess guess_series(Books const& books, std::string_view heading) {
  auto const words = words_of(heading);
  Guess guess;
  std::size_t best = 0;
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      if (!series_name(voucher.series)) {
        continue;
      }
      std::size_t shared = 0;
      for (auto const& word : words_of(voucher.event.heading)) {
        shared += words.contains(word) ? 1 : 0;
      }
      if (shared > 0 && (shared > best || (shared == best && voucher.event.date >= guess.like->event.date))) {
        best = shared;
        guess = {voucher.series, &voucher};
      }
    }
  }
  return guess;
}

std::vector<Voucher const*> find_templates(Books const& books, Event const& event, std::size_t most) {
  struct Candidate {
    Voucher const* voucher;
    std::size_t shared;
    bool same_sign;
    std::size_t position; // in the books, later is higher
  };
  auto const words = words_of(event.heading);
  std::vector<Candidate> candidates;
  std::size_t position = 0;
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      ++position;
      auto const counted = std::ranges::count_if(
          voucher.postings, [](Posting const& posting) { return posting.kind != Posting::Kind::removed; });
      if (!series_name(voucher.series) || voucher.event.amount == 0 || counted < 2) {
        continue;
      }
      std::size_t shared = 0;
      for (auto const& word : words_of(voucher.event.heading)) {
        shared += words.contains(word) ? 1 : 0;
      }
      if (shared > 0) {
        candidates.push_back({&voucher, shared, (voucher.event.amount < 0) == (event.amount < 0), position});
      }
    }
  }
  std::ranges::sort(candidates, [](Candidate const& a, Candidate const& b) {
    if (a.shared != b.shared) {
      return a.shared > b.shared;
    }
    if (a.same_sign != b.same_sign) {
      return a.same_sign;
    }
    if (a.voucher->event.date != b.voucher->event.date) {
      return a.voucher->event.date > b.voucher->event.date;
    }
    return a.position > b.position;
  });
  std::set<std::set<std::pair<std::string, bool>>> patterns;
  std::vector<Voucher const*> found;
  for (auto const& candidate : candidates) {
    std::set<std::pair<std::string, bool>> pattern;
    for (auto const& posting : candidate.voucher->postings) {
      if (posting.kind != Posting::Kind::removed) {
        pattern.insert({posting.account, posting.amount > 0});
      }
    }
    if (found.size() < most && patterns.insert(std::move(pattern)).second) {
      found.push_back(candidate.voucher);
    }
  }
  return found;
}

Postings without_present(Postings guide, Postings const& postings) {
  for (auto const& posting : postings) {
    auto const it = std::ranges::find(guide, posting.account, &Posting::account);
    if (it != guide.end()) {
      guide.erase(it);
    }
  }
  return guide;
}

Guide guide_of(Books const& books, Voucher const& like, Amount amount) {
  Guide guide;
  for (auto const& posting : like.postings) {
    if (posting.kind == Posting::Kind::removed) {
      continue;
    }
    if (!books.account(posting.account)) {
      guide.missing.push_back(posting.account);
      continue;
    }
    Posting row;
    row.account = posting.account;
    row.amount = scaled(posting.amount, amount, like.event.amount);
    guide.postings.push_back(std::move(row));
  }
  if (!guide.postings.empty()) {
    guide.postings.back().amount -= sum(guide.postings);
  }
  return guide;
}

std::string next_number(FiscalYear const& year, std::string_view series) {
  long long highest = 0;
  for (auto const& voucher : year.vouchers) {
    if (voucher.series == series && all_digits(voucher.number) && voucher.number.size() < 18) {
      highest = std::max(highest, std::stoll(voucher.number));
    }
  }
  return std::to_string(highest + 1);
}

std::expected<Row, std::string> parse_row(std::string_view text) {
  auto const words = split(text);
  if (words.empty()) {
    return std::unexpected("No account.");
  }
  Row row{words.front(), std::nullopt};
  std::string amount_text;
  for (std::size_t i = 1; i < words.size(); ++i) {
    amount_text += without_no_break_spaces(words[i]);
  }
  if (!amount_text.empty()) {
    auto const amount = amount_of(amount_text);
    if (!amount) {
      return std::unexpected(amount.error());
    }
    row.amount = *amount;
  }
  return row;
}

std::vector<Account const*> find_accounts(Books const& books, std::string_view query) {
  auto const wanted = folded(query);
  std::vector<Account const*> found;
  for (auto const& account : books.accounts) {
    if (account.number.starts_with(query) || folded(account.name).find(wanted) != std::string::npos) {
      found.push_back(&account);
    }
  }
  return found;
}

Amount Draft::remaining() const {
  return -sum(postings);
}

Amount Draft::target() const {
  return event.amount < 0 ? -event.amount : event.amount;
}

bool Draft::is_balanced() const {
  return postings.size() >= 2 && debit(postings) == target() && credit(postings) == target();
}

std::string Draft::status() const {
  if (postings.empty()) {
    return "No rows.";
  }
  if (is_balanced()) {
    return "Balanced. Enter saves.";
  }
  auto const totals = std::format("ebit {}, credit {}, amount {}", format_amount(debit(postings)),
                                  format_amount(credit(postings)), format_amount(target()));
  if (debit(postings) > target() || credit(postings) > target()) {
    return "D" + totals + ": more than the amount.";
  }
  if (remaining() != 0) {
    return std::format("Remaining {} (d{}).", format_amount(remaining()), totals);
  }
  return "D" + totals + ": debit and credit must each reach the amount.";
}

std::optional<Posting> Draft::suggestion() const {
  if (is_balanced() || postings.size() < guide_start || postings.size() - guide_start >= guide.size()) {
    return std::nullopt;
  }
  auto const next = postings.size() - guide_start;
  auto row = guide[next];
  if (next + 1 == guide.size() && remaining() != 0) {
    row.amount = remaining();
  }
  return row;
}

Postings Draft::suggested_postings() const {
  auto draft = *this;
  while (auto const row = draft.suggestion()) {
    draft.postings.push_back(*row);
  }
  return draft.postings;
}

void Draft::flip_guide() {
  for (auto& row : guide) {
    row.amount = -row.amount;
  }
  flipped = !flipped;
  if (postings.size() > guide_start) {
    postings.resize(guide_start);
  }
}

Voucher Draft::voucher(std::string number, Date registered) const {
  Voucher result{series, std::move(number), {event.heading, target(), event.date}, registered, {}, postings};
  for (auto& posting : result.postings) {
    posting.source.clear(); // not kept in the voucher (yet)
  }
  return result;
}

} // namespace cratchit::books

#include "core/date.hpp"

namespace cratchit {

std::expected<Date, std::string> parse_date(std::string_view text, std::string_view format) {
  auto const fail = [&] { return std::unexpected("invalid date \"" + std::string{text} + "\" (expected " + std::string{format} + ")"); };
  if (text.size() != format.size()) return fail();
  int year = 0;
  unsigned month = 0;
  unsigned day = 0;
  for (std::size_t i = 0; i < format.size(); ++i) {
    char const f = format[i];
    char const c = text[i];
    if (f == 'Y' || f == 'M' || f == 'D') {
      if (c < '0' || c > '9') return fail();
      int const digit = c - '0';
      if (f == 'Y') year = year * 10 + digit;
      if (f == 'M') month = month * 10 + static_cast<unsigned>(digit);
      if (f == 'D') day = day * 10 + static_cast<unsigned>(digit);
    } else if (f != c) {
      return fail();
    }
  }
  Date const date{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};
  if (!date.ok()) return fail();
  return date;
}

std::string to_yyyymmdd(Date date) {
  auto const y = static_cast<int>(date.year());
  auto const m = static_cast<unsigned>(date.month());
  auto const d = static_cast<unsigned>(date.day());
  auto const two = [](unsigned v) { return (v < 10 ? "0" : "") + std::to_string(v); };
  return std::to_string(y) + two(m) + two(d);
}

} // namespace cratchit

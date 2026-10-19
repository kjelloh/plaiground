#include "date.hpp"

#include <charconv>

namespace cratchit {

std::expected<Date, std::string> parse_date(std::string_view text) {
  auto error = [&] { return std::unexpected("Invalid date (expected YYYYMMDD): \"" + std::string{text} + "\""); };

  if (text.size() != 8) return error();
  int value = 0;
  auto const [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || end != text.data() + text.size()) return error();

  Date const date{std::chrono::year{value / 10000},
                  std::chrono::month{static_cast<unsigned>(value / 100 % 100)},
                  std::chrono::day{static_cast<unsigned>(value % 100)}};
  if (!date.ok()) return error();
  return date;
}

} // namespace cratchit

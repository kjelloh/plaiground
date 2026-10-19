#include "core/amount.hpp"

#include <limits>

namespace cratchit {

std::expected<Amount, std::string> Amount::parse(std::string_view text, char decimal_point, std::string_view thousands) {
  auto const fail = [&] { return std::unexpected("invalid amount \"" + std::string{text} + "\""); };
  bool const skip_nbsp = thousands.find(' ') != std::string_view::npos;
  std::string_view s = text;
  bool negative = false;
  if (!s.empty() && (s.front() == '-' || s.front() == '+')) {
    negative = s.front() == '-';
    s.remove_prefix(1);
  }
  std::int64_t whole = 0;
  std::int64_t fraction = 0;
  int fraction_digits = -1; // -1: no decimal point seen
  bool any_digit = false;
  for (std::size_t i = 0; i < s.size(); ++i) {
    char const c = s[i];
    if (c >= '0' && c <= '9') {
      any_digit = true;
      if (fraction_digits < 0) {
        if (whole > std::numeric_limits<std::int64_t>::max() / 1000) return fail();
        whole = whole * 10 + (c - '0');
      } else {
        if (++fraction_digits > 2) return fail();
        fraction = fraction * 10 + (c - '0');
      }
    } else if (c == decimal_point && fraction_digits < 0) {
      fraction_digits = 0;
    } else if (fraction_digits < 0 && thousands.find(c) != std::string_view::npos) {
      continue;
    } else if (skip_nbsp && fraction_digits < 0 && c == '\xC2' && i + 1 < s.size() && s[i + 1] == '\xA0') {
      ++i;
    } else {
      return fail();
    }
  }
  if (!any_digit) return fail();
  if (fraction_digits == 1) fraction *= 10;
  auto const ore = whole * 100 + fraction;
  return Amount::from_ore(negative ? -ore : ore);
}

std::string to_string(Amount amount) {
  auto const ore = amount.ore();
  auto const magnitude = ore < 0 ? -ore : ore;
  auto cents = std::to_string(magnitude % 100);
  if (cents.size() < 2) cents.insert(0, "0");
  return (ore < 0 ? "-" : "") + std::to_string(magnitude / 100) + "." + cents;
}

} // namespace cratchit

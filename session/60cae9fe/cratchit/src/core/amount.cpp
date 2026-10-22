#include "core/amount.hpp"

#include <format>

namespace cratchit {

std::optional<Amount> parse_amount(std::string_view text) {
  bool const negative = text.starts_with('-');
  if (negative) {
    text.remove_prefix(1);
  }
  auto const point = text.find('.');
  auto const whole = text.substr(0, point);
  auto const decimals = point == std::string_view::npos ? std::string_view{} : text.substr(point + 1);
  if (whole.empty() || whole.size() > 15 || (point != std::string_view::npos && decimals.empty()) ||
      decimals.size() > 2) {
    return std::nullopt;
  }
  Amount ore = 0;
  for (char const c : whole) {
    if (c < '0' || c > '9') {
      return std::nullopt;
    }
    ore = ore * 10 + (c - '0');
  }
  Amount fraction = 0;
  for (char const c : decimals) {
    if (c < '0' || c > '9') {
      return std::nullopt;
    }
    fraction = fraction * 10 + (c - '0');
  }
  if (decimals.size() == 1) {
    fraction *= 10;
  }
  ore = ore * 100 + fraction;
  return negative ? -ore : ore;
}

std::string format_amount(Amount amount) {
  auto const magnitude = amount < 0 ? -static_cast<std::uint64_t>(amount) : static_cast<std::uint64_t>(amount);
  return std::format("{}{}.{:02}", amount < 0 ? "-" : "", magnitude / 100, magnitude % 100);
}

} // namespace cratchit

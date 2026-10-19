#include "amount.hpp"

#include <cstddef>

namespace cratchit {

namespace {

constexpr std::size_t max_whole_digits = 15; // keeps cents well inside int64_t

bool is_digit(char c) { return c >= '0' && c <= '9'; }

} // namespace

std::expected<Amount, std::string> parse_amount(std::string_view text) {
  auto const original = text;
  auto error = [&](std::string_view reason) {
    return std::unexpected(std::string{reason} + ": \"" + std::string{original} + "\"");
  };

  bool negative = false;
  if (!text.empty() && text.front() == '-') {
    negative = true;
    text.remove_prefix(1);
  }

  auto const dot = text.find('.');
  auto const whole = text.substr(0, dot);
  auto const decimals = dot == std::string_view::npos ? std::string_view{} : text.substr(dot + 1);

  if (whole.empty()) return error("Amount has no whole part");
  if (whole.size() > max_whole_digits) return error("Amount too large");
  if (dot != std::string_view::npos && decimals.empty()) return error("Amount has no decimals after '.'");
  if (decimals.size() > 2) return error("Amount has more than two decimals");

  std::int64_t cents = 0;
  for (char c : whole) {
    if (!is_digit(c)) return error("Amount contains invalid character");
    cents = cents * 10 + (c - '0');
  }
  cents *= 100;

  std::int64_t scale = 10;
  for (char c : decimals) {
    if (!is_digit(c)) return error("Amount contains invalid character");
    cents += (c - '0') * scale;
    scale /= 10;
  }

  return Amount::from_cents(negative ? -cents : cents);
}

} // namespace cratchit

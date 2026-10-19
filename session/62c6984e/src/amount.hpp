#pragma once

#include <compare>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace cratchit {

// A money amount in SEK, stored as an integer number of öre (1/100 krona).
class Amount {
public:
  constexpr Amount() = default;

  static constexpr Amount from_cents(std::int64_t cents) { return Amount{cents}; }

  constexpr std::int64_t cents() const { return m_cents; }

  constexpr Amount operator-() const { return Amount{-m_cents}; }
  constexpr Amount& operator+=(Amount other) {
    m_cents += other.m_cents;
    return *this;
  }
  constexpr Amount& operator-=(Amount other) {
    m_cents -= other.m_cents;
    return *this;
  }
  friend constexpr Amount operator+(Amount a, Amount b) { return a += b; }
  friend constexpr Amount operator-(Amount a, Amount b) { return a -= b; }

  friend constexpr auto operator<=>(Amount, Amount) = default;

private:
  constexpr explicit Amount(std::int64_t cents) : m_cents{cents} {}

  std::int64_t m_cents{0};
};

// Parses an amount as written in SIE files: optional '-', digits,
// optional '.' followed by at most two decimals (e.g. "-180", "105.8", "1798.75").
std::expected<Amount, std::string> parse_amount(std::string_view text);

} // namespace cratchit

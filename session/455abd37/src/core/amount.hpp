#pragma once

// Money as whole öre (1/100 krona). Never floating point.

#include <compare>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace cratchit {

class Amount {
public:
  constexpr Amount() = default;
  static constexpr Amount from_ore(std::int64_t ore) { return Amount{ore}; }

  // Parses "-1 158,75", "250.5", "1997". At most two decimals.
  // 'thousands' lists characters to skip (e.g. " "); a no-break space (U+00A0) is
  // skipped when 'thousands' contains a blank.
  static std::expected<Amount, std::string> parse(std::string_view text, char decimal_point = '.',
                                                  std::string_view thousands = "");

  constexpr std::int64_t ore() const { return m_ore; }
  constexpr bool is_zero() const { return m_ore == 0; }

  // Drops the öre, towards zero: 123.99 -> 123.00, -123.99 -> -123.00
  constexpr Amount truncated_to_krona() const { return Amount{m_ore / 100 * 100}; }
  constexpr std::int64_t whole_kronor() const { return m_ore / 100; }

  constexpr Amount operator-() const { return Amount{-m_ore}; }
  constexpr Amount& operator+=(Amount other) {
    m_ore += other.m_ore;
    return *this;
  }
  friend constexpr Amount operator+(Amount a, Amount b) { return a += b; }
  friend constexpr Amount operator-(Amount a, Amount b) { return a += -b; }
  friend constexpr auto operator<=>(Amount, Amount) = default;

private:
  constexpr explicit Amount(std::int64_t ore) : m_ore{ore} {}
  std::int64_t m_ore{};
};

// "-250.50"
std::string to_string(Amount amount);

} // namespace cratchit

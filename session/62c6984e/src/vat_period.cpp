#include "vat_period.hpp"

#include <charconv>

namespace cratchit {

std::expected<VatPeriod, std::string> parse_vat_period(std::string_view text) {
  auto error = [&] { return std::unexpected("Invalid VAT period (expected YYYYMM): \"" + std::string{text} + "\""); };

  if (text.size() != 6) return error();
  int value = 0;
  auto const [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || end != text.data() + text.size()) return error();

  VatPeriod const period{std::chrono::year{value / 100}, std::chrono::month{static_cast<unsigned>(value % 100)}};
  if (!period.ok()) return error();
  return period;
}

std::string to_string(VatPeriod period) {
  auto const value = static_cast<int>(period.year()) * 100 + static_cast<int>(static_cast<unsigned>(period.month()));
  return std::to_string(value);
}

} // namespace cratchit

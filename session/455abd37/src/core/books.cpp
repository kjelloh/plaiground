#include "core/books.hpp"

namespace cratchit {

std::string voucher_id(Voucher const& voucher) { return voucher.m_series + "-" + voucher.m_number; }

std::optional<DateRange> Books::current_year() const {
  for (auto const& year : m_financial_years) {
    if (year.m_year_index == 0) return year.m_range;
  }
  return std::nullopt;
}

} // namespace cratchit

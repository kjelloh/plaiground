#pragma once

#include <chrono>
#include <expected>
#include <string>
#include <string_view>

namespace cratchit {

using Date = std::chrono::year_month_day;

// 'format' uses YYYY, MM and DD; other characters must match literally.
// "YYYYMMDD" (SIE and cspec), "YYYY/MM/DD" (Nordea), "YYYY-MM-DD" (Skatteverket).
std::expected<Date, std::string> parse_date(std::string_view text, std::string_view format = "YYYYMMDD");

std::string to_yyyymmdd(Date date);

struct DateRange {
  Date m_first;
  Date m_last;
  bool contains(Date date) const { return m_first <= date && date <= m_last; }
};

} // namespace cratchit

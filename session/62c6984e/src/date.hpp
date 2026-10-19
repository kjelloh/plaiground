#pragma once

#include <chrono>
#include <expected>
#include <string>
#include <string_view>

namespace cratchit {

using Date = std::chrono::year_month_day;

// Parses a date written as YYYYMMDD (as in SIE files).
std::expected<Date, std::string> parse_date(std::string_view text);

} // namespace cratchit

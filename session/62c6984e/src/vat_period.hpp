#pragma once

#include <chrono>
#include <expected>
#include <string>
#include <string_view>

namespace cratchit {

// A VAT reporting period (redovisningsperiod), identified by its last month.
// A quarterly period is named by the last month of the quarter (e.g. 202409 = Jul–Sep 2024).
using VatPeriod = std::chrono::year_month;

// Parses a period written as YYYYMM.
std::expected<VatPeriod, std::string> parse_vat_period(std::string_view text);

// Formats a period as YYYYMM.
std::string to_string(VatPeriod period);

} // namespace cratchit

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace cratchit {

// An amount of money in öre (hundredths), never floating point.
// Debit is positive, credit negative, as in SIE.
using Amount = std::int64_t;

// An amount as SIE writes it: an optional `-`, digits, and optionally `.` with
// one or two decimals, e.g. `-6498.77`, `13000` or `1.5`. No `+`, no spaces, at
// most 15 digits before the point. Nothing for anything else.
std::optional<Amount> parse_amount(std::string_view text);

// With two decimals and a point, e.g. `-6498.77`, `13000.00`
std::string format_amount(Amount amount);

} // namespace cratchit

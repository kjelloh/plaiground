#pragma once

// Journal -> VAT return boxes (interface journal_to_vat_boxes)

#include "core/books.hpp"
#include "vat/vat_spec.hpp"

#include <expected>
#include <map>
#include <string>
#include <vector>

namespace cratchit::vat {

// The reporting period, e.g. July-September 2024 is "202409" with length 3.
struct VatPeriod {
  std::chrono::year_month m_last_month;
  int m_months{3};

  static std::expected<VatPeriod, std::string> parse(std::string_view yyyymm, std::string_view period_length);
  DateRange range() const;
  std::string to_string() const; // "202409"
};

struct VatReturn {
  std::string m_org_nr;
  VatPeriod m_period;
  std::map<int, Amount> m_boxes; // whole kronor, zero boxes left out (except 49)
  std::string m_note;
  std::vector<std::string> m_vouchers_used; // "A-12", for the provenance record
};

// 'books' may hold several financial years: a VAT quarter can span two of them.
std::expected<VatReturn, std::string> compute_vat_return(std::vector<Books> const& books, VatPeriod const& period,
                                                         std::string const& settlement_series, VatSpec const& spec);

} // namespace cratchit::vat

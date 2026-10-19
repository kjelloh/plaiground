#pragma once

#include "sie_document.hpp"
#include "vat_return.hpp"

#include <expected>
#include <span>
#include <string>

namespace cratchit {

struct VatCalculationOptions {
  int m_period_months = 3;               // 1 = monthly, 3 = quarterly, 12 = yearly
  std::string m_settlement_series = "M"; // voucher series of the VAT settlement vouchers ("Momsrapport")
};

// Calculates the VAT return for the period ending with the month `period`.
//
// - Vouchers count when their date is in the period (corrections count in the period they are dated in).
// - Vouchers in the settlement series are left out: they only move the period's VAT to 1650/2650.
// - Each box is the sum of its accounts' postings (see vat_account_mapping), truncated to whole
//   kronor (öre dropped, as SKV requires). Boxes that are zero are left out.
// - Box 49 = output VAT boxes (10-12, 30-32, 60-62) - box 48, computed from the truncated values.
//
// `vouchers` may come from several SIE files, since a period can span two financial years.
// Fails if a posting is on a VAT account that has no box mapping.
std::expected<VatReturn, std::string> calculate_vat_return(std::span<Voucher const> vouchers,
                                                           std::string const& org_nr,
                                                           VatPeriod period,
                                                           VatCalculationOptions const& options = {});

} // namespace cratchit

#include "vat_calculation.hpp"

#include "vat_account_mapping.hpp"

#include <format>

namespace cratchit {

namespace {

constexpr VatBoxNumber output_vat_boxes[]{10, 11, 12, 30, 31, 32, 60, 61, 62};
constexpr VatBoxNumber input_vat_box = 48;
constexpr VatBoxNumber vat_to_pay_box = 49;

Amount truncate_to_kronor(Amount amount) { return Amount::from_cents(amount.cents() / 100 * 100); }

} // namespace

std::expected<VatReturn, std::string> calculate_vat_return(std::span<Voucher const> vouchers,
                                                           std::string const& org_nr,
                                                           VatPeriod period,
                                                           VatCalculationOptions const& options) {
  using namespace std::chrono;
  if (!period.ok()) return std::unexpected("Invalid VAT period");
  if (options.m_period_months < 1) return std::unexpected("VAT period length must be at least one month");

  Date const first_day{(period - months{options.m_period_months - 1}) / day{1}};
  Date const last_day{period / last};

  std::map<VatBoxNumber, Amount> sums;
  for (auto const& voucher : vouchers) {
    if (voucher.m_date < first_day || voucher.m_date > last_day) continue;
    if (voucher.m_series == options.m_settlement_series) continue;

    for (auto const& posting : voucher.m_postings) {
      if (auto const rule = find_vat_account_rule(posting.m_account)) {
        sums[rule->m_box] += rule->m_sign > 0 ? posting.m_amount : -posting.m_amount;
      } else if (requires_vat_account_rule(posting.m_account)) {
        return std::unexpected(std::format("Account {} has no VAT box mapping (voucher {}{} {})", posting.m_account,
                                           voucher.m_series, voucher.m_number, voucher.m_date));
      }
    }
  }

  VatReturn result{.m_org_nr = org_nr, .m_period = period};
  for (auto const& [box, sum] : sums) {
    if (auto const kronor = truncate_to_kronor(sum); kronor != Amount{}) result.m_boxes[box] = kronor;
  }

  Amount to_pay;
  for (auto box : output_vat_boxes) {
    if (auto const it = result.m_boxes.find(box); it != result.m_boxes.end()) to_pay += it->second;
  }
  if (auto const it = result.m_boxes.find(input_vat_box); it != result.m_boxes.end()) to_pay -= it->second;
  result.m_boxes[vat_to_pay_box] = to_pay;

  return result;
}

} // namespace cratchit

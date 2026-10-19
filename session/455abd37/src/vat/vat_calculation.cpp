#include "vat/vat_calculation.hpp"

#include <set>

namespace cratchit::vat {

std::expected<VatPeriod, std::string> VatPeriod::parse(std::string_view yyyymm, std::string_view period_length) {
  auto const date = parse_date(std::string{yyyymm} + "01");
  if (yyyymm.size() != 6 || !date) return std::unexpected("invalid VAT period \"" + std::string{yyyymm} + "\" (expected YYYYMM)");
  int const months = period_length == "month" ? 1 : period_length == "quarter" ? 3 : period_length == "year" ? 12 : 0;
  if (months == 0) return std::unexpected("unknown VAT period length \"" + std::string{period_length} + "\"");
  auto const month = static_cast<unsigned>(date->month());
  if (months == 3 && month % 3 != 0) return std::unexpected("a quarter ends in month 03, 06, 09 or 12");
  return VatPeriod{date->year() / date->month(), months};
}

DateRange VatPeriod::range() const {
  auto const first = m_last_month - std::chrono::months{m_months - 1};
  return {first / std::chrono::day{1}, m_last_month / std::chrono::last};
}

std::string VatPeriod::to_string() const { return to_yyyymmdd(m_last_month / std::chrono::day{1}).substr(0, 6); }

std::expected<VatReturn, std::string> compute_vat_return(std::vector<Books> const& books, VatPeriod const& period,
                                                         std::string const& settlement_series, VatSpec const& spec) {
  if (books.empty()) return std::unexpected("no books given");
  VatReturn result{books.front().m_org_nr, period, {}, {}, {}};
  auto const range = period.range();

  std::map<int, Amount> raw; // signed as the box wants it, still with öre
  std::set<std::string> seen_vouchers;
  for (auto const& year : books) {
    if (year.m_org_nr != result.m_org_nr) return std::unexpected("books for different org numbers");
    auto const year_key = year.current_year() ? to_yyyymmdd(year.current_year()->m_first) : std::string{"?"};
    for (auto const& voucher : year.m_vouchers) {
      if (!range.contains(voucher.m_date)) continue;
      if (spec.m_exclude_settlement && voucher.m_series == settlement_series) continue;
      if (!seen_vouchers.insert(year_key + ":" + voucher_id(voucher)).second) {
        return std::unexpected("voucher " + voucher_id(voucher) + " appears twice for the year starting " + year_key);
      }
      bool used = false;
      for (auto const& posting : voucher.m_postings) {
        bool mapped = false;
        for (auto const& mapping : spec.m_mappings) {
          if (!mapping.m_accounts.contains(posting.m_account)) continue;
          raw[mapping.m_box] += mapping.m_credit ? -posting.m_amount : posting.m_amount;
          mapped = used = true;
        }
        if (!mapped && spec.m_vat_accounts.contains(posting.m_account) && !spec.m_unmapped_allowed.contains(posting.m_account)) {
          return std::unexpected("voucher " + voucher_id(voucher) + " posts to VAT account " + std::to_string(posting.m_account) +
                                 " that no VAT box maps (see specs/vat/bas_to_vat_box.cspec)");
        }
      }
      if (used) result.m_vouchers_used.push_back(voucher_id(voucher));
    }
  }

  Amount to_pay;
  for (auto const& [number, amount] : raw) {
    auto const value = spec.m_truncate ? amount.truncated_to_krona() : amount;
    auto const* box = spec.find_box(number);
    if (box->m_kind == "output_vat") to_pay += value;
    if (box->m_kind == "input_vat") to_pay += -value;
    if (!value.is_zero()) result.m_boxes[number] = value;
  }
  result.m_boxes[49] = to_pay;
  return result;
}

} // namespace cratchit::vat

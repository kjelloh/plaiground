#include "legal/compliance.hpp"

#include <array>

namespace cratchit::legal {

namespace {

constexpr std::array<Mechanism, 5> known_mechanisms{{
    {"voucher_balances", "implemented"},
    {"voucher_required_fields", "implemented"},
    {"output_provenance", "partial"},
    {"spec_files_as_system_documentation", "partial"},
    {"vat_rules_from_spec", "partial"},
}};

} // namespace

std::span<Mechanism const> mechanisms() { return known_mechanisms; }

std::vector<Finding> check_voucher(Voucher const& voucher) {
  std::vector<Finding> findings;
  auto const id = voucher_id(voucher);
  Amount sum;
  for (auto const& posting : voucher.m_postings) sum += posting.m_amount;
  if (!sum.is_zero()) findings.push_back({"voucher_balances", id + " does not balance (sum " + to_string(sum) + ")"});
  if (voucher.m_series.empty() || voucher.m_number.empty()) findings.push_back({"voucher_required_fields", id + " has no series or number"});
  if (voucher.m_text.empty()) findings.push_back({"voucher_required_fields", id + " has no text"});
  if (voucher.m_postings.size() < 2) findings.push_back({"voucher_required_fields", id + " has fewer than two postings"});
  return findings;
}

std::vector<Finding> check_books(Books const& books) {
  std::vector<Finding> findings;
  for (auto const& voucher : books.m_vouchers) {
    auto more = check_voucher(voucher);
    findings.insert(findings.end(), more.begin(), more.end());
  }
  return findings;
}

} // namespace cratchit::legal

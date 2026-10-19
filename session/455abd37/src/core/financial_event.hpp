#pragma once

// A financial event (the old cratchit's HAD: Heading, Amount, Date) that seeds a voucher.
// Listing format: specs/events/financial_event.cspec

#include "core/account.hpp"
#include "core/amount.hpp"
#include "core/date.hpp"
#include "cspec/spec_tree.hpp"

#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace cratchit {

struct FinancialEvent {
  Date m_date;
  Amount m_amount; // + money in, - money out, seen from m_account
  std::string m_heading;
  std::optional<AccountNo> m_account;
  std::string m_source_interface;
  std::string m_source_file;
  std::optional<int> m_source_row;
};

// Writes events as a cspec listing (event.0001.date = …).
std::string to_cspec(std::vector<FinancialEvent> const& events);

// Reads a listing back.
std::expected<std::vector<FinancialEvent>, std::string> events_from_cspec(cspec::SpecTree const& tree);

} // namespace cratchit

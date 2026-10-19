#pragma once

// Account statement CSV -> financial events (interfaces bank_statement_import,
// skv_account_statement_import and statement_to_events).
// Columns, separators and amount format come from specs/statements/*.cspec.

#include "core/financial_event.hpp"
#include "cspec/spec_tree.hpp"

#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace cratchit::statements {

struct StatementSpec {
  std::string m_id;
  std::string m_encoding; // utf-8 | iso-8859-1
  char m_separator{';'};
  char m_quote{'"'};
  int m_header_rows{};
  std::vector<std::string> m_columns;
  char m_decimal_separator{','};
  std::string m_thousands_separators;
  std::size_t m_date_column{};
  std::string m_date_format;
  std::size_t m_amount_column{};
  std::vector<std::size_t> m_heading_columns;
  std::string m_heading_join;
  std::string m_account_setting;

  static std::expected<StatementSpec, std::string> from(cspec::SpecTree const& spec);
};

// 'content' is the raw file content; it is decoded per the spec.
std::expected<std::vector<FinancialEvent>, std::string> read_statement(std::string_view content, StatementSpec const& spec,
                                                                       std::string const& source_interface,
                                                                       std::string const& source_file,
                                                                       std::optional<AccountNo> account);

} // namespace cratchit::statements

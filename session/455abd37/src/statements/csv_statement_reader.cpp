#include "statements/csv_statement_reader.hpp"

#include "core/text.hpp"

#include <algorithm>

namespace cratchit::statements {

namespace {

std::expected<char, std::string> single_char(cspec::SpecTree const& spec, std::string const& path) {
  auto const value = spec.require(path);
  if (!value) return std::unexpected(value.error());
  if (value->size() != 1) return std::unexpected(spec.where(path) + ": expected one character");
  return value->front();
}

std::expected<std::size_t, std::string> column_index(StatementSpec const& spec, cspec::SpecTree const& tree, std::string const& path,
                                                     std::string const& name) {
  auto const it = std::ranges::find(spec.m_columns, name);
  if (it == spec.m_columns.end()) return std::unexpected(tree.where(path) + ": no column named \"" + name + "\"");
  return static_cast<std::size_t>(it - spec.m_columns.begin());
}

std::vector<std::string> split_csv_row(std::string_view line, char separator, char quote) {
  std::vector<std::string> fields(1);
  bool in_quotes = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    char const c = line[i];
    if (c == quote) {
      if (in_quotes && i + 1 < line.size() && line[i + 1] == quote) {
        fields.back() += quote;
        ++i;
      } else {
        in_quotes = !in_quotes;
      }
    } else if (c == separator && !in_quotes) {
      fields.emplace_back();
    } else {
      fields.back() += c;
    }
  }
  return fields;
}

} // namespace

std::expected<StatementSpec, std::string> StatementSpec::from(cspec::SpecTree const& tree) {
  StatementSpec spec;
  spec.m_id = tree.value_or("spec.id", "?");
  spec.m_encoding = tree.value_or("file.encoding", "utf-8");
  if (spec.m_encoding != "utf-8" && spec.m_encoding != "iso-8859-1") {
    return std::unexpected(tree.where("file.encoding") + ": unsupported encoding");
  }
  auto const separator = single_char(tree, "csv.separator");
  if (!separator) return std::unexpected(separator.error());
  spec.m_separator = *separator;
  auto const quote = single_char(tree, "csv.quote");
  if (!quote) return std::unexpected(quote.error());
  spec.m_quote = *quote;
  spec.m_header_rows = std::stoi(tree.value_or("csv.header_rows", "0"));
  for (auto const& key : tree.children("column")) spec.m_columns.push_back(tree.value_or("column." + key, ""));
  auto const decimal = single_char(tree, "amount.decimal_separator");
  if (!decimal) return std::unexpected(decimal.error());
  spec.m_decimal_separator = *decimal;
  spec.m_thousands_separators = tree.value_or("amount.thousands_separator", "");

  auto const date_column = column_index(spec, tree, "event.date.column", tree.value_or("event.date.column", ""));
  if (!date_column) return std::unexpected(date_column.error());
  spec.m_date_column = *date_column;
  auto const date_format = tree.require("event.date.format");
  if (!date_format) return std::unexpected(date_format.error());
  spec.m_date_format = *date_format;
  auto const amount_column = column_index(spec, tree, "event.amount.column", tree.value_or("event.amount.column", ""));
  if (!amount_column) return std::unexpected(amount_column.error());
  spec.m_amount_column = *amount_column;
  for (auto const& name : text::split_list(tree.value_or("event.heading.columns", ""))) {
    auto const index = column_index(spec, tree, "event.heading.columns", name);
    if (!index) return std::unexpected(index.error());
    spec.m_heading_columns.push_back(*index);
  }
  spec.m_heading_join = tree.value_or("event.heading.join", " ");
  spec.m_account_setting = tree.value_or("event.account.setting", "");
  if (tree.value_or("row.require_date", "no") != "yes") return std::unexpected(tree.source() + ": only row.require_date = yes is implemented");
  return spec;
}

std::expected<std::vector<FinancialEvent>, std::string> read_statement(std::string_view content, StatementSpec const& spec,
                                                                       std::string const& source_interface,
                                                                       std::string const& source_file,
                                                                       std::optional<AccountNo> account) {
  auto const text_utf8 = spec.m_encoding == "iso-8859-1" ? text::latin1_to_utf8(content) : std::string{text::strip_bom(content)};
  std::vector<FinancialEvent> events;
  int row_number = 0;
  for (auto line : text::split_lines(text_utf8)) {
    ++row_number;
    if (row_number <= spec.m_header_rows || text::trim(line).empty()) continue;
    auto const fields = split_csv_row(line, spec.m_separator, spec.m_quote);
    auto const field = [&](std::size_t index) { return index < fields.size() ? std::string{text::trim(fields[index])} : std::string{}; };

    auto const date = parse_date(field(spec.m_date_column), spec.m_date_format);
    if (!date) continue; // balance rows, company rows and the like (row.require_date = yes)
    auto const amount = Amount::parse(field(spec.m_amount_column), spec.m_decimal_separator, spec.m_thousands_separators);
    if (!amount) return std::unexpected(source_file + ":" + std::to_string(row_number) + ": " + amount.error());

    std::string heading;
    for (auto const index : spec.m_heading_columns) {
      auto const part = field(index);
      if (part.empty()) continue;
      if (!heading.empty()) heading += spec.m_heading_join;
      heading += part;
    }
    events.push_back({*date, *amount, heading, account, source_interface, source_file, row_number});
  }
  return events;
}

} // namespace cratchit::statements

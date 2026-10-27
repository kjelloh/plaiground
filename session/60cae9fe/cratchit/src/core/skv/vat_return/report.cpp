#include "core/skv/vat_return/report.hpp"

#include <format>

namespace cratchit::skv::vat_return {

namespace {

// Box, account, voucher, date, amount, text
std::string row(std::string_view box, std::string_view account, std::string_view voucher, std::string_view date,
                std::string_view amount, std::string_view text) {
  auto line = std::format("{:<5}{:<9}{:<9}{:<12}{:>15}", box, account, voucher, date, amount);
  if (!text.empty()) {
    line += "  ";
    line += text;
  }
  return line;
}

} // namespace

std::string quarter_name(Period period) {
  return std::format("{} Q{}", static_cast<int>(period.last.year()), (static_cast<unsigned>(period.last.month()) + 2) / 3);
}

std::vector<std::string> report_of(books::Books const& books, Document const& document, std::string_view company) {
  std::vector<std::string> lines{std::format("VAT return (momsrapport) {} - {}", format_iso_date(document.period.first),
                                             format_iso_date(document.period.last)),
                                 std::string(company), ""};
  if (document.boxes.empty()) {
    lines.push_back("No postings in the period on accounts that report in a box.");
    return lines;
  }
  lines.push_back(row("Box", "Account", "Voucher", "Date", "Amount", "Name"));
  for (auto const& box : document.boxes) {
    lines.push_back("");
    lines.push_back(row(box.box, "", "", "", format_amount(box.amount), ""));
    for (auto const& account : box.accounts) {
      auto const* known = books.account(account.account);
      lines.push_back(row("", account.account, "", "", format_amount(account.amount), known ? known->name : ""));
      for (auto const& posting : account.postings) {
        lines.push_back(row("", "", std::format("{} {}", posting.series, posting.number), format_iso_date(posting.date),
                            format_amount(posting.amount), ""));
      }
    }
  }
  lines.push_back("");
  lines.push_back("Amounts are as in the books: debit positive, credit negative. Box 49 is not calculated yet.");
  return lines;
}

} // namespace cratchit::skv::vat_return

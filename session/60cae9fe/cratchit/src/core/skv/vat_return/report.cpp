#include "core/skv/vat_return/report.hpp"

#include <algorithm>
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

// The text of the voucher a posting is in, if the books have it
std::string heading_of(books::Books const& books, VoucherPosting const& posting) {
  auto const* year = books.year_starting(posting.fiscal_year);
  if (!year) {
    return {};
  }
  auto const it = std::ranges::find_if(year->vouchers, [&](books::Voucher const& voucher) {
    return voucher.series == posting.series && voucher.number == posting.number;
  });
  return it == year->vouchers.end() ? std::string{} : it->event.heading;
}

// A voucher of a step with its date and amount
std::string step_line(std::string_view step, VoucherPosting const& voucher) {
  return std::format("  {:<10}{:>15}  {} {}  {}", step, format_amount(voucher.amount), voucher.series, voucher.number,
                     format_iso_date(voucher.date));
}

} // namespace

std::string vouchers_text(std::vector<VoucherPosting> const& vouchers) {
  std::string text;
  for (auto const& voucher : vouchers) {
    text += text.empty() ? "" : ", ";
    text += std::format("{} {}", voucher.series, voucher.number);
  }
  return text;
}

std::string steps_heading() {
  return std::format("{:>12}{:>12}{:>12}", "Period", "Cleared", "Charged");
}

std::string steps_row(PeriodSteps const& steps) {
  auto line = std::format("{:>12}{:>12}{:>12}", format_amount(steps.period_vat), format_amount(steps.cleared),
                          format_amount(steps.charged));
  if (!steps.clearings.empty()) {
    line += "  cleared by " + vouchers_text(steps.clearings);
  }
  if (!steps.charges.empty()) {
    line += ", charged by " + vouchers_text(steps.charges);
  }
  return line;
}

std::string quarter_name(Period period) {
  return std::format("{} Q{}", static_cast<int>(period.last.year()), (static_cast<unsigned>(period.last.month()) + 2) / 3);
}

std::vector<std::string> report_of(books::Books const& books, Document const& document, PeriodSteps const& steps,
                                   std::string_view company, std::optional<Date> so_far) {
  std::vector<std::string> lines{std::format("VAT return (momsrapport) {} - {}{}", format_iso_date(document.period.first),
                                             format_iso_date(document.period.last),
                                             so_far ? std::format(", so far (today {})", format_iso_date(*so_far))
                                                    : std::string{}),
                                 std::string(company), ""};
  auto const add_steps = [&] {
    lines.push_back("");
    lines.push_back("The VAT of the period:");
    lines.push_back(std::format("  {:<10}{:>15}", "Period", format_amount(steps.period_vat)));
    if (steps.clearings.empty()) {
      lines.push_back(std::format("  {:<10}{:>15}", "Cleared", format_amount(0)));
    }
    for (auto const& clearing : steps.clearings) {
      lines.push_back(step_line("Cleared", clearing));
    }
    if (steps.charges.empty()) {
      lines.push_back(std::format("  {:<10}{:>15}", "Charged", format_amount(0)));
    }
    for (auto const& charge : steps.charges) {
      lines.push_back(step_line("Charged", charge));
    }
  };
  if (document.boxes.empty()) {
    lines.push_back("No postings in the period on accounts that report in a box.");
    add_steps();
    return lines;
  }
  lines.push_back(row("Box", "Account", "Voucher", "Date", "Amount", "Text"));
  for (auto const& box : document.boxes) {
    lines.push_back("");
    lines.push_back(row(box.box, "", "", "", format_amount(box.amount), ""));
    for (auto const& account : box.accounts) {
      auto const* known = books.account(account.account);
      lines.push_back(row("", account.account, "", "", format_amount(account.amount), known ? known->name : ""));
      for (auto const& posting : account.postings) {
        lines.push_back(row("", "", std::format("{} {}", posting.series, posting.number), format_iso_date(posting.date),
                            format_amount(posting.amount), heading_of(books, posting)));
      }
    }
  }
  auto const to_pay = vat_to_pay(document);
  lines.push_back("");
  lines.push_back(row(vat_to_pay_box, "", "", "", format_amount(to_pay), vat_to_pay_expression));
  add_steps();
  lines.push_back("");
  lines.push_back("Amounts are as in the books: debit positive, credit negative.");
  lines.push_back(to_pay > 0   ? std::format("Box 49 on the form: {} to pay.", format_amount(to_pay))
                  : to_pay < 0 ? std::format("Box 49 on the form: {} to get back.", format_amount(-to_pay))
                               : std::string("Box 49 on the form: 0.00, nothing to pay or get back."));
  return lines;
}

} // namespace cratchit::skv::vat_return

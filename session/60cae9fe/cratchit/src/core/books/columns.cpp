#include "core/books/columns.hpp"

#include <format>

namespace cratchit::books {

namespace {

std::string row(std::string_view first, Sides const& sides, std::string_view rest) {
  auto line = std::format("{:<7}{:>13}{:>13}", first, sides.debit, sides.credit);
  if (!rest.empty()) {
    return std::format("{}  {}", line, rest);
  }
  line.erase(line.find_last_not_of(' ') + 1);
  return line;
}

} // namespace

Sides sides_of(Amount amount) {
  if (amount < 0) {
    return {"", format_amount(-amount)};
  }
  return {format_amount(amount), ""};
}

std::string posting_heading() {
  return row("Account", {"Debit", "Credit"}, "");
}

std::string posting_row(std::string_view account, Amount amount, std::string_view rest) {
  return row(account, sides_of(amount), rest);
}

std::string posting_total(Postings const& postings) {
  return row("Total", {format_amount(debit(postings)), format_amount(credit(postings))}, "");
}

std::string posting_brief(Posting const& posting) {
  return std::format("{} {} {}", posting.account, posting.amount < 0 ? "C" : "D",
                     format_amount(posting.amount < 0 ? -posting.amount : posting.amount));
}

} // namespace cratchit::books

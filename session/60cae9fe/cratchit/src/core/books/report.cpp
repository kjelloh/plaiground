#include "core/books/report.hpp"

#include <algorithm>
#include <format>
#include <set>

namespace cratchit::books {

namespace {

std::string year_text(FiscalYear const& year) {
  return std::format("{} - {}", format_iso_date(year.start), format_iso_date(year.end));
}

std::string account_name(Books const& books, std::string_view number) {
  auto const* account = books.account(number);
  return account ? account->name : std::string{};
}

Amount value_or_zero(std::map<std::string, Amount> const& amounts, std::string const& account) {
  auto const it = amounts.find(account);
  return it == amounts.end() ? 0 : it->second;
}

bool number_less(std::string const& a, std::string const& b) {
  return a.size() != b.size() ? a.size() < b.size() : a < b;
}

std::vector<std::string> heading(std::string_view title, FiscalYear const& year, std::string_view company) {
  return {std::format("{} {}", title, year_text(year)), std::string(company), ""};
}

} // namespace

std::vector<std::string> trial_balance(Books const& books, std::size_t year_index, std::string_view company) {
  auto const& year = books.years[year_index];
  auto const all = balances(books);
  auto const& b = all[year_index];
  auto lines = heading("Trial balance (saldobalans)", year, company);

  std::set<std::string, decltype(&number_less)> accounts(&number_less);
  for (auto const* amounts : {&b.opening, &b.change, &b.closing}) {
    for (auto const& [account, amount] : *amounts) {
      accounts.insert(account);
    }
  }
  lines.push_back(std::format("{:<8}{:>15}{:>15}{:>15}  {}", "Account", "Opening", "Change", "Closing", "Name"));
  Amount opening_total = 0;
  Amount change_total = 0;
  Amount closing_total = 0;
  for (auto const& account : accounts) {
    if (account_kind(books, account) == AccountKind::result) {
      continue;
    }
    auto const opening = value_or_zero(b.opening, account);
    auto const change = value_or_zero(b.change, account);
    auto const closing = value_or_zero(b.closing, account);
    if (opening == 0 && change == 0 && closing == 0) {
      continue;
    }
    opening_total += opening;
    change_total += change;
    closing_total += closing;
    lines.push_back(std::format("{:<8}{:>15}{:>15}{:>15}  {}", account, format_amount(opening), format_amount(change),
                                format_amount(closing), account_name(books, account)));
  }
  lines.push_back(std::format("{:<8}{:>15}{:>15}{:>15}", "Total", format_amount(opening_total),
                              format_amount(change_total), format_amount(closing_total)));
  lines.push_back("");
  lines.push_back(std::format("{:<8}{:>15}  {}", "Account", "Result", "Name"));
  Amount result_total = 0;
  for (auto const& account : accounts) {
    if (account_kind(books, account) != AccountKind::result) {
      continue;
    }
    auto const change = value_or_zero(b.change, account);
    if (change == 0) {
      continue;
    }
    result_total += change;
    lines.push_back(std::format("{:<8}{:>15}  {}", account, format_amount(change), account_name(books, account)));
  }
  lines.push_back(std::format("{:<8}{:>15}", "Total", format_amount(result_total)));
  lines.push_back("");
  lines.push_back("Amounts: debit positive, credit negative.");
  if (b.carried) {
    lines.push_back(std::format("Opening balances are carried from the closing balances {}.",
                                format_iso_date(books.years[year_index - 1].end)));
    std::set<std::string, decltype(&number_less)> reported(&number_less);
    for (auto const* amounts : {&b.opening, &year.reported_opening}) {
      for (auto const& [account, amount] : *amounts) {
        reported.insert(account);
      }
    }
    for (auto const& account : reported) {
      auto const carried = value_or_zero(b.opening, account);
      auto const as_reported = value_or_zero(year.reported_opening, account);
      if (carried != as_reported) {
        lines.push_back(std::format("  {}: the source reported {}, carried {} (difference {})", account,
                                    format_amount(as_reported), format_amount(carried),
                                    format_amount(carried - as_reported)));
      }
    }
  } else {
    lines.push_back("Opening balances are as the source reported them (the year before is not in the books).");
  }
  return lines;
}

std::vector<std::string> voucher_list(Books const& books, std::size_t year_index, std::string_view company) {
  auto const& year = books.years[year_index];
  auto lines = heading("Vouchers", year, company);
  lines.push_back(std::format("{} voucher{}", year.vouchers.size(), year.vouchers.size() == 1 ? "" : "s"));
  for (auto const& voucher : year.vouchers) {
    lines.push_back("");
    lines.push_back(std::format("{} {}  {}  {}{}", voucher.series, voucher.number, format_iso_date(voucher.date),
                                voucher.text,
                                voucher.registered ? std::format("  (registered {})", format_iso_date(*voucher.registered))
                                                   : std::string{}));
    for (auto const& t : voucher.transactions) {
      auto const mark = t.kind == Transaction::Kind::added     ? "added"
                        : t.kind == Transaction::Kind::removed ? "removed"
                                                               : "";
      auto text = t.text.empty() ? account_name(books, t.account) : t.text;
      if (!t.signature.empty() && t.kind != Transaction::Kind::normal) {
        text += std::format(" (by {})", t.signature);
      }
      lines.push_back(std::format("  {:<8}{:<8}{:>15}  {}", mark, t.account, format_amount(t.amount), text));
    }
  }
  return lines;
}

std::optional<std::vector<std::string>> account_ledger(Books const& books, std::size_t year_index,
                                                       std::string_view company, std::string_view account) {
  auto const& year = books.years[year_index];
  struct Entry {
    Date date;
    Voucher const* voucher;
    Transaction const* transaction;
  };
  std::vector<Entry> entries;
  for (auto const& voucher : year.vouchers) {
    for (auto const& t : voucher.transactions) {
      if (t.account == account && t.kind != Transaction::Kind::removed) {
        entries.push_back({t.date.value_or(voucher.date), &voucher, &t});
      }
    }
  }
  if (!books.account(account) && entries.empty()) {
    return std::nullopt;
  }
  std::ranges::stable_sort(entries, {}, &Entry::date);
  auto const all = balances(books);
  auto const number = std::string(account);
  auto const name = account_name(books, account);
  auto lines = heading(std::format("Account {}{} ledger", number, name.empty() ? "" : " " + name), year, company);
  lines.push_back(std::format("{:<12}{:<10}{:>15}{:>15}  {}", "Date", "Voucher", "Amount", "Balance", "Text"));
  Amount balance = value_or_zero(all[year_index].opening, number);
  lines.push_back(std::format("{:<22}{:>15}{:>15}", "Opening balance", "", format_amount(balance)));
  for (auto const& entry : entries) {
    balance += entry.transaction->amount;
    auto const text = entry.transaction->text.empty() ? entry.voucher->text : entry.transaction->text;
    lines.push_back(std::format("{:<12}{:<10}{:>15}{:>15}  {}", format_iso_date(entry.date),
                                std::format("{} {}", entry.voucher->series, entry.voucher->number),
                                format_amount(entry.transaction->amount), format_amount(balance), text));
  }
  lines.push_back(std::format("{:<22}{:>15}{:>15}", "Closing balance", "", format_amount(balance)));
  return lines;
}

} // namespace cratchit::books

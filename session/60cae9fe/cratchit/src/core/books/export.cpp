#include "core/books/export.hpp"

#include "core/bas.hpp"
#include "core/version.hpp"

#include <set>
#include <string>

namespace cratchit::books {

namespace {

sie::Transaction::Kind kind_of(Posting::Kind kind) {
  switch (kind) {
  case Posting::Kind::added:
    return sie::Transaction::Kind::added;
  case Posting::Kind::removed:
    return sie::Transaction::Kind::removed;
  default:
    return sie::Transaction::Kind::normal;
  }
}

sie::Voucher voucher_of(Voucher const& voucher) {
  sie::Voucher result{voucher.series, voucher.number, voucher.event.date, voucher.event.heading,
                      voucher.registered, voucher.signature, {}, 0};
  for (auto const& p : voucher.postings) {
    result.transactions.push_back({kind_of(p.kind), p.account, p.objects, p.amount, p.date, p.text, p.quantity,
                                   p.signature});
  }
  return result;
}

// #IB and #UB of the balance accounts where either is not zero; #RES of the other accounts not at zero
// (result accounts, and those BAS gives no kind, such as the 9xxx memo accounts, as Fortnox does)
void add_balances(Books const& books, YearBalances const& year, int index, sie::Document& document) {
  std::set<std::string> accounts;
  for (auto const& [account, amount] : year.opening) {
    accounts.insert(account);
  }
  for (auto const& [account, amount] : year.closing) {
    accounts.insert(account);
  }
  auto const amount_in = [](std::map<std::string, Amount> const& amounts, std::string const& account) {
    auto const it = amounts.find(account);
    return it == amounts.end() ? Amount{0} : it->second;
  };
  for (auto const& account : accounts) {
    if (account_kind(books, account) == AccountKind::balance) {
      auto const opening = amount_in(year.opening, account);
      auto const closing = amount_in(year.closing, account);
      if (opening != 0 || closing != 0) {
        document.opening.push_back({index, account, opening, {}});
        document.closing.push_back({index, account, closing, {}});
      }
    }
  }
  for (auto const& [account, amount] : year.change) {
    if (account_kind(books, account) != AccountKind::balance && amount != 0) {
      document.results.push_back({index, account, amount, {}});
    }
  }
}

} // namespace

sie::Document export_document(Books const& books, Identity const& identity, std::size_t index, Date generated) {
  sie::Document document;
  auto const& year = books.years[index];
  document.flag = "0";
  document.format = "PC8";
  document.sie_type = "4";
  document.program = "cratchit";
  document.program_version = std::string(version());
  document.generated = generated;
  document.company_id = books.company_id;
  document.company_name = identity.name;
  document.orgnr = identity.orgnr;
  document.address = {books.address.contact, books.address.street, books.address.postal, books.address.phone};
  document.years.push_back({0, year.start, year.end});
  if (index > 0) {
    document.years.push_back({-1, books.years[index - 1].start, books.years[index - 1].end});
  }
  document.covers_until = year.end;
  document.chart_type = books.chart_type;
  for (auto const& account : books.accounts) {
    document.accounts.push_back({account.number, account.name, account.type, account.sru, account.unit});
  }
  for (auto const& dimension : books.dimensions) {
    document.dimensions.push_back({dimension.number, dimension.name, dimension.super});
  }
  for (auto const& object : books.objects) {
    document.objects.push_back({object.dimension, object.number, object.name});
  }
  auto const all = balances(books);
  add_balances(books, all[index], 0, document);
  if (index > 0) {
    add_balances(books, all[index - 1], -1, document);
  }
  for (auto const& voucher : year.vouchers) {
    document.vouchers.push_back(voucher_of(voucher));
  }
  return document;
}

} // namespace cratchit::books

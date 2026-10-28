#include "core/skv/vat_return/document.hpp"

#include <algorithm>
#include <map>
#include <utility>

namespace cratchit::skv::vat_return {

std::optional<Period> quarter_of(std::chrono::year year, unsigned quarter) {
  if (quarter < 1 || quarter > 4) {
    return std::nullopt;
  }
  auto const first = std::chrono::month{3 * quarter - 2};
  auto const last = std::chrono::month{3 * quarter};
  return Period{year / first / 1, year / last / std::chrono::last};
}

Period quarter_holding(Date date) {
  return *quarter_of(date.year(), (static_cast<unsigned>(date.month()) + 2) / 3);
}

std::vector<Period> quarters_ending_in(Date first, Date last) {
  std::vector<Period> quarters;
  for (auto year = first.year(); year <= last.year(); ++year) {
    for (unsigned quarter = 1; quarter <= 4; ++quarter) {
      auto const period = *quarter_of(year, quarter);
      if (first <= period.last && period.last <= last) {
        quarters.push_back(period);
      }
    }
  }
  return quarters;
}

Document document_of(books::Books const& books, Period period) {
  // box -> account -> its postings
  std::map<Box, std::map<std::string, std::vector<VoucherPosting>>> found;
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      auto const date = voucher.event.date;
      if (date < period.first || period.last < date) {
        continue;
      }
      for (auto const& posting : voucher.postings) {
        if (posting.kind == books::Posting::Kind::removed) {
          continue;
        }
        if (auto const box = box_of(posting.account)) {
          found[*box][posting.account].push_back({year.start, voucher.series, voucher.number, date, posting.amount});
        }
      }
    }
  }

  Document document{period, {}};
  for (auto& [box, accounts] : found) {
    BoxAmount box_amount{box, 0, {}};
    for (auto& [account, postings] : accounts) {
      AccountAmount account_amount{account, 0, std::move(postings)};
      for (auto const& posting : account_amount.postings) {
        account_amount.amount += posting.amount;
      }
      box_amount.amount += account_amount.amount;
      box_amount.accounts.push_back(std::move(account_amount));
    }
    box_amount.amount = box_amount.amount / 100 * 100; // the form takes whole kronor, öre dropped
    document.boxes.push_back(std::move(box_amount));
  }
  return document;
}

Amount vat_to_pay(Document const& document) {
  Amount total = 0;
  for (auto const& box : document.boxes) {
    if (std::ranges::find(vat_to_pay_added, box.box) != vat_to_pay_added.end()) {
      total -= box.amount; // output VAT is credit in the books
    }
    if (std::ranges::find(vat_to_pay_subtracted, box.box) != vat_to_pay_subtracted.end()) {
      total -= box.amount; // input VAT is debit in the books
    }
  }
  return total;
}

} // namespace cratchit::skv::vat_return

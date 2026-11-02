#include "core/ar/closing.hpp"

#include <algorithm>
#include <format>

namespace cratchit::ar {

namespace {

bool is_account(std::string_view account) {
  return account.size() == 4 && std::ranges::all_of(account, [](char c) { return c >= '0' && c <= '9'; });
}

// The sum of the accounts first to last, four digit numbers compared as text
Amount sum_of(Balances const& balances, std::string_view first, std::string_view last) {
  Amount sum = 0;
  for (auto const& [account, amount] : balances) {
    if (is_account(account) && first <= account && account <= last) {
      sum += amount;
    }
  }
  return sum;
}

} // namespace

std::vector<std::string> check_closing(Balances const& balances, Date balance_day) {
  std::vector<std::string> notes;
  auto const day = format_iso_date(balance_day);
  if (auto const vat = sum_of(balances, "2610", "2659"); vat > 0) {
    notes.push_back(std::format("Moms (2610-2659) nets to a receivable {} at {}; BAS: move it to 1650 Momsfordran",
                                format_amount(vat), day));
  }
  if (auto const tax = sum_of(balances, "2500", "2599"); tax > 0) {
    notes.push_back(std::format(
        "Skatteskulder (2500-2599) net to a receivable {} at {}; BAS: move it to 1640 Skattefordringar",
        format_amount(tax), day));
  }
  if (auto const tax_account = sum_of(balances, "1630", "1630"); tax_account < 0) {
    notes.push_back(std::format("1630 Skattekonto has a credit balance {} at {}; BAS: move it to 2850",
                                format_amount(tax_account), day));
  }
  auto const held = sum_of(balances, "1990", "1999");
  if (auto const owed = sum_of(balances, "2830", "2839"); held != 0 && held + owed != 0) {
    notes.push_back(std::format("Redovisningsmedel (1990-1999) {} at {} is not matched by Avräkning för annans "
                                "räkning (2830-2839) {}; BAS 1990 is money held for others, not a suspense account",
                                format_amount(held), day, format_amount(-owed)));
  }
  if (auto const suspense = sum_of(balances, "2999", "2999"); suspense != 0) {
    notes.push_back(std::format("2999 OBS-konto is {} at {}; BAS: it must be zero at year end", format_amount(suspense), day));
  }
  return notes;
}

} // namespace cratchit::ar

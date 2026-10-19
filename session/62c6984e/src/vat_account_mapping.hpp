#pragma once

#include "sie_document.hpp"
#include "vat_box.hpp"

#include <span>

namespace cratchit {

// Which VAT box a BAS account's postings are reported in.
struct VatAccountRule {
  AccountNumber m_account;
  VatBoxNumber m_box;
  int m_sign;  // +1: debit is reported as positive (input VAT, purchase bases); -1: credit is (output VAT, sales)
};

// Default mapping, based on the BAS chart of accounts plus the Fortnox base accounts 9021/9022.
// Accounts not listed are not part of the VAT return.
std::span<VatAccountRule const> default_vat_account_rules();

// The rule for an account, or nullptr if the account is not part of the VAT return.
VatAccountRule const* find_vat_account_rule(AccountNumber account);

// True for VAT accounts that must have a rule: the 26xx VAT accounts, except the settlement
// accounts 2650 (Redovisningskonto för moms) and 2651 (Bokslutsmoms).
bool requires_vat_account_rule(AccountNumber account);

} // namespace cratchit

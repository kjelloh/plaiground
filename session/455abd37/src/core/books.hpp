#pragma once

// The books of one financial year: what one SIE 4 file holds

#include "core/account.hpp"
#include "core/amount.hpp"
#include "core/date.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace cratchit {

struct Posting {
  AccountNo m_account{};
  Amount m_amount; // debit +, credit -
  std::optional<Date> m_transaction_date;
  std::string m_text;
};

struct Voucher {
  std::string m_series;
  std::string m_number;
  Date m_date;
  std::string m_text;
  std::optional<Date> m_registration_date;
  std::vector<Posting> m_postings;
};

// "A-39"
std::string voucher_id(Voucher const& voucher);

struct AccountInfo {
  std::string m_name;
  std::optional<int> m_sru_code;
  std::optional<char> m_type; // SIE #KTYP: T, S, I or K
};

struct FinancialYear {
  int m_year_index{}; // 0 current, -1 previous, …
  DateRange m_range;
};

struct Books {
  std::string m_company_name;
  std::string m_org_nr;
  std::vector<FinancialYear> m_financial_years;
  std::map<AccountNo, AccountInfo> m_accounts;
  std::map<AccountNo, Amount> m_opening_balances; // #IB 0
  std::map<AccountNo, Amount> m_closing_balances; // #UB 0
  std::map<AccountNo, Amount> m_results;          // #RES 0
  std::vector<Voucher> m_vouchers;

  // The range of year index 0, if the file has one.
  std::optional<DateRange> current_year() const;
};

} // namespace cratchit

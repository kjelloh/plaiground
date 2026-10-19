#pragma once

#include "amount.hpp"
#include "date.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace cratchit {

using AccountNumber = int;

// SIE #RAR: financial year. index 0 = current year, -1 = previous year, ...
struct FinancialYear {
  int m_index = 0;
  Date m_start;
  Date m_end;
};

// SIE #TRANS: one posting (kontering) in a voucher.
struct Posting {
  AccountNumber m_account = 0;
  Amount m_amount;
  std::optional<Date> m_date;  // only when it differs from the voucher date
  std::string m_text;
};

// SIE #VER: a voucher (verifikation) with its postings.
struct Voucher {
  std::string m_series;
  std::string m_number;
  Date m_date;
  std::string m_text;
  std::vector<Posting> m_postings;
};

// The content of a SIE file that cratchit uses.
struct SieDocument {
  std::string m_org_nr;                             // #ORGNR
  std::vector<FinancialYear> m_financial_years;     // #RAR
  std::map<AccountNumber, std::string> m_accounts;  // #KONTO number -> name
  std::vector<Voucher> m_vouchers;                  // #VER with #TRANS
};

} // namespace cratchit

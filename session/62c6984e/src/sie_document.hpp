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
  int index = 0;
  Date start;
  Date end;
};

// SIE #TRANS: one posting (kontering) in a voucher.
struct Posting {
  AccountNumber account = 0;
  Amount amount;
  std::optional<Date> date;  // only when it differs from the voucher date
  std::string text;
};

// SIE #VER: a voucher (verifikation) with its postings.
struct Voucher {
  std::string series;
  std::string number;
  Date date;
  std::string text;
  std::vector<Posting> postings;
};

// The content of a SIE file that cratchit uses.
struct SieDocument {
  std::string org_nr;                             // #ORGNR
  std::vector<FinancialYear> financial_years;     // #RAR
  std::map<AccountNumber, std::string> accounts;  // #KONTO number -> name
  std::vector<Voucher> vouchers;                  // #VER with #TRANS
};

} // namespace cratchit

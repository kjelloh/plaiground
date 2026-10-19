#include "sie/sie_to_journal.hpp"

namespace cratchit::sie {

namespace {

std::string field(Record const& record, std::string const& name) {
  auto const it = record.m_fields.find(name);
  return it == record.m_fields.end() ? std::string{} : it->second;
}

// Field values were type-checked by the reader, so parsing here only fails on a spec/code mismatch.
template <typename T>
std::expected<T, std::string> must(std::expected<T, std::string> value, Record const& record) {
  if (!value) return std::unexpected("line " + std::to_string(record.m_line) + ": " + value.error());
  return value;
}

std::optional<Date> optional_date(Record const& record, std::string const& name) {
  auto const text = field(record, name);
  if (text.empty()) return std::nullopt;
  auto const date = parse_date(text);
  return date ? std::optional{*date} : std::nullopt;
}

std::expected<void, std::string> add_balance(Record const& record, std::map<AccountNo, Amount>& target) {
  if (field(record, "year_index") != "0") return {};
  auto const account = must(parse_account_no(field(record, "account")), record);
  if (!account) return std::unexpected(account.error());
  auto const balance = must(Amount::parse(field(record, "balance")), record);
  if (!balance) return std::unexpected(balance.error());
  target[*account] = *balance;
  return {};
}

std::expected<Voucher, std::string> to_voucher(Record const& record) {
  auto const date = must(parse_date(field(record, "date")), record);
  if (!date) return std::unexpected(date.error());
  Voucher voucher{field(record, "series"), field(record, "number"), *date, field(record, "text"),
                  optional_date(record, "registration_date"), {}};
  for (auto const& sub : record.m_block) {
    if (sub.m_label != "TRANS") continue; // #RTRANS is repeated as #TRANS, #BTRANS is removed (see the spec)
    auto const account = must(parse_account_no(field(sub, "account")), sub);
    if (!account) return std::unexpected(account.error());
    auto const amount = must(Amount::parse(field(sub, "amount")), sub);
    if (!amount) return std::unexpected(amount.error());
    voucher.m_postings.push_back({*account, *amount, optional_date(sub, "transaction_date"), field(sub, "text")});
  }
  return voucher;
}

} // namespace

std::set<std::string> const& handled_labels() {
  static std::set<std::string> const labels{"FORMAT", "SIETYP", "FNAMN", "ORGNR", "RAR", "KONTO", "KTYP",
                                            "SRU",    "IB",     "UB",    "RES",   "VER", "TRANS"};
  return labels;
}

std::expected<Books, std::string> to_books(std::vector<Record> const& records) {
  Books books;
  for (auto const& record : records) {
    auto const& label = record.m_label;
    if (label == "FORMAT") {
      if (field(record, "format") != "PC8") return std::unexpected("only #FORMAT PC8 is supported");
    } else if (label == "SIETYP") {
      if (field(record, "type") != "4") return std::unexpected("only #SIETYP 4 is supported");
    } else if (label == "FNAMN") {
      books.m_company_name = field(record, "company_name");
    } else if (label == "ORGNR") {
      books.m_org_nr = field(record, "org_nr");
    } else if (label == "RAR") {
      auto const start = must(parse_date(field(record, "start")), record);
      auto const end = must(parse_date(field(record, "end")), record);
      if (!start || !end) return std::unexpected(start ? end.error() : start.error());
      books.m_financial_years.push_back({std::stoi(field(record, "year_index")), {*start, *end}});
    } else if (label == "KONTO" || label == "KTYP" || label == "SRU") {
      auto const account = must(parse_account_no(field(record, "account")), record);
      if (!account) return std::unexpected(account.error());
      auto& info = books.m_accounts[*account];
      if (label == "KONTO") info.m_name = field(record, "name");
      if (label == "KTYP") info.m_type = field(record, "type").front();
      if (label == "SRU") info.m_sru_code = std::stoi(field(record, "sru_code"));
    } else if (label == "IB" || label == "UB" || label == "RES") {
      auto& target = label == "IB" ? books.m_opening_balances : label == "UB" ? books.m_closing_balances : books.m_results;
      if (auto ok = add_balance(record, target); !ok) return std::unexpected(ok.error());
    } else if (label == "VER") {
      auto voucher = to_voucher(record);
      if (!voucher) return std::unexpected(voucher.error());
      books.m_vouchers.push_back(std::move(*voucher));
    }
  }
  return books;
}

} // namespace cratchit::sie

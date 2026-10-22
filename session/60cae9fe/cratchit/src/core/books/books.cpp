#include "core/books/books.hpp"

#include "core/csv.hpp"
#include "core/path_value.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <sstream>

namespace cratchit::books {

namespace fs = std::filesystem;

namespace {

constexpr std::string_view books_file = "books.txt";
constexpr std::string_view accounts_file = "accounts.csv";
constexpr std::string_view dimensions_file = "dimensions.csv";
constexpr std::string_view objects_file = "objects.csv";
constexpr std::string_view opening_file = "opening.csv";
constexpr std::string_view vouchers_file = "vouchers.csv";
constexpr std::string_view transactions_file = "transactions.csv";

CsvRow const accounts_header{"account", "name", "type", "sru", "unit"};
CsvRow const dimensions_header{"dimension", "name", "super"};
CsvRow const objects_header{"dimension", "object", "name"};
CsvRow const opening_header{"account", "amount"};
CsvRow const vouchers_header{"series", "number", "date", "text", "registered", "signature"};
CsvRow const transactions_header{"series", "number", "kind",     "account",  "objects",
                                 "amount", "date",   "text",     "quantity", "signature"};

std::string_view kind_name(Transaction::Kind kind) {
  switch (kind) {
  case Transaction::Kind::added:
    return "added";
  case Transaction::Kind::removed:
    return "removed";
  default:
    return "normal";
  }
}

std::optional<Transaction::Kind> kind_from_name(std::string_view name) {
  if (name == "normal") {
    return Transaction::Kind::normal;
  }
  if (name == "added") {
    return Transaction::Kind::added;
  }
  if (name == "removed") {
    return Transaction::Kind::removed;
  }
  return std::nullopt;
}

// Object pairs as one CSV field: the items as a CSV line of their own
std::string format_objects(Objects const& objects) {
  if (objects.empty()) {
    return {};
  }
  CsvRow items;
  for (auto const& [dimension, object] : objects) {
    items.push_back(dimension);
    items.push_back(object);
  }
  auto line = format_csv_row(items);
  line.pop_back();
  return line;
}

std::optional<Objects> parse_objects(std::string_view text) {
  if (text.empty()) {
    return Objects{};
  }
  auto const rows = parse_csv(text);
  if (!rows || rows->size() != 1 || rows->front().size() % 2 != 0) {
    return std::nullopt;
  }
  Objects objects;
  auto const& items = rows->front();
  for (std::size_t i = 0; i < items.size(); i += 2) {
    objects.emplace_back(items[i], items[i + 1]);
  }
  return objects;
}

std::string optional_date_text(std::optional<Date> const& date) {
  return date ? format_iso_date(*date) : std::string{};
}

std::string csv_text(CsvRow const& header, std::vector<CsvRow> const& rows) {
  std::string text = format_csv_row(header);
  for (auto const& row : rows) {
    text += format_csv_row(row);
  }
  return text;
}

std::string year_folder_name(Date start) {
  return format_iso_date(start);
}

// The rows of a CSV file after its header, each with the header's number of fields
std::expected<std::vector<CsvRow>, std::string> read_table(fs::path const& file, CsvRow const& header) {
  auto rows = read_csv(file);
  if (!rows) {
    return std::unexpected(rows.error());
  }
  if (rows->empty() || rows->front() != header) {
    return std::unexpected(std::format("{}: the first line must be {}", file.string(),
                                       format_csv_row(header).substr(0, format_csv_row(header).size() - 1)));
  }
  rows->erase(rows->begin());
  for (std::size_t i = 0; i < rows->size(); ++i) {
    if ((*rows)[i].size() != header.size()) {
      return std::unexpected(
          std::format("{}: row {} has {} fields, not {}", file.string(), i + 2, (*rows)[i].size(), header.size()));
    }
  }
  return rows;
}

std::expected<Date, std::string> date_field(fs::path const& file, std::string const& text) {
  auto const date = parse_iso_date(text);
  if (!date) {
    return std::unexpected(std::format("{}: '{}' is not a date YYYY-MM-DD", file.string(), text));
  }
  return *date;
}

std::expected<std::optional<Date>, std::string> optional_date_field(fs::path const& file, std::string const& text) {
  if (text.empty()) {
    return std::optional<Date>{};
  }
  auto const date = date_field(file, text);
  if (!date) {
    return std::unexpected(date.error());
  }
  return std::optional<Date>{*date};
}

std::expected<Amount, std::string> amount_field(fs::path const& file, std::string const& text) {
  auto const amount = parse_amount(text);
  if (!amount) {
    return std::unexpected(std::format("{}: '{}' is not an amount", file.string(), text));
  }
  return *amount;
}

std::expected<FiscalYear, std::string> read_year(fs::path const& folder, Date start, Date end) {
  FiscalYear year{start, end, {}, {}};
  auto const year_folder = folder / year_folder_name(start);
  auto const opening_path = year_folder / opening_file;
  auto const opening = read_table(opening_path, opening_header);
  if (!opening) {
    return std::unexpected(opening.error());
  }
  for (auto const& row : *opening) {
    auto const amount = amount_field(opening_path, row[1]);
    if (!amount) {
      return std::unexpected(amount.error());
    }
    year.reported_opening[row[0]] = *amount;
  }
  auto const vouchers_path = year_folder / vouchers_file;
  auto const vouchers = read_table(vouchers_path, vouchers_header);
  if (!vouchers) {
    return std::unexpected(vouchers.error());
  }
  for (auto const& row : *vouchers) {
    auto const date = date_field(vouchers_path, row[2]);
    if (!date) {
      return std::unexpected(date.error());
    }
    auto const registered = optional_date_field(vouchers_path, row[4]);
    if (!registered) {
      return std::unexpected(registered.error());
    }
    year.vouchers.push_back({row[0], row[1], *date, row[3], *registered, row[5], {}});
  }
  auto const transactions_path = year_folder / transactions_file;
  auto const transactions = read_table(transactions_path, transactions_header);
  if (!transactions) {
    return std::unexpected(transactions.error());
  }
  for (auto const& row : *transactions) {
    auto const voucher = std::ranges::find_if(
        year.vouchers, [&](Voucher const& v) { return v.series == row[0] && v.number == row[1]; });
    if (voucher == year.vouchers.end()) {
      return std::unexpected(
          std::format("{}: voucher {} {} is not in {}", transactions_path.string(), row[0], row[1], vouchers_file));
    }
    auto const kind = kind_from_name(row[2]);
    auto const objects = parse_objects(row[4]);
    auto const amount = amount_field(transactions_path, row[5]);
    auto const date = optional_date_field(transactions_path, row[6]);
    if (!kind || !objects) {
      return std::unexpected(std::format("{}: '{}' is not a kind or '{}' not objects", transactions_path.string(),
                                         row[2], row[4]));
    }
    if (!amount) {
      return std::unexpected(amount.error());
    }
    if (!date) {
      return std::unexpected(date.error());
    }
    voucher->transactions.push_back({*kind, row[3], *objects, *amount, *date, row[7], row[8], row[9]});
  }
  return year;
}

// Writes text to file when it differs from what the file holds; whether it was written and created
std::expected<std::optional<bool>, std::string> write_if_changed(fs::path const& file, std::string const& text) {
  std::error_code ec;
  bool const exists = fs::exists(file, ec);
  if (exists) {
    std::ifstream in(file, std::ios::binary);
    std::ostringstream current;
    current << in.rdbuf();
    if (current.str() == text) {
      return std::optional<bool>{};
    }
  }
  fs::create_directories(file.parent_path(), ec);
  std::ofstream out(file, std::ios::binary | std::ios::trunc);
  out << text;
  if (!out) {
    return std::unexpected(std::format("Cannot write {}", file.string()));
  }
  return std::optional<bool>{!exists};
}

bool number_less(std::string const& a, std::string const& b) {
  return a.size() != b.size() ? a.size() < b.size() : a < b;
}

} // namespace

bool FiscalYear::holds(Date date) const {
  return start <= date && date <= end;
}

FiscalYear const* Books::year_starting(Date start) const {
  auto const it = std::ranges::find(years, start, &FiscalYear::start);
  return it == years.end() ? nullptr : &*it;
}

Account const* Books::account(std::string_view number) const {
  auto const it = std::ranges::find(accounts, number, &Account::number);
  return it == accounts.end() ? nullptr : &*it;
}

AccountKind account_kind(Books const& books, std::string_view number) {
  auto const* account = books.account(number);
  return cratchit::account_kind(number, account ? std::string_view{account->type} : std::string_view{});
}

std::vector<YearBalances> balances(Books const& books) {
  std::vector<YearBalances> result;
  for (std::size_t i = 0; i < books.years.size(); ++i) {
    auto const& year = books.years[i];
    YearBalances balances;
    balances.carried = i > 0 && next_day(books.years[i - 1].end) == year.start;
    if (balances.carried) {
      for (auto const& [account, amount] : result.back().closing) {
        if (account_kind(books, account) == AccountKind::balance) {
          balances.opening[account] = amount;
        }
      }
    } else {
      balances.opening = year.reported_opening;
    }
    for (auto const& voucher : year.vouchers) {
      for (auto const& transaction : voucher.transactions) {
        if (transaction.kind != Transaction::Kind::removed) {
          balances.change[transaction.account] += transaction.amount;
        }
      }
    }
    balances.closing = balances.opening;
    for (auto const& [account, amount] : balances.change) {
      balances.closing[account] += amount;
    }
    result.push_back(std::move(balances));
  }
  return result;
}

fs::path books_folder(fs::path const& project_folder) {
  return project_folder / ".cratchit" / "books";
}

std::expected<Books, std::string> read_books(fs::path const& project_folder) {
  auto const folder = books_folder(project_folder);
  std::error_code ec;
  if (!fs::exists(folder / books_file, ec)) {
    return Books{};
  }
  auto const entries = read_path_values(folder / books_file);
  if (!entries) {
    return std::unexpected(entries.error());
  }
  Books books;
  std::vector<std::pair<Date, Date>> years;
  std::vector<Date> closed;
  for (auto const& [path, value] : *entries) {
    if (path == "chart_type") {
      books.chart_type = value;
    } else if (path == "company_id") {
      books.company_id = value;
    } else if (path == "address.contact") {
      books.address.contact = value;
    } else if (path == "address.street") {
      books.address.street = value;
    } else if (path == "address.postal") {
      books.address.postal = value;
    } else if (path == "address.phone") {
      books.address.phone = value;
    } else if (path.starts_with("year.")) {
      auto const start = parse_iso_date(std::string_view{path}.substr(5));
      auto const end = parse_iso_date(value);
      if (!start || !end) {
        return std::unexpected(std::format("{}: '{} = {}' is not 'year.<YYYY-MM-DD> = <YYYY-MM-DD>'",
                                           (folder / books_file).string(), path, value));
      }
      years.emplace_back(*start, *end);
    } else if (path.starts_with("closed.")) {
      auto const start = parse_iso_date(std::string_view{path}.substr(7));
      if (!start || value != "yes") {
        return std::unexpected(std::format("{}: '{} = {}' is not 'closed.<YYYY-MM-DD> = yes'",
                                           (folder / books_file).string(), path, value));
      }
      closed.push_back(*start);
    }
  }
  auto const accounts = read_table(folder / accounts_file, accounts_header);
  if (!accounts) {
    return std::unexpected(accounts.error());
  }
  for (auto const& row : *accounts) {
    books.accounts.push_back({row[0], row[1], row[2], row[3], row[4]});
  }
  auto const dimensions = read_table(folder / dimensions_file, dimensions_header);
  if (!dimensions) {
    return std::unexpected(dimensions.error());
  }
  for (auto const& row : *dimensions) {
    books.dimensions.push_back({row[0], row[1], row[2]});
  }
  auto const objects = read_table(folder / objects_file, objects_header);
  if (!objects) {
    return std::unexpected(objects.error());
  }
  for (auto const& row : *objects) {
    books.objects.push_back({row[0], row[1], row[2]});
  }
  std::ranges::sort(years);
  for (auto const& [start, end] : years) {
    auto year = read_year(folder, start, end);
    if (!year) {
      return std::unexpected(year.error());
    }
    year->closed = std::ranges::contains(closed, start);
    books.years.push_back(std::move(*year));
  }
  return books;
}

std::expected<std::vector<Written>, std::string> write_books(fs::path const& project_folder, Books const& books) {
  auto const folder = books_folder(project_folder);
  std::vector<std::pair<fs::path, std::string>> files;

  PathValues entries{{"chart_type", books.chart_type},
                     {"company_id", books.company_id},
                     {"address.contact", books.address.contact},
                     {"address.street", books.address.street},
                     {"address.postal", books.address.postal},
                     {"address.phone", books.address.phone}};
  auto years = books.years;
  std::ranges::sort(years, {}, &FiscalYear::start);
  for (auto const& year : years) {
    entries.push_back({"year." + format_iso_date(year.start), format_iso_date(year.end)});
  }
  for (auto const& year : years) {
    if (year.closed) {
      entries.push_back({"closed." + format_iso_date(year.start), "yes"});
    }
  }
  files.emplace_back(books_file, format_path_values(entries));

  auto accounts = books.accounts;
  std::ranges::sort(accounts, number_less, &Account::number);
  std::vector<CsvRow> rows;
  for (auto const& account : accounts) {
    rows.push_back({account.number, account.name, account.type, account.sru, account.unit});
  }
  files.emplace_back(accounts_file, csv_text(accounts_header, rows));
  rows.clear();
  for (auto const& dimension : books.dimensions) {
    rows.push_back({dimension.number, dimension.name, dimension.super});
  }
  files.emplace_back(dimensions_file, csv_text(dimensions_header, rows));
  rows.clear();
  for (auto const& object : books.objects) {
    rows.push_back({object.dimension, object.number, object.name});
  }
  files.emplace_back(objects_file, csv_text(objects_header, rows));

  for (auto const& year : years) {
    auto const year_folder = fs::path(year_folder_name(year.start));
    rows.clear();
    for (auto const& [account, amount] : year.reported_opening) {
      rows.push_back({account, format_amount(amount)});
    }
    files.emplace_back(year_folder / opening_file, csv_text(opening_header, rows));
    rows.clear();
    std::vector<CsvRow> transaction_rows;
    for (auto const& voucher : year.vouchers) {
      rows.push_back({voucher.series, voucher.number, format_iso_date(voucher.date), voucher.text,
                      optional_date_text(voucher.registered), voucher.signature});
      for (auto const& t : voucher.transactions) {
        transaction_rows.push_back({voucher.series, voucher.number, std::string(kind_name(t.kind)), t.account,
                                    format_objects(t.objects), format_amount(t.amount), optional_date_text(t.date),
                                    t.text, t.quantity, t.signature});
      }
    }
    files.emplace_back(year_folder / vouchers_file, csv_text(vouchers_header, rows));
    files.emplace_back(year_folder / transactions_file, csv_text(transactions_header, transaction_rows));
  }

  std::vector<Written> written;
  for (auto const& [file, text] : files) {
    auto const result = write_if_changed(folder / file, text);
    if (!result) {
      return std::unexpected(result.error());
    }
    if (*result) {
      written.push_back({fs::path(".cratchit") / "books" / file, **result});
    }
  }
  return written;
}

} // namespace cratchit::books

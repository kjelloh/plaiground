#include "core/books/import.hpp"

#include "core/path_value.hpp"
#include "core/sie/check.hpp"
#include "core/sie/file.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <fstream>
#include <iterator>
#include <set>
#include <tuple>

namespace cratchit::books {

namespace {

// Lists at most this many items in one note
constexpr std::size_t max_listed = 10;

Posting::Kind kind_of(sie::Transaction::Kind kind) {
  switch (kind) {
  case sie::Transaction::Kind::added:
    return Posting::Kind::added;
  case sie::Transaction::Kind::removed:
    return Posting::Kind::removed;
  default:
    return Posting::Kind::normal;
  }
}

// The voucher of a #VER; SIE gives no amount, so it is the debit total
Voucher voucher_of(sie::Voucher const& voucher) {
  Voucher result{voucher.series, voucher.number, {voucher.text, 0, voucher.date}, voucher.registered, voucher.signature,
                 {}};
  for (auto const& t : voucher.transactions) {
    result.postings.push_back({kind_of(t.kind), t.account, t.objects, t.amount, t.date, t.text, t.quantity, t.signature});
  }
  result.event.amount = debit(result.postings);
  return result;
}

std::string voucher_name(Voucher const& voucher) {
  return std::format("{} {}", voucher.series, voucher.number);
}

std::string_view kind_text(Posting::Kind kind) {
  switch (kind) {
  case Posting::Kind::added:
    return "added ";
  case Posting::Kind::removed:
    return "removed ";
  default:
    return "";
  }
}

std::string posting_text(Posting const& t) {
  return std::format("{}row {} {}{}", kind_text(t.kind), t.account, format_amount(t.amount),
                     t.text.empty() ? "" : std::format(" \"{}\"", t.text));
}

// The same row but for its kind and signature, e.g. a row and the same row struck
bool same_row(Posting const& a, Posting const& b) {
  return a.account == b.account && a.objects == b.objects && a.amount == b.amount && a.date == b.date &&
         a.text == b.text && a.quantity == b.quantity;
}

// Why the file's voucher is not the one in the books with history added, or nothing when it is: the same
// registration and signature (the date and text may be edited); each row of the books in its place, unchanged or
// struck (#BTRANS), a struck row still struck; any rows after them added (#RTRANS, possibly struck since)
std::optional<std::string> not_extended(Voucher const& books, Voucher const& file) {
  if (books.registered != file.registered) {
    return "the registration date differs";
  }
  if (books.signature != file.signature) {
    return "the signature differs";
  }
  if (file.postings.size() < books.postings.size()) {
    return std::format("{} rows in the books, {} in the file; a row may be struck, not left out",
                       books.postings.size(), file.postings.size());
  }
  for (std::size_t i = 0; i < books.postings.size(); ++i) {
    auto const& b = books.postings[i];
    auto const& f = file.postings[i];
    if (f == b || (b.kind != Posting::Kind::removed && f.kind == Posting::Kind::removed && same_row(b, f))) {
      continue;
    }
    if (b.kind == Posting::Kind::removed) {
      return std::format("row {} is struck in the books ({}), the file has {}", i + 1, posting_text(b), posting_text(f));
    }
    return std::format("row {} is {} in the books, {} in the file; a row may only be struck", i + 1, posting_text(b),
                       posting_text(f));
  }
  for (std::size_t i = books.postings.size(); i < file.postings.size(); ++i) {
    if (file.postings[i].kind == Posting::Kind::normal) {
      return std::format("row {} ({}) is new in the file but not added (#RTRANS)", i + 1, posting_text(file.postings[i]));
    }
  }
  return std::nullopt;
}

// What a change to a voucher did, e.g. "date 2026-04-20 to 2026-04-22, 2 rows struck, 2 added (signed Kjell-Olov
// Högdahl)"; rows only when rows changed
std::string change_text(Voucher const& books, Voucher const& file) {
  std::vector<std::string> parts;
  if (books.event.heading != file.event.heading) {
    parts.push_back(std::format("text \"{}\" to \"{}\"", books.event.heading, file.event.heading));
  }
  if (books.event.date != file.event.date) {
    parts.push_back(
        std::format("date {} to {}", format_iso_date(books.event.date), format_iso_date(file.event.date)));
  }
  std::size_t struck = 0;
  std::vector<std::string> signatures;
  auto const sign = [&](Posting const& p) {
    if (!p.signature.empty() && std::ranges::find(signatures, p.signature) == signatures.end()) {
      signatures.push_back(p.signature);
    }
  };
  for (std::size_t i = 0; i < file.postings.size(); ++i) {
    auto const& f = file.postings[i];
    auto const is_new = i >= books.postings.size();
    if (f.kind == Posting::Kind::removed && (is_new || books.postings[i].kind != Posting::Kind::removed)) {
      ++struck;
      sign(f);
    } else if (is_new) {
      sign(f);
    }
  }
  auto const added = file.postings.size() - books.postings.size();
  std::string signed_by;
  for (auto const& signature : signatures) {
    signed_by += (signed_by.empty() ? " (signed " : ", ") + signature;
  }
  if (struck > 0 || added > 0) {
    parts.push_back(std::format("{} row{} struck, {} added{}", struck, struck == 1 ? "" : "s", added,
                                signed_by.empty() ? "" : signed_by + ")"));
  }
  std::string result;
  for (auto const& part : parts) {
    result += (result.empty() ? "" : ", ") + part;
  }
  return result;
}

std::map<std::string, Amount> by_account(std::vector<sie::Balance> const& balances, int year) {
  std::map<std::string, Amount> result;
  for (auto const& balance : balances) {
    if (balance.year == year) {
      result[balance.account] += balance.amount;
    }
  }
  return result;
}

Amount value_or_zero(std::map<std::string, Amount> const& amounts, std::string const& account) {
  auto const it = amounts.find(account);
  return it == amounts.end() ? 0 : it->second;
}

std::set<std::string> accounts_in(std::map<std::string, Amount> const& a, std::map<std::string, Amount> const& b) {
  std::set<std::string> accounts;
  for (auto const* amounts : {&a, &b}) {
    for (auto const& [account, amount] : *amounts) {
      accounts.insert(account);
    }
  }
  return accounts;
}

std::string account_label(Books const& books, std::string const& number) {
  auto const* account = books.account(number);
  return !account || account->name.empty() ? number : std::format("{} {}", number, account->name);
}

std::string year_text(FiscalYear const& year) {
  return std::format("{} - {}", format_iso_date(year.start), format_iso_date(year.end));
}

// Vouchers of a year whose amounts on an account add up to the difference, either way
std::string possible_causes(FiscalYear const& year, std::string const& account, Amount difference) {
  std::vector<std::string> causes;
  for (auto const& voucher : year.vouchers) {
    Amount sum = 0;
    bool touches = false;
    for (auto const& t : voucher.postings) {
      if (t.account == account && t.kind != Posting::Kind::removed) {
        sum += t.amount;
        touches = true;
      }
    }
    if (touches && (sum == difference || sum == -difference)) {
      causes.push_back(std::format("{} {} \"{}\"", voucher_name(voucher), format_iso_date(voucher.event.date),
                                   voucher.event.heading));
    }
  }
  if (causes.empty()) {
    return {};
  }
  std::string text = ". Possibly from voucher";
  text += causes.size() > 1 ? "s " : " ";
  for (std::size_t i = 0; i < std::min(causes.size(), max_listed); ++i) {
    text += (i > 0 ? ", " : "") + causes[i];
  }
  if (causes.size() > max_listed) {
    text += std::format(" and {} more", causes.size() - max_listed);
  }
  return text;
}

std::size_t index_of(Books const& books, Date start) {
  return static_cast<std::size_t>(std::ranges::find(books.years, start, &FiscalYear::start) - books.years.begin());
}

void merge_master_data(ImportPlan& plan, sie::Document const& document, bool is_latest) {
  auto& books = plan.books;
  for (auto const& account : document.accounts) {
    Account const incoming{account.number, account.name, account.type, account.sru, account.unit};
    auto it = std::ranges::find(books.accounts, account.number, &Account::number);
    if (it == books.accounts.end()) {
      books.accounts.push_back(incoming);
      ++plan.accounts_added;
    } else if (is_latest && *it != incoming) {
      std::vector<std::string> changes;
      if (it->name != incoming.name) {
        changes.push_back(std::format("name \"{}\" becomes \"{}\"", it->name, incoming.name));
      }
      if (it->type != incoming.type) {
        changes.push_back(std::format("type '{}' becomes '{}'", it->type, incoming.type));
      }
      if (it->sru != incoming.sru) {
        changes.push_back(std::format("SRU code '{}' becomes '{}'", it->sru, incoming.sru));
      }
      if (it->unit != incoming.unit) {
        changes.push_back(std::format("unit '{}' becomes '{}'", it->unit, incoming.unit));
      }
      std::string text;
      for (auto const& change : changes) {
        text += (text.empty() ? "" : ", ") + change;
      }
      plan.notes.push_back(std::format("Account {}: {}", account.number, text));
      *it = incoming;
      ++plan.accounts_changed;
    }
  }
  for (auto const& dimension : document.dimensions) {
    Dimension const incoming{dimension.number, dimension.name, dimension.super};
    auto it = std::ranges::find(books.dimensions, dimension.number, &Dimension::number);
    if (it == books.dimensions.end()) {
      books.dimensions.push_back(incoming);
    } else if (is_latest && *it != incoming) {
      plan.notes.push_back(std::format("Dimension {}: name \"{}\" becomes \"{}\"", dimension.number, it->name,
                                       incoming.name));
      *it = incoming;
    }
  }
  for (auto const& object : document.objects) {
    Object const incoming{object.dimension, object.number, object.name};
    auto it = std::ranges::find_if(books.objects, [&](Object const& o) {
      return o.dimension == object.dimension && o.number == object.number;
    });
    if (it == books.objects.end()) {
      books.objects.push_back(incoming);
    } else if (is_latest && *it != incoming) {
      plan.notes.push_back(std::format("Object {} in dimension {}: name \"{}\" becomes \"{}\"", object.number,
                                       object.dimension, it->name, incoming.name));
      *it = incoming;
    }
  }
  auto const take = [&](std::string& field, std::string const& value, std::string_view what) {
    if (field == value || (!is_latest && !field.empty())) {
      return;
    }
    if (!field.empty()) {
      plan.notes.push_back(std::format("{} \"{}\" becomes \"{}\"", what, field, value));
    }
    field = value;
  };
  std::ranges::sort(books.accounts, [](Account const& a, Account const& b) {
    return a.number.size() != b.number.size() ? a.number.size() < b.number.size() : a.number < b.number;
  });
  take(books.chart_type, document.chart_type, "Chart of accounts type");
  take(books.company_id, document.company_id, "Company id");
  Address const address{document.address.contact, document.address.street, document.address.postal,
                        document.address.phone};
  if (books.address != address && (is_latest || books.address == Address{})) {
    if (books.address != Address{}) {
      plan.notes.push_back("The company's address changes");
    }
    books.address = address;
  }
}

// Adds the file's new vouchers to the year; the number of vouchers in the books missing from the file
std::size_t merge_vouchers(ImportPlan& plan, sie::Document const& document, FiscalYear& year, Forcing const& forcing) {
  std::set<std::pair<std::string, std::string>> in_file;
  std::vector<Voucher> added;
  for (auto const& record : document.vouchers) {
    if (record.series.empty() || record.number.empty()) {
      continue; // an error already
    }
    auto voucher = voucher_of(record);
    in_file.insert({voucher.series, voucher.number});
    auto const it = std::ranges::find_if(year.vouchers, [&](Voucher const& v) {
      return v.series == voucher.series && v.number == voucher.number;
    });
    if (it == year.vouchers.end()) {
      added.push_back(std::move(voucher));
    } else if (*it == voucher) {
      ++plan.vouchers_known;
    } else if (auto const reason = not_extended(*it, voucher)) {
      auto const forceable = !year.closed && forcing.imported.contains({year.start, voucher.series, voucher.number});
      if (forceable && forcing.force) {
        plan.notes.push_back(std::format("Voucher {} is forced: the file's replaces the one in the books ({})",
                                         voucher_name(voucher), *reason));
        plan.forced.push_back(std::format("{} in fiscal year {}: in the books {}; in the file {}", voucher_name(voucher),
                                          year_text(year), voucher_text(*it), voucher_text(voucher)));
        *it = std::move(voucher);
        ++plan.vouchers_forced;
        continue;
      }
      plan.errors.push_back(std::format("Voucher {} in the file differs from the one in the books ({}). A voucher in "
                                        "the books may only change by struck (#BTRANS) and added (#RTRANS) rows",
                                        voucher_name(voucher), *reason));
      if (forceable) {
        ++plan.forceable;
      }
    } else if (year.closed) {
      plan.errors.push_back(std::format("Voucher {} in the file changes the one in the books ({}), but fiscal year {} "
                                        "is closed. Open the year to import the change",
                                        voucher_name(voucher), change_text(*it, voucher), year_text(year)));
    } else {
      plan.notes.push_back(
          std::format("Voucher {} changes: {}", voucher_name(voucher), change_text(*it, voucher)));
      *it = std::move(voucher);
      ++plan.vouchers_changed;
    }
  }
  std::vector<std::string> missing;
  for (auto const& voucher : year.vouchers) {
    if (!in_file.contains({voucher.series, voucher.number})) {
      missing.push_back(voucher_name(voucher));
    }
  }
  if (!missing.empty()) {
    std::string list;
    for (std::size_t i = 0; i < std::min(missing.size(), max_listed); ++i) {
      list += (i > 0 ? ", " : "") + missing[i];
    }
    if (missing.size() > max_listed) {
      list += std::format(" and {} more", missing.size() - max_listed);
    }
    plan.notes.push_back(std::format("{} voucher{} of {} in the books {} not in the file: {}", missing.size(),
                                     missing.size() == 1 ? "" : "s", year_text(year),
                                     missing.size() == 1 ? "is" : "are", list));
  }
  plan.vouchers_added = added.size();
  for (auto& voucher : added) {
    year.vouchers.push_back(std::move(voucher));
  }
  return missing.size();
}

// Compares cratchit's closing balances and results of a year with what the file says
void check_year_end(ImportPlan& plan, sie::Document const& document, int index, std::size_t year_index,
                    YearBalances const& balances, std::map<std::string, Amount> const& explained) {
  auto const& books = plan.books;
  auto const& year = books.years[year_index];
  auto const file_closing = by_account(document.closing, index);
  auto const file_results = by_account(document.results, index);
  for (auto const& account : accounts_in(balances.closing, file_closing)) {
    if (account_kind(books, account) != AccountKind::balance) {
      continue;
    }
    auto const ours = value_or_zero(balances.closing, account);
    auto const theirs = value_or_zero(file_closing, account);
    if (ours - theirs != value_or_zero(explained, account)) {
      plan.notes.push_back(std::format("Closing balance {}, {}: the file says {}, cratchit's books give {} "
                                       "(difference {}){}",
                                       format_iso_date(year.end), account_label(books, account),
                                       format_amount(theirs), format_amount(ours), format_amount(ours - theirs),
                                       possible_causes(year, account, ours - theirs)));
    }
  }
  for (auto const& account : accounts_in(balances.change, file_results)) {
    if (account_kind(books, account) != AccountKind::result) {
      continue;
    }
    auto const ours = value_or_zero(balances.change, account);
    auto const theirs = value_or_zero(file_results, account);
    if (ours != theirs) {
      plan.notes.push_back(std::format("Result {}, {}: the file says {}, cratchit's books give {} (difference {}){}",
                                       year_text(year), account_label(books, account), format_amount(theirs),
                                       format_amount(ours), format_amount(ours - theirs),
                                       possible_causes(year, account, ours - theirs)));
    }
  }
}

void check_balances(ImportPlan& plan, Books const& before_books, sie::Document const& document,
                    std::size_t vouchers_missing) {
  auto const& books = plan.books;
  auto const before = balances(before_books);
  auto const after = balances(books);
  auto const current = index_of(books, plan.year_start);
  auto const file_opening = by_account(document.opening, 0);

  // The current year's opening balances, when carried from the year before
  std::map<std::string, Amount> explained;
  if (after[current].carried) {
    auto const& previous = books.years[current - 1];
    for (auto const& account : accounts_in(after[current].opening, file_opening)) {
      if (account_kind(books, account) != AccountKind::balance) {
        continue;
      }
      auto const ours = value_or_zero(after[current].opening, account);
      auto const theirs = value_or_zero(file_opening, account);
      if (ours != theirs) {
        explained[account] = ours - theirs;
        plan.notes.push_back(std::format("Opening balance {}, {}: the file says {}, cratchit carries {} from its "
                                         "closing balance {} (difference {}) and uses its own{}",
                                         format_iso_date(plan.year_start), account_label(books, account),
                                         format_amount(theirs), format_amount(ours), format_iso_date(previous.end),
                                         format_amount(ours - theirs),
                                         possible_causes(previous, account, ours - theirs)));
      }
    }
  }
  // The file's own year end is checked against its own balances (sie::check_document); the books
  // differ from it beyond the carried opening balances only when they hold vouchers the file lacks
  if (vouchers_missing > 0) {
    check_year_end(plan, document, 0, current, after[current], explained);
  }

  // Earlier years in the file that are in the books
  for (auto const& year : document.years) {
    if (year.index >= 0 || !books.year_starting(year.start)) {
      continue;
    }
    auto const index = index_of(books, year.start);
    if (books.years[index].end != year.end) {
      plan.notes.push_back(std::format("The file's fiscal year {} ({} - {}) ends otherwise than the one in the books "
                                       "({}); its balances are not compared",
                                       year.index, format_iso_date(year.start), format_iso_date(year.end),
                                       year_text(books.years[index])));
      continue;
    }
    check_year_end(plan, document, year.index, index, after[index], {});
  }

  // Opening balances of other years that the import changes
  for (std::size_t i = 0; i < before_books.years.size(); ++i) {
    auto const& year = before_books.years[i];
    if (year.start == plan.year_start) {
      continue;
    }
    auto const index = index_of(books, year.start);
    for (auto const& account : accounts_in(before[i].opening, after[index].opening)) {
      auto const was = value_or_zero(before[i].opening, account);
      auto const is = value_or_zero(after[index].opening, account);
      if (was != is) {
        plan.notes.push_back(std::format("Opening balance {}, {}: changes from {} to {}, carried from the closing "
                                         "balance {}",
                                         format_iso_date(year.start), account_label(books, account),
                                         format_amount(was), format_amount(is),
                                         format_iso_date(books.years[index - 1].end)));
      }
    }
  }
}

} // namespace

ImportPlan plan_import(Books const& books, Identity const& identity, sie::Document const& document,
                       Forcing const& forcing) {
  ImportPlan plan;
  plan.books = books;
  plan.identity = identity;
  auto findings = sie::check_document(document);
  plan.errors = std::move(findings.errors);
  plan.notes = std::move(findings.notes);
  auto const year = document.year(0);
  if (!year) {
    return plan;
  }
  plan.year_start = year->start;
  plan.year_end = year->end;

  if (identity.orgnr.empty()) {
    if (!document.orgnr.empty()) {
      plan.identity.orgnr = document.orgnr;
      plan.identity_changed = true;
      plan.notes.push_back(std::format("The project gets organisation number {} (#ORGNR)", document.orgnr));
      if (!document.company_name.empty() && document.company_name != identity.name) {
        plan.identity.name = document.company_name;
        plan.notes.push_back(
            std::format("The project's name changes from \"{}\" to \"{}\" (#FNAMN)", identity.name, document.company_name));
      }
    }
  } else {
    if (!document.orgnr.empty() && document.orgnr != identity.orgnr) {
      plan.errors.push_back(std::format("The file is for organisation number {}, but the project {} has {}",
                                        document.orgnr, identity.name, identity.orgnr));
    }
    if (!document.company_name.empty() && document.company_name != identity.name) {
      plan.errors.push_back(std::format("The file's company name \"{}\" differs from the project's name \"{}\"",
                                        document.company_name, identity.name));
    }
  }

  auto& years = plan.books.years;
  auto const same_start = std::ranges::find(years, year->start, &FiscalYear::start);
  if (same_start != years.end()) {
    if (same_start->end != year->end) {
      plan.errors.push_back(std::format("The file's fiscal year {} - {} ends otherwise than the one in the books ({})",
                                        format_iso_date(year->start), format_iso_date(year->end),
                                        year_text(*same_start)));
      return plan;
    }
  } else {
    for (auto const& other : years) {
      if (!(other.end < year->start || other.start > year->end)) {
        plan.errors.push_back(std::format("The file's fiscal year {} - {} overlaps the year {} in the books",
                                          format_iso_date(year->start), format_iso_date(year->end), year_text(other)));
        return plan;
      }
    }
    auto const at = std::ranges::find_if(years, [&](FiscalYear const& y) { return y.start > year->start; });
    years.insert(at, FiscalYear{year->start, year->end, {}, {}});
  }
  bool const is_latest = std::ranges::none_of(books.years, [&](FiscalYear const& y) { return y.start > year->start; });
  merge_master_data(plan, document, is_latest);

  auto& target = years[index_of(plan.books, year->start)];
  target.reported_opening = by_account(document.opening, 0);
  auto const vouchers_missing = merge_vouchers(plan, document, target, forcing);
  check_balances(plan, books, document, vouchers_missing);
  return plan;
}

std::expected<Saved, std::string> save_import(std::filesystem::path const& project_folder, ImportPlan const& plan,
                                              ImportSource const& source) {
  namespace fs = std::filesystem;
  Saved saved;
  auto written = write_books(project_folder, plan.books);
  if (!written) {
    return std::unexpected(written.error());
  }
  saved.written = std::move(*written);

  auto const imports = fs::path(".cratchit") / "imports";
  std::error_code ec;
  fs::create_directories(project_folder / imports, ec);
  if (ec) {
    return std::unexpected(std::format("Cannot create {}: {}", (project_folder / imports).string(), ec.message()));
  }
  for (std::size_t n = 1;; ++n) {
    auto const name = std::format("{}_{}_{}", source.today, n, source.file.filename().string());
    auto const stash = imports / fs::path(std::u8string(name.begin(), name.end()));
    auto const record = fs::path(stash).concat(".txt");
    if (fs::exists(project_folder / stash, ec) || fs::exists(project_folder / record, ec)) {
      continue;
    }
    if (!fs::copy_file(source.file, project_folder / stash, ec)) {
      return std::unexpected(std::format("Cannot copy {} to {}: {}", source.file.string(),
                                         (project_folder / stash).string(), ec.message()));
    }
    PathValues entries{
        {"imported", source.today},
        {"source", source.file.string()},
        {"encoding", source.encoding},
        {"program", source.program},
        {"generated", source.generated ? format_iso_date(*source.generated) : std::string{}},
        {"company", plan.identity.name},
        {"orgnr", plan.identity.orgnr},
        {"year", std::format("{} - {}", format_iso_date(plan.year_start), format_iso_date(plan.year_end))},
        {"vouchers_added", std::to_string(plan.vouchers_added)},
        {"vouchers_changed", std::to_string(plan.vouchers_changed)},
        {"vouchers_known", std::to_string(plan.vouchers_known)},
        {"notes", std::to_string(plan.notes.size())},
    };
    if (plan.vouchers_forced > 0) {
      std::string names;
      for (auto const& forced : plan.forced) {
        names += (names.empty() ? "" : ", ") + forced.substr(0, forced.find(" in fiscal year"));
      }
      entries.push_back({"forced", names});
    }
    if (auto const result = write_path_values(project_folder / record, entries); !result) {
      return std::unexpected(result.error());
    }
    saved.stashed = stash;
    saved.record = record;
    return saved;
  }
}

std::string voucher_text(Voucher const& voucher) {
  auto text = std::format("{} \"{}\"", format_iso_date(voucher.event.date), voucher.event.heading);
  if (voucher.registered) {
    text += " registered " + format_iso_date(*voucher.registered);
  }
  if (!voucher.signature.empty()) {
    text += " signed " + voucher.signature;
  }
  for (std::size_t i = 0; i < voucher.postings.size(); ++i) {
    text += (i == 0 ? ": " : ", ") + posting_text(voucher.postings[i]);
  }
  return text;
}

bool nothing_to_import(Books const& books, ImportPlan const& plan) {
  return plan.vouchers_added == 0 && plan.vouchers_changed == 0 && plan.vouchers_forced == 0 &&
         plan.accounts_added == 0 && plan.accounts_changed == 0 &&
         !plan.identity_changed && books.year_starting(plan.year_start);
}

SieFolder sie_files_in(std::filesystem::path const& folder, Books const& books, Identity const& identity) {
  namespace fs = std::filesystem;
  SieFolder result;
  std::error_code ec;
  for (auto const& entry : fs::directory_iterator(folder, ec)) {
    if (!entry.is_regular_file(ec) || entry.path().filename().string().starts_with('.')) {
      continue;
    }
    std::ifstream in(entry.path(), std::ios::binary);
    std::vector<std::uint8_t> const bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    auto const lexed = sie::lex_bytes(bytes);
    auto parsed = lexed ? sie::parse_document(lexed->lines) : sie::Parsed{};
    if (!in || !lexed || parsed.document.sie_type.empty()) {
      ++result.others;
      continue;
    }
    auto const plan = plan_import(books, identity, parsed.document);
    SieFileSummary summary{entry.path(), std::move(parsed.document), false, plan.errors.size() + parsed.errors.size(),
                           false, plan.vouchers_added, plan.vouchers_changed};
    summary.other_company = !identity.orgnr.empty() && summary.document.orgnr != identity.orgnr;
    summary.nothing_new = summary.errors == 0 && nothing_to_import(books, plan);
    result.files.push_back(std::move(summary));
  }
  auto const key = [](SieFileSummary const& summary) {
    auto const year = summary.document.year(0);
    return std::tuple{year ? std::optional{year->start} : std::nullopt, summary.document.generated};
  };
  std::ranges::sort(result.files, [&](SieFileSummary const& a, SieFileSummary const& b) {
    auto const ka = key(a);
    auto const kb = key(b);
    return ka != kb ? ka > kb : a.file < b.file;
  });
  return result;
}

std::expected<std::set<VoucherKey>, std::string> imported_vouchers(std::filesystem::path const& project_folder) {
  namespace fs = std::filesystem;
  std::set<VoucherKey> keys;
  auto const imports = project_folder / ".cratchit" / "imports";
  std::error_code ec;
  if (!fs::is_directory(imports, ec)) {
    return keys;
  }
  for (auto const& entry : fs::directory_iterator(imports, ec)) {
    if (!entry.is_regular_file(ec) || entry.path().extension() == ".txt") {
      continue;
    }
    std::ifstream in(entry.path(), std::ios::binary);
    if (!in) {
      return std::unexpected(std::format("Cannot read {}", entry.path().string()));
    }
    std::vector<std::uint8_t> const bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    auto const lexed = sie::lex_bytes(bytes);
    if (!lexed) {
      return std::unexpected(std::format("{}: {}", entry.path().string(), lexed.error()));
    }
    auto const document = sie::parse_document(lexed->lines).document;
    auto const year = document.year(0);
    if (!year) {
      return std::unexpected(std::format("{}: no fiscal year 0", entry.path().string()));
    }
    for (auto const& voucher : document.vouchers) {
      keys.insert({year->start, voucher.series, voucher.number});
    }
  }
  if (ec) {
    return std::unexpected(std::format("Cannot list {}: {}", imports.string(), ec.message()));
  }
  return keys;
}

} // namespace cratchit::books

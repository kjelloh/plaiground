#include "core/books/statements.hpp"

#include "core/path_value.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <tuple>

namespace cratchit::books {

namespace {

namespace fs = std::filesystem;

fs::path const statements_folder = fs::path(".cratchit") / "statements";

// What tells statement postings apart
using Key = std::tuple<Date, Amount, std::optional<Amount>, std::string>;

Key key_of(statement::Posting const& posting) {
  return {posting.date, posting.amount, posting.saldo, posting.text};
}

std::map<Key, std::size_t> counts_of(std::vector<statement::Posting> const& postings) {
  std::map<Key, std::size_t> counts;
  for (auto const& posting : postings) {
    ++counts[key_of(posting)];
  }
  return counts;
}

std::expected<std::optional<Date>, std::string> optional_date(PathValues const& record, std::string_view path,
                                                              fs::path const& file) {
  auto const text = find_value(record, path).value_or("");
  if (text.empty()) {
    return std::nullopt;
  }
  auto const date = parse_iso_date(text);
  if (!date) {
    return std::unexpected(std::format("{}: {} = {} is not a date", file.string(), path, text));
  }
  return *date;
}

} // namespace

std::expected<std::vector<StatementImport>, std::string> read_statement_imports(fs::path const& project_folder) {
  std::vector<StatementImport> imports;
  auto const folder = project_folder / statements_folder;
  std::error_code ec;
  if (!fs::is_directory(folder, ec)) {
    return imports;
  }
  std::vector<fs::path> stashed;
  for (auto const& entry : fs::directory_iterator(folder, ec)) {
    if (entry.is_regular_file(ec) && entry.path().extension() != ".txt") {
      stashed.push_back(entry.path());
    }
  }
  if (ec) {
    return std::unexpected(std::format("Cannot list {}: {}", folder.string(), ec.message()));
  }
  std::ranges::sort(stashed);
  for (auto const& file : stashed) {
    auto const record_file = fs::path(file).concat(".txt");
    auto const record = read_path_values(record_file);
    if (!record) {
      return std::unexpected(record.error());
    }
    StatementImport import;
    import.stashed = statements_folder / file.filename();
    import.account = find_value(*record, "account").value_or("");
    if (import.account.empty()) {
      return std::unexpected(std::format("{}: no account", record_file.string()));
    }
    auto from = optional_date(*record, "from", record_file);
    auto to = optional_date(*record, "to", record_file);
    if (!from || !to) {
      return std::unexpected(!from ? from.error() : to.error());
    }
    import.from = *from;
    import.to = *to;
    std::ifstream in(file, std::ios::binary);
    if (!in) {
      return std::unexpected(std::format("Cannot read {}", file.string()));
    }
    std::vector<std::uint8_t> const bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    auto document = statement::parse_statement(bytes);
    if (!document) {
      return std::unexpected(std::format("{}: {}", file.string(), document.error()));
    }
    import.document = std::move(*document);
    imports.push_back(std::move(import));
  }
  return imports;
}

std::vector<statement::Posting> counted_postings(StatementImport const& import) {
  std::vector<statement::Posting> postings;
  for (auto const& posting : import.document.postings) {
    if ((!import.from || posting.date >= *import.from) && (!import.to || posting.date <= *import.to)) {
      postings.push_back(posting);
    }
  }
  return postings;
}

std::vector<statement::Posting> known_postings(std::vector<StatementImport> const& imports, std::string_view account) {
  std::map<Key, std::size_t> most;
  std::map<Key, statement::Posting> first;
  for (auto const& import : imports) {
    if (import.account != account) {
      continue;
    }
    auto const postings = counted_postings(import);
    for (auto const& [key, count] : counts_of(postings)) {
      most[key] = std::max(most[key], count);
    }
    for (auto const& posting : postings) {
      first.try_emplace(key_of(posting), posting);
    }
  }
  std::vector<statement::Posting> known;
  for (auto const& [key, count] : most) {
    known.insert(known.end(), count, first.at(key));
  }
  return known;
}

std::size_t most_known(std::vector<StatementImport> const& imports, statement::Document const& document) {
  std::set<std::string> accounts;
  for (auto const& import : imports) {
    accounts.insert(import.account);
  }
  std::size_t most = 0;
  for (auto const& account : accounts) {
    auto known = counts_of(known_postings(imports, account));
    std::size_t count = 0;
    for (auto const& posting : document.postings) {
      auto const it = known.find(key_of(posting));
      if (it != known.end() && it->second > 0) {
        --it->second;
        ++count;
      }
    }
    most = std::max(most, count);
  }
  return most;
}

std::optional<AccountGuess> guess_statement_account(Books const& books, std::vector<StatementImport> const& imports,
                                                    statement::Document const& document) {
  auto const n = document.postings.size();
  // Book postings by account: their dates and amounts
  std::map<std::string, std::set<std::pair<Date, Amount>>> booked;
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      for (auto const& posting : voucher.postings) {
        if (posting.kind != Posting::Kind::removed) {
          booked[posting.account].insert({posting.date.value_or(voucher.event.date), posting.amount});
        }
      }
    }
  }
  auto const best_of = [&](std::map<std::string, std::size_t> const& scores) -> std::optional<std::string> {
    std::optional<std::string> best;
    std::size_t best_score = 0;
    bool tie = false;
    for (auto const& [account, score] : scores) {
      if (score > best_score) {
        best = account;
        best_score = score;
        tie = false;
      } else if (score == best_score && score > 0) {
        tie = true;
      }
    }
    return tie ? std::nullopt : best;
  };
  std::map<std::string, std::size_t> in_books;
  for (auto const& [account, dated] : booked) {
    for (auto const& posting : document.postings) {
      in_books[account] += dated.contains({posting.date, posting.amount}) ? 1 : 0;
    }
  }
  if (auto const account = best_of(in_books)) {
    return AccountGuess{*account, std::format("{} of {} postings match postings on {} in the books",
                                              in_books[*account], n, *account)};
  }
  std::set<std::string> accounts;
  for (auto const& import : imports) {
    accounts.insert(import.account);
  }
  std::map<std::string, std::size_t> in_statements;
  for (auto const& account : accounts) {
    auto known = counts_of(known_postings(imports, account));
    for (auto const& posting : document.postings) {
      auto const it = known.find(key_of(posting));
      if (it != known.end() && it->second > 0) {
        --it->second;
        ++in_statements[account];
      }
    }
  }
  if (auto const account = best_of(in_statements)) {
    return AccountGuess{*account, std::format("{} of {} postings are in statements imported for {}",
                                              in_statements[*account], n, *account)};
  }
  if (document.opening) {
    for (auto const& account : accounts) {
      StatementImport const* latest = nullptr;
      for (auto const& import : imports) {
        if (import.account == account && !import.document.postings.empty() &&
            (!latest || import.document.postings.back().date >= latest->document.postings.back().date)) {
          latest = &import;
        }
      }
      if (latest && latest->document.closing == document.opening) {
        return AccountGuess{account, std::format("the latest statement imported for {} ends with saldo {}, which "
                                                 "this one starts with",
                                                 account, format_amount(*document.opening))};
      }
    }
  }
  return std::nullopt;
}

StatementPlan plan_statement(Books const& books, std::vector<StatementImport> const& imports,
                             statement::Document const& document, std::string account, FiscalYear const* year) {
  StatementPlan plan;
  plan.account = std::move(account);
  plan.postings = document.postings.size();
  if (year) {
    plan.from = year->start;
    plan.to = year->end;
  } else if (!books.years.empty()) {
    plan.from = books.years.front().start;
    plan.to = books.years.back().end;
  }
  auto known = counts_of(known_postings(imports, plan.account));
  for (auto const& posting : document.postings) {
    bool const inside = year ? year->holds(posting.date)
                             : std::ranges::any_of(books.years, [&](FiscalYear const& y) { return y.holds(posting.date); });
    if (!inside) {
      ++plan.outside;
      continue;
    }
    auto const it = known.find(key_of(posting));
    if (it != known.end() && it->second > 0) {
      --it->second;
      ++plan.known;
    } else {
      plan.added.push_back(posting);
    }
  }
  return plan;
}

std::expected<SavedStatement, std::string> save_statement(fs::path const& project_folder, fs::path const& file,
                                                          StatementPlan const& plan, std::string_view today) {
  std::error_code ec;
  fs::create_directories(project_folder / statements_folder, ec);
  if (ec) {
    return std::unexpected(
        std::format("Cannot create {}: {}", (project_folder / statements_folder).string(), ec.message()));
  }
  for (std::size_t n = 1;; ++n) {
    auto const name = std::format("{}_{}_{}", today, n, file.filename().string());
    auto const stash = statements_folder / fs::path(std::u8string(name.begin(), name.end()));
    auto const record = fs::path(stash).concat(".txt");
    if (fs::exists(project_folder / stash, ec) || fs::exists(project_folder / record, ec)) {
      continue;
    }
    if (!fs::copy_file(file, project_folder / stash, ec)) {
      return std::unexpected(
          std::format("Cannot copy {} to {}: {}", file.string(), (project_folder / stash).string(), ec.message()));
    }
    PathValues const entries{
        {"imported", std::string(today)},
        {"source", file.string()},
        {"account", plan.account},
        {"from", plan.from ? format_iso_date(*plan.from) : std::string{}},
        {"to", plan.to ? format_iso_date(*plan.to) : std::string{}},
        {"postings", std::to_string(plan.postings)},
        {"added", std::to_string(plan.added.size())},
        {"known", std::to_string(plan.known)},
        {"outside", std::to_string(plan.outside)},
    };
    if (auto const written = write_path_values(project_folder / record, entries); !written) {
      return std::unexpected(written.error());
    }
    return SavedStatement{stash, record};
  }
}

} // namespace cratchit::books

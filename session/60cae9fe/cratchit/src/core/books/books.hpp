#pragma once

#include "core/amount.hpp"
#include "core/bas.hpp"
#include "core/date.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// cratchit's own books of a project: the chart of accounts, the fiscal years
// and their vouchers. Independent of where the data came from (e.g. SIE).
namespace cratchit::books {

struct Account {
  std::string number;
  std::string name;
  std::string type; // T, S, K or I when given; else the BAS class decides (see account_kind)
  std::string sru;
  std::string unit;

  bool operator==(Account const&) const = default;
};

struct Dimension {
  std::string number;
  std::string name;
  std::string super; // the super dimension of a sub dimension, else empty

  bool operator==(Dimension const&) const = default;
};

struct Object {
  std::string dimension;
  std::string number;
  std::string name;

  bool operator==(Object const&) const = default;
};

// Dimension and object pairs
using Objects = std::vector<std::pair<std::string, std::string>>;

struct Transaction {
  enum class Kind {
    normal,
    added,   // added after the voucher was registered
    removed, // removed (struck) after the voucher was registered; kept for the history, does not count
  };

  Kind kind = Kind::normal;
  std::string account;
  Objects objects;
  Amount amount = 0;
  std::optional<Date> date; // when not given, the voucher's date
  std::string text;
  std::string quantity;
  std::string signature;

  bool operator==(Transaction const&) const = default;
};

// A financial event. Once in the books it is never changed.
struct Voucher {
  std::string series;
  std::string number;
  Date date;
  std::string text;
  std::optional<Date> registered;
  std::string signature;
  std::vector<Transaction> transactions;

  bool operator==(Voucher const&) const = default;
};

struct FiscalYear {
  Date start;
  Date end;
  // The opening balances the source reported. Used only when the year before
  // is not in the books; else the year before's closing balances are carried.
  std::map<std::string, Amount> reported_opening;
  std::vector<Voucher> vouchers;
};

struct Address {
  std::string contact;
  std::string street;
  std::string postal;
  std::string phone;

  bool operator==(Address const&) const = default;
};

struct Books {
  std::string chart_type; // e.g. EUBAS97
  std::string company_id; // the source program's id of the company
  Address address;
  std::vector<Account> accounts;     // by number
  std::vector<Dimension> dimensions; // by number
  std::vector<Object> objects;
  std::vector<FiscalYear> years; // by start date

  FiscalYear const* year_starting(Date start) const;
  Account const* account(std::string_view number) const;
};

// The kind of an account in the books (by its type, else its BAS class)
AccountKind account_kind(Books const& books, std::string_view number);

// The balances of one fiscal year, per account
struct YearBalances {
  std::map<std::string, Amount> opening; // balance accounts only
  std::map<std::string, Amount> change;  // the year's transactions (not removed ones)
  std::map<std::string, Amount> closing; // opening + change
  bool carried = false; // opening carried from the year before's closing, not as reported
};

// The balances of every year, in the order of books.years. A year directly
// after another in the books opens with that year's closing balances of the
// balance accounts; any other year opens with its reported opening balances.
// Result accounts open at zero.
std::vector<YearBalances> balances(Books const& books);

// A file written by write_books
struct Written {
  std::filesystem::path file; // relative to the project folder, e.g. .cratchit/books/accounts.csv
  bool created = false;       // else updated
};

// The folder holding the books: <project folder>/.cratchit/books
std::filesystem::path books_folder(std::filesystem::path const& project_folder);

// Reads the books of a project. No books folder gives empty books.
// Error text names the file that is missing or malformed.
std::expected<Books, std::string> read_books(std::filesystem::path const& project_folder);

// Writes the books of a project, creating folders as needed. Only files whose
// content changes are written; those are returned, in the order written.
std::expected<std::vector<Written>, std::string> write_books(std::filesystem::path const& project_folder,
                                                             Books const& books);

} // namespace cratchit::books

#pragma once

#include "core/books/books.hpp"
#include "core/statement/statement.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Account statement files imported into a project. Each is stashed unchanged in
// .cratchit/statements with a record of the BAS account it is for. The statement
// entries are kept nowhere else: they are read from the stashed files when needed.
namespace cratchit::books {

// A stashed statement file, its record and its entries
struct StatementImport {
  std::filesystem::path stashed; // relative to the project folder
  std::string account;           // the BAS account the statement is for
  std::optional<Date> from;      // only the entries from this day to `to` count: the days of the books'
  std::optional<Date> to;        // fiscal years when imported (of one year, when imported at its prompt)
  statement::Document document;
};

// The statement files imported into the project, by stashed file name. No
// .cratchit/statements gives none. Error text names a file that cannot be read.
std::expected<std::vector<StatementImport>, std::string> read_statement_imports(
    std::filesystem::path const& project_folder);

// The entries of an import that count: those within its days, if it has them
std::vector<statement::Entry> counted_entries(StatementImport const& import);

// The statement entries known for an account, from all imports for it.
// An entry in several files is known once: identical entries (date, amount,
// saldo, text) as many times as the file with the most of them has them.
std::vector<statement::Entry> known_entries(std::vector<StatementImport> const& imports, std::string_view account);

// How many entries of the document the imports already know, for the account that knows the most of them
std::size_t most_known(std::vector<StatementImport> const& imports, statement::Document const& document);

// A statement entry not yet booked, with its account and where it is
struct OpenEntry {
  std::string account;   // the BAS account of its statement
  statement::Entry entry;
  std::string source;    // the stashed file and line, e.g. .cratchit/statements/2026-10-24_1_pg.csv:5
};

// The entries of the imported statements that are not yet booked, by account
// and then oldest first. An entry is booked when a posting on its account has
// its date and amount, in a voucher (not removed) or a waiting event; each
// posting books one entry. An entry in several files counts once (see
// known_entries). Only entries in the days of year, or else of any fiscal year
// in the books.
std::vector<OpenEntry> open_entries(Books const& books, std::vector<StatementImport> const& imports,
                                    FiscalYear const* year);

// A BAS account suggested for a statement, and why
struct AccountGuess {
  std::string account;
  std::string reason; // e.g. "5 of 7 entries match postings on 1920 in the books"
};

// The account that matches the most entries of the document: postings
// in the books' vouchers (not removed) with the same date and amount, else
// known statement entries of the account (same date, amount, saldo and text).
// On a tie, none. Without a match, the account whose latest imported statement
// ends with the saldo this one starts with. Else none.
std::optional<AccountGuess> guess_statement_account(Books const& books, std::vector<StatementImport> const& imports,
                                                    statement::Document const& document);

// What importing a statement for an account would add
struct StatementPlan {
  std::string account;
  std::optional<Date> from; // the fiscal year's days when imported at its prompt, else the first year's start
  std::optional<Date> to;   // and the last year's end (none without years)
  std::size_t entries = 0; // in the file
  std::size_t outside = 0;  // outside those days, or else outside every fiscal year in the books
  std::size_t known = 0;    // already imported
  std::vector<statement::Entry> added; // the new ones, oldest first
};

// The plan for a document's entries to account; year limits it to that fiscal year's days
StatementPlan plan_statement(Books const& books, std::vector<StatementImport> const& imports,
                             statement::Document const& document, std::string account, FiscalYear const* year);

// What save_statement wrote
struct SavedStatement {
  std::filesystem::path stashed; // relative to the project folder
  std::filesystem::path record;
};

// Stashes an unchanged copy of the file as .cratchit/statements/<today>_<n>_<file
// name> (n from 1, the first free) with a record <copy>.txt of `path = value`
// lines: imported, source, account, from, to (empty for no limit), entries,
// added, known, outside.
std::expected<SavedStatement, std::string> save_statement(std::filesystem::path const& project_folder,
                                                          std::filesystem::path const& file, StatementPlan const& plan,
                                                          std::string_view today);

} // namespace cratchit::books

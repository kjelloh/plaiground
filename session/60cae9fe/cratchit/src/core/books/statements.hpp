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
// postings are kept nowhere else: they are read from the stashed files when needed.
namespace cratchit::books {

// A stashed statement file, its record and its postings
struct StatementImport {
  std::filesystem::path stashed; // relative to the project folder
  std::string account;           // the BAS account the statement is for
  std::optional<Date> from;      // only the postings from this day to `to` count: the days of the books'
  std::optional<Date> to;        // fiscal years when imported (of one year, when imported at its prompt)
  statement::Document document;
};

// The statement files imported into the project, oldest import first. No
// .cratchit/statements gives none. Error text names a file that cannot be read.
std::expected<std::vector<StatementImport>, std::string> read_statement_imports(
    std::filesystem::path const& project_folder);

// The postings of an import that count: those within its days, if it has them
std::vector<statement::Posting> counted_postings(StatementImport const& import);

// The statement postings known for an account, from all imports for it. A
// posting in several files is known once: identical postings (date, amount,
// saldo, text) as many times as the file with the most of them has them.
std::vector<statement::Posting> known_postings(std::vector<StatementImport> const& imports, std::string_view account);

// How many postings of the document the imports already know, for the account that knows the most of them
std::size_t most_known(std::vector<StatementImport> const& imports, statement::Document const& document);

// A BAS account suggested for a statement, and why
struct AccountGuess {
  std::string account;
  std::string reason; // e.g. "5 of 7 postings match postings on 1920 in the books"
};

// The account whose postings match the most postings of the document: postings
// in the books' vouchers (not removed) with the same date and amount, else
// known statement postings of the account (same date, amount, saldo and text).
// On a tie, none. Without a match, the account whose latest imported statement
// ends with the saldo this one starts with. Else none.
std::optional<AccountGuess> guess_statement_account(Books const& books, std::vector<StatementImport> const& imports,
                                                    statement::Document const& document);

// What importing a statement for an account would add
struct StatementPlan {
  std::string account;
  std::optional<Date> from; // the fiscal year's days when imported at its prompt, else the first year's start
  std::optional<Date> to;   // and the last year's end (none without years)
  std::size_t postings = 0; // in the file
  std::size_t outside = 0;  // outside those days, or else outside every fiscal year in the books
  std::size_t known = 0;    // already imported
  std::vector<statement::Posting> added; // the new ones, oldest first
};

// The plan for a document's postings to account; year limits it to that fiscal year's days
StatementPlan plan_statement(Books const& books, std::vector<StatementImport> const& imports,
                             statement::Document const& document, std::string account, FiscalYear const* year);

// What save_statement wrote
struct SavedStatement {
  std::filesystem::path stashed; // relative to the project folder
  std::filesystem::path record;
};

// Stashes an unchanged copy of the file as .cratchit/statements/<today>_<n>_<file
// name> (n from 1, the first free) with a record <copy>.txt of `path = value`
// lines: imported, source, account, from, to (empty for no limit), postings,
// added, known, outside.
std::expected<SavedStatement, std::string> save_statement(std::filesystem::path const& project_folder,
                                                          std::filesystem::path const& file, StatementPlan const& plan,
                                                          std::string_view today);

} // namespace cratchit::books

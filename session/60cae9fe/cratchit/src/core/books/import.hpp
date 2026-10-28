#pragma once

#include "core/books/books.hpp"
#include "core/sie/document.hpp"

#include <cstddef>
#include <expected>
#include <compare>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace cratchit::books {

// Who the books are for: the official name and organisation number of the project
struct Identity {
  std::string name;
  std::string orgnr; // empty until the first import sets it
};

// What importing an SIE file into the books would do
struct ImportPlan {
  std::vector<std::string> errors; // when any, nothing may be imported
  std::vector<std::string> notes;  // worth knowing, but do not stop the import
  Books books;                     // the books after the import
  Identity identity;               // the project's identity after the import
  bool identity_changed = false;
  Date year_start;
  Date year_end;
  std::size_t vouchers_added = 0;
  std::size_t vouchers_changed = 0; // in the books, with rows struck or added in the file
  std::size_t vouchers_known = 0;   // already in the books, identical
  std::size_t accounts_added = 0;
  std::size_t accounts_changed = 0;
};

// Plans importing a parsed SIE 4E file (see sie::check_document for the file's own
// errors and notes, which are included). The file's fiscal year 0 is imported:
//
// - Identity: the first import (no orgnr yet) sets the organisation number and
//   name from #ORGNR and #FNAMN. Later, a different orgnr or name is an error.
// - Fiscal year: a new year must not overlap a year in the books; a year with
//   the same start must have the same end.
// - Vouchers: new ones are added. One already in the books (same series and
//   number) is skipped when identical. It is taken from the file (and noted)
//   when only history is added: the same date, text, registration and
//   signature, each row of the books in its place unchanged or struck
//   (#BTRANS), a struck row still struck, and any further rows added (#RTRANS).
//   Any other difference is an error, and so is a change in a closed year.
//   Vouchers in the books but not in the file are noted.
// - Accounts, dimensions, objects and company details: new ones are added;
//   changes are taken (and noted) only from a file whose year is the latest in
//   the books, so an older file does not undo newer names.
// - Balances are cratchit's own: a year directly after another opens with its
//   closing balances. The file's opening and closing balances (#IB, #UB, #RES,
//   also of earlier years in the books) are only checked; each difference is
//   noted with the vouchers that may explain it (those with that amount on the
//   account), and so is any opening balance the import changes in other years.
ImportPlan plan_import(Books const& books, Identity const& identity, sie::Document const& document);

// True when the plan (without errors) adds nothing to the books: no new voucher, account or identity, and its year
// is already there
bool nothing_to_import(Books const& books, ImportPlan const& plan);

// An SIE file in a folder, as listed to pick one to import
struct SieFileSummary {
  std::filesystem::path file;
  sie::Document document;      // as parsed (only what the list shows is used)
  bool other_company = false;  // the project has an orgnr and the file another one
  std::size_t errors = 0;      // the plan's errors (the file's own included)
  bool nothing_new = false;    // see nothing_to_import
  std::size_t vouchers_added = 0;
  std::size_t vouchers_changed = 0;
};

// The SIE files in a folder: the files (not hidden) that read and parse as SIE (have #SIETYP), each planned against
// the books, newest fiscal year first, then the newest #GEN, then by name. `others` counts the files left out.
// Does not log.
struct SieFolder {
  std::vector<SieFileSummary> files;
  std::size_t others = 0;
};
SieFolder sie_files_in(std::filesystem::path const& folder, Books const& books, Identity const& identity);

// Where an imported file came from, for the record kept with its stashed copy
struct ImportSource {
  std::filesystem::path file; // as given, in full
  std::string encoding;       // as read, e.g. CP437
  std::string program;        // #PROGRAM name and version
  std::optional<Date> generated;
  std::string today; // YYYY-MM-DD
};

// What save_import wrote
struct Saved {
  std::vector<Written> written;  // books files, see write_books
  std::filesystem::path stashed; // the copy of the file, relative to the project folder
  std::filesystem::path record;  // the import's record next to it
};

// Writes the planned books, then stashes an unchanged copy of the imported file
// as .cratchit/imports/<today>_<n>_<file name> (n from 1, the first free) with a
// record <copy>.txt of `path = value` lines: imported, source, encoding,
// program, generated, company, orgnr, year, vouchers_added, vouchers_changed,
// vouchers_known, notes. Must not be called for a plan with errors.
std::expected<Saved, std::string> save_import(std::filesystem::path const& project_folder, ImportPlan const& plan,
                                              ImportSource const& source);

// A voucher by the start of its fiscal year, its series and its number
struct VoucherKey {
  Date year;
  std::string series;
  std::string number;

  auto operator<=>(VoucherKey const&) const = default;
};

// The vouchers that came with an SIE import: those of the stashed copies in
// .cratchit/imports (every file there but the .txt records). No such folder
// gives none. Error text names a stashed file that cannot be read. Does not log.
std::expected<std::set<VoucherKey>, std::string> imported_vouchers(std::filesystem::path const& project_folder);

} // namespace cratchit::books

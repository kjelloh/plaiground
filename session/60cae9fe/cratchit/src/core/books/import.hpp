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

// A voucher by the start of its fiscal year, its series and its number
struct VoucherKey {
  Date year;
  std::string series;
  std::string number;

  auto operator<=>(VoucherKey const&) const = default;
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
  std::size_t vouchers_forced = 0;  // in the books, replaced by the file's against the history rule (see Forcing)
  std::vector<std::string> forced;  // each forced voucher with its old and new version, for the log and the record
  std::size_t forceable = 0;        // the errors a forced import would take (each also in errors)
  std::size_t vouchers_known = 0;   // already in the books, identical
  std::size_t accounts_added = 0;
  std::size_t accounts_changed = 0;
  // Making the file the truth of its year: what leaves the books or changes against the history rule
  std::vector<std::string> renamed;  // the same voucher under another name, e.g. "D 5 as D 7"
  std::vector<std::string> replaced; // in the books, replaced by the file's voucher with the same series and number
  std::vector<std::string> dropped;  // in the books, a correction and the voucher it corrects, both left out
  std::vector<std::string> demoted;  // in the books, now waiting events
  std::vector<std::string> reverted; // each of them in full, for the log
  std::size_t registered_after_file = 0; // replaced, dropped and demoted ones registered after the file's #GEN

  // True when there are errors and a forced import would take every one of them
  bool can_force() const { return !errors.empty() && forceable == errors.size(); }
  // True when the import makes the file the truth: a voucher in the books is renamed, replaced, dropped or demoted
  bool reverts() const { return !renamed.empty() || !replaced.empty() || !dropped.empty() || !demoted.empty(); }
};

// The vouchers that came with SIE imports (see imported_vouchers), and whether to force: a voucher of an open year
// that differs from the one in the books against the history rule is then taken from the file, when the one in the
// books was imported (one entered in cratchit is another event with the same number, never forced)
struct Forcing {
  std::set<VoucherKey> imported;
  bool force = false;
};

// Plans importing a parsed SIE 4E file (see sie::check_document for the file's own
// errors and notes, which are included). The file's fiscal year 0 is imported:
//
// - Identity: the first import (no orgnr yet) sets the organisation number and
//   name from #ORGNR and #FNAMN. Later, a different orgnr or name is an error.
// - Fiscal year: a new year must not overlap a year in the books; a year with
//   the same start must have the same end.
// - Vouchers: the file is the truth of its year 0. One already in the books (same
//   series and number) is skipped when identical. It is taken from the file (and
//   noted) when only history is added: the same registration and signature (the
//   date and text may be edited), each row of the books in its place unchanged or
//   struck (#BTRANS), a struck row still struck, and any further rows added
//   (#RTRANS). With forcing, a voucher that differs otherwise and was imported, in
//   an open year, is taken from the file (see Forcing). The rest of the year's
//   vouchers make the file the truth (see ImportPlan::reverts), in this order:
//   a voucher in the books with the same date and counted postings (account and
//   amount) as a voucher of the file is the same voucher under another name, the
//   file's replaces it (renamed; the same text first, then the lowest number);
//   one with the same series and number as a voucher of the file is replaced by
//   it; of those left, a voucher with the same date as another and its counted
//   postings turned (a correction and what it corrects; not a credit note on
//   another day) is dropped with it; the others become
//   waiting events with their counted postings (demoted), noted on their
//   registration day. The file's other vouchers are added. Any change in a
//   closed year is an error.
// - Accounts, dimensions, objects and company details: new ones are added;
//   changes are taken (and noted) only from a file whose year is the latest in
//   the books, so an older file does not undo newer names.
// - Balances are cratchit's own: a year directly after another opens with its
//   closing balances. The file's opening and closing balances (#IB, #UB, #RES,
//   also of earlier years in the books) are only checked; each difference is
//   noted with the vouchers that may explain it (those with that amount on the
//   account), and so is any opening balance the import changes in other years.
ImportPlan plan_import(Books const& books, Identity const& identity, sie::Document const& document,
                       Forcing const& forcing = {});

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
  std::size_t vouchers_renamed = 0;
  std::size_t vouchers_leaving = 0; // replaced, dropped and demoted (see ImportPlan::reverts)
};

// The SIE files in a folder: the files (not hidden) that read and parse as SIE (have #SIETYP), each planned against
// the books, newest fiscal year first, then the newest #GEN, then by name. `others` counts the files left out.
// Does not log.
struct SieFolder {
  std::vector<SieFileSummary> files;
  std::size_t others = 0;
};
SieFolder sie_files_in(std::filesystem::path const& folder, Books const& books, Identity const& identity);

// A voucher in full for the log, e.g. `2026-04-20 "Loopia" registered 2026-07-15: row 1920 -323.75, row 2440 323.75`
std::string voucher_text(Voucher const& voucher);

// Where an imported file came from, for the record kept with its stashed copy
struct ImportSource {
  std::filesystem::path file; // as given, in full
  std::string encoding;       // as read, e.g. CP437
  std::string program;        // #PROGRAM name and version
  std::optional<Date> generated;
  std::string today; // YYYY-MM-DD
  std::string exported_before{}; // the export of the year written before the import made the file the truth, if any
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
// vouchers_known, notes, and when any (the vouchers, comma separated) `forced`, `renamed`, `replaced`, `dropped`,
// `demoted`, and `exported_before` (the file name). Must not be called for a plan with errors.
std::expected<Saved, std::string> save_import(std::filesystem::path const& project_folder, ImportPlan const& plan,
                                              ImportSource const& source);

// The vouchers that came with an SIE import: those of the stashed copies in
// .cratchit/imports (every file there but the .txt records). No such folder
// gives none. Error text names a stashed file that cannot be read. Does not log.
std::expected<std::set<VoucherKey>, std::string> imported_vouchers(std::filesystem::path const& project_folder);

} // namespace cratchit::books

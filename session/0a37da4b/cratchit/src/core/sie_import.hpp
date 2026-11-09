#pragma once

#include "core/books/import.hpp"
#include "core/project.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// The folder in a project folder that imported SIE files are moved to
inline constexpr std::string_view imported_sie_folder_name = "importerade_sie_filer";

// A voucher already in the books that the import changes: under the SIE rules
// for a registered voucher, only rows struck (#BTRANS) and rows added (#RTRANS)
struct ChangedVoucher {
  std::string series;
  std::string number;
  std::string date;    // YYYY-MM-DD
  std::string heading; // the voucher's text
  std::size_t struck = 0;
  std::size_t added = 0;
};

// "A 12  2025-06-03  Loopia: 1 rad struken, 1 rad tillagd"
std::string change_text(ChangedVoucher const& changed);

// What importing an SIE file into a project will do. Read from the books and the file; nothing written yet.
struct SieImportPlan {
  std::filesystem::path sie_file;
  std::string program;   // #PROGRAM name and version, e.g. "Fortnox 3.62"
  std::string generated; // #GEN as YYYY-MM-DD, or empty
  std::string alert;     // when the file was not read as CP437 (see sie::lex_file), else empty
  bool new_year = false; // the file's fiscal year is not in the books yet
  books::ImportPlan import;
  std::vector<ChangedVoucher> changed; // vouchers in the books with rows struck or added, by series and number
};

// Plans importing the SIE file's fiscal year 0 into the project's books (see
// books::plan_import), with the project's name and organisation number read
// now. Error text, and nothing to import, when the books or the file cannot be
// read, the file is no SIE file or has errors, the plan has errors (e.g. another
// organisation number, or a fiscal year that overlaps one in the books), the
// file has nothing new, or the import would change vouchers already in the
// books otherwise than by struck and added rows (another date or text), or
// rename, replace, drop or demote them: for now cratchit only adds vouchers,
// and rows to vouchers as SIE allows. Writes nothing.
std::expected<SieImportPlan, std::string> plan_sie_import(Project const& project, std::filesystem::path const& sie_file);

// What import_sie did
struct SieImported {
  std::filesystem::path moved_to; // where the SIE file is now
};

// Imports as planned: writes the books (only the files that change, see
// books::write_books), the organisation number (and name) in project.txt when
// the import gives the project one, and moves the SIE file into
// <project folder>/importerade_sie_filer/ (created when missing; a file of the
// same name there is never overwritten: the name gets _2, _3, ...). Error text
// when any step fails; what was written before stays.
std::expected<SieImported, std::string> import_sie(Project const& project, SieImportPlan const& plan);

} // namespace cratchit

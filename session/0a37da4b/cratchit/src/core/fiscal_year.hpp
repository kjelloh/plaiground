#pragma once

#include "core/project.hpp"

#include <expected>
#include <string>
#include <vector>

namespace cratchit {

// A fiscal year (räkenskapsår) of a project, as dates YYYY-MM-DD
struct FiscalYear {
  std::string start; // e.g. 2025-05-01
  std::string end;   // e.g. 2026-04-30

  bool operator==(FiscalYear const&) const = default;
};

// The fiscal years of the project: the `year.<start> = <end>` entries of
// <project folder>/.cratchit/books/books.txt (as written by take 3), the latest
// first. No books.txt means no fiscal years yet. Never creates or changes
// anything. Error text when books.txt cannot be read, or a date is not YYYY-MM-DD.
std::expected<std::vector<FiscalYear>, std::string> list_fiscal_years(Project const& project);

// How the year is named in the UI: "2025" when it starts and ends in the same
// calendar year, else "2025/26" (and "1999/2000" across a century)
std::string label_of(FiscalYear const& year);

} // namespace cratchit

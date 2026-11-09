#pragma once

#include "core/dot_folder.hpp"
#include "core/fiscal_year.hpp"
#include "core/project.hpp"

#include <chrono>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

namespace cratchit {

// The folder of a new project named name in the workspace: <workspace folder>/<name>.
// Error text when name cannot be a folder name (see is_folder_name) or the folder exists.
std::expected<std::filesystem::path, std::string> new_project_folder(Workspace const& workspace, std::string_view name);

// A Swedish organisation number as cratchit keeps it, NNNNNN-NNNN, from ten
// digits with or without the dash (spaces around are ignored). The last digit
// must be the check digit (Luhn). Error text otherwise.
std::expected<std::string, std::string> parse_orgnr(std::string_view text);

// The first month of the first fiscal year, YYYY-MM (e.g. 2026-05). Error text otherwise.
std::expected<std::chrono::year_month, std::string> parse_first_month(std::string_view text);

// The length of the first fiscal year in months, 1 to 18: a new company's first
// fiscal year may be shortened or extended up to 18 months. Error text otherwise.
std::expected<int, std::string> parse_first_year_length(std::string_view text);

// The fiscal year starting on the first day of first_month and lasting months
// months, e.g. 2026-05 and 12 give 2026-05-01 – 2027-04-30
FiscalYear first_fiscal_year(std::chrono::year_month first_month, int months);

// Creates a project in folder (see new_project_folder) from the details:
// the folder, .cratchit/project.txt (`created`, `uuid` (new), `name`, `orgnr`)
// and .cratchit/books/books.txt holding only `year.<start> = <end>`. No other
// files: no chart of accounts yet, and the books read missing tables as empty.
// Error text when any step fails; what was created before stays.
std::expected<Project, std::string> create_project_from_details(std::filesystem::path const& folder, std::string_view name,
                                                                std::string_view orgnr, FiscalYear const& year,
                                                                std::string_view today);

} // namespace cratchit

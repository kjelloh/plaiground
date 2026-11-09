#pragma once

#include "core/dot_folder.hpp"
#include "core/fiscal_year.hpp"
#include "core/project.hpp"

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

// The first fiscal year from its first and last day, YYYY-MM-DD each,
// separated by spaces or a dash between spaces (e.g. "2026-01-01 – 2026-12-31").
// The first day must be the first of a month, the last day the last of a month
// after it, and the year at most 18 months long (a first fiscal year may be
// extended). Error text otherwise.
std::expected<FiscalYear, std::string> parse_first_fiscal_year(std::string_view text);

// Creates a project in folder (see new_project_folder) from the details:
// the folder, .cratchit/project.txt (`created`, `uuid` (new), `name`, `orgnr`)
// and .cratchit/books/books.txt holding only `year.<start> = <end>`. No other
// files: no chart of accounts yet, and the books read missing tables as empty.
// Error text when any step fails; what was created before stays.
std::expected<Project, std::string> create_project_from_details(std::filesystem::path const& folder, std::string_view name,
                                                                std::string_view orgnr, FiscalYear const& year,
                                                                std::string_view today);

} // namespace cratchit

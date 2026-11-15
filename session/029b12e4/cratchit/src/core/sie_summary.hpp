#pragma once

#include "core/date.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cratchit {

// What an SIE file holds, in short, so that the user can tell files apart
// whose names say nothing (e.g. when looking one up with Tab)
struct SieSummary {
  std::optional<std::pair<Date, Date>> year;          // #RAR 0
  std::optional<Date> generated;                      // #GEN
  std::size_t vouchers = 0;                           // #VER
  std::optional<Date> last_date;                      // the latest voucher date
  std::vector<std::pair<std::string, std::string>> series; // each series and its highest number, by series
};

// Reads and summarizes the SIE file. Does not log, since it is read each time
// the user lists files. Error text when the file cannot be read or is no SIE
// file (#SIETYP missing); other errors in the file are left to whoever imports it.
std::expected<SieSummary, std::string> summarize_sie_file(std::filesystem::path const& file);

// The summary on one line, e.g.
// "2025-05-01 – 2026-04-30  skapad 2026-11-12  118 ver. t.o.m. 2026-04-30  A 82 · D 11 · M 23".
// Parts the file lacks are left out.
std::string summary_line(SieSummary const& summary);

// The lines to show under an SIE file's name: its summary line, or why it cannot be read
std::vector<std::string> describe_sie_file(std::filesystem::path const& file);

// The SIE files and folders found with Tab, laid out to choose from: a heading
// per fiscal year (#RAR 0, latest first), and under it a line per file in
// columns: name, created (#GEN), vouchers and the latest date, and the highest
// number per series. Within a year the latest created file comes first. Then
// "Utan räkenskapsår", "Går inte att läsa" (with the reason) and "Mappar".
// Headings are not indented; their lines are indented two spaces.
std::vector<std::string> list_sie_files(std::vector<std::filesystem::path> const& files,
                                        std::vector<std::filesystem::path> const& folders);

} // namespace cratchit

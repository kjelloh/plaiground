#include "core/fiscal_year.hpp"

#include "core/path_value.hpp"

#include <algorithm>
#include <format>
#include <system_error>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

constexpr std::string_view year_prefix = "year.";

bool is_iso_date(std::string_view text) {
  if (text.size() != 10 || text[4] != '-' || text[7] != '-') return false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (i == 4 || i == 7) continue;
    if (text[i] < '0' || text[i] > '9') return false;
  }
  return true;
}

} // namespace

std::expected<std::vector<FiscalYear>, std::string> list_fiscal_years(Project const& project) {
  auto const file = project.folder / ".cratchit" / "books" / "books.txt";
  std::error_code ec;
  if (!fs::exists(file, ec)) return std::vector<FiscalYear>{};
  auto const entries = read_path_values(file);
  if (!entries) return std::unexpected(entries.error());
  std::vector<FiscalYear> result;
  for (auto const& entry : *entries) {
    if (!entry.path.starts_with(year_prefix)) continue;
    FiscalYear year{entry.path.substr(year_prefix.size()), entry.value};
    if (!is_iso_date(year.start) || !is_iso_date(year.end)) {
      return std::unexpected(std::format("{}: '{} = {}' är inget räkenskapsår", file.string(), entry.path, entry.value));
    }
    result.push_back(std::move(year));
  }
  std::ranges::sort(result, std::ranges::greater{}, &FiscalYear::start);
  return result;
}

std::string label_of(FiscalYear const& year) {
  auto const start = year.start.substr(0, 4);
  auto const end = year.end.substr(0, 4);
  if (start == end) return start;
  if (start.substr(0, 2) == end.substr(0, 2)) return std::format("{}/{}", start, end.substr(2));
  return std::format("{}/{}", start, end);
}

} // namespace cratchit

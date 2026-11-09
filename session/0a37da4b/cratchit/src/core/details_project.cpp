#include "core/details_project.hpp"

#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/path_value.hpp"
#include "core/sie_project.hpp"

#include <chrono>
#include <format>
#include <system_error>
#include <vector>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

std::string_view trimmed(std::string_view text) {
  auto const first = text.find_first_not_of(" \t");
  if (first == std::string_view::npos) return {};
  return text.substr(first, text.find_last_not_of(" \t") - first + 1);
}

// The words of text, split at spaces and tabs
std::vector<std::string_view> words_of(std::string_view text) {
  std::vector<std::string_view> words;
  while (!(text = trimmed(text)).empty()) {
    auto const end = text.find_first_of(" \t");
    words.push_back(text.substr(0, end));
    if (end == std::string_view::npos) break;
    text.remove_prefix(end);
  }
  return words;
}

bool luhn_ok(std::string_view digits) {
  int sum = 0;
  for (std::size_t i = 0; i < digits.size(); ++i) {
    int value = digits[i] - '0';
    if (i % 2 == 0) value *= 2; // from the left: 2, 1, 2, 1, ... over ten digits
    sum += value > 9 ? value - 9 : value;
  }
  return sum % 10 == 0;
}

} // namespace

std::expected<fs::path, std::string> new_project_folder(Workspace const& workspace, std::string_view name) {
  name = trimmed(name);
  if (name.empty()) return std::unexpected("Skriv företagets namn");
  if (!is_folder_name(name)) {
    return std::unexpected(std::format("Namnet \"{}\" går inte att använda som namn på en mapp", name));
  }
  auto const folder = workspace.folder / fs::path(std::u8string(name.begin(), name.end()));
  std::error_code ec;
  if (fs::exists(folder, ec)) return std::unexpected(std::format("Mappen {} finns redan", folder.string()));
  return folder;
}

std::expected<std::string, std::string> parse_orgnr(std::string_view text) {
  text = trimmed(text);
  std::string digits;
  for (std::size_t i = 0; i < text.size(); ++i) {
    auto const c = text[i];
    if (c >= '0' && c <= '9') {
      digits.push_back(c);
    } else if (!(c == '-' && i == 6)) {
      return std::unexpected(std::format("\"{}\" är inget organisationsnummer. Skriv tio siffror, till exempel 556677-8899", text));
    }
  }
  if (digits.size() != 10) {
    return std::unexpected(std::format("\"{}\" är inget organisationsnummer. Skriv tio siffror, till exempel 556677-8899", text));
  }
  if (!luhn_ok(digits)) return std::unexpected(std::format("Kontrollsiffran i {} stämmer inte", text));
  return digits.substr(0, 6) + "-" + digits.substr(6);
}

std::expected<FiscalYear, std::string> parse_first_fiscal_year(std::string_view text) {
  auto words = words_of(text);
  if (words.size() == 3 && (words[1] == "-" || words[1] == "–")) words.erase(words.begin() + 1);
  auto const usage = std::string("Skriv första och sista dagen, till exempel 2026-01-01 2026-12-31");
  if (words.size() != 2) return std::unexpected(usage);
  auto const start = parse_iso_date(words[0]);
  auto const end = parse_iso_date(words[1]);
  if (!start || !end) return std::unexpected(usage);
  using namespace std::chrono;
  if (start->day() != day{1}) return std::unexpected(std::format("{} är inte den första dagen i en månad", words[0]));
  if (year_month_day_last(end->year(), month_day_last(end->month())) != *end) {
    return std::unexpected(std::format("{} är inte den sista dagen i en månad", words[1]));
  }
  if (!(*start < *end)) return std::unexpected("Sista dagen måste komma efter första dagen");
  auto const months = (end->year() - start->year()).count() * 12 +
                      (static_cast<int>(static_cast<unsigned>(end->month())) - static_cast<int>(static_cast<unsigned>(start->month()))) + 1;
  if (months > 18) return std::unexpected(std::format("Räkenskapsåret är {} månader. Det får vara högst 18", months));
  return FiscalYear{format_iso_date(*start), format_iso_date(*end)};
}

std::expected<Project, std::string> create_project_from_details(fs::path const& folder, std::string_view name,
                                                                std::string_view orgnr, FiscalYear const& year,
                                                                std::string_view today) {
  std::error_code ec;
  if (!fs::create_directory(folder, ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", folder.string(), ec ? ec.message() : std::string("mappen finns redan")));
  }
  if (!fs::create_directory(folder / ".cratchit", ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", (folder / ".cratchit").string(), ec.message()));
  }
  Project project{folder, folder / ".cratchit" / "project.txt", new_uuid(), std::string(name)};
  PathValues const entries{{"created", std::string(today)}, {"uuid", project.uuid}, {"name", project.name},
                           {"orgnr", std::string(orgnr)}};
  if (auto written = write_path_values(project.file, entries); !written) return std::unexpected(written.error());
  // Only the fiscal year: the tables of the books come when there is something to put in them
  auto const books_folder = books::books_folder(folder);
  if (!fs::create_directory(books_folder, ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", books_folder.string(), ec.message()));
  }
  if (auto written = write_path_values(books_folder / "books.txt", {{"year." + year.start, year.end}}); !written) {
    return std::unexpected(written.error());
  }
  return project;
}

} // namespace cratchit

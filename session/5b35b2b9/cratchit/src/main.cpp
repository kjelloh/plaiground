#include "core/ar/closing.hpp"
#include "core/ar/layout.hpp"
#include "core/ar/statement.hpp"
#include "core/ar/text.hpp"
#include "core/date.hpp"
#include "core/manual.hpp"
#include "core/sie/check.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <filesystem>
#include <print>
#include <string_view>

// cratchit_ar [--accounts] <file.se>: prints the Swedish årsredovisning of an
// SIE 4 file, or with --accounts the BAS accounts in each of its posts.
// cratchit_ar --help or -h prints the manual.
int main(int argc, char* argv[]) {
  if (argc == 2 && (std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h")) {
    std::print("{}", cratchit::manual_text());
    return 0;
  }
  bool const accounts = argc == 3 && std::string_view(argv[1]) == "--accounts";
  if ((argc != 2 && !accounts) || std::string_view(argv[argc - 1]).starts_with('-')) {
    std::println(stderr, "Usage: cratchit_ar [--accounts] <file.se>\nSee cratchit_ar --help");
    return 2;
  }
  std::filesystem::path const file = argv[argc - 1];
  auto const lexed = cratchit::sie::lex_file(file);
  if (!lexed) {
    std::println(stderr, "Error: {}", lexed.error());
    return 1;
  }
  if (!lexed->alert.empty()) {
    std::println(stderr, "Note: {}", lexed->alert);
  }
  auto const parsed = cratchit::sie::parse_document(lexed->lines);
  for (auto const& error : parsed.errors) {
    std::println(stderr, "Error: {}", error);
  }
  auto const findings = cratchit::sie::check_document(parsed.document);
  for (auto const& error : findings.errors) {
    std::println(stderr, "Error: {}", error);
  }
  for (auto const& note : findings.notes) {
    std::println(stderr, "Note: {}", note);
  }
  if (!parsed.errors.empty() || !findings.errors.empty()) {
    return 1;
  }
  auto const layout = cratchit::ar::parse_layout(cratchit::ar::layout_2024_text());
  if (!layout) {
    std::println(stderr, "Error: {}", layout.error());
    return 1;
  }
  auto const& document = parsed.document;
  // The year of the file and the year before, when the file has it
  std::vector<cratchit::sie::FiscalYear> years;
  std::vector<cratchit::ar::Balances> balances;
  for (int index = 0; index >= -1; --index) {
    if (auto const year = document.year(index)) {
      years.push_back(*year);
      balances.push_back(cratchit::ar::year_balances(document, index));
    }
  }
  auto const statement = cratchit::ar::build_statement(*layout, balances);
  auto errors = statement.errors;
  for (auto const& finding : cratchit::ar::check_statement(statement)) {
    errors.push_back(finding);
  }
  for (auto const& error : errors) {
    std::println(stderr, "Error: {}", error);
  }
  // Closing entries BAS asks for that the books lack; the årsredovisning shows the books as they are
  for (std::size_t index = 0; index < years.size(); ++index) {
    for (auto const& note : cratchit::ar::check_closing(balances[index], years[index].end)) {
      std::println(stderr, "Note: {}", note);
    }
  }
  int const exit_code = errors.empty() ? 0 : 1;
  if (accounts) {
    std::print("{}", cratchit::ar::format_accounts(statement, years));
    return exit_code;
  }
  std::println("{}", document.company_name);
  std::println("{}", document.orgnr);
  std::println("Årsredovisning för räkenskapsåret {} - {}", cratchit::format_iso_date(years.front().start),
               cratchit::format_iso_date(years.front().end));
  std::println("");
  std::print("{}", cratchit::ar::format_statements(statement, years));
  return exit_code;
}

#include "core/ar/closing.hpp"
#include "core/ar/disposition.hpp"
#include "core/ar/equity.hpp"
#include "core/ar/layout.hpp"
#include "core/ar/notes.hpp"
#include "core/ar/overview.hpp"
#include "core/ar/requirements.hpp"
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

// cratchit_ar [--trace] <file.se>: prints the Swedish årsredovisning of an
// SIE 4 file, or with --trace the BAS accounts and vouchers behind each of its amounts.
// cratchit_ar --help or -h prints the manual.
int main(int argc, char* argv[]) {
  if (argc == 2 && (std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h")) {
    std::print("{}", cratchit::manual_text());
    return 0;
  }
  bool const trace = argc == 3 && std::string_view(argv[1]) == "--trace";
  if ((argc != 2 && !trace) || std::string_view(argv[argc - 1]).starts_with('-')) {
    std::println(stderr, "Usage: cratchit_ar [--trace] <file.se>\nSee cratchit_ar --help");
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
  auto const layout = cratchit::ar::parse_layout(cratchit::ar::layout_text());
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
  auto const equity = cratchit::ar::equity_changes(*layout, document);
  for (auto const& error : equity.errors) {
    std::println(stderr, "Error: {}", error);
  }
  for (auto const& note : equity.notes) {
    std::println(stderr, "Note: {}", note);
  }
  auto const notes = cratchit::ar::build_notes(statement, document);
  for (auto const& error : notes.errors) {
    std::println(stderr, "Error: {}", error);
  }
  auto const overview = cratchit::ar::build_overview(statement);
  auto const disposition = cratchit::ar::build_disposition(statement);
  auto const check = cratchit::ar::check_requirements(statement, overview, notes);
  std::println(stderr, "Note: K2 check: {} required {} missing (TBD in the årsredovisning), see the end of the output",
               check.missing.size(), check.missing.size() == 1 ? "item is" : "items are");
  int const exit_code = errors.empty() && equity.errors.empty() && notes.errors.empty() ? 0 : 1;
  if (trace) {
    std::print("{}", cratchit::ar::format_trace(overview, disposition, equity, statement, notes, document, years));
    return exit_code;
  }
  // In the order of BFNAR 2016:10 p. 3.2: förvaltningsberättelse, resultaträkning, balansräkning, noter
  std::println("{}", cratchit::ar::format_front(document, years));
  std::println("{}", cratchit::ar::format_business(check));
  std::println("{}", cratchit::ar::format_overview(overview, years, check));
  std::println("{}", cratchit::ar::format_equity_changes(equity));
  std::println("{}", cratchit::ar::format_disposition(disposition, check));
  std::println("{}", cratchit::ar::format_statements(statement, years, cratchit::ar::note_references(notes)));
  std::println("{}", cratchit::ar::format_notes(notes, years, check));
  std::println("{}", cratchit::ar::format_signatures(document, years, check));
  std::print("{}", cratchit::ar::format_check(check));
  return exit_code;
}

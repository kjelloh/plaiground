#include "core/ar/arguments.hpp"
#include "core/ar/closing.hpp"
#include "core/ar/disposition.hpp"
#include "core/ar/equity.hpp"
#include "core/ar/layout.hpp"
#include "core/ar/notes.hpp"
#include "core/ar/output.hpp"
#include "core/ar/overview.hpp"
#include "core/ar/requirements.hpp"
#include "core/ar/statement.hpp"
#include "core/ar/text.hpp"
#include "core/date.hpp"
#include "core/manual.hpp"
#include "core/sie/check.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <ctime>
#include <filesystem>
#include <optional>
#include <string>
#include <format>
#include <print>
#include <string_view>
#include <vector>

namespace {

// The local date and time now, e.g. 2026-11-02 18:04
std::string now_text() {
  auto const now = std::time(nullptr);
  std::tm local{};
  localtime_r(&now, &local);
  char text[32];
  std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M", &local);
  return text;
}

} // namespace

// cratchit_ar [--trace] [--bas-year <year>] <file.se>: writes the Swedish
// årsredovisning of an SIE 4 file, its report and a copy of the SIE file into the
// folder <orgnr>_<start>_<end> of the working directory; with --trace prints the
// BAS accounts and vouchers behind each of its amounts instead. The mapping is the
// one for the BAS year of the books. cratchit_ar --help or -h prints the manual.
int main(int argc, char* argv[]) {
  auto const arguments = cratchit::ar::parse_arguments(std::vector<std::string_view>(argv + 1, argv + argc));
  if (!arguments) {
    std::println(stderr, "Error: {}\n{}\nSee cratchit_ar --help", arguments.error(), cratchit::ar::usage);
    return 2;
  }
  if (arguments->help) {
    std::print("{}", cratchit::manual_text());
    return 0;
  }
  bool const trace = arguments->trace;
  auto const bas_year = arguments->bas_year.value_or(cratchit::ar::default_bas_year);
  auto const mapping = cratchit::ar::layout_for(bas_year);
  if (!mapping) {
    std::println(stderr, "Error: no mapping for BAS {}; the oldest is for BAS {}\n{}\nSee cratchit_ar --help", bas_year,
                 cratchit::ar::built_in_layouts().front().year, cratchit::ar::usage);
    return 2;
  }
  auto const& file = arguments->file;
  // Errors and notes are collected: --trace prints them, a normal run writes them to the report
  std::vector<std::string> errors;
  std::vector<std::string> notes_found;
  auto const lexed = cratchit::sie::lex_file(file);
  if (!lexed) {
    std::println(stderr, "Error: {}", lexed.error());
    return 1;
  }
  if (!lexed->alert.empty()) {
    notes_found.push_back(lexed->alert);
  }
  auto const parsed = cratchit::sie::parse_document(lexed->lines);
  auto const findings = cratchit::sie::check_document(parsed.document);
  // An SIE file that cannot be read is not worth a folder: its errors go to the terminal only
  if (!parsed.errors.empty() || !findings.errors.empty()) {
    for (auto const& error : parsed.errors) {
      std::println(stderr, "Error: {}", error);
    }
    for (auto const& error : findings.errors) {
      std::println(stderr, "Error: {}", error);
    }
    return 1;
  }
  for (auto const& note : findings.notes) {
    notes_found.push_back(note);
  }
  auto const mapping_text =
      std::format("{} ({}; books following BAS {}{})", mapping->file, cratchit::ar::bas_years_of(*mapping), bas_year,
                  arguments->bas_year ? "" : ", the default; see --bas-year");
  auto const layout = cratchit::ar::parse_layout(mapping->text);
  if (!layout) {
    std::println(stderr, "Error: {}", layout.error());
    return 1;
  }
  // Books not yet closed: show the årsredovisning as if the year's result were booked, and say so
  auto const unbooked = cratchit::ar::unbooked_result(parsed.document);
  auto const document = unbooked ? cratchit::ar::with_closing_entry(parsed.document, *unbooked) : parsed.document;
  if (auto const year = document.year(0); unbooked && year) {
    notes_found.push_back(std::format(
        "Closing: the books for {} - {} are not closed: the year's result {} is not booked to 2099 (nothing on 8999). "
        "The årsredovisning is a preview that shows it as booked (see --trace).",
        cratchit::format_iso_date(year->start), cratchit::format_iso_date(year->end), cratchit::format_amount(-*unbooked)));
  }
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
  errors.insert(errors.end(), statement.errors.begin(), statement.errors.end());
  for (auto const& finding : cratchit::ar::check_statement(statement)) {
    errors.push_back(finding);
  }
  // Closing entries BAS asks for that the books lack; the årsredovisning shows the books as they are
  for (std::size_t index = 0; index < years.size(); ++index) {
    for (auto const& note : cratchit::ar::check_closing(balances[index], years[index].end)) {
      notes_found.push_back("Closing: " + note);
    }
  }
  auto const equity = cratchit::ar::equity_changes(*layout, document);
  errors.insert(errors.end(), equity.errors.begin(), equity.errors.end());
  notes_found.insert(notes_found.end(), equity.notes.begin(), equity.notes.end());
  auto const notes = cratchit::ar::build_notes(statement, document);
  errors.insert(errors.end(), notes.errors.begin(), notes.errors.end());
  auto const overview = cratchit::ar::build_overview(statement);
  auto const disposition = cratchit::ar::build_disposition(statement);
  auto const check = cratchit::ar::check_requirements(statement, overview, notes, unbooked);
  int const exit_code = errors.empty() ? 0 : 1;
  for (auto const& error : errors) {
    std::println(stderr, "Error: {}", error);
  }

  if (trace) {
    std::println(stderr, "Note: mapping {}", mapping_text);
    for (auto const& note : notes_found) {
      std::println(stderr, "Note: {}", note);
    }
    std::println("Mapping: {}", mapping_text);
    std::print("{}", cratchit::ar::format_trace(overview, disposition, equity, statement, notes, document, years));
    return exit_code;
  }

  // The årsredovisning, in the order of BFNAR 2016:10 p. 3.2: förvaltningsberättelse, resultaträkning,
  // balansräkning, noter; none when there are errors
  std::optional<std::string> ar;
  if (errors.empty()) {
    ar = std::format("{}\n{}\n{}\n{}\n{}\n{}\n{}\n{}", cratchit::ar::format_front(document, years, check),
                     cratchit::ar::format_business(check), cratchit::ar::format_overview(overview, years, check),
                     cratchit::ar::format_equity_changes(equity), cratchit::ar::format_disposition(disposition, check),
                     cratchit::ar::format_statements(statement, years, cratchit::ar::note_references(notes)),
                     cratchit::ar::format_notes(notes, years, check),
                     cratchit::ar::format_signatures(document, years, check));
  }
  auto const paths = cratchit::ar::output_paths(std::filesystem::current_path(), document.orgnr, years.front());
  auto const items = std::format("{} {} TBD", check.missing.size(), check.missing.size() == 1 ? "item" : "items");
  auto const status = !errors.empty() ? std::format("{} {}: the årsredovisning was not written",
                                                    errors.size(), errors.size() == 1 ? "error" : "errors")
                      : unbooked     ? std::format("PRELIMINÄR: the books are not closed; {}", items)
                      : check.missing.empty() ? std::string("complete")
                                              : std::format("not complete; {}", items);
  std::string sie_details;
  if (!document.program.empty()) {
    sie_details = std::format("#PROGRAM {} {}", document.program, document.program_version);
  }
  if (document.generated) {
    sie_details += std::format("{}#GEN {}", sie_details.empty() ? "" : ", ", cratchit::format_iso_date(*document.generated));
  }
  cratchit::ar::ReportHeader const header{paths.ar.filename().string(),
                                          now_text(),
                                          "cratchit_ar " CRATCHIT_AR_VERSION,
                                          file.string(),
                                          paths.sie.filename().string(),
                                          sie_details,
                                          mapping_text,
                                          status};
  auto const report = cratchit::ar::format_report(header, errors, notes_found,
                                                  cratchit::ar::format_check(check, mapping_text));
  auto const written = cratchit::ar::write_outputs(paths, ar, report, file);
  if (!written) {
    std::println(stderr, "Error: {}", written.error());
    return 1;
  }
  std::println("Folder {}", paths.folder.string());
  if (written->ar) {
    std::println("  Wrote {}: {}", paths.ar.filename().string(), status);
    std::println("    (replaced on every run: copy it before editing)");
  } else {
    std::println("  Did not write {}: {} (an earlier one is left as it was)", paths.ar.filename().string(), status);
  }
  std::println("  Wrote {}: {} {}, the K2 check", paths.report.filename().string(), notes_found.size(),
               notes_found.size() == 1 ? "note" : "notes");
  if (written->sie_copied) {
    std::println("  Copied the SIE file as {}", paths.sie.filename().string());
  } else {
    std::println("  Kept {}{}", paths.sie.filename().string(),
                 written->ar ? " (the same as the input)" : " (it matches the earlier årsredovisning)");
  }
  return exit_code;
}

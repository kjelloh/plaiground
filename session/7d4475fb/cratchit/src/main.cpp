#include "core/balances.hpp"
#include "core/closing.hpp"
#include "core/date.hpp"
#include "core/ink2/arguments.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/mapping.hpp"
#include "core/ink2/output.hpp"
#include "core/manual.hpp"
#include "core/sie/check.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <format>
#include <print>
#include <string>
#include <string_view>
#include <vector>

// cratchit_ink2 <file.se>: prints INK2R, the räkenskapsschema of the Swedish
// inkomstdeklaration 2, of fiscal year 0 of an SIE 4 file, by the BAS 2023 table to INK2.
// Errors and notes go to stderr. cratchit_ink2 --help or -h prints the manual.
int main(int argc, char* argv[]) {
  auto const arguments = cratchit::ink2::parse_arguments(std::vector<std::string_view>(argv + 1, argv + argc));
  if (!arguments) {
    std::println(stderr, "Error: {}\n{}\nSee cratchit_ink2 --help", arguments.error(), cratchit::ink2::usage);
    return 2;
  }
  if (arguments->help) {
    std::print("{}", cratchit::manual_text());
    return 0;
  }
  auto const lexed = cratchit::sie::lex_file(arguments->file);
  if (!lexed) {
    std::println(stderr, "Error: {}", lexed.error());
    return 1;
  }
  std::vector<std::string> notes;
  if (!lexed->alert.empty()) {
    notes.push_back(lexed->alert);
  }
  auto const parsed = cratchit::sie::parse_document(lexed->lines);
  auto const findings = cratchit::sie::check_document(parsed.document);
  if (!parsed.errors.empty() || !findings.errors.empty()) {
    for (auto const& error : parsed.errors) {
      std::println(stderr, "Error: {}", error);
    }
    for (auto const& error : findings.errors) {
      std::println(stderr, "Error: {}", error);
    }
    return 1;
  }
  notes.insert(notes.end(), findings.notes.begin(), findings.notes.end());
  auto const year = parsed.document.year(0);
  if (!year) {
    std::println(stderr, "Error: the SIE file has no fiscal year 0 (#RAR 0)");
    return 1;
  }
  auto const lines = cratchit::ink2::parse_mapping(cratchit::ink2::mapping_text());
  if (!lines) {
    std::println(stderr, "Error: {}: {}", cratchit::ink2::mapping_file, lines.error());
    return 1;
  }
  // Books not yet closed: show the form as if the year's result were booked, and say so
  auto const unbooked = cratchit::unbooked_result(parsed.document);
  auto const document = unbooked ? cratchit::with_closing_entry(parsed.document, *unbooked) : parsed.document;
  if (unbooked) {
    notes.push_back(std::format("Closing: the books for {} - {} are not closed: the year's result {} is not booked to "
                                "2099 (nothing on 8999). The form is a preview that shows it as booked.",
                                cratchit::format_iso_date(year->start), cratchit::format_iso_date(year->end),
                                cratchit::format_amount(-*unbooked)));
  }
  auto const balances = cratchit::year_balances(document, 0);
  // Closing entries BAS asks for that the books lack; the form shows the books as they are
  for (auto const& note : cratchit::check_closing(balances, year->end)) {
    notes.push_back("Closing: " + note);
  }
  auto const form = cratchit::ink2::build_form(*lines, balances, cratchit::ink2::sru_codes(document));
  notes.insert(notes.end(), form.notes.begin(), form.notes.end());
  cratchit::ink2::Header const header{document.company_name, document.orgnr, year->start, year->end,
                                      std::string(cratchit::ink2::mapping_file), unbooked.has_value()};
  std::print("{}", cratchit::ink2::format_form(header, *lines, form));
  for (auto const& error : form.errors) {
    std::println(stderr, "Error: {}", error);
  }
  for (auto const& note : notes) {
    std::println(stderr, "Note: {}", note);
  }
  return form.errors.empty() ? 0 : 1;
}

#include "core/balances.hpp"
#include "core/closing.hpp"
#include "core/date.hpp"
#include "core/ink2/arguments.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/mapping.hpp"
#include "core/ink2/output.hpp"
#include "core/ink2/previous.hpp"
#include "core/ink2/skv.hpp"
#include "core/ink2/tax.hpp"
#include "core/manual.hpp"
#include "core/sie/check.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"
#include "core/sru.hpp"

#include <format>
#include <print>
#include <string>
#include <string_view>
#include <vector>

// cratchit_ink2 <file.se> <last year's BLANKETTER.SRU or its folder>, or with
// --previous-deficit <kronor> instead of last year's return: prints INK2R, INK2S and INK2 page 1
// of the Swedish inkomstdeklaration 2 for fiscal year 0 of an SIE 4 file. Errors and notes go to
// stderr. cratchit_ink2 --help or -h prints the manual.
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
  auto const lexed = cratchit::sie::lex_file(arguments->sie);
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
  auto const skv = cratchit::ink2::parse_skv_fields(cratchit::ink2::skv_fields_text());
  if (!skv) {
    std::println(stderr, "Error: {}: {}", cratchit::ink2::skv_fields_file, skv.error());
    return 1;
  }
  // Last year's return, for its underskott
  cratchit::ink2::Previous previous;
  if (arguments->previous) {
    auto const file = cratchit::ink2::sru_file_in(*arguments->previous);
    if (!file) {
      std::println(stderr, "Error: {}", file.error());
      return 1;
    }
    auto const forms = cratchit::read_sru(*file);
    if (!forms) {
      std::println(stderr, "Error: {}", forms.error());
      return 1;
    }
    auto const read = cratchit::ink2::previous_from_sru(*forms, file->string(), parsed.document.orgnr, year->start);
    if (!read) {
      std::println(stderr, "Error: {}", read.error());
      return 1;
    }
    previous = *read;
  } else {
    previous = cratchit::ink2::previous_deficit(*arguments->previous_deficit, "--previous-deficit");
  }
  notes.push_back(arguments->previous ? std::format("INK2S 4.14a: last year's underskott is {} kr (4.16 of {})",
                                              previous.deficit, previous.source)
                                : std::format("INK2S 4.14a: last year's underskott is {} kr ({}; typed, not checked "
                                              "against last year's return)",
                                              previous.deficit, previous.source));
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
  auto const codes = cratchit::ink2::sru_codes(document);
  auto const form = cratchit::ink2::build_form(*lines, balances, codes);
  notes.insert(notes.end(), form.notes.begin(), form.notes.end());
  // This year's opening balances against last year's filed balance sheet
  if (previous.ink2r) {
    auto const opening = cratchit::ink2::build_form(*lines, cratchit::ink2::opening_balances(document), codes);
    for (auto const& note : cratchit::ink2::check_opening(opening, *previous.ink2r)) {
      notes.push_back(note);
    }
  }
  auto const tax = cratchit::ink2::build_tax_return(*skv, form, previous);
  notes.insert(notes.end(), tax.notes.begin(), tax.notes.end());
  cratchit::ink2::Header const header{document.company_name, document.orgnr, year->start, year->end,
                                      std::string(cratchit::ink2::mapping_file), unbooked.has_value()};
  std::print("{}", cratchit::ink2::format_form(header, *lines, form));
  std::print("\n{}", cratchit::ink2::format_skv_form("INK2S Skattemässiga justeringar",
                                                      cratchit::ink2::fields_of(*skv, "INK2S"), tax.ink2s,
                                                      tax.ink2s_marks));
  std::print("\n{}", cratchit::ink2::format_skv_form("INK2 Inkomstdeklaration 2, sidan 1",
                                                      cratchit::ink2::fields_of(*skv, "INK2"), tax.ink2, {}));
  std::vector<std::string> errors = form.errors;
  errors.insert(errors.end(), tax.errors.begin(), tax.errors.end());
  for (auto const& error : errors) {
    std::println(stderr, "Error: {}", error);
  }
  for (auto const& note : notes) {
    std::println(stderr, "Note: {}", note);
  }
  return errors.empty() ? 0 : 1;
}

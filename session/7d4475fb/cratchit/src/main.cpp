#include "core/balances.hpp"
#include "core/closing.hpp"
#include "core/date.hpp"
#include "core/ink2/arguments.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/mapping.hpp"
#include "core/ink2/output.hpp"
#include "core/ink2/previous.hpp"
#include "core/ink2/skv.hpp"
#include "core/ink2/sru_output.hpp"
#include "core/ink2/tax.hpp"
#include "core/manual.hpp"
#include "core/sie/check.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"
#include "core/sru.hpp"

#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace {

// The local date and time now, for the SRU files
cratchit::ink2::Stamp now() {
  auto const time = std::time(nullptr);
  std::tm local{};
  localtime_r(&time, &local);
  cratchit::Date const date{std::chrono::year{local.tm_year + 1900}, std::chrono::month{static_cast<unsigned>(local.tm_mon + 1)},
                            std::chrono::day{static_cast<unsigned>(local.tm_mday)}};
  return {date, std::format("{:02}{:02}{:02}", local.tm_hour, local.tm_min, local.tm_sec)};
}

} // namespace

// cratchit_ink2 <file.se> <last year's BLANKETTER.SRU or its folder>, or with
// --previous-deficit <kronor> instead of last year's return: prints INK2R, INK2S and INK2 page 1
// of the Swedish inkomstdeklaration 2 for fiscal year 0 of an SIE 4 file, and writes them as SRU
// files to the folder <orgnr>_<start>_<end> of the working directory with a copy of the SIE file.
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
  std::optional<std::filesystem::path> previous_file;
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
    previous_file = *file;
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
  // The return as SRU files, for filing: not with errors, nor while the books are not closed
  auto const paths = cratchit::ink2::output_paths(std::filesystem::current_path(), document.orgnr, *year);
  if (!errors.empty() || unbooked) {
    std::println("\nFolder {}\n  Did not write the SRU files: {} (earlier ones are left as they were)",
                 paths.folder.string(),
                 !errors.empty() ? "there are errors" : "the books are not closed (PRELIMINÄR)");
  } else {
    // INFO.SRU's postnummer and postort: this year's books, else last year's INFO.SRU
    auto postal = cratchit::ink2::postal_of(document.address.postal);
    if (postal) {
      notes.push_back(std::format("INFO.SRU: postnummer and postort {} {} from the SIE file's #ADRESS", postal->first,
                                  postal->second));
    } else if (auto const info = previous_file ? cratchit::ink2::info_file_in(previous_file->parent_path())
                                               : std::nullopt) {
      std::ifstream in(*info, std::ios::binary);
      std::string const text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
      postal = cratchit::ink2::postal_of_info(text);
      if (postal) {
        notes.push_back(std::format("INFO.SRU: postnummer and postort {} {} from {}", postal->first, postal->second,
                                    info->string()));
      }
    }
    if (!postal) {
      errors.push_back("INFO.SRU needs postnummer and postort: not in the SIE file's #ADRESS (e.g. \"17141 Solna\"), "
                       "nor in last year's INFO.SRU");
    } else {
      cratchit::ink2::Filer const filer{document.orgnr, document.company_name, postal->first, postal->second};
      auto const stamp = now();
      auto const written = cratchit::ink2::write_outputs(
          paths, cratchit::ink2::format_blanketter(*skv, filer, *year, form, tax, stamp),
          cratchit::ink2::format_info(filer, stamp, "cratchit_ink2 " CRATCHIT_INK2_VERSION), arguments->sie);
      if (!written) {
        errors.push_back(written.error());
      } else {
        std::println("\nFolder {}", paths.folder.string());
        std::println("  Wrote BLANKETTER.SRU and INFO.SRU (replaced on every run)");
        std::println("  {} {}", written->sie_copied ? "Copied the SIE file as" : "Kept",
                     paths.sie.filename().string() + (written->sie_copied ? "" : " (the same as the input)"));
      }
    }
  }
  for (auto const& error : errors) {
    std::println(stderr, "Error: {}", error);
  }
  for (auto const& note : notes) {
    std::println(stderr, "Note: {}", note);
  }
  return errors.empty() ? 0 : 1;
}

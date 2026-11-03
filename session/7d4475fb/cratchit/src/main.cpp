#include "core/balances.hpp"
#include "core/closing.hpp"
#include "core/date.hpp"
#include "core/ink2/arguments.hpp"
#include "core/ink2/check.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/mapping.hpp"
#include "core/ink2/output.hpp"
#include "core/ink2/previous.hpp"
#include "core/ink2/report.hpp"
#include "core/ink2/skv.hpp"
#include "core/ink2/sru_output.hpp"
#include "core/ink2/tax.hpp"
#include "core/ink2/trace.hpp"
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

// The local date and time now, for the SRU files and the report
struct Now {
  cratchit::ink2::Stamp stamp;
  std::string text; // e.g. 2026-11-03 18:04
};

Now now() {
  auto const time = std::time(nullptr);
  std::tm local{};
  localtime_r(&time, &local);
  cratchit::Date const date{std::chrono::year{local.tm_year + 1900},
                            std::chrono::month{static_cast<unsigned>(local.tm_mon + 1)},
                            std::chrono::day{static_cast<unsigned>(local.tm_mday)}};
  return {{date, std::format("{:02}{:02}{:02}", local.tm_hour, local.tm_min, local.tm_sec)},
          std::format("{} {:02}:{:02}", cratchit::format_iso_date(date), local.tm_hour, local.tm_min)};
}

std::string count_of(std::size_t count, std::string_view one, std::string_view many) {
  return std::format("{} {}", count, count == 1 ? one : many);
}

// A file name as UTF-8 text
std::string name_of(std::filesystem::path const& path) {
  auto const name = path.filename().u8string();
  return {name.begin(), name.end()};
}

} // namespace

// cratchit_ink2 [--trace] <file.se> <last year's BLANKETTER.SRU or its folder>, or with
// --previous-deficit <kronor> instead of last year's return: makes INK2R, INK2S and INK2 page 1
// of the Swedish inkomstdeklaration 2 for fiscal year 0 of an SIE 4 file, and writes them to the
// folder <orgnr>_<start>_<end> of the working directory: as text with a report (the INK2 check),
// and as SRU files when nothing known is missing, with a copy of the SIE file. With --trace it
// prints how each amount is made instead. cratchit_ink2 --help or -h prints the manual.
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
  // An SIE file or last year's return that cannot be read is not worth a folder: errors to the terminal only
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
  auto const last_year = previous_file ? std::format("4.16 Underskott {} kr of {}", previous.deficit, previous.source)
                                       : std::format("4.16 Underskott {} kr by --previous-deficit, not checked",
                                                     previous.deficit);
  // Books not yet closed: the return shows the result as if booked, and says so
  auto const unbooked = cratchit::unbooked_result(parsed.document);
  auto const document = unbooked ? cratchit::with_closing_entry(parsed.document, *unbooked) : parsed.document;
  if (unbooked) {
    notes.push_back(std::format("Closing: the books for {} - {} are not closed: the year's result {} is not booked to "
                                "2099 (nothing on 8999). The return is a preview that shows it as booked (see --trace).",
                                cratchit::format_iso_date(year->start), cratchit::format_iso_date(year->end),
                                cratchit::format_amount(-*unbooked)));
  }
  auto const balances = cratchit::year_balances(document, 0);
  // Closing entries BAS asks for that the books lack; the return shows the books as they are
  for (auto const& note : cratchit::check_closing(balances, year->end)) {
    notes.push_back("Closing: " + note);
  }
  auto const codes = cratchit::ink2::sru_codes(document);
  auto const form = cratchit::ink2::build_form(*lines, balances, codes);
  notes.insert(notes.end(), form.notes.begin(), form.notes.end());
  // This year's opening balances against last year's filed balance sheet
  auto const opening_balances = cratchit::ink2::opening_balances(document);
  if (previous.ink2r) {
    auto const opening = cratchit::ink2::build_form(*lines, opening_balances, codes);
    for (auto const& note : cratchit::ink2::check_opening(opening, *previous.ink2r)) {
      notes.push_back(note);
    }
  }
  auto const tax = cratchit::ink2::build_tax_return(*skv, form, previous);
  notes.insert(notes.end(), tax.notes.begin(), tax.notes.end());
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
  auto const check = cratchit::ink2::check_return(form, opening_balances, balances, previous, !previous_file, unbooked,
                                                  postal.has_value());
  std::vector<std::string> errors = form.errors;
  errors.insert(errors.end(), tax.errors.begin(), tax.errors.end());

  if (arguments->trace) {
    std::print("{}", cratchit::ink2::format_trace(document, *lines, *skv, balances, form, tax, previous));
    for (auto const& error : errors) {
      std::println(stderr, "Error: {}", error);
    }
    for (auto const& note : notes) {
      std::println(stderr, "Note: {}", note);
    }
    return errors.empty() ? 0 : 1;
  }

  // The folder: the report always, the return as text unless there are errors, the SRU files when complete
  auto const paths = cratchit::ink2::output_paths(std::filesystem::current_path(), document.orgnr, *year);
  auto const items = count_of(check.missing.size(), "item", "items") + " TBD";
  auto const status = !errors.empty() ? std::format("{}: the return was not written",
                                                    count_of(errors.size(), "error", "errors"))
                      : unbooked     ? std::format("PRELIMINÄR: the books are not closed; {}", items)
                      : check.missing.empty() ? std::string("complete")
                                              : std::format("not complete; {}", items);
  bool const complete = errors.empty() && check.missing.empty();
  auto const stamp = now();
  std::optional<std::string> fields;
  if (errors.empty()) {
    cratchit::ink2::Header const header{document.company_name, document.orgnr, year->start, year->end, status};
    fields = cratchit::ink2::format_return(header, *lines, *skv, form, tax, check.markers());
  }
  std::optional<cratchit::ink2::SruFiles> sru;
  if (complete) {
    cratchit::ink2::Filer const filer{document.orgnr, document.company_name, postal->first, postal->second};
    sru = cratchit::ink2::SruFiles{
        cratchit::ink2::format_blanketter(*skv, filer, *year, form, tax, stamp.stamp),
        cratchit::ink2::format_info(filer, stamp.stamp, "cratchit_ink2 " CRATCHIT_INK2_VERSION)};
  }
  std::error_code exists_error;
  bool const old_sru = !complete && (std::filesystem::exists(paths.blanketter, exists_error) ||
                                     std::filesystem::exists(paths.info, exists_error));
  auto const sru_text = complete ? std::string("BLANKETTER.SRU and INFO.SRU written")
                                 : std::format("not written, as the return is not complete{}",
                                               old_sru ? "; the BLANKETTER.SRU and INFO.SRU in the folder are of an "
                                                         "earlier run: do not file them"
                                                       : "");
  std::string sie_details;
  if (!document.program.empty()) {
    sie_details = std::format("#PROGRAM {} {}", document.program, document.program_version);
  }
  if (document.generated) {
    sie_details += std::format("{}#GEN {}", sie_details.empty() ? "" : ", ",
                               cratchit::format_iso_date(*document.generated));
  }
  cratchit::ink2::ReportHeader const report_header{
      name_of(paths.fields),
      stamp.text,
      "cratchit_ink2 " CRATCHIT_INK2_VERSION,
      arguments->sie.string(),
      name_of(paths.sie),
      sie_details,
      last_year,
      std::format("{} and {}", cratchit::ink2::mapping_file, cratchit::ink2::skv_fields_file),
      status,
      sru_text};
  auto const report =
      cratchit::ink2::format_report(report_header, errors, notes, cratchit::ink2::format_check(check));
  auto const written = cratchit::ink2::write_outputs(paths, report, fields, sru, arguments->sie);
  for (auto const& error : errors) {
    std::println(stderr, "Error: {}", error);
  }
  if (!written) {
    std::println(stderr, "Error: {}", written.error());
    return 1;
  }
  std::println("Folder {}", paths.folder.string());
  if (written->fields) {
    std::println("  Wrote {}: {}", name_of(paths.fields), status);
  } else {
    std::println("  Did not write {}: {} (an earlier one is left as it was)", name_of(paths.fields), status);
  }
  std::println("  Wrote {}: {}, the INK2 check", name_of(paths.report), count_of(notes.size(), "note", "notes"));
  if (written->sru) {
    std::println("  Wrote BLANKETTER.SRU and INFO.SRU (replaced on every run)");
  } else {
    std::println("  Did not write the SRU files: the return is not complete{}",
                 written->old_sru ? " (the ones in the folder are of an earlier run: do not file them)" : "");
  }
  if (written->sie_copied) {
    std::println("  Copied the SIE file as {}", name_of(paths.sie));
  } else {
    std::println("  Kept {}{}", name_of(paths.sie),
                 written->fields ? " (the same as the input)" : " (it matches the earlier return)");
  }
  return errors.empty() ? 0 : 1;
}

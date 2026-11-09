#include "core/sie_import.hpp"

#include "core/date.hpp"
#include "core/path_value.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <algorithm>
#include <format>
#include <system_error>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

constexpr std::size_t shown_errors = 3;

std::string errors_text(std::vector<std::string> const& errors) {
  std::string text = std::format("SIE-filen har {} fel:", errors.size());
  for (std::size_t i = 0; i < std::min(errors.size(), shown_errors); ++i) text += " " + errors[i] + ".";
  if (errors.size() > shown_errors) text += " ...";
  return text;
}

// The first free name for file in folder: its own, else with _2, _3, ... before the extension
fs::path free_name_in(fs::path const& folder, fs::path const& file) {
  std::error_code ec;
  auto target = folder / file.filename();
  for (int index = 2; fs::exists(target, ec); ++index) {
    target = folder / std::format("{}_{}{}", file.stem().string(), index, file.extension().string());
  }
  return target;
}

// The vouchers of the year starting on start that the planned books hold otherwise than the books
// before. Error text when one has another date or text: only struck and added rows are taken.
std::expected<std::vector<ChangedVoucher>, std::string> changed_vouchers(books::Books const& before,
                                                                         books::Books const& after, Date start) {
  std::vector<ChangedVoucher> result;
  auto const* old_year = before.year_starting(start);
  auto const* new_year = after.year_starting(start);
  if (old_year == nullptr || new_year == nullptr) return result;
  for (auto const& old_voucher : old_year->vouchers) {
    auto const found = std::ranges::find_if(new_year->vouchers, [&](books::Voucher const& v) {
      return v.series == old_voucher.series && v.number == old_voucher.number;
    });
    if (found == new_year->vouchers.end() || *found == old_voucher) continue;
    if (found->event.date != old_voucher.event.date || found->event.heading != old_voucher.event.heading) {
      return std::unexpected(std::format("Verifikationen {} {} har ett annat datum eller en annan text i filen. cratchit tar "
                                         "bara emot strukna och tillagda rader i befintliga verifikationer",
                                         old_voucher.series, old_voucher.number));
    }
    ChangedVoucher changed{old_voucher.series, old_voucher.number, format_iso_date(old_voucher.event.date),
                           old_voucher.event.heading};
    auto const removed = [](books::Posting const& p) { return p.kind == books::Posting::Kind::removed; };
    auto const struck_before = static_cast<std::size_t>(std::ranges::count_if(old_voucher.postings, removed));
    auto const struck_after = static_cast<std::size_t>(std::ranges::count_if(found->postings, removed));
    changed.struck = struck_after > struck_before ? struck_after - struck_before : 0;
    changed.added = found->postings.size() > old_voucher.postings.size() ? found->postings.size() - old_voucher.postings.size() : 0;
    result.push_back(std::move(changed));
  }
  return result;
}

} // namespace

std::string change_text(ChangedVoucher const& changed) {
  auto const rows = [](std::size_t count, std::string_view one, std::string_view many) {
    return std::format("{} {}", count, count == 1 ? one : many);
  };
  std::string what;
  if (changed.struck > 0) what = rows(changed.struck, "rad struken", "rader strukna");
  if (changed.added > 0) what += (what.empty() ? "" : ", ") + rows(changed.added, "rad tillagd", "rader tillagda");
  if (what.empty()) what = "ändrad";
  return std::format("{} {}  {}  {}: {}", changed.series, changed.number, changed.date, changed.heading, what);
}

std::expected<SieImportPlan, std::string> plan_sie_import(Project const& project, fs::path const& sie_file) {
  std::error_code ec;
  if (!sie_file.is_absolute()) return std::unexpected(std::format("{} är inte en hel sökväg", sie_file.string()));
  if (!fs::is_regular_file(sie_file, ec)) return std::unexpected(std::format("{} är ingen fil", sie_file.string()));
  auto books = books::read_books(project.folder);
  if (!books) return std::unexpected(books.error());
  books::Identity identity{project.name, {}};
  if (auto const entries = read_path_values(project.file)) identity.orgnr = find_value(*entries, "orgnr").value_or("");
  auto const lexed = sie::lex_file(sie_file);
  if (!lexed) return std::unexpected(lexed.error());
  auto const parsed = sie::parse_document(lexed->lines);
  auto const& document = parsed.document;
  if (document.sie_type.empty()) return std::unexpected(std::format("{} är ingen SIE-fil (#SIETYP saknas)", sie_file.string()));
  if (!parsed.errors.empty()) return std::unexpected(errors_text(parsed.errors));
  if (!document.year(0)) return std::unexpected("SIE-filen saknar räkenskapsår (#RAR 0)");

  SieImportPlan plan;
  plan.sie_file = sie_file;
  plan.program = document.program_version.empty() ? document.program : std::format("{} {}", document.program, document.program_version);
  if (document.generated) plan.generated = format_iso_date(*document.generated);
  plan.alert = lexed->alert;
  plan.new_year = books->year_starting(document.year(0)->start) == nullptr;
  plan.import = books::plan_import(*books, identity, document);
  auto const& import = plan.import;
  if (!import.errors.empty()) return std::unexpected(errors_text(import.errors));
  if (import.reverts() || import.vouchers_forced > 0) {
    return std::unexpected(std::format(
        "Importen skulle ta bort eller byta ut verifikationer som redan finns i bokföringen ({} med annat nummer, {} "
        "ersatta, {} borttagna, {} till väntande). cratchit importerar än så länge bara nya verifikationer och strukna "
        "eller tillagda rader",
        import.renamed.size(), import.replaced.size(), import.dropped.size(), import.demoted.size()));
  }
  auto changed = changed_vouchers(*books, import.books, import.year_start);
  if (!changed) return std::unexpected(changed.error());
  plan.changed = std::move(*changed);
  if (books::nothing_to_import(*books, import)) return std::unexpected("Allt i SIE-filen finns redan i bokföringen");
  return plan;
}

std::expected<SieImported, std::string> import_sie(Project const& project, SieImportPlan const& plan) {
  if (auto written = books::write_books(project.folder, plan.import.books); !written) return std::unexpected(written.error());
  if (plan.import.identity_changed) {
    auto entries = read_path_values(project.file);
    if (!entries) return std::unexpected(entries.error());
    set_value(*entries, "orgnr", plan.import.identity.orgnr);
    set_value(*entries, "name", plan.import.identity.name);
    if (auto written = write_path_values(project.file, *entries); !written) return std::unexpected(written.error());
  }
  auto const folder = project.folder / imported_sie_folder_name;
  std::error_code ec;
  fs::create_directories(folder, ec);
  if (ec) return std::unexpected(std::format("Kan inte skapa {}: {}", folder.string(), ec.message()));
  auto const target = free_name_in(folder, plan.sie_file);
  fs::rename(plan.sie_file, target, ec);
  if (ec) {
    // Across file systems: copy, then remove
    ec.clear();
    fs::copy_file(plan.sie_file, target, ec);
    if (ec) return std::unexpected(std::format("Kan inte flytta {} till {}: {}", plan.sie_file.string(), folder.string(), ec.message()));
    fs::remove(plan.sie_file, ec);
    if (ec) {
      return std::unexpected(std::format("Kopierade {} till {} men kan inte ta bort originalet: {}", plan.sie_file.string(),
                                         folder.string(), ec.message()));
    }
  }
  return SieImported{target};
}

} // namespace cratchit

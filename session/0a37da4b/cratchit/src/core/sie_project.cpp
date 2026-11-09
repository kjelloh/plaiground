#include "core/sie_project.hpp"

#include "core/path_value.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <format>
#include <random>
#include <system_error>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

// At most this many errors are given when a file has errors
constexpr std::size_t shown_errors = 3;

std::string errors_text(std::vector<std::string> const& errors) {
  std::string text = std::format("SIE-filen har {} fel:", errors.size());
  for (std::size_t i = 0; i < std::min(errors.size(), shown_errors); ++i) text += " " + errors[i] + ".";
  if (errors.size() > shown_errors) text += " ...";
  return text;
}

// Moves file to folder, also across file systems (copy, then remove)
std::expected<void, std::string> move_into(fs::path const& file, fs::path const& folder) {
  auto const target = folder / file.filename();
  std::error_code ec;
  fs::rename(file, target, ec);
  if (!ec) return {};
  fs::copy_file(file, target, ec);
  if (ec) return std::unexpected(std::format("Kan inte flytta {} till {}: {}", file.string(), folder.string(), ec.message()));
  fs::remove(file, ec);
  if (ec) return std::unexpected(std::format("Kopierade {} till {} men kan inte ta bort originalet: {}", file.string(),
                                             folder.string(), ec.message()));
  return {};
}

} // namespace

bool is_folder_name(std::string_view name) {
  if (name.empty() || name == "." || name == "..") return false;
  if (name.back() == ' ' || name.back() == '.') return false;
  for (auto const c : name) {
    if (static_cast<unsigned char>(c) < 0x20 || std::string_view("/\\:*?\"<>|").find(c) != std::string_view::npos) {
      return false;
    }
  }
  return true;
}

std::string new_uuid() {
  std::random_device device;
  std::seed_seq seed{device(), device(), device(), device(), device(), device(), device(), device()};
  std::mt19937_64 engine(seed);
  auto high = engine();
  auto low = engine();
  high = (high & ~std::uint64_t{0xF000}) | 0x4000;                        // version 4
  low = (low & ~(std::uint64_t{0xC} << 60)) | std::uint64_t{0x8} << 60; // variant 10xx
  return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}", high >> 32, (high >> 16) & 0xFFFF, high & 0xFFFF,
                     low >> 48, low & 0xFFFF'FFFF'FFFF);
}

bool is_uuid_like(std::string_view text) {
  if (text.size() != 36) return false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    auto const c = text[i];
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (c != '-') return false;
    } else if (!std::isxdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  return true;
}

bool is_sie_file_name(fs::path const& file) {
  auto extension = file.extension().string();
  std::ranges::transform(extension, extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return extension == ".se" || extension == ".si" || extension == ".sie";
}

std::expected<SieProjectPlan, std::string> plan_project_from_sie(fs::path const& sie_file) {
  std::error_code ec;
  if (!sie_file.is_absolute()) return std::unexpected(std::format("{} är inte en hel sökväg", sie_file.string()));
  if (!fs::is_regular_file(sie_file, ec)) return std::unexpected(std::format("{} är ingen fil", sie_file.string()));
  auto const lexed = sie::lex_file(sie_file);
  if (!lexed) return std::unexpected(lexed.error());
  auto const parsed = sie::parse_document(lexed->lines);
  auto const& document = parsed.document;
  if (document.sie_type.empty()) return std::unexpected(std::format("{} är ingen SIE-fil (#SIETYP saknas)", sie_file.string()));
  if (!parsed.errors.empty()) return std::unexpected(errors_text(parsed.errors));
  if (document.company_name.empty()) return std::unexpected("SIE-filen saknar företagets namn (#FNAMN)");
  if (!document.year(0)) return std::unexpected("SIE-filen saknar räkenskapsår (#RAR 0)");
  if (!is_folder_name(document.company_name)) {
    return std::unexpected(std::format("Företagsnamnet \"{}\" går inte att använda som namn på en mapp", document.company_name));
  }
  auto const sie_folder = sie_file.parent_path();
  if (fs::exists(sie_folder / ".cratchit" / "project.txt", ec)) {
    return std::unexpected(std::format("SIE-filen ligger i mappen för en bokföring ({})", sie_folder.string()));
  }
  SieProjectPlan plan;
  plan.sie_file = sie_file;
  plan.folder = sie_folder / fs::path(std::u8string(document.company_name.begin(), document.company_name.end()));
  if (fs::exists(plan.folder, ec)) return std::unexpected(std::format("Mappen {} finns redan", plan.folder.string()));
  plan.name = document.company_name;
  plan.orgnr = document.orgnr;
  plan.alert = lexed->alert;
  plan.import = books::plan_import({}, {plan.name, {}}, document);
  if (!plan.import.errors.empty()) return std::unexpected(errors_text(plan.import.errors));
  return plan;
}

std::expected<Project, std::string> create_project_from_sie(SieProjectPlan const& plan, std::string_view today) {
  std::error_code ec;
  if (!fs::create_directory(plan.folder, ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", plan.folder.string(),
                                       ec ? ec.message() : std::string("mappen finns redan")));
  }
  if (!fs::create_directory(plan.folder / ".cratchit", ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", (plan.folder / ".cratchit").string(), ec.message()));
  }
  Project project{plan.folder, plan.folder / ".cratchit" / "project.txt", new_uuid(), plan.name};
  PathValues entries{{"created", std::string(today)}, {"uuid", project.uuid}, {"name", plan.name}};
  if (!plan.orgnr.empty()) entries.push_back({"orgnr", plan.orgnr});
  if (auto written = write_path_values(project.file, entries); !written) return std::unexpected(written.error());
  if (auto written = books::write_books(plan.folder, plan.import.books); !written) return std::unexpected(written.error());
  if (auto moved = move_into(plan.sie_file, plan.folder); !moved) return std::unexpected(moved.error());
  return project;
}

} // namespace cratchit

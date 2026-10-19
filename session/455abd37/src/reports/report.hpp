#pragma once

// Every report cratchit produces for an authority is a ReportGenerator.
// VAT return is implemented. Bokslut, Årsredovisning and INK2 are registered as
// planned, so adding one means implementing generate() and nothing else.

#include "app/settings.hpp"
#include "core/books.hpp"

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::reports {

struct OutputFile {
  std::string m_name;    // file name, no directory
  std::string m_content; // bytes, already in the target encoding
};

struct ReportRequest {
  std::vector<Books> const& m_books;
  std::vector<std::filesystem::path> const& m_input_files; // for the provenance record
  std::string m_period;                                    // meaning depends on the report, e.g. "202409"
  Settings const& m_settings;
  std::filesystem::path m_specs_dir;
  std::string m_note;
};

class ReportGenerator {
public:
  virtual ~ReportGenerator() = default;
  virtual std::string_view id() const = 0;           // command line name, e.g. "vat"
  virtual std::string_view interface_id() const = 0; // id in specs/interfaces.cspec
  virtual bool implemented() const = 0;
  virtual std::expected<std::vector<OutputFile>, std::string> generate(ReportRequest const& request) const = 0;
};

std::vector<std::unique_ptr<ReportGenerator>> make_report_generators();

// Defined in vat_return_report.cpp and planned_reports.cpp
std::unique_ptr<ReportGenerator> make_vat_return_report();
std::vector<std::unique_ptr<ReportGenerator>> make_planned_reports();

} // namespace cratchit::reports

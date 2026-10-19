// Reports registered now, implemented in later milestones.
// Each one names its interface in specs/interfaces.cspec, where its specs are listed.

#include "reports/report.hpp"

namespace cratchit::reports {

namespace {

class PlannedReport final : public ReportGenerator {
public:
  PlannedReport(std::string_view id, std::string_view interface_id, std::string_view milestone)
      : m_id{id}, m_interface_id{interface_id}, m_milestone{milestone} {}

  std::string_view id() const override { return m_id; }
  std::string_view interface_id() const override { return m_interface_id; }
  bool implemented() const override { return false; }

  std::expected<std::vector<OutputFile>, std::string> generate(ReportRequest const&) const override {
    return std::unexpected("report '" + std::string{m_id} + "' is planned for milestone " + std::string{m_milestone} +
                           ", see interface " + std::string{m_interface_id} + " in specs/interfaces.cspec");
  }

private:
  std::string_view m_id;
  std::string_view m_interface_id;
  std::string_view m_milestone;
};

} // namespace

std::vector<std::unique_ptr<ReportGenerator>> make_planned_reports() {
  std::vector<std::unique_ptr<ReportGenerator>> reports;
  reports.push_back(std::make_unique<PlannedReport>("bokslut", "year_end_closing", "M4"));
  reports.push_back(std::make_unique<PlannedReport>("arsredovisning", "annual_report_ixbrl", "M5"));
  reports.push_back(std::make_unique<PlannedReport>("ink2", "ink2_sru", "M6"));
  return reports;
}

std::vector<std::unique_ptr<ReportGenerator>> make_report_generators() {
  std::vector<std::unique_ptr<ReportGenerator>> reports;
  reports.push_back(make_vat_return_report());
  for (auto& report : make_planned_reports()) reports.push_back(std::move(report));
  return reports;
}

} // namespace cratchit::reports

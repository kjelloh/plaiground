#include "reports/provenance.hpp"
#include "reports/report.hpp"
#include "vat/eskd_writer.hpp"

namespace cratchit::reports {

namespace {

class VatReturnReport final : public ReportGenerator {
public:
  std::string_view id() const override { return "vat"; }
  std::string_view interface_id() const override { return "vat_return_eskd"; }
  bool implemented() const override { return true; }

  std::expected<std::vector<OutputFile>, std::string> generate(ReportRequest const& request) const override {
    auto const vat_spec = vat::load_vat_spec(request.m_specs_dir);
    if (!vat_spec) return std::unexpected(vat_spec.error());
    auto const eskd_spec = vat::load_eskd_spec(request.m_specs_dir);
    if (!eskd_spec) return std::unexpected(eskd_spec.error());
    auto const period = vat::VatPeriod::parse(request.m_period, request.m_settings.get("vat.period_length"));
    if (!period) return std::unexpected(period.error());

    auto vat_return = vat::compute_vat_return(request.m_books, *period, request.m_settings.get("vat.settlement_series"), *vat_spec);
    if (!vat_return) return std::unexpected(vat_return.error());
    vat_return->m_note = request.m_note;
    auto const eskd = vat::to_eskd(*vat_return, *eskd_spec, *vat_spec);
    if (!eskd) return std::unexpected(eskd.error());

    auto const name = "moms_" + period->to_string() + ".eskd";
    Provenance provenance{"vat_return_eskd", name, request.m_input_files, vat_spec->m_used_specs, vat_return->m_vouchers_used, {}};
    provenance.m_specs.push_back(eskd_spec->m_version_info);
    auto const range = period->range();
    for (auto const& books : request.m_books) {
      for (auto const& voucher : books.m_vouchers) {
        if (!range.contains(voucher.m_date)) continue;
        auto findings = legal::check_voucher(voucher);
        provenance.m_findings.insert(provenance.m_findings.end(), findings.begin(), findings.end());
      }
    }
    return std::vector<OutputFile>{{name, *eskd}, {name + ".provenance.cspec", to_cspec(provenance)}};
  }
};

} // namespace

std::unique_ptr<ReportGenerator> make_vat_return_report() { return std::make_unique<VatReturnReport>(); }

} // namespace cratchit::reports

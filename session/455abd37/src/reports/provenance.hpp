#pragma once

// Provenance record written next to every generated file (mechanism output_provenance,
// requirement BFL-5-11 behandlingshistorik). It is a cspec file.

#include "legal/compliance.hpp"
#include "vat/vat_spec.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace cratchit::reports {

struct Provenance {
  std::string m_report;
  std::string m_output;
  std::vector<std::filesystem::path> m_inputs;
  std::vector<vat::SpecVersion> m_specs;
  std::vector<std::string> m_vouchers;
  std::vector<legal::Finding> m_findings;
};

// FNV-1a 64 of the file, as hex. (sha256 is planned.)
std::string file_fingerprint(std::filesystem::path const& path);

std::string to_cspec(Provenance const& provenance);

} // namespace cratchit::reports

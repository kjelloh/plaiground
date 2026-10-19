#pragma once

// VAT return -> eSKDUpload 6.0 file (interface vat_return_eskd), layout from specs/vat/eskd_6p0.cspec

#include "vat/vat_calculation.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cratchit::vat {

struct EskdSpec {
  std::string m_xml_declaration;
  std::string m_doctype;
  bool m_include_doctype{};
  bool m_body_on_one_line{};
  bool m_final_newline{};
  std::string m_root_element;
  std::string m_version;
  std::string m_org_nr_element;
  std::string m_moms_element;
  std::string m_period_element;
  std::vector<std::string> m_order; // box numbers ("05") and "note"
  std::string m_note_element;
  std::size_t m_note_max_length{};
  SpecVersion m_version_info;
};

std::expected<EskdSpec, std::string> load_eskd_spec(std::filesystem::path const& specs_dir);

// The file content in ISO-8859-1.
std::expected<std::string, std::string> to_eskd(VatReturn const& vat_return, EskdSpec const& eskd, VatSpec const& vat);

} // namespace cratchit::vat

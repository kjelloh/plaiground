#pragma once

// VAT boxes, account mapping and calculation rules, loaded from
// specs/vat/vat_boxes.cspec, specs/vat/bas_to_vat_box.cspec and specs/vat/vat_rules.cspec

#include "core/account.hpp"
#include "cspec/spec_tree.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace cratchit::vat {

struct BoxDef {
  int m_number{};
  std::string m_element;
  std::string m_title;
  std::string m_kind; // base | output_vat | input_vat | result
};

struct BoxMapping {
  int m_box{};
  AccountList m_accounts;
  bool m_credit{}; // credit: box = -(sum of postings)
};

struct SpecVersion {
  std::string m_id;
  std::string m_version;
  std::string m_status;
};

struct VatSpec {
  std::vector<BoxDef> m_boxes;
  std::vector<BoxMapping> m_mappings;
  AccountList m_vat_accounts;
  AccountList m_unmapped_allowed;
  bool m_truncate{};
  bool m_exclude_settlement{};
  std::vector<SpecVersion> m_used_specs; // for provenance records

  BoxDef const* find_box(int number) const;
};

std::expected<VatSpec, std::string> load_vat_spec(std::filesystem::path const& specs_dir);

SpecVersion spec_version(cspec::SpecTree const& tree);

} // namespace cratchit::vat

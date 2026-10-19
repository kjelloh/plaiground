#pragma once

// Reads the boxes back from a filed eSKD file, to reconcile cratchit's VAT return
// with what was actually filed. Element names come from specs/vat/vat_boxes.cspec.

#include "vat/vat_calculation.hpp"

#include <expected>
#include <map>
#include <string>
#include <string_view>

namespace cratchit::vat {

struct FiledVatReturn {
  std::string m_org_nr;
  std::string m_period;           // YYYYMM
  std::map<int, Amount> m_boxes;  // all boxes in the file, zeros included
};

std::expected<FiledVatReturn, std::string> read_eskd(std::string_view content, VatSpec const& spec);

// Same values, treating a box that is written as 0 like a box that is left out.
bool same_values(std::map<int, Amount> const& a, std::map<int, Amount> const& b);

} // namespace cratchit::vat

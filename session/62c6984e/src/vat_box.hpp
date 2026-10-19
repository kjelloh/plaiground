#pragma once

#include <span>
#include <string_view>

namespace cratchit {

// Box (ruta) number on the Swedish VAT return, e.g. 48 = MomsIngAvdr.
using VatBoxNumber = int;

struct VatBoxInfo {
  VatBoxNumber m_number;
  std::string_view m_element;  // XML element name in the eskd file
  std::string_view m_description;
};

// All boxes, in the element order of the eskd file (SKV's "all fields" example, eSKDUpload 6.0).
std::span<VatBoxInfo const> vat_boxes();

// The box with this number, or nullptr if there is no such box.
VatBoxInfo const* find_vat_box(VatBoxNumber number);

// The box with this eskd element name, or nullptr if there is no such box.
VatBoxInfo const* find_vat_box(std::string_view element);

} // namespace cratchit

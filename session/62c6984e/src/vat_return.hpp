#pragma once

#include "amount.hpp"
#include "vat_box.hpp"
#include "vat_period.hpp"

#include <map>
#include <string>

namespace cratchit {

// A VAT return (momsdeklaration) as sent to SKV.
// Only boxes with a value are present. Amounts are whole kronor when sent to SKV.
struct VatReturn {
  std::string m_org_nr;  // NNNNNN-NNNN
  VatPeriod m_period{};
  std::map<VatBoxNumber, Amount> m_boxes;
  std::string m_note;  // TextUpplysningMoms, optional, max 300 characters

  friend bool operator==(VatReturn const&, VatReturn const&) = default;
};

} // namespace cratchit

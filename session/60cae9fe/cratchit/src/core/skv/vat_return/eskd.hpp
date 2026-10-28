#pragma once

#include "core/amount.hpp"
#include "core/skv/vat_return/document.hpp"

#include <string>
#include <string_view>
#include <vector>

// The VAT return as SKV's eSKD file (eSKDUpload 6.0) gives it, in memory: the
// tags it fills in, in file order. Writing the file comes later.
namespace cratchit::skv::vat_return::eskd {

// One element of the file, e.g. <MomsUtgHog>250</MomsUtgHog>
struct Field {
  std::string tag;
  Amount amount = 0; // the box amount as on the form, whole kronor

  bool operator==(Field const&) const = default;
};

struct Document {
  std::string orgnr;         // as in the file, e.g. 556000-0175
  std::string period;        // year and last month of the period, e.g. 202603
  std::vector<Field> fields; // in file order

  bool operator==(Document const&) const = default;
};

// The eSKD content of a VAT return: each box with postings, its amount as it
// is, in the tag of the built-in table, then MomsBetala (box 49, see vat_to_pay)
// always. A box without a tag is left out and logged as a design insufficiency.
Document document_of(vat_return::Document const& form, std::string_view orgnr);

} // namespace cratchit::skv::vat_return::eskd

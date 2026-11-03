#pragma once

#include "core/date.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/mapping.hpp"

#include <string>
#include <vector>

namespace cratchit::ink2 {

// What the form is of
struct Header {
  std::string company;
  std::string orgnr;
  Date start; // fiscal year 0
  Date end;
  std::string mapping; // which mapping, e.g. its file
  bool preliminary = false; // the books are not closed
};

// INK2R as text: the header with the fiscal year as fields 7011 and 7012, then for
// each line of the mapping its number, text, SRU code(s) and amount(s) in whole kronor
// (blank when no account is in the field), then the sums.
std::string format_form(Header const& header, std::vector<Line> const& lines, Form const& form);

// Whole kronor grouped by thousands with spaces, e.g. -1 234 567
std::string format_kronor(Kronor kronor);

} // namespace cratchit::ink2

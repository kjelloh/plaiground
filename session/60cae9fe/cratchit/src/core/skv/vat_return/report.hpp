#pragma once

#include "core/books/books.hpp"
#include "core/skv/vat_return/document.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace cratchit::skv::vat_return {

// A quarter's name, e.g. 2026 Q3 for July - September 2026
std::string quarter_name(Period period);

// Report lines of a VAT return, headed by the period and the company: each box
// with its amount, each of its accounts with its sum and name, and the voucher
// postings behind it. Amounts keep their sign as in the books.
std::vector<std::string> report_of(books::Books const& books, Document const& document, std::string_view company);

} // namespace cratchit::skv::vat_return

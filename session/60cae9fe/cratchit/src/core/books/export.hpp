#pragma once

#include "core/books/books.hpp"
#include "core/books/import.hpp"
#include "core/date.hpp"
#include "core/sie/document.hpp"

#include <cstddef>

// Exporting a fiscal year of the books as an SIE 4E file: the inverse of the import
namespace cratchit::books {

// The SIE 4E document of the fiscal year at index (into books.years) as year 0,
// and of the year before in the books as -1, as Fortnox exports them:
// identification (cratchit as the program, generated on the given day, the
// project's name and orgnr, the books' company id, address and chart type,
// #OMFATTN the year's end), every account, dimension and object, the balances
// cratchit calculates (see balances): #IB and #UB of each balance account where
// either is not zero, #RES of each other account (result accounts, and the
// 9xxx memo accounts BAS gives no kind) where it is not zero, then
// the year's vouchers (year 0 only) with their postings as #TRANS, #RTRANS
// (added) and #BTRANS (removed). Waiting events are not exported; nor are
// object or period balances, which the books do not keep.
sie::Document export_document(Books const& books, Identity const& identity, std::size_t index, Date generated);

} // namespace cratchit::books

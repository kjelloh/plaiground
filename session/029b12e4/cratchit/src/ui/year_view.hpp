#pragma once

#include "core/books/books.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ui {

// The rows a fiscal year shows at most, for now
inline constexpr std::size_t year_view_rows = 10;

// What a fiscal year's view shows of its current zoom. For now the zoom is
// always the whole year.
struct YearView {
  // What the zoom holds, e.g. "Hela året: 37 verifikationer, 1 utkast" or
  // "Hela året: inget bokfört"
  std::string summary;
  // The zoom's vouchers and drafts, newest first (by date; on the same date a
  // draft before the vouchers, and later ones before earlier ones), at most
  // max_rows of them, then "… 27 äldre" when older ones are left out. A
  // voucher shows its series and number, a draft its stage (see stage_text).
  // Not options: just a view.
  std::vector<std::string> rows;
};

// The view of year; nullptr is a year with nothing in the books yet
YearView year_view(books::FiscalYear const* year, std::size_t max_rows = year_view_rows);

// The stage of a draft for the user: "händelse", "kontoutdrag", "obalanserat" or "klart"
std::string_view stage_text(books::DraftStage stage);

} // namespace cratchit::ui

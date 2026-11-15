#include "ui/year_view.hpp"

#include "core/amount.hpp"
#include "core/date.hpp"

#include <algorithm>
#include <format>
#include <ranges>

namespace cratchit::ui {

namespace {

struct Row {
  Date date;
  std::string text;
};

std::string row_text(std::string_view first, Date date, Amount amount, std::string_view heading) {
  return std::format("  {:<11} {}  {:>12}  {}", first, format_iso_date(date), format_amount(amount), heading);
}

// The amount a draft shows: its event's, else what its postings move
Amount shown_amount(books::Draft const& draft) {
  if (draft.event.amount != 0) return draft.event.amount;
  return std::max(books::debit(draft.postings), books::credit(draft.postings));
}

std::string summary_of(books::FiscalYear const* year) {
  auto const vouchers = year != nullptr ? year->vouchers.size() : 0;
  auto const drafts = year != nullptr ? year->drafts.size() : 0;
  auto text = std::string("Hela året: ");
  if (vouchers == 0) {
    text += "inget bokfört";
  } else {
    text += std::format("{} {}", vouchers, vouchers == 1 ? "verifikation" : "verifikationer");
  }
  if (drafts > 0) text += std::format(", {} utkast", drafts);
  return text;
}

} // namespace

std::string_view stage_text(books::DraftStage stage) {
  switch (stage) {
  case books::DraftStage::event:
    return "händelse";
  case books::DraftStage::statement:
    return "kontoutdrag";
  case books::DraftStage::unbalanced:
    return "obalanserat";
  case books::DraftStage::ready:
    return "klart";
  }
  return "";
}

YearView year_view(books::FiscalYear const* year, std::size_t max_rows) {
  YearView view{summary_of(year), {}};
  if (year == nullptr) return view;

  // Oldest first: by date, vouchers in the order of the books, then the drafts of the same date
  std::vector<Row> rows;
  for (auto const& voucher : year->vouchers) {
    rows.push_back({voucher.event.date, row_text(std::format("{} {}", voucher.series, voucher.number),
                                                 voucher.event.date, voucher.event.amount, voucher.event.heading)});
  }
  for (auto const& draft : year->drafts) {
    rows.push_back({draft.event.date, row_text(stage_text(stage(draft)), draft.event.date, shown_amount(draft),
                                               draft.event.heading)});
  }
  std::ranges::stable_sort(rows, {}, &Row::date);

  auto const shown = std::min(rows.size(), max_rows);
  for (auto const& row : rows | std::views::reverse | std::views::take(shown)) {
    view.rows.push_back(row.text);
  }
  if (rows.size() > shown) view.rows.push_back(std::format("  … {} äldre", rows.size() - shown));
  return view;
}

} // namespace cratchit::ui

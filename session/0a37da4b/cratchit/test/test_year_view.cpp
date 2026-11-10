#include "ui/year_view.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace std::chrono;
using cratchit::books::Draft;
using cratchit::books::FiscalYear;
using cratchit::books::Posting;
using cratchit::books::Voucher;
using cratchit::ui::year_view;
using Rows = std::vector<std::string>;

namespace {

cratchit::Date date(int y, unsigned m, unsigned d) {
  return year_month_day(year{y}, month{m}, day{d});
}

Posting row(std::string account, cratchit::Amount amount) {
  return {Posting::Kind::normal, std::move(account), {}, amount, {}, "", "", ""};
}

Voucher voucher(std::string number, cratchit::Date when, std::string heading, cratchit::Amount amount) {
  return {"A", std::move(number), {std::move(heading), amount, when}, when, "", {row("1930", amount), row("3010", -amount)}};
}

FiscalYear year_with(std::vector<Voucher> vouchers) {
  FiscalYear year{date(2026, 5, 1), date(2027, 4, 30), {}, std::move(vouchers)};
  return year;
}

} // namespace

TEST(YearView, AYearNotInTheBooksHasNothingBooked) {
  auto const view = year_view(nullptr);
  EXPECT_EQ(view.summary, "Hela året: inget bokfört");
  EXPECT_TRUE(view.rows.empty());
}

TEST(YearView, TheSummaryCountsVouchersAndDrafts) {
  auto year = year_with({voucher("1", date(2026, 6, 1), "Sale", 10000)});
  EXPECT_EQ(year_view(&year).summary, "Hela året: 1 verifikation");
  year.vouchers.push_back(voucher("2", date(2026, 6, 2), "Sale", 10000));
  EXPECT_EQ(year_view(&year).summary, "Hela året: 2 verifikationer");
  year.place_draft({{"Telia", 111800, date(2026, 10, 20)}, date(2026, 10, 23)});
  EXPECT_EQ(year_view(&year).summary, "Hela året: 2 verifikationer, 1 utkast");
  year.vouchers.clear();
  EXPECT_EQ(year_view(&year).summary, "Hela året: inget bokfört, 1 utkast");
}

TEST(YearView, RowsAreNewestFirst) {
  // In the books out of date order, as an SIE file may have them
  auto const year = year_with({voucher("1", date(2026, 6, 1), "Första", 10000),
                               voucher("3", date(2026, 9, 3), "Kontorsmaterial", 41250),
                               voucher("2", date(2026, 6, 1), "Andra", 20000)});
  EXPECT_EQ(year_view(&year).rows, (Rows{"  A 3         2026-09-03        412.50  Kontorsmaterial",
                                         "  A 2         2026-06-01        200.00  Andra",
                                         "  A 1         2026-06-01        100.00  Första"}));
}

TEST(YearView, DraftsShowTheirStageAndComeBeforeVouchersOfTheSameDate) {
  auto year = year_with({voucher("1", date(2026, 10, 20), "Sale", 10000)});
  year.place_draft({{"Telia oktober", 111800, date(2026, 10, 20)}, date(2026, 10, 23)});
  year.place_draft({{"", 0, date(2026, 10, 21)}, date(2026, 10, 23), {row("1930", -111800)}});
  year.place_draft({{"Loopia", 130875, date(2026, 9, 28)}, date(2026, 10, 23),
                    {row("6540", 130875), row("1930", -130875)}});
  EXPECT_EQ(year_view(&year).rows, (Rows{"  kontoutdrag 2026-10-21       1118.00  ",
                                         "  händelse    2026-10-20       1118.00  Telia oktober",
                                         "  A 1         2026-10-20        100.00  Sale",
                                         "  klart       2026-09-28       1308.75  Loopia"}));
}

TEST(YearView, AtMostMaxRowsAreShownThenHowManyOlder) {
  std::vector<Voucher> vouchers;
  for (unsigned day = 1; day <= 12; ++day) {
    vouchers.push_back(voucher(std::to_string(day), date(2026, 6, day), "Sale", 100));
  }
  auto const year = year_with(std::move(vouchers));
  auto const rows = year_view(&year).rows;
  ASSERT_EQ(rows.size(), 11u);
  EXPECT_TRUE(rows[0].starts_with("  A 12 ")) << rows[0];
  EXPECT_TRUE(rows[9].starts_with("  A 3 ")) << rows[9];
  EXPECT_EQ(rows[10], "  … 2 äldre");
  EXPECT_EQ(year_view(&year, 12).rows.size(), 12u); // all shown: no row about older ones
}

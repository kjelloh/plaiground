#include "core/skv/vat_return/account_boxes.hpp"
#include "core/skv/vat_return/box.hpp"
#include "core/skv/vat_return/document.hpp"
#include "core/skv/vat_return/report.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

using namespace cratchit;
using namespace cratchit::skv::vat_return;
using namespace std::chrono;

namespace {

books::Posting posting(std::string account, Amount amount, books::Posting::Kind kind = books::Posting::Kind::normal) {
  return {.kind = kind, .account = std::move(account), .amount = amount};
}

books::Voucher voucher(std::string series, std::string number, Date date, books::Postings postings) {
  return {.series = std::move(series),
          .number = std::move(number),
          .event = {"Event", 0, date},
          .postings = std::move(postings)};
}

// Fiscal years 2025-05-01 - 2026-04-30 and 2026-05-01 - 2027-04-30, as a
// company whose year ends in April: the second quarter of 2026 spans both
books::Books two_years() {
  books::Books books;
  books.years.push_back({.start = 2025y / May / 1, .end = 2026y / April / 30});
  books.years.push_back({.start = 2026y / May / 1, .end = 2027y / April / 30});
  auto& first = books.years[0].vouchers;
  first.push_back(voucher("B", "1", 2026y / March / 31, {posting("1510", 12500), posting("3001", -10000),
                                                         posting("2611", -2500)}));
  first.push_back(voucher("B", "2", 2026y / April / 1, {posting("1510", 6250), posting("3001", -5000),
                                                        posting("2611", -1250)}));
  first.push_back(voucher("D", "1", 2026y / April / 30, {posting("2440", -1250), posting("6540", 1000),
                                                         posting("2641", 250)}));
  auto& second = books.years[1].vouchers;
  second.push_back(voucher("B", "1", 2026y / May / 2, {posting("1510", 2500), posting("3002", -2000),
                                                       posting("2611", -500, books::Posting::Kind::added),
                                                       posting("2621", -500, books::Posting::Kind::removed)}));
  second.push_back(voucher("D", "1", 2026y / July / 1, {posting("2440", -1250), posting("6540", 1000),
                                                        posting("2641", 250)}));
  return books;
}

} // namespace

TEST(VatReturn, TheBuiltInBoxesOfAccountsRead) {
  auto const boxes = account_boxes(account_boxes_text());
  ASSERT_TRUE(boxes.has_value()) << boxes.error();
  EXPECT_GT(boxes->size(), 100u);
}

TEST(VatReturn, BoxOfAnAccount) {
  EXPECT_EQ(box_of("3001"), "05");
  EXPECT_EQ(box_of("2611"), "10");
  EXPECT_EQ(box_of("2641"), "48");
  EXPECT_EQ(box_of("3305"), "40");
  EXPECT_EQ(box_of("1930"), std::nullopt);
  EXPECT_EQ(box_of(""), std::nullopt);
}

TEST(VatReturn, AccountBoxesText) {
  auto const boxes = account_boxes("\xEF\xBB\xBF" "3001;05;MP1\r\n\r\n2611;10\n2641;48;I");
  ASSERT_TRUE(boxes.has_value()) << boxes.error();
  EXPECT_EQ(*boxes, (std::map<std::string, Box>{{"2611", "10"}, {"2641", "48"}, {"3001", "05"}}));
}

TEST(VatReturn, AccountBoxesErrorsNameTheLine) {
  EXPECT_EQ(account_boxes("3001;05;MP1\n301;05;MP1\n").error(), "Line 2: the account is not four digits: 301;05;MP1");
  EXPECT_EQ(account_boxes("3001;5;MP1\n").error(), "Line 1: the box is not two digits: 3001;5;MP1");
  EXPECT_EQ(account_boxes("3001\n").error(), "Line 1: the account is not four digits: 3001");
  EXPECT_EQ(account_boxes("3001;05;MP1\n\n3001;06;X\n").error(), "Line 3: account 3001 is listed before");
}

TEST(VatReturn, CalendarQuarters) {
  EXPECT_EQ(quarter_of(2026y, 1), (Period{2026y / January / 1, 2026y / March / 31}));
  EXPECT_EQ(quarter_of(2026y, 2), (Period{2026y / April / 1, 2026y / June / 30}));
  EXPECT_EQ(quarter_of(2026y, 3), (Period{2026y / July / 1, 2026y / September / 30}));
  EXPECT_EQ(quarter_of(2026y, 4), (Period{2026y / October / 1, 2026y / December / 31}));
  EXPECT_EQ(quarter_of(2026y, 0), std::nullopt);
  EXPECT_EQ(quarter_of(2026y, 5), std::nullopt);
}

TEST(VatReturn, QuartersEndingInAFiscalYear) {
  EXPECT_EQ(quarters_ending_in(2026y / January / 1, 2026y / December / 31),
            (std::vector<Period>{*quarter_of(2026y, 1), *quarter_of(2026y, 2), *quarter_of(2026y, 3),
                                 *quarter_of(2026y, 4)}));
  // A year ending in April holds the quarter ending in June, but not the one it ends in
  EXPECT_EQ(quarters_ending_in(2026y / May / 1, 2027y / April / 30),
            (std::vector<Period>{*quarter_of(2026y, 2), *quarter_of(2026y, 3), *quarter_of(2026y, 4),
                                 *quarter_of(2027y, 1)}));
  EXPECT_TRUE(quarters_ending_in(2026y / April / 1, 2026y / May / 31).empty());
}

TEST(VatReturn, QuarterName) {
  EXPECT_EQ(quarter_name(*quarter_of(2026y, 1)), "2026 Q1");
  EXPECT_EQ(quarter_name(*quarter_of(2026y, 3)), "2026 Q3");
}

TEST(VatReturn, AQuarterAcrossTwoFiscalYears) {
  auto const period = *quarter_of(2026y, 2);
  auto const document = document_of(two_years(), period);
  EXPECT_EQ(document.period, period);
  EXPECT_EQ(document.boxes,
            (std::vector<BoxAmount>{
                {"05", -7000,
                 {{"3001", -5000, {{2025y / May / 1, "B", "2", 2026y / April / 1, -5000}}},
                  {"3002", -2000, {{2026y / May / 1, "B", "1", 2026y / May / 2, -2000}}}}},
                {"10", -1750,
                 {{"2611", -1750,
                   {{2025y / May / 1, "B", "2", 2026y / April / 1, -1250},
                    {2026y / May / 1, "B", "1", 2026y / May / 2, -500}}}}},
                {"48", 250, {{"2641", 250, {{2025y / May / 1, "D", "1", 2026y / April / 30, 250}}}}},
            }));
}

TEST(VatReturn, NoVouchersInThePeriodGiveNoBoxes) {
  auto const document = document_of(two_years(), *quarter_of(2025y, 4));
  EXPECT_TRUE(document.boxes.empty());
}

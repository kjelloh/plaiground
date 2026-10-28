#include "core/skv/vat_return/account_boxes.hpp"
#include "core/skv/vat_return/box.hpp"
#include "core/skv/vat_return/document.hpp"
#include "core/skv/vat_return/report.hpp"
#include "core/skv/vat_return/steps.hpp"

#include <gtest/gtest.h>

#include <algorithm>
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
  EXPECT_EQ(box_of("2650"), "49");
  EXPECT_EQ(box_of("1930"), std::nullopt);
  EXPECT_EQ(box_of(""), std::nullopt);
}

TEST(VatReturn, AccountBoxesText) {
  auto const boxes = account_boxes("\xEF\xBB\xBF" "3001;05;MP1\r\n\r\n2611;10\n2641;48;I");
  ASSERT_TRUE(boxes.has_value()) << boxes.error();
  EXPECT_EQ(*boxes, (std::map<std::string, Box>{{"2611", "10"}, {"2641", "48"}, {"3001", "05"}}));
}

TEST(VatReturn, AccountBoxesTextWithHeadingAndOutColumn) {
  auto const boxes = account_boxes("in;vat;out;hint\r\n3001;05;;MP1\r\n;49;2650;R2\r\n");
  ASSERT_TRUE(boxes.has_value()) << boxes.error();
  EXPECT_EQ(*boxes, (std::map<std::string, Box>{{"2650", "49"}, {"3001", "05"}}));
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

TEST(VatReturn, QuarterHoldingADate) {
  EXPECT_EQ(quarter_holding(2026y / January / 1), *quarter_of(2026y, 1));
  EXPECT_EQ(quarter_holding(2026y / June / 30), *quarter_of(2026y, 2));
  EXPECT_EQ(quarter_holding(2026y / October / 27), *quarter_of(2026y, 4));
  EXPECT_EQ(quarter_holding(2026y / December / 31), *quarter_of(2026y, 4));
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

namespace {

// A year of quarterly VAT: Q1 to pay 250 (cleared M 1 with rounding, charged in May), Q2 to get back 120
// (cleared M 2 to 1650, credited in August), Q3 not cleared; a payment to the tax account and a charge that
// matches nothing
books::Books steps_year() {
  books::Books books;
  books.years.push_back({.start = 2026y / January / 1, .end = 2026y / December / 31});
  auto& v = books.years[0].vouchers;
  v.push_back(voucher("B", "1", 2026y / February / 10, {posting("1510", 125040), posting("3001", -100032),
                                                        posting("2611", -25008)}));
  v.push_back(voucher("M", "1", 2026y / March / 31, {posting("2611", 25008), posting("2650", -25000),
                                                     posting("3740", -8)}));
  v.push_back(voucher("D", "1", 2026y / April / 3, {posting("2440", -60000), posting("4010", 48000),
                                                    posting("2641", 12000)}));
  v.push_back(voucher("A", "1", 2026y / May / 12, {posting("2650", 25000), posting("1630", -25000)}));
  v.push_back(voucher("M", "2", 2026y / June / 30, {posting("2641", -12000), posting("1650", 12000)}));
  v.push_back(voucher("A", "2", 2026y / July / 3, {posting("1630", 50000), posting("1930", -50000)}));
  v.push_back(voucher("A", "3", 2026y / August / 12, {posting("1630", 12000), posting("1650", -12000)}));
  v.push_back(voucher("A", "4", 2026y / August / 13, {posting("2650", 999), posting("1630", -999)}));
  v.push_back(voucher("B", "2", 2026y / September / 1, {posting("1510", 5000), posting("3001", -4000),
                                                        posting("2611", -1000),
                                                        posting("2650", -1000, books::Posting::Kind::removed)}));
  return books;
}

} // namespace

TEST(VatReturn, ClearingAccountsAndVatAccounts) {
  EXPECT_TRUE(is_vat_clearing_account(vat_liability_account));
  EXPECT_TRUE(is_vat_clearing_account(vat_receivable_account));
  EXPECT_FALSE(is_vat_clearing_account("2611"));
  EXPECT_TRUE(is_vat_account("2611"));
  EXPECT_TRUE(is_vat_account("2641"));
  EXPECT_FALSE(is_vat_account("2650"));
  EXPECT_FALSE(is_vat_account("3001"));
}

TEST(VatReturn, StepsOfQuarters) {
  auto const steps = steps_of(steps_year(), quarters_ending_in(2026y / January / 1, 2026y / December / 31));
  ASSERT_EQ(steps.periods.size(), 4u);
  auto const& q1 = steps.periods[0];
  EXPECT_EQ(q1.period_vat, -25008);
  EXPECT_EQ(q1.cleared, -25000);
  EXPECT_EQ(q1.clearings, (std::vector<VoucherPosting>{{2026y / January / 1, "M", "1", 2026y / March / 31, -25000}}));
  EXPECT_EQ(q1.charged, -25000);
  EXPECT_EQ(q1.charges, (std::vector<VoucherPosting>{{2026y / January / 1, "A", "1", 2026y / May / 12, -25000}}));
  auto const& q2 = steps.periods[1];
  EXPECT_EQ(q2.period_vat, 12000);
  EXPECT_EQ(q2.cleared, 12000);
  EXPECT_EQ(q2.charged, 12000);
  ASSERT_EQ(q2.charges.size(), 1u);
  EXPECT_EQ(q2.charges[0].number, "3");
  // A removed posting on 2650 does not make B 2 a clearing voucher
  auto const& q3 = steps.periods[2];
  EXPECT_EQ(q3.period_vat, -1000);
  EXPECT_EQ(q3.cleared, 0);
  EXPECT_TRUE(q3.clearings.empty());
  EXPECT_EQ(q3.charged, 0);
  EXPECT_EQ(steps.periods[3], (PeriodSteps{*quarter_of(2026y, 4), 0, 0, {}, 0, {}}));
  // A 2 pays into the tax account and is no step; A 4 matches no cleared quarter
  EXPECT_EQ(steps.unmatched_charges,
            (std::vector<VoucherPosting>{{2026y / January / 1, "A", "4", 2026y / August / 13, -999}}));
}

TEST(VatReturn, AChargeGoesToTheOldestQuarterClearedByItsAmountAndOnlyOnce) {
  books::Books books;
  books.years.push_back({.start = 2026y / January / 1, .end = 2026y / December / 31});
  auto& v = books.years[0].vouchers;
  v.push_back(voucher("B", "1", 2026y / February / 1, {posting("1510", 500), posting("2611", -500)}));
  v.push_back(voucher("M", "1", 2026y / March / 31, {posting("2611", 500), posting("2650", -500)}));
  v.push_back(voucher("B", "2", 2026y / May / 1, {posting("1510", 500), posting("2611", -500)}));
  v.push_back(voucher("M", "2", 2026y / June / 30, {posting("2611", 500), posting("2650", -500)}));
  // Charged before the second quarter starts: only the first can take it
  v.push_back(voucher("A", "1", 2026y / March / 31, {posting("2650", 500), posting("1630", -500)}));
  v.push_back(voucher("A", "2", 2026y / August / 12, {posting("2650", 500), posting("1630", -500)}));
  v.push_back(voucher("A", "3", 2026y / November / 12, {posting("2650", 500), posting("1630", -500)}));
  auto const steps = steps_of(books, quarters_ending_in(2026y / January / 1, 2026y / December / 31));
  EXPECT_EQ(steps.periods[0].charges[0].number, "1");
  EXPECT_EQ(steps.periods[1].charges[0].number, "2");
  ASSERT_EQ(steps.unmatched_charges.size(), 1u);
  EXPECT_EQ(steps.unmatched_charges[0].number, "3");
}

TEST(VatReturn, AChargeFindsItsQuarterInTheYearBefore) {
  auto books = two_years();
  // Q1 2026 (in 2025/26) cleared in April, charged in the next fiscal year
  books.years[0].vouchers.push_back(voucher("M", "1", 2026y / March / 31, {posting("2611", 2500), posting("2650", -2500)}));
  books.years[1].vouchers.push_back(voucher("A", "1", 2026y / May / 12, {posting("2650", 2500), posting("1630", -2500)}));
  auto const steps = steps_of(books, quarters_ending_in(2025y / May / 1, 2027y / April / 30));
  auto const q1 = std::ranges::find(steps.periods, *quarter_of(2026y, 1), &PeriodSteps::period);
  ASSERT_NE(q1, steps.periods.end());
  EXPECT_EQ(q1->cleared, -2500);
  EXPECT_EQ(q1->charged, -2500);
  EXPECT_EQ(q1->charges[0].fiscal_year, 2026y / May / 1);
  EXPECT_TRUE(steps.unmatched_charges.empty());
}

TEST(VatReturn, StepsRow) {
  EXPECT_EQ(steps_heading(), "      Period     Cleared     Charged");
  PeriodSteps const steps{*quarter_of(2026y, 1), -25008, -25000,
                          {{2026y / January / 1, "M", "1", 2026y / March / 31, -25000}}, 0, {}};
  EXPECT_EQ(steps_row(steps), "     -250.08     -250.00        0.00  cleared by M 1");
}

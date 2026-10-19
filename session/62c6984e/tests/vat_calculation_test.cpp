#include "vat_calculation.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace std::chrono;
using cratchit::Amount;
using cratchit::calculate_vat_return;
using cratchit::Date;
using cratchit::Posting;
using cratchit::Voucher;

namespace {

std::string const org_nr = "556000-0175";

Amount sek(double value) { return Amount::from_cents(static_cast<std::int64_t>(value * 100 + (value < 0 ? -0.5 : 0.5))); }

Voucher voucher(std::string series, Date date, std::vector<std::pair<int, double>> postings) {
  Voucher result{.m_series = std::move(series), .m_number = "1", .m_date = date};
  for (auto [account, amount] : postings) result.m_postings.push_back(Posting{.m_account = account, .m_amount = sek(amount)});
  return result;
}

// A purchase of 100 + 25 % input VAT, paid from the bank.
Voucher purchase(Date date, double vat) {
  return voucher("A", date, {{5410, vat * 4}, {2641, vat}, {1930, -vat * 5}});
}

} // namespace

TEST(CalculateVatReturn, InputVatOnlyGivesRefund) {
  std::vector const vouchers{purchase(2024y / July / 30d, 100.0), purchase(2024y / September / 30d, 50.5)};
  auto const result = calculate_vat_return(vouchers, org_nr, 2024y / September).value();

  EXPECT_EQ(result.m_org_nr, org_nr);
  EXPECT_EQ(result.m_period, 2024y / September);
  EXPECT_EQ(result.m_boxes, (std::map<int, Amount>{{48, sek(150)}, {49, sek(-150)}}));
}

TEST(CalculateVatReturn, OnlyVouchersDatedInTheQuarterCount) {
  std::vector const vouchers{
      purchase(2024y / June / 30d, 1000),      // before
      purchase(2024y / July / 1d, 1),          // first day
      purchase(2024y / September / 30d, 2),    // last day
      purchase(2024y / October / 1d, 1000),    // after
  };
  auto const result = calculate_vat_return(vouchers, org_nr, 2024y / September).value();
  EXPECT_EQ(result.m_boxes.at(48), sek(3));
}

TEST(CalculateVatReturn, MonthlyPeriod) {
  std::vector const vouchers{purchase(2024y / August / 31d, 10), purchase(2024y / September / 1d, 20)};
  auto const result = calculate_vat_return(vouchers, org_nr, 2024y / September, {.m_period_months = 1}).value();
  EXPECT_EQ(result.m_boxes.at(48), sek(20));
}

TEST(CalculateVatReturn, SettlementVoucherIsLeftOut) {
  std::vector const vouchers{
      purchase(2024y / September / 1d, 1234.56),
      voucher("M", 2024y / September / 30d, {{2641, -1234.56}, {1650, 1234}, {3740, 0.56}}),
  };
  auto const result = calculate_vat_return(vouchers, org_nr, 2024y / September).value();
  EXPECT_EQ(result.m_boxes.at(48), sek(1234));
}

TEST(CalculateVatReturn, CorrectionCountsInItsOwnPeriod) {
  std::vector const vouchers{
      purchase(2024y / May / 10d, 500.40),
      voucher("A", 2024y / May / 12d, {{1650, 200}, {2641, -200}}),
  };
  auto const result = calculate_vat_return(vouchers, org_nr, 2024y / June).value();
  EXPECT_EQ(result.m_boxes.at(48), sek(300));
}

TEST(CalculateVatReturn, OreAreTruncated) {
  std::vector const vouchers{purchase(2024y / September / 1d, 123.99)};
  auto const result = calculate_vat_return(vouchers, org_nr, 2024y / September).value();
  EXPECT_EQ(result.m_boxes.at(48), sek(123));
}

TEST(CalculateVatReturn, OutputVatGivesVatToPay) {
  std::vector const vouchers{
      voucher("B", 2021y / May / 3d, {{1510, 50000}, {3001, -40000}, {2611, -10000}}),
      purchase(2021y / June / 1d, 512.34),
  };
  auto const result = calculate_vat_return(vouchers, org_nr, 2021y / June).value();
  EXPECT_EQ(result.m_boxes,
            (std::map<int, Amount>{{5, sek(40000)}, {10, sek(10000)}, {48, sek(512)}, {49, sek(9488)}}));
}

TEST(CalculateVatReturn, ReverseChargeAndImport) {
  // The box values are truncated first, then box 49 is computed from them:
  // 1000 + 300 - 2001 = -701 (truncating the exact -700.29 instead would give -700).
  std::vector const vouchers{
      voucher("A", 2018y / August / 1d, {{2614, -1000.75}, {2645, 1000.75}}),
      voucher("A", 2018y / August / 2d, {{2615, -300.60}, {1999, 300.60}}),
      voucher("A", 2018y / August / 3d, {{2641, 800.90}, {2640, 199.99}, {1999, -1000.89}}),
  };
  auto const result = calculate_vat_return(vouchers, org_nr, 2018y / September).value();
  EXPECT_EQ(result.m_boxes.at(30), sek(1000));
  EXPECT_EQ(result.m_boxes.at(60), sek(300));
  EXPECT_EQ(result.m_boxes.at(48), sek(2001));
  EXPECT_EQ(result.m_boxes.at(49), sek(-701));
}

TEST(CalculateVatReturn, FortnoxBaseAccounts) {
  std::vector const vouchers{voucher("A", 2019y / June / 1d, {{9021, 4321.50}, {9099, -4321.50}})};
  auto const result = calculate_vat_return(vouchers, org_nr, 2019y / June).value();
  EXPECT_EQ(result.m_boxes.at(20), sek(4321));
}

TEST(CalculateVatReturn, EmptyPeriodHasOnlyBox49) {
  auto const result = calculate_vat_return({}, org_nr, 2024y / September).value();
  EXPECT_EQ(result.m_boxes, (std::map<int, Amount>{{49, Amount{}}}));
}

TEST(CalculateVatReturn, UnmappedVatAccountIsAnError) {
  std::vector const vouchers{voucher("A", 2024y / September / 1d, {{2648, 100}, {1930, -100}})};
  auto const result = calculate_vat_return(vouchers, org_nr, 2024y / September);
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("2648"), std::string::npos) << result.error();
}

TEST(CalculateVatReturn, SettlementAccountsNeedNoMapping) {
  std::vector const vouchers{voucher("A", 2024y / September / 1d, {{1630, 400}, {2650, -400}})};
  EXPECT_TRUE(calculate_vat_return(vouchers, org_nr, 2024y / September).has_value());
}

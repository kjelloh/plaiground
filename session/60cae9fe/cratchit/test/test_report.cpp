#include "core/books/import.hpp"
#include "core/books/report.hpp"

#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using cratchit::books::Books;
using sie_test::balanced_balances;
using sie_test::parse_sie;
using sie_test::sale_and_cost;
using sie_test::sie_file;

namespace {

Books books_2025_2026(std::string_view opening_2026 = "1490.00") {
  auto const first = cratchit::books::plan_import({}, {"Test", ""},
                                                  parse_sie(sie_file(2025, balanced_balances(), sale_and_cost(2025))));
  auto const second = cratchit::books::plan_import(
      first.books, first.identity,
      parse_sie(sie_file(2026, std::format("#IB 0 1930 {}\n#IB 0 2099 -1000\n", opening_2026),
                         "#VER A 1 20260110 \"Sale\"\n{\n#TRANS 1930 {} 100\n#TRANS 3010 {} -100\n}\n"
                         "#VER B 1 20260111 \"Fix\"\n{\n#TRANS 1930 {} 0\n#BTRANS 6570 {} 5 \"\" \"Wrong\" 0 \"Kim\"\n"
                         "#RTRANS 1930 {} -0 \"\" \"Added\" 0 \"Kim\"\n#TRANS 1930 {} -0\n}\n")));
  EXPECT_TRUE(second.errors.empty());
  return second.books;
}

std::string text(std::vector<std::string> const& lines) {
  std::string result;
  for (auto const& line : lines) {
    result += line + "\n";
  }
  return result;
}

} // namespace

TEST(Report, TrialBalanceOfTheFirstYear) {
  auto const books = books_2025_2026();
  EXPECT_EQ(text(cratchit::books::trial_balance(books, 0, "Test AB, 556000-0001")),
            "Trial balance (saldobalans) 2025-01-01 - 2025-12-31\n"
            "Test AB, 556000-0001\n"
            "\n"
            "Account         Opening         Change        Closing  Name\n"
            "1930            1000.00         490.00        1490.00  Bank\n"
            "2099           -1000.00           0.00       -1000.00  Result\n"
            "Total              0.00         490.00         490.00\n"
            "\n"
            "Account          Result  Name\n"
            "3010            -500.00  Sales\n"
            "6570              10.00  Bank costs\n"
            "Total           -490.00\n"
            "\n"
            "Amounts: debit positive, credit negative.\n"
            "Opening balances are as the source reported them (the year before is not in the books).\n");
}

TEST(Report, TrialBalanceListsCarriedOpeningBalancesThatDifferFromTheSource) {
  auto const books = books_2025_2026("1500.00");
  auto const report = text(cratchit::books::trial_balance(books, 1, "Test AB"));
  EXPECT_NE(report.find("1930            1490.00         100.00        1590.00  Bank\n"), std::string::npos) << report;
  EXPECT_NE(report.find("Opening balances are carried from the closing balances 2025-12-31.\n"
                        "  1930: the source reported 1500.00, carried 1490.00 (difference -10.00)\n"),
            std::string::npos)
      << report;
}

TEST(Report, VoucherListMarksAddedAndRemovedRows) {
  auto const books = books_2025_2026();
  EXPECT_EQ(text(cratchit::books::voucher_list(books, 1, "Test AB")),
            "Vouchers 2026-01-01 - 2026-12-31\n"
            "Test AB\n"
            "\n"
            "2 vouchers\n"
            "\n"
            "A 1  2026-01-10  Sale\n"
            "          1930             100.00  Bank\n"
            "          3010            -100.00  Sales\n"
            "\n"
            "B 1  2026-01-11  Fix\n"
            "          1930               0.00  Bank\n"
            "  removed 6570               5.00  Wrong (by Kim)\n"
            "  added   1930               0.00  Added (by Kim)\n");
}

TEST(Report, AccountLedgerWithRunningBalance) {
  auto const books = books_2025_2026();
  auto const ledger = cratchit::books::account_ledger(books, 0, "Test AB", "1930");
  ASSERT_TRUE(ledger.has_value());
  EXPECT_EQ(text(*ledger), "Account 1930 Bank ledger 2025-01-01 - 2025-12-31\n"
                           "Test AB\n"
                           "\n"
                           "Date        Voucher            Amount        Balance  Text\n"
                           "Opening balance                              1000.00\n"
                           "2025-02-01  A 1                500.00        1500.00  Sale\n"
                           "2025-03-01  A 2                -10.00        1490.00  Bank cost\n"
                           "Closing balance                              1490.00\n");
}

TEST(Report, LedgerOfAnUnknownAccountIsNothing) {
  auto const books = books_2025_2026();
  EXPECT_FALSE(cratchit::books::account_ledger(books, 0, "Test AB", "9999").has_value());
  EXPECT_TRUE(cratchit::books::account_ledger(books, 0, "Test AB", "2099").has_value()); // defined, no rows
}

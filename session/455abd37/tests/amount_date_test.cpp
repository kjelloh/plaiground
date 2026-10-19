#include "core/account.hpp"
#include "core/amount.hpp"
#include "core/date.hpp"

#include <gtest/gtest.h>

using namespace cratchit;

TEST(Amount, ParsesSieAndStatementFormats) {
  EXPECT_EQ(Amount::parse("-250.5")->ore(), -25050);
  EXPECT_EQ(Amount::parse("100")->ore(), 10000);
  EXPECT_EQ(Amount::parse("-1158,75", ',')->ore(), -115875);
  EXPECT_EQ(Amount::parse("-1 997", ',', " ")->ore(), -199700);
  EXPECT_EQ(Amount::parse("1\xC2\xA0" "997", ',', " ")->ore(), 199700);
  EXPECT_FALSE(Amount::parse("1.234"));
  EXPECT_FALSE(Amount::parse(""));
  EXPECT_FALSE(Amount::parse("1 997"));
}

TEST(Amount, TruncatesTowardsZero) {
  EXPECT_EQ(Amount::from_ore(12399).truncated_to_krona().ore(), 12300);
  EXPECT_EQ(Amount::from_ore(-12399).truncated_to_krona().ore(), -12300);
  EXPECT_EQ(to_string(Amount::from_ore(-5)), "-0.05");
}

TEST(Date, ParsesFormats) {
  using namespace std::chrono;
  EXPECT_EQ(parse_date("20240715"), 2024y / July / 15d);
  EXPECT_EQ(parse_date("2025/12/29", "YYYY/MM/DD"), 2025y / December / 29d);
  EXPECT_EQ(parse_date("2025-10-04", "YYYY-MM-DD"), 2025y / October / 4d);
  EXPECT_FALSE(parse_date("20240231"));
  EXPECT_FALSE(parse_date("2025-10-04", "YYYY/MM/DD"));
  EXPECT_EQ(to_yyyymmdd(2024y / May / 1d), "20240501");
}

TEST(AccountList, ParsesRangesAndSingles) {
  auto const list = AccountList::parse("2640-2642, 2645-2647");
  ASSERT_TRUE(list);
  EXPECT_TRUE(list->contains(2641));
  EXPECT_TRUE(list->contains(2647));
  EXPECT_FALSE(list->contains(2648));
  EXPECT_FALSE(AccountList::parse("2650-2640"));
  EXPECT_FALSE(AccountList::parse("26x0"));
}

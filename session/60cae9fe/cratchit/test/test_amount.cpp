#include "core/amount.hpp"

#include <gtest/gtest.h>

using cratchit::format_amount;
using cratchit::parse_amount;

TEST(Amount, ParsesSieAmounts) {
  EXPECT_EQ(parse_amount("13000"), 1300000);
  EXPECT_EQ(parse_amount("-6498.77"), -649877);
  EXPECT_EQ(parse_amount("1.5"), 150);
  EXPECT_EQ(parse_amount("1.85"), 185);
  EXPECT_EQ(parse_amount("0"), 0);
  EXPECT_EQ(parse_amount("-0.01"), -1);
  EXPECT_EQ(parse_amount("999999999999999.99"), 99999999999999999);
}

TEST(Amount, RejectsWhatSieDoesNotAllow) {
  EXPECT_FALSE(parse_amount(""));
  EXPECT_FALSE(parse_amount("-"));
  EXPECT_FALSE(parse_amount("+5"));
  EXPECT_FALSE(parse_amount("1,50"));
  EXPECT_FALSE(parse_amount("1.505"));
  EXPECT_FALSE(parse_amount("1."));
  EXPECT_FALSE(parse_amount(".5"));
  EXPECT_FALSE(parse_amount("1 000"));
  EXPECT_FALSE(parse_amount("1e3"));
  EXPECT_FALSE(parse_amount("1000000000000000")); // 16 digits
}

TEST(Amount, FormatsWithTwoDecimals) {
  EXPECT_EQ(format_amount(1300000), "13000.00");
  EXPECT_EQ(format_amount(-649877), "-6498.77");
  EXPECT_EQ(format_amount(-1), "-0.01");
  EXPECT_EQ(format_amount(0), "0.00");
  EXPECT_EQ(parse_amount(format_amount(-123456789)), -123456789);
}

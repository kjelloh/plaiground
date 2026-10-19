#include "amount.hpp"

#include <gtest/gtest.h>

using cratchit::Amount;
using cratchit::parse_amount;

TEST(Amount, DefaultIsZero) { EXPECT_EQ(Amount{}.cents(), 0); }

TEST(Amount, Arithmetic) {
  auto const a = Amount::from_cents(10'050);
  auto const b = Amount::from_cents(-2'025);
  EXPECT_EQ((a + b).cents(), 8'025);
  EXPECT_EQ((a - b).cents(), 12'075);
  EXPECT_EQ((-a).cents(), -10'050);
  EXPECT_LT(b, a);
}

TEST(ParseAmount, WholeKronor) {
  EXPECT_EQ(parse_amount("0").value().cents(), 0);
  EXPECT_EQ(parse_amount("9810").value().cents(), 981'000);
  EXPECT_EQ(parse_amount("-180").value().cents(), -18'000);
}

TEST(ParseAmount, Decimals) {
  EXPECT_EQ(parse_amount("105.8").value().cents(), 10'580);
  EXPECT_EQ(parse_amount("1798.75").value().cents(), 179'875);
  EXPECT_EQ(parse_amount("-0.05").value().cents(), -5);
}

TEST(ParseAmount, RejectsMalformed) {
  for (auto text : {"", "-", ".5", "12.", "1.234", "12a", "1,50", "+5", "1234567890123456"}) {
    EXPECT_FALSE(parse_amount(text).has_value()) << "accepted \"" << text << "\"";
  }
}

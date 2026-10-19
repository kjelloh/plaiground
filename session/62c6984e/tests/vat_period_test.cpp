#include "vat_period.hpp"

#include <gtest/gtest.h>

using namespace std::chrono;
using cratchit::parse_vat_period;
using cratchit::to_string;

TEST(VatPeriod, ParseAndFormat) {
  EXPECT_EQ(parse_vat_period("202409").value(), 2024y / September);
  EXPECT_EQ(to_string(2024y / September), "202409");
  EXPECT_EQ(to_string(2025y / March), "202503");
}

TEST(VatPeriod, RejectsMalformed) {
  for (auto text : {"", "20249", "2024090", "202413", "202400", "2024-9"}) {
    EXPECT_FALSE(parse_vat_period(text).has_value()) << "accepted \"" << text << "\"";
  }
}

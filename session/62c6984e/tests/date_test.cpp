#include "date.hpp"

#include <gtest/gtest.h>

using namespace std::chrono;
using cratchit::parse_date;

TEST(ParseDate, Valid) {
  EXPECT_EQ(parse_date("20240501").value(), 2024y / May / 1d);
  EXPECT_EQ(parse_date("20240229").value(), 2024y / February / 29d);
}

TEST(ParseDate, RejectsMalformed) {
  for (auto text : {"", "2024051", "202405011", "2024-05-01", "20240230", "20241301", "2023022a"}) {
    EXPECT_FALSE(parse_date(text).has_value()) << "accepted \"" << text << "\"";
  }
}

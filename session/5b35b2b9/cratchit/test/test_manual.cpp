#include "core/manual.hpp"

#include <gtest/gtest.h>

#include <string_view>

TEST(Manual, StartsWithTheProgramName) {
  EXPECT_TRUE(cratchit::manual_text().starts_with("# cratchit_ar\n"));
}

TEST(Manual, NamesEveryOption) {
  for (std::string_view const option : {"--accounts", "--help", "-h"}) {
    EXPECT_NE(cratchit::manual_text().find(option), std::string_view::npos) << option;
  }
}

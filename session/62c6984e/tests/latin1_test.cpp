#include "latin1.hpp"

#include <gtest/gtest.h>

using cratchit::latin1_to_utf8;
using cratchit::utf8_to_latin1;

TEST(Latin1, RoundTripSwedishLetters) {
  auto const latin1 = utf8_to_latin1("Moms åäö ÅÄÖ é").value();
  EXPECT_EQ(latin1, "Moms \xE5\xE4\xF6 \xC5\xC4\xD6 \xE9");
  EXPECT_EQ(latin1_to_utf8(latin1), "Moms åäö ÅÄÖ é");
}

TEST(Latin1, RejectsCharactersOutsideLatin1) {
  EXPECT_FALSE(utf8_to_latin1("€").has_value());
  EXPECT_FALSE(utf8_to_latin1("─").has_value());
  EXPECT_FALSE(utf8_to_latin1("\xC3").has_value());  // truncated sequence
}

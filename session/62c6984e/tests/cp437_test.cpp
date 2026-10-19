#include "cp437.hpp"

#include <gtest/gtest.h>

using cratchit::cp437_to_utf8;

TEST(Cp437, AsciiIsUnchanged) { EXPECT_EQ(cp437_to_utf8("#KONTO 1920 \"Bank\""), "#KONTO 1920 \"Bank\""); }

TEST(Cp437, SwedishLetters) {
  EXPECT_EQ(cp437_to_utf8("\x86\x84\x94\x8F\x8E\x99"), "åäöÅÄÖ");
  EXPECT_EQ(cp437_to_utf8("Ing\x86""ende moms"), "Ingående moms");
}

TEST(Cp437, ThreeByteUtf8) { EXPECT_EQ(cp437_to_utf8("\xC4"), "─"); }

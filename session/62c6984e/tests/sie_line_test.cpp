#include "sie_line.hpp"

#include <gtest/gtest.h>

using cratchit::split_sie_line;

TEST(SplitSieLine, PlainAndQuotedFields) {
  auto const fields = split_sie_line("#KONTO 2641\t\"Debiterad ingående moms\"").value();
  ASSERT_EQ(fields.size(), 3u);
  EXPECT_EQ(fields[0].text, "#KONTO");
  EXPECT_EQ(fields[1].text, "2641");
  EXPECT_EQ(fields[2].text, "Debiterad ingående moms");
}

TEST(SplitSieLine, TransWithEmptyObjectListAndEmptyStrings) {
  auto const fields = split_sie_line("#TRANS 1920 {} -200 \"\" \"privat inköp\" 0").value();
  ASSERT_EQ(fields.size(), 7u);
  EXPECT_TRUE(fields[2].is_object_list);
  EXPECT_EQ(fields[2].text, "");
  EXPECT_EQ(fields[3].text, "-200");
  EXPECT_FALSE(fields[4].is_object_list);
  EXPECT_EQ(fields[4].text, "");
  EXPECT_EQ(fields[5].text, "privat inköp");
}

TEST(SplitSieLine, EscapedQuotes) {
  auto const fields = split_sie_line(R"sie(#TRANS 1227 {} 353 "" "plate (1/4\" thread)" 0)sie").value();
  EXPECT_EQ(fields[5].text, "plate (1/4\" thread)");

  auto const ending = split_sie_line(R"sie(#X "kit\"" "\"quoted\"")sie").value();
  EXPECT_EQ(ending[1].text, "kit\"");
  EXPECT_EQ(ending[2].text, "\"quoted\"");
}

TEST(SplitSieLine, ObjectListWithContent) {
  auto const fields = split_sie_line(R"sie(#TRANS 3010 {1 "100" 6 "P}1"} -500)sie").value();
  ASSERT_EQ(fields.size(), 4u);
  EXPECT_TRUE(fields[2].is_object_list);
  EXPECT_EQ(fields[2].text, R"sie(1 "100" 6 "P}1")sie");
  EXPECT_EQ(fields[3].text, "-500");
}

TEST(SplitSieLine, RejectsUnterminated) {
  EXPECT_FALSE(split_sie_line("#KONTO 1920 \"Bank").has_value());
  EXPECT_FALSE(split_sie_line("#TRANS 1920 {1 \"x\" -1").has_value());
}

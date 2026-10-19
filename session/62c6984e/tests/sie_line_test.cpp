#include "sie_line.hpp"

#include <gtest/gtest.h>

using cratchit::split_sie_line;

TEST(SplitSieLine, PlainAndQuotedFields) {
  auto const fields = split_sie_line("#KONTO 2641\t\"Debiterad ingående moms\"").value();
  ASSERT_EQ(fields.size(), 3u);
  EXPECT_EQ(fields[0].m_text, "#KONTO");
  EXPECT_EQ(fields[1].m_text, "2641");
  EXPECT_EQ(fields[2].m_text, "Debiterad ingående moms");
}

TEST(SplitSieLine, TransWithEmptyObjectListAndEmptyStrings) {
  auto const fields = split_sie_line("#TRANS 1920 {} -250 \"\" \"Kontorsmaterial åäö\" 0").value();
  ASSERT_EQ(fields.size(), 7u);
  EXPECT_TRUE(fields[2].m_is_object_list);
  EXPECT_EQ(fields[2].m_text, "");
  EXPECT_EQ(fields[3].m_text, "-250");
  EXPECT_FALSE(fields[4].m_is_object_list);
  EXPECT_EQ(fields[4].m_text, "");
  EXPECT_EQ(fields[5].m_text, "Kontorsmaterial åäö");
}

TEST(SplitSieLine, EscapedQuotes) {
  auto const fields = split_sie_line(R"sie(#TRANS 1220 {} 400 "" "Skruv (1/4\" gänga)" 0)sie").value();
  EXPECT_EQ(fields[5].m_text, "Skruv (1/4\" gänga)");

  auto const ending = split_sie_line(R"sie(#X "kit\"" "\"quoted\"")sie").value();
  EXPECT_EQ(ending[1].m_text, "kit\"");
  EXPECT_EQ(ending[2].m_text, "\"quoted\"");
}

TEST(SplitSieLine, ObjectListWithContent) {
  auto const fields = split_sie_line(R"sie(#TRANS 3010 {1 "100" 6 "P}1"} -500)sie").value();
  ASSERT_EQ(fields.size(), 4u);
  EXPECT_TRUE(fields[2].m_is_object_list);
  EXPECT_EQ(fields[2].m_text, R"sie(1 "100" 6 "P}1")sie");
  EXPECT_EQ(fields[3].m_text, "-500");
}

TEST(SplitSieLine, RejectsUnterminated) {
  EXPECT_FALSE(split_sie_line("#KONTO 1920 \"Bank").has_value());
  EXPECT_FALSE(split_sie_line("#TRANS 1920 {1 \"x\" -1").has_value());
}

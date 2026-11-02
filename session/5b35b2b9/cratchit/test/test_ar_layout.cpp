#include "core/ar/layout.hpp"

#include <gtest/gtest.h>

#include <format>
#include <string>
#include <vector>

using cratchit::ar::LineKind;
using cratchit::ar::parse_account_set;
using cratchit::ar::parse_layout;

namespace {

// The accounts 1000-9999 in a set, written for comparing
std::string members(std::string_view text) {
  auto const set = parse_account_set(text);
  EXPECT_TRUE(set.has_value()) << (set ? "" : set.error());
  std::string out;
  for (int number = 1000; set && number <= 9999; ++number) {
    if (set->contains(std::to_string(number))) {
      out += std::to_string(number) + ' ';
    }
  }
  return out;
}

} // namespace

TEST(ArAccountSet, SingleAccount) {
  EXPECT_EQ(members("2099"), "2099 ");
}

TEST(ArAccountSet, Range) {
  EXPECT_EQ(members("1336-1337"), "1336 1337 ");
}

TEST(ArAccountSet, CommasAndEller) {
  EXPECT_EQ(members("2081, 2083 eller 2084"), "2081 2083 2084 ");
  EXPECT_EQ(members("1410-1411, 1430, 1431 eller 1438"), "1410 1411 1430 1431 1438 ");
}

TEST(ArAccountSet, ExclusionList) {
  EXPECT_EQ(members("8100-8105 (förutom 8101, 8102 och 8104-8105)"), "8100 8103 ");
}

TEST(ArAccountSet, ExclusionThenMore) {
  EXPECT_EQ(members("1570-1574 (förutom 1573) eller 1670-1674 (förutom 1673)"),
            "1570 1571 1572 1574 1670 1671 1672 1674 ");
  EXPECT_EQ(members("2490-2493 (förutom 2492), 2600-2601 eller 2880"), "2490 2491 2493 2600 2601 2880 ");
}

TEST(ArAccountSet, OnlyFourDigitAccounts) {
  auto const set = parse_account_set("1000-1999");
  ASSERT_TRUE(set);
  EXPECT_TRUE(set->contains("1930"));
  EXPECT_FALSE(set->contains("193"));
  EXPECT_FALSE(set->contains("19300"));
  EXPECT_FALSE(set->contains("19a0"));
}

TEST(ArAccountSet, Unreadable) {
  EXPECT_FALSE(parse_account_set(""));
  EXPECT_FALSE(parse_account_set("1000-"));
  EXPECT_FALSE(parse_account_set("1999-1000"));
  EXPECT_FALSE(parse_account_set("1000 1001"));
  EXPECT_FALSE(parse_account_set("1000 (utom 1001)"));
  EXPECT_FALSE(parse_account_set("1000-1010 (förutom 1001"));
  EXPECT_FALSE(parse_account_set("100"));
  EXPECT_FALSE(parse_account_set("2650 vid debetsaldo"));
}

TEST(ArLayout, Rows) {
  auto const lines = parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Rubrik;;Resultaträkning\n3;Post;3000-3799;Nettoomsättning\n"
                                  "1;Summa;;Årets resultat\n;Utanför;8999;Bokslutspost\n");
  ASSERT_TRUE(lines) << lines.error();
  ASSERT_EQ(lines->size(), 4u);
  EXPECT_EQ((*lines)[0].kind, LineKind::heading);
  EXPECT_EQ((*lines)[1].kind, LineKind::item);
  EXPECT_EQ((*lines)[1].level, 3);
  EXPECT_TRUE((*lines)[1].accounts.contains("3308"));
  EXPECT_EQ((*lines)[2].kind, LineKind::sum);
  EXPECT_EQ((*lines)[3].kind, LineKind::outside);
  EXPECT_EQ((*lines)[3].level, 0);
}

TEST(ArLayout, Errors) {
  EXPECT_FALSE(parse_layout("BAS Range;ÅR\n3000;Nettoomsättning\n"));
  EXPECT_FALSE(parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Okänd;;X\n"));
  EXPECT_FALSE(parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Rubrik;3000;X\n"));
  EXPECT_FALSE(parse_layout("Nivå;Typ;BAS Range;ÅR\n;Post;3000;X\n"));
  EXPECT_FALSE(parse_layout("Nivå;Typ;BAS Range;ÅR\n3;Post;30x0;X\n"));
  EXPECT_FALSE(parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Utanför;8999;X\n"));
}

TEST(ArLayout, BuiltInMappingReads) {
  auto const lines = parse_layout(cratchit::ar::layout_2024_text());
  ASSERT_TRUE(lines) << lines.error();
  EXPECT_GT(lines->size(), 100u);
}

TEST(ArLayout, BuiltInMappingPutsNoAccountOnTwoLines) {
  auto const lines = parse_layout(cratchit::ar::layout_2024_text());
  ASSERT_TRUE(lines);
  for (int number = 1000; number <= 9999; ++number) {
    auto const account = std::to_string(number);
    std::vector<std::string> texts;
    for (auto const& line : *lines) {
      if (line.accounts.contains(account)) {
        texts.push_back(line.text);
      }
    }
    EXPECT_LE(texts.size(), 1u) << account << " is on " << texts.front() << " and " << texts.back();
  }
}

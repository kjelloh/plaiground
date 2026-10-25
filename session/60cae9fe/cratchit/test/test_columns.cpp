#include "core/books/columns.hpp"

#include <gtest/gtest.h>

using namespace cratchit::books;

namespace {

Posting posting(std::string account, cratchit::Amount amount, Posting::Kind kind = Posting::Kind::normal) {
  Posting p;
  p.kind = kind;
  p.account = std::move(account);
  p.amount = amount;
  return p;
}

} // namespace

TEST(Columns, DebitFromZeroUpCreditBelowZero) {
  EXPECT_EQ(sides_of(89440).debit, "894.40");
  EXPECT_EQ(sides_of(89440).credit, "");
  EXPECT_EQ(sides_of(0).debit, "0.00");
  EXPECT_EQ(sides_of(0).credit, "");
  EXPECT_EQ(sides_of(-111800).debit, "");
  EXPECT_EQ(sides_of(-111800).credit, "1118.00");
}

TEST(Columns, RowsLineUpUnderTheHeading) {
  EXPECT_EQ(posting_heading(), "Account        Debit       Credit");
  EXPECT_EQ(posting_row("1930", -111800, "Företagskonto"), "1930                      1118.00  Företagskonto");
  EXPECT_EQ(posting_row("6212", 89440, "Mobiltelefon"), "6212          894.40               Mobiltelefon");
  EXPECT_EQ(posting_row("6212", 89440, ""), "6212          894.40");
}

TEST(Columns, TotalLeavesOutRemovedPostings) {
  Postings const postings{posting("1930", -111800), posting("6212", 89440), posting("2641", 22360),
                          posting("6212", 5000, Posting::Kind::removed)};
  EXPECT_EQ(posting_total(postings), "Total        1118.00      1118.00");
}

TEST(Columns, BriefSaysDebitOrCredit) {
  EXPECT_EQ(posting_brief(posting("1930", -111800)), "1930 C 1118.00");
  EXPECT_EQ(posting_brief(posting("6212", 89440)), "6212 D 894.40");
  EXPECT_EQ(posting_brief(posting("3740", 0)), "3740 D 0.00");
}

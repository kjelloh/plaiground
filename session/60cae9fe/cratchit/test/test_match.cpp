#include "core/books/match.hpp"

#include <gtest/gtest.h>

#include <chrono>

using namespace cratchit;
using namespace cratchit::books;
using namespace std::chrono;

namespace {

WaitingEvent waiting(std::string heading, Amount amount, Date date, Postings postings = {}) {
  return {{std::move(heading), amount, date}, Date{2026y, October, 20d}, std::move(postings)};
}

Posting posting(std::string account, Amount amount, Date date, std::string source = {}) {
  Posting result;
  result.account = std::move(account);
  result.amount = amount;
  result.date = date;
  result.source = std::move(source);
  return result;
}

Voucher voucher(std::string series, std::string number, std::string heading, Amount amount, Date date,
                Postings postings) {
  return {std::move(series), std::move(number), {std::move(heading), amount, date}, std::nullopt, {}, std::move(postings)};
}

FiscalYear fiscal_year() {
  FiscalYear year;
  year.start = Date{2026y, May, 1d};
  year.end = Date{2027y, April, 30d};
  return year;
}

} // namespace

TEST(Match, SameAmountWithinTheWindow) {
  auto year = fiscal_year();
  year.place_waiting(waiting("Telia faktura", 111800, Date{2026y, October, 20d}));
  year.place_waiting(waiting("Telia betalning", 111800, Date{2026y, October, 24d})); // 4 days: in
  year.place_waiting(waiting("Telia sen", 111800, Date{2026y, October, 25d}));       // 5 days from the first: out
  year.place_waiting(waiting("Kvitto", 24900, Date{2026y, October, 21d}));           // another amount
  auto const matches = find_matches(year);
  ASSERT_EQ(matches.size(), 2u);
  // Listed by date: faktura, Kvitto, betalning, sen. Both pairs share "telia"; fewer days apart first
  EXPECT_EQ(matches[0], (Match{Match::Kind::same, 2, 3, 1, {"telia"}, false}));
  EXPECT_EQ(matches[1], (Match{Match::Kind::same, 0, 2, 4, {"telia"}, false}));
}

TEST(Match, TheWindowIsAConstant) {
  EXPECT_EQ(match_window_days, 4);
  auto year = fiscal_year();
  year.place_waiting(waiting("A", 100, Date{2026y, October, 20d}));
  year.place_waiting(waiting("B", 100, Date{2026y, October, 22d}));
  EXPECT_EQ(find_matches(year, 1).size(), 0u);
  EXPECT_EQ(find_matches(year, 2).size(), 1u);
}

TEST(Match, PostingsThatBalanceTogetherComeFirst) {
  Date const day{2026y, October, 2d};
  auto year = fiscal_year();
  year.place_waiting(waiting("Kvitto lunch", 50000, day));
  year.place_waiting(waiting("Kvitto middag", 50000, day));
  year.place_waiting(waiting("Överföring", 50000, day, {posting("1920", -50000, day, "a.csv:3")}));
  year.place_waiting(waiting("Insättning", 50000, day, {posting("1930", 50000, day, "b.csv:7")}));
  auto const matches = find_matches(year);
  ASSERT_FALSE(matches.empty());
  EXPECT_EQ(matches[0].waiting, 2u);
  EXPECT_EQ(matches[0].other, 3u);
  EXPECT_TRUE(matches[0].fits);
  EXPECT_EQ(matches[1].words, std::vector<std::string>{"kvitto"});
}

TEST(Match, PostingsThatCancelOnOneAccountDoNotFit) {
  Date const day{2026y, July, 18d};
  auto year = fiscal_year();
  year.place_waiting(waiting("Momsbeslut", 229200, day, {posting("1630", 229200, day)}));
  year.place_waiting(waiting("Utbetalning", 229200, Date{2026y, July, 21d}, {posting("1630", -229200, day)}));
  auto const matches = find_matches(year);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_FALSE(matches[0].fits);
}

TEST(Match, VouchersInTheBooksMatchToo) {
  Date const day{2026y, October, 20d};
  auto year = fiscal_year();
  year.vouchers.push_back(voucher("D", "4", "Telia faktura oktober", 111800, Date{2026y, October, 21d},
                                  {posting("6212", 111800, Date{2026y, October, 21d}),
                                   posting("1920", -111800, Date{2026y, October, 23d})}));
  year.vouchers.push_back(voucher("D", "5", "Hyra", 111800, Date{2026y, October, 30d}, {}));
  year.place_waiting(waiting("Telia", 111800, day, {posting("1920", -111800, Date{2026y, October, 23d})}));
  auto const matches = find_matches(year);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_EQ(matches[0], (Match{Match::Kind::books, 0, 0, 1, {"telia"}, true}));
}

TEST(Match, UnbookedPostingsAreThoseTheVoucherLacks) {
  Date const day{2026y, October, 20d};
  auto const event = waiting("Telia", 100, day, {posting("1920", -100, day), posting("1920", -100, day)});
  auto const booking = voucher("D", "4", "Telia", 100, day, {posting("6212", 100, day), posting("1920", -100, day)});
  auto const left = unbooked(event, booking);
  ASSERT_EQ(left.size(), 1u); // each voucher posting books one
  EXPECT_EQ(left[0].account, "1920");
}

TEST(Match, MergedKeepsThePostingsOfBothOnce) {
  Date const day{2026y, October, 2d};
  auto a = waiting("Överföring", 50000, day, {posting("1920", -50000, day, "a.csv:3")});
  auto b = waiting("Insättning", 50000, Date{2026y, October, 3d},
                   {posting("1920", -50000, day, "a.csv:3"), posting("1930", 50000, day, "b.csv:7")});
  a.noted = Date{2026y, October, 4d};
  b.noted = Date{2026y, October, 3d};
  auto const one = merged(a, b, {"Överföring sparkonto", 50000, day});
  EXPECT_EQ(one.event.heading, "Överföring sparkonto");
  EXPECT_EQ(one.noted, (Date{2026y, October, 3d}));
  ASSERT_EQ(one.postings.size(), 2u);
  EXPECT_EQ(one.postings[0].source, "a.csv:3");
  EXPECT_EQ(one.postings[1].source, "b.csv:7");
}

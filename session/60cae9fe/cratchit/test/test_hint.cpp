#include "core/books/hint.hpp"

#include <gtest/gtest.h>

#include <chrono>

using namespace cratchit;
using namespace cratchit::books;
using namespace std::chrono;

namespace {

Posting posting(std::string account, Amount amount, Posting::Kind kind = Posting::Kind::normal) {
  Posting result;
  result.kind = kind;
  result.account = std::move(account);
  result.amount = amount;
  return result;
}

Voucher voucher(std::string number, Date date, Postings postings) {
  return {"D", std::move(number), {"Faktura", 100, date}, std::nullopt, {}, std::move(postings)};
}

WaitingEvent waiting(Date date, Postings postings) {
  return {{"Bank", 100, date}, date, std::move(postings)};
}

// 1920 on credit in D 1 (against 2440), D 2 (6540 and 2641) and D 3 (2440), on debit in D 4
Books books_with_payments() {
  Books books;
  FiscalYear older;
  older.start = Date{2025y, May, 1d};
  older.end = Date{2026y, April, 30d};
  older.vouchers.push_back(voucher("1", Date{2025y, June, 1d}, {posting("1920", -100), posting("2440", 100)}));
  FiscalYear newer;
  newer.start = Date{2026y, May, 1d};
  newer.end = Date{2027y, April, 30d};
  newer.vouchers.push_back(
      voucher("2", Date{2026y, June, 1d}, {posting("6540", 80), posting("2641", 20), posting("1920", -100)}));
  newer.vouchers.push_back(voucher("3", Date{2026y, July, 1d}, {posting("2440", 100), posting("1920", -100)}));
  newer.vouchers.push_back(voucher("4", Date{2026y, August, 1d}, {posting("1920", 100), posting("3010", -100)}));
  books.years = {older, newer};
  return books;
}

} // namespace

TEST(Hint, CountsTheCounterAccountsOnTheOtherSideLatestFirst) {
  auto const hint = hint_for(books_with_payments(), "1920", false);
  EXPECT_EQ(hint.account, "1920");
  EXPECT_FALSE(hint.debit);
  EXPECT_EQ(hint.counted, 3u);
  EXPECT_EQ(hint.latest, (std::vector<HintSource>{{1, 1, false}, {1, 0, false}, {0, 0, false}}));
  // 2440 in two; 6540 before 2641 as in the voucher
  EXPECT_EQ(hint.counters, (std::vector<CounterAccount>{
                               {"2440", 2, {1, 1, false}}, {"6540", 1, {1, 0, false}}, {"2641", 1, {1, 0, false}}}));
}

TEST(Hint, OnlyTheSameSide) {
  auto const hint = hint_for(books_with_payments(), "1920", true);
  EXPECT_EQ(hint.latest, (std::vector<HintSource>{{1, 2, false}}));
  EXPECT_EQ(hint.counters, (std::vector<CounterAccount>{{"3010", 1, {1, 2, false}}}));
}

TEST(Hint, CountsAndListsOnlyTheLatest) {
  auto const hint = hint_for(books_with_payments(), "1920", false, std::nullopt, 2, 1);
  EXPECT_EQ(hint.counted, 2u);
  EXPECT_EQ(hint.latest, (std::vector<HintSource>{{1, 1, false}}));
  EXPECT_EQ(hint.counters, (std::vector<CounterAccount>{
                               {"2440", 1, {1, 1, false}}, {"6540", 1, {1, 0, false}}, {"2641", 1, {1, 0, false}}}));
}

TEST(Hint, WaitingEventsCountUnlessExcluded) {
  auto books = books_with_payments();
  books.years[1].waiting.push_back(waiting(Date{2026y, September, 1d}, {posting("1920", -50)}));
  books.years[1].waiting.push_back(waiting(Date{2026y, September, 2d}, {posting("1920", -60), posting("2650", 60)}));
  auto const hint = hint_for(books, "1920", false);
  EXPECT_EQ(hint.counted, 5u);
  EXPECT_EQ(hint.latest.front(), (HintSource{1, 1, true}));
  EXPECT_EQ(hint.latest[1], (HintSource{1, 0, true}));
  EXPECT_EQ(hint.counters[1], (CounterAccount{"2650", 1, {1, 1, true}}));
  auto const without = hint_for(books, "1920", false, HintSource{1, 1, true});
  EXPECT_EQ(without.counted, 4u);
  EXPECT_EQ(without.latest.front(), (HintSource{1, 0, true}));
}

TEST(Hint, RemovedPostingsDoNotCount) {
  auto books = books_with_payments();
  books.years[1].vouchers[1].postings[0].kind = Posting::Kind::removed;
  books.years[1].vouchers[1].postings.push_back(posting("2441", 100, Posting::Kind::added));
  auto const hint = hint_for(books, "1920", false);
  EXPECT_EQ(hint.counters.front(), (CounterAccount{"2441", 1, {1, 1, false}}));
}

TEST(Hint, SameDateTheLaterInTheBooksFirst) {
  auto books = books_with_payments();
  books.years[1].vouchers.push_back(voucher("5", Date{2026y, July, 1d}, {posting("1920", -10), posting("6570", 10)}));
  auto const hint = hint_for(books, "1920", false);
  EXPECT_EQ(hint.latest[0], (HintSource{1, 3, false}));
  EXPECT_EQ(hint.latest[1], (HintSource{1, 1, false}));
}

TEST(Hint, SidesOfTheCountedPostingsOnce) {
  EXPECT_EQ(sides_in({posting("1920", -100), posting("6540", 80), posting("1920", -5), posting("1920", 5),
                      posting("2641", 20, Posting::Kind::removed)}),
            (std::vector<AccountSide>{{"1920", false}, {"6540", true}, {"1920", true}}));
}

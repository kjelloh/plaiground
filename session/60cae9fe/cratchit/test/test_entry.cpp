#include "core/books/entry.hpp"

#include <gtest/gtest.h>

#include <chrono>

using namespace cratchit;
using namespace cratchit::books;
using namespace std::chrono;

namespace {

Date const today{2026y, October, 22d};

// The readings of a line that parses
std::vector<Event> readings(std::string_view text) {
  auto const parsed = parse_event(text, today);
  EXPECT_TRUE(parsed.has_value()) << text << ": " << parsed.error();
  return parsed.value_or(std::vector<Event>{});
}

// The only reading of a line whose amount is clear
Event event(std::string_view text) {
  auto const read = readings(text);
  EXPECT_EQ(read.size(), 1u) << text;
  return read.empty() ? Event{} : read.front();
}

std::string error_of(std::string_view text) {
  auto const parsed = parse_event(text, today);
  EXPECT_FALSE(parsed.has_value()) << text;
  return parsed ? std::string{} : parsed.error();
}

Voucher voucher(std::string series, std::string number, Date date, std::string text) {
  return {std::move(series), std::move(number), {std::move(text), 0, date}, std::nullopt, {}, {}};
}

Books two_years() {
  Books books;
  books.accounts = {{"1930", "Företagskonto", "", "", ""},
                    {"2641", "Debiterad ingående moms", "", "", ""},
                    {"6540", "IT-tjänster", "", "", ""},
                    {"6570", "Bankkostnader", "", "", ""}};
  books.years.push_back({Date{2025y, May, 1d}, Date{2026y, April, 30d}, {}, {}, false});
  books.years.push_back({Date{2026y, May, 1d}, Date{2027y, April, 30d}, {}, {}, false});
  books.years[0].vouchers = {voucher("D", "3", Date{2025y, May, 18d}, "Faktura: Telia"),
                             voucher("E", "2", Date{2025y, June, 17d}, "Betalning: Telia"),
                             voucher("A", "2", Date{2025y, May, 6d}, "AVGIFTER NORDEA")};
  books.years[1].vouchers = {voucher("D", "1", Date{2026y, May, 8d}, "Faktura: Fortnox"),
                             voucher("D", "12", Date{2026y, May, 18d}, "Faktura: Telia"),
                             voucher("X", "1", Date{2026y, June, 1d}, "Telia Telia faktura")};
  return books;
}

} // namespace

TEST(ParseEvent, HeadingAmountAndDateInAnyOrder) {
  EXPECT_EQ(event("Telia faktura oktober 1118,00 2026-10-20"),
            (Event{"Telia faktura oktober", 111800, Date{2026y, October, 20d}}));
  EXPECT_EQ(event("20261020 1118.5 Telia faktura"), (Event{"Telia faktura", 111850, Date{2026y, October, 20d}}));
  EXPECT_EQ(event("Telia 2026-10-20 faktura 1118"), (Event{"Telia faktura", 111800, Date{2026y, October, 20d}}));
}

TEST(ParseEvent, NoDateMeansToday) {
  EXPECT_EQ(event("Bankavgift 35"), (Event{"Bankavgift", 3500, today}));
}

TEST(ParseEvent, TheLastNumberIsTheAmount) {
  EXPECT_EQ(event("Faktura 87 Payex 500"), (Event{"Faktura 87 Payex", 50000, today}));
  // Not groups of thousands: the last word has a sign or is not three digits, or the word before is too long
  EXPECT_EQ(event("Faktura 87 -500"), (Event{"Faktura 87", -50000, today}));
  EXPECT_EQ(event("Faktura 87 50,00"), (Event{"Faktura 87", 5000, today}));
  EXPECT_EQ(event("Faktura 8700 500"), (Event{"Faktura 8700", 50000, today}));
  EXPECT_EQ(event("Faktura 87,5 500"), (Event{"Faktura 87,5", 50000, today}));
}

TEST(ParseEvent, SpacedNumbersGiveOneReadingPerGrouping) {
  EXPECT_EQ(readings("Faktura 87 500"),
            (std::vector<Event>{{"Faktura 87", 50000, today}, {"Faktura", 8750000, today}}));
  EXPECT_EQ(readings("Telia 1 118,00"), (std::vector<Event>{{"Telia 1", 11800, today}, {"Telia", 111800, today}}));
  EXPECT_EQ(readings("Lön 1 234 567,50"), (std::vector<Event>{{"Lön 1 234", 56750, today},
                                                               {"Lön 1", 23456750, today},
                                                               {"Lön", 123456750, today}}));
  EXPECT_EQ(readings("Utbetalning -1 118,00"),
            (std::vector<Event>{{"Utbetalning -1", 11800, today}, {"Utbetalning", -111800, today}}));
  // A reading without heading is left out
  EXPECT_EQ(readings("1 118,00 Telia"), (std::vector<Event>{{"1 Telia", 11800, today}, {"Telia", 111800, today}}));
  EXPECT_EQ(readings("Telia 1 118"), (std::vector<Event>{{"Telia 1", 11800, today}, {"Telia", 111800, today}}));
}

TEST(ParseEvent, NoBreakSpaceInsideANumberIsClear) {
  EXPECT_EQ(event("Telia 1\xC2\xA0" "118,00"), (Event{"Telia", 111800, today}));
}

TEST(ParseEvent, TheLastDateIsTheDate) {
  EXPECT_EQ(event("Momsrapport 2026-07-01 2026-09-30 47,11"),
            (Event{"Momsrapport 2026-07-01", 4711, Date{2026y, September, 30d}}));
}

TEST(ParseEvent, Errors) {
  EXPECT_EQ(error_of("Telia faktura"), "No amount. Type it with digits, e.g. 1118,00");
  EXPECT_EQ(error_of("1118 2026-10-20"), "No heading. Type what happened, e.g. Telia faktura oktober");
  EXPECT_EQ(error_of("Telia 0,00"), "The amount is zero.");
  EXPECT_EQ(error_of("Telia 1118,005"), "Not an amount: 1118,005 (at most two decimals)");
  EXPECT_EQ(error_of(""), "No amount. Type it with digits, e.g. 1118,00");
}

TEST(ParseEvent, NoDateAndNoTodayIsAnError) {
  EXPECT_EQ(parse_event("Telia 1118", std::nullopt).error(),
            "No date, and today is outside the fiscal year. Type the date.");
  EXPECT_EQ(parse_event("Telia 1118 2026-10-01", std::nullopt)->front().date, (Date{2026y, October, 1d}));
}

TEST(FiscalYear, HoldsItsDays) {
  auto const books = two_years();
  EXPECT_TRUE(books.years[1].holds(Date{2026y, May, 1d}));
  EXPECT_TRUE(books.years[1].holds(Date{2027y, April, 30d}));
  EXPECT_FALSE(books.years[1].holds(Date{2027y, May, 1d}));
  EXPECT_FALSE(books.years[1].holds(Date{2026y, April, 30d}));
}

TEST(Series, AToN) {
  ASSERT_EQ(series_names().size(), 14u);
  EXPECT_EQ(series_names().front().letter, "A");
  EXPECT_EQ(series_names().back().letter, "N");
  EXPECT_EQ(series_name("D")->name, "Leverantörsfakturor");
  EXPECT_FALSE(series_name("X"));
  EXPECT_FALSE(series_name("d"));
}

TEST(GuessSeries, FromTheMostSimilarLatestVoucher) {
  auto const books = two_years();
  auto const guess = guess_series(books, "telia FAKTURA oktober");
  EXPECT_EQ(guess.series, "D");
  ASSERT_NE(guess.like, nullptr);
  EXPECT_EQ(guess.like->number, "12"); // the latest of the two; X is not a series offered
  EXPECT_EQ(guess_series(books, "Betalning Telia").series, "E");
  EXPECT_EQ(guess_series(books, "Avgifter nordea").series, "A");
}

TEST(GuessSeries, NothingSimilarGivesA) {
  auto const guess = guess_series(two_years(), "Lunch 2026");
  EXPECT_EQ(guess.series, "A");
  EXPECT_EQ(guess.like, nullptr);
}

TEST(NextNumber, HighestInTheSeriesPlusOne) {
  auto const books = two_years();
  EXPECT_EQ(next_number(books.years[1], "D"), "13");
  EXPECT_EQ(next_number(books.years[1], "A"), "1");
}

TEST(ParseRow, AccountAndOptionalAmount) {
  EXPECT_EQ(parse_row("1930 -5590"), (Row{"1930", -559000}));
  EXPECT_EQ(parse_row("6540  4 472,00"), (Row{"6540", 447200}));
  EXPECT_EQ(parse_row("2641"), (Row{"2641", std::nullopt}));
  EXPECT_EQ(parse_row("moms"), (Row{"moms", std::nullopt}));
  EXPECT_EQ(parse_row("1930 abc").error(), "Not an amount: abc");
}

TEST(FindAccounts, ByNumberPrefixOrName) {
  auto const books = two_years();
  auto const numbers = [&](std::string_view query) {
    std::vector<std::string> result;
    for (auto const* account : find_accounts(books, query)) {
      result.push_back(account->number);
    }
    return result;
  };
  EXPECT_EQ(numbers("65"), (std::vector<std::string>{"6540", "6570"}));
  EXPECT_EQ(numbers("MOMS"), (std::vector<std::string>{"2641"}));
  EXPECT_EQ(numbers("FÖRETAG"), (std::vector<std::string>{"1930"}));
  EXPECT_TRUE(numbers("lager").empty());
}

TEST(Draft, RemainingAndBalance) {
  Draft draft{{"Telia", -559000, today}, 1, "D", {}};
  EXPECT_EQ(draft.remaining(), 0);
  EXPECT_FALSE(draft.is_balanced());
  EXPECT_EQ(draft.status(), "No rows.");
  draft.postings.push_back({.account = "1930", .amount = -559000});
  EXPECT_EQ(draft.remaining(), 559000);
  EXPECT_EQ(draft.status(), "Remaining 5590.00 (debit 0.00, credit 5590.00, amount 5590.00).");
  draft.postings.push_back({.account = "6540", .amount = 447200});
  EXPECT_EQ(draft.remaining(), 111800);
  draft.postings.push_back({.account = "2641", .amount = 111800});
  EXPECT_TRUE(draft.is_balanced());
  EXPECT_EQ(draft.status(), "Balanced. Enter saves.");
  EXPECT_EQ(debit(draft.postings), 559000);
  EXPECT_EQ(credit(draft.postings), 559000);
  auto const saved = draft.voucher("13", today);
  EXPECT_EQ(saved.series, "D");
  EXPECT_EQ(saved.number, "13");
  EXPECT_EQ(saved.event, (Event{"Telia", 559000, today})); // the amount without sign
  EXPECT_EQ(saved.registered, today);
  EXPECT_EQ(saved.postings.size(), 3u);
}

TEST(Draft, RowsAddingUpToZeroAreNotEnough) {
  Draft draft{{"Telia", 111800, today}, 1, "D", {}};
  draft.postings.push_back({.account = "2641", .amount = 22360});
  draft.postings.push_back({.account = "6212", .amount = -22360});
  EXPECT_EQ(draft.remaining(), 0);
  EXPECT_FALSE(draft.is_balanced());
  EXPECT_EQ(draft.status(), "Debit 223.60, credit 223.60, amount 1118.00: debit and credit must each reach the amount.");
  draft.postings.push_back({.account = "1930", .amount = -200000});
  EXPECT_EQ(draft.status(), "Debit 223.60, credit 2223.60, amount 1118.00: more than the amount.");
}

namespace {

Voucher posted(std::string series, std::string number, Date date, std::string text, Amount amount,
               std::vector<std::pair<std::string, Amount>> const& rows) {
  Voucher result{std::move(series), std::move(number), {std::move(text), amount, date}, std::nullopt, {}, {}};
  for (auto const& [account, row_amount] : rows) {
    result.postings.push_back({.account = account, .amount = row_amount});
  }
  return result;
}

Books telia_books() {
  Books books;
  books.accounts = {{"1930", "Företagskonto", "", "", ""},
                    {"2440", "Leverantörsskulder", "", "", ""},
                    {"2641", "Debiterad ingående moms", "", "", ""},
                    {"6212", "Mobiltelefon", "", "", ""}};
  books.years.push_back({Date{2026y, May, 1d}, Date{2027y, April, 30d}, {}, {}, false});
  auto& vouchers = books.years[0].vouchers;
  vouchers.push_back(posted("D", "3", Date{2026y, June, 20d}, "Telia faktura juni", 98700,
                            {{"2440", -98700}, {"6212", 78960}, {"2641", 19740}}));
  vouchers.push_back(posted("D", "9", Date{2026y, October, 22d}, "Telia faktura oktober", 111800,
                            {{"2440", -111800}, {"6212", 89440}, {"2641", 22360}}));
  vouchers.push_back(posted("E", "4", Date{2026y, July, 1d}, "Betalning Telia", 98700,
                            {{"2440", 98700}, {"1930", -98700}}));
  vouchers.push_back(posted("A", "7", Date{2026y, July, 2d}, "Telia faktura rättelse", 1000,
                            {{"1930", 1000}})); // one posting: no template
  vouchers.push_back(posted("X", "1", Date{2026y, July, 3d}, "Telia faktura", 1000,
                            {{"1930", 1000}, {"2440", -1000}})); // not a series offered
  return books;
}

std::vector<std::string> names(std::vector<Voucher const*> const& vouchers) {
  std::vector<std::string> result;
  for (auto const* voucher : vouchers) {
    result.push_back(voucher->series + " " + voucher->number);
  }
  return result;
}

} // namespace

TEST(FindTemplates, MostSharedWordsThenLatestAndOncePerPattern) {
  auto const books = telia_books();
  // D 3 posts like D 9, so only the latest of them shows
  EXPECT_EQ(names(find_templates(books, {"Telia faktura november", 124500, today})),
            (std::vector<std::string>{"D 9", "E 4"}));
  EXPECT_EQ(names(find_templates(books, {"Betalning Telia", 124500, today})), (std::vector<std::string>{"E 4", "D 9"}));
  EXPECT_EQ(names(find_templates(books, {"Telia faktura november", 124500, today}, 1)),
            (std::vector<std::string>{"D 9"}));
  EXPECT_TRUE(find_templates(books, {"Lunch", 124500, today}).empty());
}

TEST(FindTemplates, AnAmountOfTheSameSignIsBetter) {
  auto books = telia_books();
  books.years[0].vouchers.push_back(posted("D", "10", Date{2026y, June, 1d}, "Telia faktura kredit", -20000,
                                           {{"2440", 20000}, {"6212", -16000}, {"2641", -4000}}));
  EXPECT_EQ(names(find_templates(books, {"Telia faktura", 124500, today})).front(), "D 9");
  EXPECT_EQ(names(find_templates(books, {"Telia faktura", -124500, today})).front(), "D 10");
}

TEST(GuideOf, ScaledToTheAmountAndBalanced) {
  auto const books = telia_books();
  auto const& like = books.years[0].vouchers[1]; // D 9, 1118.00
  auto const guide = guide_of(books, like, 124500);
  ASSERT_EQ(guide.postings.size(), 3u);
  EXPECT_EQ(guide.postings[0].account, "2440");
  EXPECT_EQ(guide.postings[0].amount, -124500);
  EXPECT_EQ(guide.postings[1].amount, 99600); // 894.40 * 1245 / 1118 = 995.996
  EXPECT_EQ(guide.postings[2].amount, 24900); // what makes them add up to zero
  EXPECT_TRUE(guide.missing.empty());
  // A negative amount flips the postings
  EXPECT_EQ(guide_of(books, like, -124500).postings[0].amount, 124500);
}

TEST(GuideOf, LeavesOutRemovedPostingsAndUnknownAccounts) {
  auto const books = telia_books();
  auto like = books.years[0].vouchers[1];
  like.postings[1].account = "6213";
  like.postings.push_back({.kind = Posting::Kind::removed, .account = "1930", .amount = 5});
  auto const guide = guide_of(books, like, 111800);
  ASSERT_EQ(guide.postings.size(), 2u);
  EXPECT_EQ(guide.postings[1].account, "2641");
  EXPECT_EQ(guide.postings[1].amount, 111800); // the last row balances the rows left
  EXPECT_EQ(guide.missing, (std::vector<std::string>{"6213"}));
}

TEST(Draft, SuggestsTheGuideRowByRow) {
  auto const books = telia_books();
  Draft draft{{"Telia faktura november", 124500, today}, 0, "D", {}};
  EXPECT_FALSE(draft.suggestion());
  draft.guide = guide_of(books, books.years[0].vouchers[1], 124500).postings;
  EXPECT_EQ(draft.suggestion()->amount, -124500);
  draft.postings.push_back(*draft.suggestion());
  EXPECT_EQ(draft.suggestion()->account, "6212");
  draft.postings.push_back({.account = "6212", .amount = 100000}); // typed instead of the suggestion
  EXPECT_EQ(draft.suggestion()->account, "2641");
  EXPECT_EQ(draft.suggestion()->amount, 24500); // the last row: what remains
  draft.postings.push_back(*draft.suggestion());
  EXPECT_TRUE(draft.is_balanced());
  EXPECT_FALSE(draft.suggestion());
}

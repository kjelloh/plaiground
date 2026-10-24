#include "core/books/books.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace std::chrono;
using cratchit::books::Books;
using cratchit::books::FiscalYear;
using cratchit::books::Posting;
using cratchit::books::Voucher;

namespace {

cratchit::Date date(int y, unsigned m, unsigned d) {
  return year_month_day(year{y}, month{m}, day{d});
}

Voucher voucher(std::string number, cratchit::Date when, std::string account, cratchit::Amount amount,
                std::string other = "3010") {
  return {"A", std::move(number), {"Text, with \"quotes\"", amount < 0 ? -amount : amount, when}, date(2026, 1, 10), "", {
      {Posting::Kind::normal, std::move(account), {}, amount, {}, "", "", ""},
      {Posting::Kind::normal, std::move(other), {{"1", "100"}, {"6", "P 1"}}, -amount, when, "rad", "2", ""},
  }};
}

Books sample() {
  Books books;
  books.chart_type = "EUBAS97";
  books.company_id = "503072";
  books.address = {"Kim", "Street 1", "17141 Solna", "070"};
  books.accounts = {{"1930", "Bank", "", "7281", ""}, {"2099", "Result", "", "", ""},
                    {"3010", "Sales", "", "", ""},   {"6570", "Bank costs", "K", "", ""}};
  books.dimensions = {{"1", "Kostnadsställe", ""}};
  books.objects = {{"1", "100", "Sälj"}};
  FiscalYear y2025{date(2025, 1, 1), date(2025, 12, 31), {{"1930", 100000}, {"2099", -100000}}, {}};
  y2025.vouchers.push_back(voucher("1", date(2025, 2, 1), "1930", 50000));
  y2025.vouchers.push_back(voucher("2", date(2025, 3, 1), "6570", 1000, "1930"));
  y2025.vouchers.back().postings.push_back({Posting::Kind::removed, "6110", {}, 999, {}, "", "", "Kim"});
  FiscalYear y2026{date(2026, 1, 1), date(2026, 12, 31), {{"1930", 1}}, {}};
  y2026.vouchers.push_back(voucher("1", date(2026, 2, 1), "1930", 700));
  books.years = {y2025, y2026};
  return books;
}

std::string read_file(fs::path const& path) {
  std::ifstream in(path);
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

class BooksFiles : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_books_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path root;
};

} // namespace

TEST(Postings, SumsDoNotCountRemovedPostings) {
  cratchit::books::Postings const postings{
      {.account = "6570", .amount = 1000},
      {.account = "1930", .amount = -1000},
      {.kind = Posting::Kind::removed, .account = "6110", .amount = 999},
  };
  EXPECT_EQ(cratchit::books::debit(postings), 1000);
  EXPECT_EQ(cratchit::books::credit(postings), 1000);
  EXPECT_EQ(cratchit::books::sum(postings), 0);
}

TEST(Balances, CarriedFromTheYearBeforeOnlyForBalanceAccounts) {
  auto const all = cratchit::books::balances(sample());
  ASSERT_EQ(all.size(), 2u);
  EXPECT_FALSE(all[0].carried);
  EXPECT_EQ(all[0].opening.at("1930"), 100000);
  EXPECT_EQ(all[0].change.at("1930"), 50000 - 1000);
  EXPECT_EQ(all[0].change.at("3010"), -50000);
  EXPECT_EQ(all[0].change.count("6110"), 0u); // removed rows do not count
  EXPECT_EQ(all[0].closing.at("1930"), 149000);
  EXPECT_TRUE(all[1].carried);
  EXPECT_EQ(all[1].opening.at("1930"), 149000); // not the reported 0.01
  EXPECT_EQ(all[1].opening.at("2099"), -100000);
  EXPECT_EQ(all[1].opening.count("3010"), 0u); // result accounts open at zero
  EXPECT_EQ(all[1].closing.at("1930"), 149700);
}

TEST(Balances, AGapMeansTheReportedOpeningBalances) {
  auto books = sample();
  books.years[1].start = date(2026, 1, 2);
  auto const all = cratchit::books::balances(books);
  EXPECT_FALSE(all[1].carried);
  EXPECT_EQ(all[1].opening.at("1930"), 1);
}

TEST_F(BooksFiles, NoBooksReadsAsEmpty) {
  auto const books = cratchit::books::read_books(root);
  ASSERT_TRUE(books.has_value()) << books.error();
  EXPECT_TRUE(books->years.empty());
  EXPECT_TRUE(books->accounts.empty());
}

TEST_F(BooksFiles, WriteThenReadGivesTheSameBooks) {
  auto const books = sample();
  auto const written = cratchit::books::write_books(root, books);
  ASSERT_TRUE(written.has_value()) << written.error();
  EXPECT_EQ(written->size(), 10u);
  EXPECT_TRUE(written->front().created);
  EXPECT_EQ(written->front().file, fs::path(".cratchit") / "books" / "books.txt");
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  EXPECT_EQ(back->chart_type, books.chart_type);
  EXPECT_EQ(back->company_id, books.company_id);
  EXPECT_EQ(back->address, books.address);
  EXPECT_EQ(back->accounts, books.accounts);
  EXPECT_EQ(back->dimensions, books.dimensions);
  EXPECT_EQ(back->objects, books.objects);
  ASSERT_EQ(back->years.size(), 2u);
  for (std::size_t i = 0; i < 2; ++i) {
    EXPECT_EQ(back->years[i].start, books.years[i].start);
    EXPECT_EQ(back->years[i].end, books.years[i].end);
    EXPECT_EQ(back->years[i].reported_opening, books.years[i].reported_opening);
    EXPECT_EQ(back->years[i].vouchers, books.years[i].vouchers);
  }
}

TEST_F(BooksFiles, FileFormats) {
  ASSERT_TRUE(cratchit::books::write_books(root, sample()).has_value());
  auto const folder = root / ".cratchit" / "books";
  EXPECT_EQ(read_file(folder / "books.txt"), "chart_type = EUBAS97\ncompany_id = 503072\naddress.contact = Kim\n"
                                             "address.street = Street 1\naddress.postal = 17141 Solna\n"
                                             "address.phone = 070\nyear.2025-01-01 = 2025-12-31\n"
                                             "year.2026-01-01 = 2026-12-31\n");
  EXPECT_EQ(read_file(folder / "2025-01-01" / "opening.csv"), "account,amount\n1930,1000.00\n2099,-1000.00\n");
  EXPECT_EQ(read_file(folder / "2026-01-01" / "vouchers.csv"),
            "series,number,date,text,amount,registered,signature\n"
            "A,1,2026-02-01,\"Text, with \"\"quotes\"\"\",7.00,2026-01-10,\n");
  EXPECT_EQ(read_file(folder / "2026-01-01" / "postings.csv"),
            "series,number,kind,account,objects,amount,date,text,quantity,signature\n"
            "A,1,normal,1930,,7.00,,,,\n"
            "A,1,normal,3010,\"1,100,6,P 1\",-7.00,2026-02-01,rad,2,\n");
}

TEST_F(BooksFiles, OnlyChangedFilesAreWritten) {
  auto books = sample();
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  auto const again = cratchit::books::write_books(root, books);
  ASSERT_TRUE(again.has_value());
  EXPECT_TRUE(again->empty());
  books.years[1].vouchers.push_back(voucher("2", date(2026, 3, 1), "1930", 1));
  auto const changed = cratchit::books::write_books(root, books);
  ASSERT_TRUE(changed.has_value());
  ASSERT_EQ(changed->size(), 2u);
  EXPECT_EQ(changed->at(0).file, fs::path(".cratchit") / "books" / "2026-01-01" / "vouchers.csv");
  EXPECT_FALSE(changed->at(0).created);
}

TEST_F(BooksFiles, MalformedFileIsAnError) {
  ASSERT_TRUE(cratchit::books::write_books(root, sample()).has_value());
  std::ofstream(root / ".cratchit" / "books" / "2025-01-01" / "opening.csv") << "account,amount\n1930,1.234\n";
  auto const books = cratchit::books::read_books(root);
  ASSERT_FALSE(books.has_value());
  EXPECT_NE(books.error().find("'1.234' is not an amount"), std::string::npos) << books.error();
}

TEST_F(BooksFiles, MissingYearFileIsAnError) {
  ASSERT_TRUE(cratchit::books::write_books(root, sample()).has_value());
  fs::remove(root / ".cratchit" / "books" / "2026-01-01" / "vouchers.csv");
  auto const books = cratchit::books::read_books(root);
  ASSERT_FALSE(books.has_value());
  EXPECT_NE(books.error().find("Cannot read"), std::string::npos) << books.error();
}

TEST_F(BooksFiles, ClosedYearsAreWrittenAndReadBack) {
  auto books = sample();
  books.years[0].closed = true;
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  EXPECT_NE(read_file(root / ".cratchit" / "books" / "books.txt").find("closed.2025-01-01 = yes\n"), std::string::npos);
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  EXPECT_TRUE(back->years[0].closed);
  EXPECT_FALSE(back->years[1].closed);
}

TEST_F(BooksFiles, WaitingEventsAreKeptOnlyWhileThereAreAny) {
  auto books = sample();
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  auto const events = root / ".cratchit" / "books" / "2026-01-01" / "events.csv";
  EXPECT_FALSE(fs::exists(events));
  books.years[1].waiting.push_back({{"Telia, oktober", 111800, date(2026, 10, 20)}, date(2026, 10, 23)});
  auto const added = cratchit::books::write_books(root, books);
  ASSERT_TRUE(added.has_value()) << added.error();
  ASSERT_EQ(added->size(), 1u);
  EXPECT_EQ(added->at(0).file, fs::path(".cratchit") / "books" / "2026-01-01" / "events.csv");
  EXPECT_TRUE(added->at(0).created);
  EXPECT_EQ(read_file(events), "date,text,amount,noted\n2026-10-20,\"Telia, oktober\",1118.00,2026-10-23\n");
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  EXPECT_EQ(back->years[1].waiting, books.years[1].waiting);
  EXPECT_TRUE(back->years[0].waiting.empty());
  books.years[1].waiting.clear();
  auto const removed = cratchit::books::write_books(root, books);
  ASSERT_TRUE(removed.has_value()) << removed.error();
  ASSERT_EQ(removed->size(), 1u);
  EXPECT_TRUE(removed->at(0).removed);
  EXPECT_FALSE(fs::exists(events));
}

TEST_F(BooksFiles, TransactionsCsvIsReadAndBecomesPostingsCsv) {
  auto const books = sample();
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  auto const year = root / ".cratchit" / "books" / "2026-01-01";
  fs::rename(year / "postings.csv", year / "transactions.csv");
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  EXPECT_EQ(back->years[1].vouchers, books.years[1].vouchers);
  auto const written = cratchit::books::write_books(root, *back);
  ASSERT_TRUE(written.has_value()) << written.error();
  ASSERT_EQ(written->size(), 2u);
  EXPECT_EQ(written->at(0).file, fs::path(".cratchit") / "books" / "2026-01-01" / "postings.csv");
  EXPECT_TRUE(written->at(0).created);
  EXPECT_EQ(written->at(1).file, fs::path(".cratchit") / "books" / "2026-01-01" / "transactions.csv");
  EXPECT_TRUE(written->at(1).removed);
  EXPECT_TRUE(fs::exists(year / "postings.csv"));
  EXPECT_FALSE(fs::exists(year / "transactions.csv"));
}

TEST_F(BooksFiles, WaitingEventPostingsAreKeptOnlyWhileThereAreAny) {
  auto books = sample();
  books.years[1].waiting.push_back({{"Telia", 111800, date(2026, 10, 20)}, date(2026, 10, 23)});
  books.years[1].waiting.push_back({{"Till sparkonto", 5000000, date(2026, 1, 24)}, date(2026, 10, 24)});
  cratchit::books::Posting out;
  out.account = "1920";
  out.amount = -5000000;
  out.date = date(2026, 1, 24);
  out.text = "PG TILL SPAR";
  out.source = ".cratchit/statements/2026-10-24_1_pg.csv:12";
  auto in = out;
  in.account = "1930";
  in.amount = 5000000;
  in.source = ".cratchit/statements/2026-10-24_2_spar.csv:3";
  books.years[1].waiting[1].postings = {out, in};
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  auto const file = root / ".cratchit" / "books" / "2026-01-01" / "event_postings.csv";
  EXPECT_EQ(read_file(file), "event,account,amount,date,text,source\n"
                             "2,1920,-50000.00,2026-01-24,PG TILL SPAR,.cratchit/statements/2026-10-24_1_pg.csv:12\n"
                             "2,1930,50000.00,2026-01-24,PG TILL SPAR,.cratchit/statements/2026-10-24_2_spar.csv:3\n");
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  EXPECT_EQ(back->years[1].waiting, books.years[1].waiting);
  books.years[1].waiting.erase(books.years[1].waiting.begin() + 1);
  auto const removed = cratchit::books::write_books(root, books);
  ASSERT_TRUE(removed.has_value()) << removed.error();
  EXPECT_FALSE(fs::exists(file));
  std::ofstream(file) << "event,account,amount,date,text,source\n3,1920,1.00,,,\n";
  auto const bad = cratchit::books::read_books(root);
  ASSERT_FALSE(bad.has_value());
  EXPECT_NE(bad.error().find("event 3 is not in events.csv"), std::string::npos) << bad.error();
}

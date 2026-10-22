#include "core/books/books.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace std::chrono;
using cratchit::books::Books;
using cratchit::books::FiscalYear;
using cratchit::books::Transaction;
using cratchit::books::Voucher;

namespace {

cratchit::Date date(int y, unsigned m, unsigned d) {
  return year_month_day(year{y}, month{m}, day{d});
}

Voucher voucher(std::string number, cratchit::Date when, std::string account, cratchit::Amount amount,
                std::string other = "3010") {
  return {"A", std::move(number), when, "Text, with \"quotes\"", date(2026, 1, 10), "", {
      {Transaction::Kind::normal, std::move(account), {}, amount, {}, "", "", ""},
      {Transaction::Kind::normal, std::move(other), {{"1", "100"}, {"6", "P 1"}}, -amount, when, "rad", "2", ""},
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
  y2025.vouchers.back().transactions.push_back({Transaction::Kind::removed, "6110", {}, 999, {}, "", "", "Kim"});
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
            "series,number,date,text,registered,signature\nA,1,2026-02-01,\"Text, with \"\"quotes\"\"\",2026-01-10,\n");
  EXPECT_EQ(read_file(folder / "2026-01-01" / "transactions.csv"),
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

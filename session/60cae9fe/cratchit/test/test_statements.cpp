#include "core/books/statements.hpp"

#include "core/path_value.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

using namespace cratchit;
using namespace cratchit::books;
using namespace std::chrono;
namespace fs = std::filesystem;

namespace {

std::string const bom = "\xEF\xBB\xBF";

// Newest first, with saldo, as Nordea writes it
std::string const july = bom + "Datum;Belopp;Text;Saldo;\n"
                               "2026/07/23;2292,00;Skatt;8932,98;\n"
                               "2026/07/03;-3,70;AVGIFTER NORDEA;6640,98;\n";
// Overlaps july by one posting
std::string const august = bom + "Datum;Belopp;Text;Saldo;\n"
                                 "2026/08/26;-923,00;Fortnox;8009,98;\n"
                                 "2026/07/23;2292,00;Skatt;8932,98;\n";
// Starts where august ends, nothing in common
std::string const september = bom + "Datum;Belopp;Text;Saldo;\n"
                                    "2026/09/03;-1,85;AVGIFTER NORDEA;7696,88;\n"
                                    "2026/09/01;-311,25;LOOPIA;7698,73;\n";

statement::Document document_of(std::string const& text) {
  auto const document = statement::parse_statement(std::vector<std::uint8_t>(text.begin(), text.end()));
  EXPECT_TRUE(document.has_value()) << document.error();
  return document.value_or(statement::Document{});
}

Books books_2026() {
  Books books;
  books.accounts = {{"1920", "Plusgiro", "", "", ""}, {"1630", "Skattekonto", "", "", ""}};
  books.years.push_back({Date{2026y, May, 1d}, Date{2027y, April, 30d}, {}, {}, false});
  return books;
}

Voucher voucher_with(std::string account, Amount amount, Date date) {
  Voucher voucher{"A", "1", {"x", amount < 0 ? -amount : amount, date}, std::nullopt, {}, {}};
  voucher.postings.push_back({.account = std::move(account), .amount = amount});
  voucher.postings.push_back({.account = "2440", .amount = -amount});
  return voucher;
}

class Statements : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_statements_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root / "project");
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path file(std::string const& name, std::string const& text) const {
    auto const path = root / name;
    std::ofstream(path, std::ios::binary) << text;
    return path;
  }

  // Imports the file for the account, as the user would
  void import(std::string const& name, std::string const& text, std::string const& account,
              FiscalYear const* year = nullptr) {
    auto const imports = read_statement_imports(root / "project");
    ASSERT_TRUE(imports.has_value()) << imports.error();
    auto const plan = plan_statement(books_2026(), *imports, document_of(text), account, year);
    ASSERT_TRUE(save_statement(root / "project", file(name, text), plan, "2026-10-24").has_value());
  }

  std::vector<StatementImport> imports() const {
    auto imports = read_statement_imports(root / "project");
    EXPECT_TRUE(imports.has_value()) << imports.error();
    return imports.value_or(std::vector<StatementImport>{});
  }

  fs::path root;
};

} // namespace

TEST_F(Statements, NoneImportedYet) {
  EXPECT_TRUE(imports().empty());
}

TEST_F(Statements, SavedWithARecordAndReadBack) {
  import("july.csv", july, "1920");
  EXPECT_TRUE(fs::exists(root / "project" / ".cratchit" / "statements" / "2026-10-24_1_july.csv"));
  auto const record = read_path_values(root / "project" / ".cratchit" / "statements" / "2026-10-24_1_july.csv.txt");
  ASSERT_TRUE(record.has_value()) << record.error();
  EXPECT_EQ(format_path_values(*record), "imported = 2026-10-24\nsource = " + (root / "july.csv").string() +
                                             "\naccount = 1920\nfrom = 2026-05-01\nto = 2027-04-30\npostings = 2\nadded = 2\nknown = 0\n"
                                             "outside = 0\n");
  auto const read = imports();
  ASSERT_EQ(read.size(), 1u);
  EXPECT_EQ(read[0].stashed, fs::path(".cratchit") / "statements" / "2026-10-24_1_july.csv");
  EXPECT_EQ(read[0].account, "1920");
  EXPECT_EQ(read[0].from, (Date{2026y, May, 1d}));
  EXPECT_EQ(read[0].document.postings.size(), 2u);
}

TEST_F(Statements, KnownPostingsCountOnceAcrossFiles) {
  import("july.csv", july, "1920");
  import("august.csv", august, "1920");
  EXPECT_EQ(known_postings(imports(), "1920").size(), 3u);
  EXPECT_EQ(most_known(imports(), document_of(august)), 2u);
  EXPECT_EQ(most_known(imports(), document_of(september)), 0u);
  EXPECT_TRUE(known_postings(imports(), "1930").empty());
  // The same file again has nothing new
  auto const plan = plan_statement(books_2026(), imports(), document_of(august), "1920", nullptr);
  EXPECT_EQ(plan.known, 2u);
  EXPECT_TRUE(plan.added.empty());
}

TEST_F(Statements, AtAFiscalYearOnlyItsDaysCount) {
  auto books = books_2026();
  books.years.insert(books.years.begin(), {Date{2025y, May, 1d}, Date{2026y, April, 30d}, {}, {}, false});
  auto const spanning = document_of(bom + "Datum;Belopp;Text;Saldo;\n"
                                          "2026/05/02;-1,00;b;9,00;\n"
                                          "2026/04/30;-1,00;a;10,00;\n");
  auto const plan = plan_statement(books, {}, spanning, "1920", &books.years[1]);
  EXPECT_EQ(plan.from, (Date{2026y, May, 1d}));
  EXPECT_EQ(plan.outside, 1u);
  ASSERT_EQ(plan.added.size(), 1u);
  EXPECT_EQ(plan.added[0].text, "b");
  // At the project, outside every fiscal year; the days are those of the years
  auto const project = plan_statement(books, {}, spanning, "1920", nullptr);
  EXPECT_EQ(project.outside, 0u);
  EXPECT_EQ(project.from, (Date{2025y, May, 1d}));
  EXPECT_EQ(project.to, (Date{2027y, April, 30d}));
  EXPECT_EQ(plan_statement(books_2026(), {}, spanning, "1920", nullptr).outside, 1u);

  import("july.csv", july, "1920", &books_2026().years[0]);
  auto const read = imports();
  ASSERT_EQ(read.size(), 1u);
  EXPECT_EQ(read[0].to, (Date{2027y, April, 30d}));
}

TEST_F(Statements, GuessByBookPostingsFirst) {
  auto books = books_2026();
  books.years[0].vouchers.push_back(voucher_with("1920", 229200, Date{2026y, July, 23d}));
  books.years[0].vouchers.push_back(voucher_with("1630", -370, Date{2026y, July, 3d}));
  books.years[0].vouchers.push_back(voucher_with("1920", -370, Date{2026y, July, 3d}));
  auto const guess = guess_statement_account(books, {}, document_of(july));
  ASSERT_TRUE(guess);
  EXPECT_EQ(guess->account, "1920");
  EXPECT_EQ(guess->reason, "2 of 2 postings match postings on 1920 in the books");
  // A tie suggests nothing
  books.years[0].vouchers.push_back(voucher_with("1630", 229200, Date{2026y, July, 23d}));
  EXPECT_FALSE(guess_statement_account(books, {}, document_of(july)));
}

TEST_F(Statements, ThenByImportedStatementsThenBySaldo) {
  import("july.csv", july, "1920");
  auto guess = guess_statement_account(books_2026(), imports(), document_of(august));
  ASSERT_TRUE(guess);
  EXPECT_EQ(guess->account, "1920");
  EXPECT_EQ(guess->reason, "1 of 2 postings are in statements imported for 1920");
  import("august.csv", august, "1920");
  guess = guess_statement_account(books_2026(), imports(), document_of(september));
  ASSERT_TRUE(guess);
  EXPECT_EQ(guess->reason, "the latest statement imported for 1920 ends with saldo 8009.98, which this one starts with");
  EXPECT_FALSE(guess_statement_account(books_2026(), {}, document_of(september)));
}

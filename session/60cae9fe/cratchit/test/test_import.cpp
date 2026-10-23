#include "core/books/import.hpp"

#include "core/path_value.hpp"
#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace std::chrono;
using cratchit::books::Books;
using cratchit::books::Identity;
using cratchit::books::ImportPlan;
using cratchit::books::plan_import;
using sie_test::balanced_balances;
using sie_test::parse_sie;
using sie_test::sale_and_cost;
using sie_test::sie_file;

namespace {

bool has(std::vector<std::string> const& texts, std::string const& text) {
  return std::ranges::find(texts, text) != texts.end();
}

std::string all(std::vector<std::string> const& texts) {
  std::string result;
  for (auto const& text : texts) {
    result += text + "\n";
  }
  return result;
}

Identity const fresh{"Test", ""};
Identity const known{"Test AB", "556000-0001"};

// The books after importing the files in order, each of which must import without errors
Books imported(std::vector<std::string> const& files) {
  Books books;
  Identity identity = fresh;
  for (auto const& file : files) {
    auto const plan = plan_import(books, identity, parse_sie(file));
    EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
    books = plan.books;
    identity = plan.identity;
  }
  return books;
}

std::string file_2025() {
  return sie_file(2025, balanced_balances(), sale_and_cost(2025));
}

// The 2026 file: opens with 2025's closing balances, sells 100 more
std::string file_2026(std::string_view opening_1930 = "1490.00") {
  return sie_file(2026,
                  std::format("#UB -1 1930 1490.00\n#UB -1 2099 -1000.00\n#RES -1 3010 -500\n#RES -1 6570 10\n"
                              "#IB 0 1930 {0}\n#IB 0 2099 -1000.00\n#UB 0 1930 {1}\n#UB 0 2099 -1000.00\n"
                              "#RES 0 3010 -100\n",
                              opening_1930, cratchit::format_amount(*cratchit::parse_amount(opening_1930) + 10000)),
                  "#VER A 1 20260110 \"Sale\"\n{\n#TRANS 1930 {} 100\n#TRANS 3010 {} -100\n}\n");
}

} // namespace

TEST(Import, FirstImportSetsIdentityAndAddsTheYear) {
  auto const plan = plan_import({}, fresh, parse_sie(file_2025()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(plan.identity_changed);
  EXPECT_EQ(plan.identity.orgnr, "556000-0001");
  EXPECT_EQ(plan.identity.name, "Test AB");
  EXPECT_EQ(plan.notes, (std::vector<std::string>{"The project gets organisation number 556000-0001 (#ORGNR)",
                                                  "The project's name changes from \"Test\" to \"Test AB\" (#FNAMN)"}));
  EXPECT_EQ(plan.year_start, year_month_day(year{2025}, January, day{1}));
  EXPECT_EQ(plan.vouchers_added, 2u);
  EXPECT_EQ(plan.accounts_added, 4u);
  ASSERT_EQ(plan.books.years.size(), 1u);
  EXPECT_EQ(plan.books.years[0].vouchers.size(), 2u);
  EXPECT_EQ(plan.books.years[0].reported_opening.at("1930"), 100000);
  EXPECT_EQ(plan.books.chart_type, "EUBAS97");
}

TEST(Import, OtherOrganisationOrNameIsAnError) {
  auto const plan = plan_import({}, {"Other AB", "556999-9999"}, parse_sie(file_2025()));
  EXPECT_TRUE(has(plan.errors, "The file is for organisation number 556000-0001, but the project Other AB has "
                               "556999-9999"));
  EXPECT_TRUE(has(plan.errors, "The file's company name \"Test AB\" differs from the project's name \"Other AB\""));
  EXPECT_FALSE(plan.identity_changed);
}

TEST(Import, FileErrorsAreIncluded) {
  auto const plan = plan_import({}, fresh, parse_sie(sie_file(2025, "", "#VER A 1 20250101 \"\"\n{\n#TRANS 1930 {} 1\n}\n")));
  EXPECT_TRUE(has(plan.errors, "line 16: voucher A 1 does not balance: its amounts add up to 1.00"));
}

TEST(Import, IdenticalVouchersAreSkipped) {
  auto const books = imported({file_2025()});
  auto const plan = plan_import(books, known, parse_sie(file_2025()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_EQ(plan.vouchers_added, 0u);
  EXPECT_EQ(plan.vouchers_known, 2u);
  EXPECT_FALSE(plan.identity_changed);
  EXPECT_EQ(plan.books.years[0].vouchers, books.years[0].vouchers);
}

TEST(Import, AnImportedVoucherCarriesItsDebitTotalAsAmount) {
  auto const books = imported({sie_file(2025, "", "#VER A 1 20250201 \"Sale\" 20250205\n{\n#TRANS 1930 {} 500.00\n"
                                                  "#TRANS 3010 {} -400.00\n#TRANS 2099 {} -100.00\n}\n")});
  ASSERT_EQ(books.years.at(0).vouchers.size(), 1u);
  EXPECT_EQ(books.years[0].vouchers[0].event.heading, "Sale");
  EXPECT_EQ(books.years[0].vouchers[0].event.amount, 50000);
}

TEST(Import, NewVouchersOfAKnownYearAreAdded) {
  auto const books = imported({sie_file(2025, "", "#VER A 1 20250201 \"Sale\" 20250205\n{\n#TRANS 1930 {} 500.00\n"
                                                  "#TRANS 3010 {} -500.00\n}\n")});
  auto const plan = plan_import(books, known, parse_sie(file_2025()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_EQ(plan.vouchers_added, 1u);
  EXPECT_EQ(plan.vouchers_known, 1u);
  EXPECT_EQ(plan.books.years[0].vouchers.size(), 2u);
}

TEST(Import, AChangedVoucherIsAnError) {
  auto const books = imported({file_2025()});
  auto changed = file_2025();
  changed.replace(changed.find("\"Bank cost\""), 11, "\"Bank fee\"");
  auto const plan = plan_import(books, known, parse_sie(changed));
  EXPECT_TRUE(has(plan.errors, "Voucher A 2 in the file differs from the one in the books (text \"Bank cost\" in the "
                               "books, \"Bank fee\" in the file). The books never change a voucher"))
      << all(plan.errors);
}

TEST(Import, VouchersMissingFromTheFileAreNoted) {
  auto const books = imported({file_2025()});
  auto const plan = plan_import(books, known,
                                parse_sie(sie_file(2025, "", "#VER A 1 20250201 \"Sale\" 20250205\n{\n#TRANS 1930 {} "
                                                             "500.00\n#TRANS 3010 {} -500.00\n}\n")));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "1 voucher of 2025-01-01 - 2025-12-31 in the books is not in the file: A 2")) << all(plan.notes);
  EXPECT_EQ(plan.books.years[0].vouchers.size(), 2u); // nothing is removed
}

TEST(Import, OverlappingOrDifferentlyEndingYearIsAnError) {
  auto const books = imported({file_2025()});
  auto shifted = file_2025();
  shifted.replace(shifted.find("#RAR 0 20250101 20251231"), 24, "#RAR 0 20250701 20260630");
  auto const overlap = plan_import(books, known, parse_sie(shifted));
  EXPECT_TRUE(has(overlap.errors, "The file's fiscal year 2025-07-01 - 2026-06-30 overlaps the year 2025-01-01 - "
                                  "2025-12-31 in the books"))
      << all(overlap.errors);
  auto ending = file_2025();
  ending.replace(ending.find("#RAR 0 20250101 20251231"), 24, "#RAR 0 20250101 20251130");
  auto const other_end = plan_import(books, known, parse_sie(ending));
  EXPECT_TRUE(has(other_end.errors, "The file's fiscal year 2025-01-01 - 2025-11-30 ends otherwise than the one in "
                                    "the books (2025-01-01 - 2025-12-31)"))
      << all(other_end.errors);
}

TEST(Import, YearsInEitherOrderGiveTheSameBooks) {
  auto const forward = imported({file_2025(), file_2026()});
  auto const backward = imported({file_2026(), file_2025()});
  ASSERT_EQ(forward.years.size(), 2u);
  ASSERT_EQ(backward.years.size(), 2u);
  EXPECT_EQ(forward.years[0].start, backward.years[0].start);
  EXPECT_EQ(forward.years[1].vouchers, backward.years[1].vouchers);
  EXPECT_EQ(forward.accounts, backward.accounts);
  auto const a = cratchit::books::balances(forward);
  auto const b = cratchit::books::balances(backward);
  EXPECT_EQ(a[1].closing, b[1].closing);
  EXPECT_EQ(a[1].closing.at("1930"), 159000);
}

TEST(Import, ConsistentNextYearHasNoBalanceNotes) {
  auto const books = imported({file_2025()});
  auto const plan = plan_import(books, known, parse_sie(file_2026()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(plan.notes.empty()) << all(plan.notes);
}

TEST(Import, OtherOpeningBalanceIsNotedAndCratchitsOwnIsUsed) {
  auto const books = imported({file_2025()});
  // The file opens 1930 with 1500, cratchit's 2025 closing is 1490: 10 off, as voucher A 2 (bank cost 10)
  auto const plan = plan_import(books, known, parse_sie(file_2026("1500.00")));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "Opening balance 2026-01-01, 1930 Bank: the file says 1500.00, cratchit carries 1490.00 "
                              "from its closing balance 2025-12-31 (difference -10.00) and uses its own. Possibly from "
                              "voucher A 2 2025-03-01 \"Bank cost\""))
      << all(plan.notes);
  auto const after = cratchit::books::balances(plan.books);
  EXPECT_EQ(after[1].opening.at("1930"), 149000);
}

TEST(Import, EarlierYearInTheFileIsCheckedAgainstTheBooks) {
  auto const books = imported({file_2025()});
  auto file = file_2026();
  file.replace(file.find("#UB -1 1930 1490.00"), 19, "#UB -1 1930 1480.00");
  file.replace(file.find("#RES -1 6570 10"), 15, "#RES -1 6570 20");
  auto const plan = plan_import(books, known, parse_sie(file));
  EXPECT_TRUE(has(plan.notes, "Closing balance 2025-12-31, 1930 Bank: the file says 1480.00, cratchit's books give "
                              "1490.00 (difference 10.00). Possibly from voucher A 2 2025-03-01 \"Bank cost\""))
      << all(plan.notes);
  EXPECT_TRUE(has(plan.notes, "Result 2025-01-01 - 2025-12-31, 6570 Bank costs: the file says 20.00, cratchit's books "
                              "give 10.00 (difference -10.00). Possibly from voucher A 2 2025-03-01 \"Bank cost\""))
      << all(plan.notes);
}

TEST(Import, AnEarlierYearChangesTheNextYearsOpeningBalances) {
  auto const books = imported({file_2026("1500.00")});
  auto const plan = plan_import(books, known, parse_sie(file_2025()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "Opening balance 2026-01-01, 1930 Bank: changes from 1500.00 to 1490.00, carried from "
                              "the closing balance 2025-12-31"))
      << all(plan.notes);
}

TEST(Import, AccountChangesOnlyFromTheLatestYear) {
  auto renamed = [](std::string file) {
    file.replace(file.find("#KONTO 1930 \"Bank\""), 18, "#KONTO 1930 \"Bank account\"");
    return file;
  };
  // The 2025 file renames 1930 but 2026 is in the books: the name stays
  auto const newer = imported({file_2026()});
  auto const older = plan_import(newer, known, parse_sie(renamed(file_2025())));
  EXPECT_EQ(older.books.account("1930")->name, "Bank");
  EXPECT_EQ(older.accounts_changed, 0u);
  // The 2026 file renames it with 2025 in the books: the name changes, noted
  auto const latest = plan_import(imported({file_2025()}), known, parse_sie(renamed(file_2026())));
  EXPECT_EQ(latest.books.account("1930")->name, "Bank account");
  EXPECT_EQ(latest.accounts_changed, 1u);
  EXPECT_TRUE(has(latest.notes, "Account 1930: name \"Bank\" becomes \"Bank account\"")) << all(latest.notes);
}

namespace {

class SaveImport : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_import_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root / "project");
    source = root / "export.se";
    std::ofstream(source, std::ios::binary) << file_2025();
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path root;
  fs::path source;
};

} // namespace

TEST_F(SaveImport, WritesBooksStashesTheFileAndRecordsTheImport) {
  auto const plan = plan_import({}, fresh, parse_sie(file_2025()));
  cratchit::books::ImportSource const from{source, "CP437", "Test 1.0", year_month_day(year{2026}, October, day{19}),
                                           "2026-10-22"};
  auto const saved = cratchit::books::save_import(root / "project", plan, from);
  ASSERT_TRUE(saved.has_value()) << saved.error();
  EXPECT_EQ(saved->written.size(), 7u);
  EXPECT_EQ(saved->stashed, fs::path(".cratchit") / "imports" / "2026-10-22_1_export.se");
  EXPECT_EQ(saved->record, fs::path(".cratchit") / "imports" / "2026-10-22_1_export.se.txt");
  std::ifstream in(root / "project" / saved->stashed, std::ios::binary);
  std::ostringstream copy;
  copy << in.rdbuf();
  EXPECT_EQ(copy.str(), file_2025());
  auto const record = cratchit::read_path_values(root / "project" / saved->record);
  ASSERT_TRUE(record.has_value()) << record.error();
  EXPECT_EQ(cratchit::format_path_values(*record), "imported = 2026-10-22\nsource = " + source.string() +
                                                       "\nencoding = CP437\nprogram = Test 1.0\ngenerated = 2026-10-19\n"
                                                       "company = Test AB\norgnr = 556000-0001\n"
                                                       "year = 2025-01-01 - 2025-12-31\nvouchers_added = 2\n"
                                                       "vouchers_known = 0\nnotes = 2\n");
  auto const books = cratchit::books::read_books(root / "project");
  ASSERT_TRUE(books.has_value()) << books.error();
  EXPECT_EQ(books->years.size(), 1u);
  // The same file again the same day gets the next number
  auto const again = cratchit::books::save_import(root / "project", plan, from);
  ASSERT_TRUE(again.has_value()) << again.error();
  EXPECT_EQ(again->stashed, fs::path(".cratchit") / "imports" / "2026-10-22_2_export.se");
  EXPECT_TRUE(again->written.empty());
}

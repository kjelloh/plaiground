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

TEST(Import, AnotherRegistrationDateIsAnError) {
  auto const books = imported({file_2025()});
  auto changed = file_2025();
  changed.replace(changed.find("\"Sale\" 20250205"), 15, "\"Sale\" 20250206");
  auto const plan = plan_import(books, known, parse_sie(changed));
  EXPECT_TRUE(has(plan.errors, "Voucher A 1 in the file differs from the one in the books (the registration date "
                               "differs). A voucher in the books may only change by struck (#BTRANS) and added "
                               "(#RTRANS) rows"))
      << all(plan.errors);
}

// The 2025 file with voucher A 1 registered a day later, which the history rule refuses
std::string registered_later() {
  auto file = file_2025();
  file.replace(file.find("\"Sale\" 20250205"), 15, "\"Sale\" 20250206");
  return file;
}

cratchit::books::VoucherKey const a1{2025y / January / 1, "A", "1"};

TEST(Import, AnImportedVoucherThatDiffersCanBeForced) {
  auto const books = imported({file_2025()});
  auto const refused = plan_import(books, known, parse_sie(registered_later()), {{a1}, false});
  EXPECT_EQ(refused.errors.size(), 1u) << all(refused.errors);
  EXPECT_EQ(refused.forceable, 1u);
  EXPECT_TRUE(refused.can_force());
  auto const forced = plan_import(books, known, parse_sie(registered_later()), {{a1}, true});
  EXPECT_TRUE(forced.errors.empty()) << all(forced.errors);
  EXPECT_EQ(forced.vouchers_forced, 1u);
  EXPECT_TRUE(has(forced.notes, "Voucher A 1 is forced: the file's replaces the one in the books (the registration "
                                "date differs)"))
      << all(forced.notes);
  ASSERT_EQ(forced.forced.size(), 1u);
  EXPECT_EQ(forced.forced[0], "A 1 in fiscal year 2025-01-01 - 2025-12-31: in the books 2025-02-01 \"Sale\" registered "
                              "2025-02-05: row 1930 500.00, row 3010 -500.00; in the file 2025-02-01 \"Sale\" "
                              "registered 2025-02-06: row 1930 500.00, row 3010 -500.00");
  EXPECT_EQ(forced.books.years[0].vouchers[0].registered, 2025y / February / 6);
  EXPECT_FALSE(cratchit::books::nothing_to_import(books, forced));
}

TEST(Import, NotForcedInAClosedYearOrOverAnEnteredVoucher) {
  auto books = imported({file_2025()});
  // Not imported: entered in cratchit, another event with the same number
  auto const entered = plan_import(books, known, parse_sie(registered_later()), {{}, true});
  EXPECT_EQ(entered.errors.size(), 1u);
  EXPECT_EQ(entered.forceable, 0u);
  EXPECT_FALSE(entered.can_force());
  books.years[0].closed = true;
  auto const closed = plan_import(books, known, parse_sie(registered_later()), {{a1}, true});
  EXPECT_EQ(closed.errors.size(), 1u);
  EXPECT_EQ(closed.vouchers_forced, 0u);
  EXPECT_FALSE(closed.can_force());
}

TEST(Import, NoForceWithOtherErrors) {
  auto const books = imported({file_2025()});
  auto const file = registered_later() + "#VER A 3 20250401 \"Odd\"\r\n{\r\n#TRANS 1930 {} 1\r\n}\r\n";
  auto const plan = plan_import(books, known, parse_sie(file), {{a1}, false});
  EXPECT_EQ(plan.forceable, 1u);
  EXPECT_GT(plan.errors.size(), 1u);
  EXPECT_FALSE(plan.can_force());
}

TEST(Import, AnEditedTextOrDateChangesAVoucher) {
  auto const books = imported({file_2025()});
  auto text = file_2025();
  text.replace(text.find("\"Bank cost\""), 11, "\"Bank fee\"");
  auto const by_text = plan_import(books, known, parse_sie(text));
  EXPECT_TRUE(by_text.errors.empty()) << all(by_text.errors);
  EXPECT_TRUE(has(by_text.notes, "Voucher A 2 changes: text \"Bank cost\" to \"Bank fee\"")) << all(by_text.notes);
  EXPECT_EQ(by_text.vouchers_changed, 1u);
  EXPECT_EQ(by_text.books.years[0].vouchers[1].event.heading, "Bank fee");
  auto date = file_2025();
  date.replace(date.find("A 2 20250301"), 12, "A 2 20250303");
  auto const by_date = plan_import(books, known, parse_sie(date));
  EXPECT_TRUE(by_date.errors.empty()) << all(by_date.errors);
  EXPECT_TRUE(has(by_date.notes, "Voucher A 2 changes: date 2025-03-01 to 2025-03-03")) << all(by_date.notes);
  EXPECT_EQ(by_date.books.years[0].vouchers[1].event.date, 2025y / March / 3);
}

// The 2025 file with the given rows (CRLF lines) in voucher A 2 instead of 6570 10 and 1930 -10
std::string with_a2_rows(std::string_view rows) {
  auto file = file_2025();
  std::string_view const original = "#TRANS 6570 {} 10\r\n#TRANS 1930 {} -10\r\n";
  file.replace(file.find(original), original.size(), rows);
  return file;
}

// A 2 with both rows struck and new ones added for 12, as Fortnox writes a changed voucher
std::string const struck_and_added = "#BTRANS 6570 {} 10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                     "#BTRANS 1930 {} -10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                     "#RTRANS 6570 {} 12 \"\" \"\" \"\" \"Kjell\"\r\n#TRANS 6570 {} 12\r\n"
                                     "#RTRANS 1930 {} -12 \"\" \"\" \"\" \"Kjell\"\r\n#TRANS 1930 {} -12\r\n";

TEST(Import, StruckAndAddedRowsChangeAVoucher) {
  auto const books = imported({file_2025()});
  auto const plan = plan_import(books, known, parse_sie(with_a2_rows(struck_and_added)));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "Voucher A 2 changes: 2 rows struck, 2 added (signed Kjell)")) << all(plan.notes);
  EXPECT_EQ(plan.vouchers_changed, 1u);
  EXPECT_EQ(plan.vouchers_known, 1u);
  auto const& a2 = plan.books.years[0].vouchers[1];
  ASSERT_EQ(a2.postings.size(), 4u);
  EXPECT_EQ(a2.postings[0].kind, cratchit::books::Posting::Kind::removed);
  EXPECT_EQ(a2.postings[2].kind, cratchit::books::Posting::Kind::added);
  EXPECT_EQ(a2.event.amount, 1200);
  // The same file again: nothing to import
  auto const again = plan_import(plan.books, known, parse_sie(with_a2_rows(struck_and_added)));
  EXPECT_TRUE(again.errors.empty()) << all(again.errors);
  EXPECT_EQ(again.vouchers_changed, 0u);
  EXPECT_TRUE(cratchit::books::nothing_to_import(plan.books, again));
}

TEST(Import, AnEditedDateWithStruckAndAddedRows) {
  auto const books = imported({file_2025()});
  auto file = with_a2_rows(struck_and_added);
  file.replace(file.find("A 2 20250301"), 12, "A 2 20250303");
  auto const plan = plan_import(books, known, parse_sie(file));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "Voucher A 2 changes: date 2025-03-01 to 2025-03-03, 2 rows struck, 2 added (signed "
                              "Kjell)"))
      << all(plan.notes);
}

TEST(Import, AnAddedRowMayBeStruckLater) {
  auto const books = imported({file_2025(), with_a2_rows(struck_and_added)});
  auto const plan = plan_import(books, known,
                                parse_sie(with_a2_rows("#BTRANS 6570 {} 10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                                       "#BTRANS 1930 {} -10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                                       "#BTRANS 6570 {} 12 \"\" \"\" \"\" \"Anna\"\r\n"
                                                       "#BTRANS 1930 {} -12 \"\" \"\" \"\" \"Anna\"\r\n"
                                                       "#RTRANS 6570 {} 11 \"\" \"\" \"\" \"Anna\"\r\n"
                                                       "#TRANS 6570 {} 11\r\n"
                                                       "#RTRANS 1930 {} -11 \"\" \"\" \"\" \"Anna\"\r\n"
                                                       "#TRANS 1930 {} -11\r\n")));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "Voucher A 2 changes: 2 rows struck, 2 added (signed Anna)")) << all(plan.notes);
  EXPECT_EQ(plan.books.years[0].vouchers[1].event.amount, 1100);
}

TEST(Import, OtherChangesToAVoucherAreErrors) {
  auto const books = imported({file_2025()});
  auto const prefix = std::string{"Voucher A 2 in the file differs from the one in the books ("};
  auto const suffix =
      std::string{"). A voucher in the books may only change by struck (#BTRANS) and added (#RTRANS) rows"};
  auto const changed_row = plan_import(books, known, parse_sie(with_a2_rows("#TRANS 6570 {} 12\r\n#TRANS 1930 {} -12\r\n")));
  EXPECT_TRUE(has(changed_row.errors, prefix + "row 1 is row 6570 10.00 in the books, row 6570 12.00 in the file; a "
                                               "row may only be struck" + suffix))
      << all(changed_row.errors);
  auto const new_row = plan_import(books, known,
                                   parse_sie(with_a2_rows("#TRANS 6570 {} 10\r\n#TRANS 1930 {} -10\r\n"
                                                          "#TRANS 6570 {} 1\r\n#TRANS 1930 {} -1\r\n")));
  EXPECT_TRUE(has(new_row.errors, prefix + "row 3 (row 6570 1.00) is new in the file but not added (#RTRANS)" + suffix))
      << all(new_row.errors);
  // A struck row may not come back
  auto const changed = imported({file_2025(), with_a2_rows(struck_and_added)});
  auto const unstruck = plan_import(changed, known,
                                    parse_sie(with_a2_rows("#TRANS 6570 {} 10\r\n#TRANS 1930 {} -10\r\n"
                                                           "#RTRANS 6570 {} 12 \"\" \"\" \"\" \"Kjell\"\r\n"
                                                           "#TRANS 6570 {} 12\r\n"
                                                           "#RTRANS 1930 {} -12 \"\" \"\" \"\" \"Kjell\"\r\n"
                                                           "#TRANS 1930 {} -12\r\n")));
  EXPECT_TRUE(has(unstruck.errors, prefix + "row 1 is struck in the books (removed row 6570 10.00), the file has row "
                                            "6570 10.00" + suffix))
      << all(unstruck.errors);
  // Nor a row be left out
  auto const fewer = plan_import(changed, known, parse_sie(file_2025()));
  EXPECT_TRUE(has(fewer.errors, prefix + "4 rows in the books, 2 in the file; a row may be struck, not left out" + suffix))
      << all(fewer.errors);
  EXPECT_EQ(changed_row.vouchers_changed, 0u);
}

TEST(Import, AChangeInAClosedYearIsAnError) {
  auto books = imported({file_2025()});
  books.years[0].closed = true;
  auto const plan = plan_import(books, known, parse_sie(with_a2_rows(struck_and_added)));
  EXPECT_TRUE(has(plan.errors, "Voucher A 2 in the file changes the one in the books (2 rows struck, 2 added (signed "
                               "Kjell)), but fiscal year 2025-01-01 - 2025-12-31 is closed. Open the year to import "
                               "the change"))
      << all(plan.errors);
  EXPECT_EQ(plan.vouchers_changed, 0u);
  // Also an edited text
  auto text = file_2025();
  text.replace(text.find("\"Bank cost\""), 11, "\"Bank fee\"");
  auto const by_text = plan_import(books, known, parse_sie(text));
  EXPECT_TRUE(has(by_text.errors, "Voucher A 2 in the file changes the one in the books (text \"Bank cost\" to \"Bank "
                                  "fee\"), but fiscal year 2025-01-01 - 2025-12-31 is closed. Open the year to import "
                                  "the change"))
      << all(by_text.errors);
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
                                                       "vouchers_changed = 0\nvouchers_known = 0\nnotes = 2\n");
  auto const books = cratchit::books::read_books(root / "project");
  ASSERT_TRUE(books.has_value()) << books.error();
  EXPECT_EQ(books->years.size(), 1u);
  // The same file again the same day gets the next number
  auto const again = cratchit::books::save_import(root / "project", plan, from);
  ASSERT_TRUE(again.has_value()) << again.error();
  EXPECT_EQ(again->stashed, fs::path(".cratchit") / "imports" / "2026-10-22_2_export.se");
  EXPECT_TRUE(again->written.empty());
}

TEST_F(SaveImport, ImportedVouchersAreThoseOfTheStashedFiles) {
  auto const none = cratchit::books::imported_vouchers(root / "project");
  ASSERT_TRUE(none.has_value()) << none.error();
  EXPECT_TRUE(none->empty());
  auto const plan = plan_import({}, fresh, parse_sie(file_2025()));
  cratchit::books::ImportSource const from{source, "CP437", "Test 1.0", std::nullopt, "2026-10-22"};
  ASSERT_TRUE(cratchit::books::save_import(root / "project", plan, from).has_value());
  auto const keys = cratchit::books::imported_vouchers(root / "project");
  ASSERT_TRUE(keys.has_value()) << keys.error();
  cratchit::Date const start = year_month_day(year{2025}, January, day{1});
  EXPECT_EQ(*keys, (std::set<cratchit::books::VoucherKey>{{start, "A", "1"}, {start, "A", "2"}}));
}

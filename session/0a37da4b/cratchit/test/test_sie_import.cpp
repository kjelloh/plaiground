#include "core/sie_import.hpp"

#include "core/fiscal_year.hpp"
#include "core/path_value.hpp"
#include "core/sie_project.hpp"
#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using sie_test::balanced_balances;
using sie_test::sale_and_cost;
using sie_test::sie_file;

namespace {

// A project created from Test AB's 2025 file (vouchers A 1 and A 2), in a folder of its own, removed afterwards
class SieImportTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_sie_import_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
    auto const plan = cratchit::plan_project_from_sie(write("start.se", sie_file(2025, balanced_balances(), sale_and_cost(2025))));
    ASSERT_TRUE(plan) << plan.error();
    auto created = cratchit::create_project_from_sie(*plan, "2026-11-09");
    ASSERT_TRUE(created) << created.error();
    project = *created;
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path write(std::string const& name, std::string const& text) const {
    std::ofstream(root / name, std::ios::binary) << text;
    return root / name;
  }

  static std::string sale(int year, int number, std::string const& day) {
    return std::format("#VER A {1} {0}{2} \"Sale\"\r\n{{\r\n#TRANS 1930 {{}} 100\r\n#TRANS 3010 {{}} -100\r\n}}\r\n", year,
                       number, day);
  }

  fs::path root;
  cratchit::Project project;
};

} // namespace

TEST_F(SieImportTest, NewYearIsPlannedThenImportedAndTheFileMoved) {
  auto const file = write("next.se", sie_file(2026, "", sale(2026, 1, "0110")));
  auto const plan = cratchit::plan_sie_import(project, file);
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_TRUE(plan->new_year);
  EXPECT_EQ(plan->import.vouchers_added, 1u);
  EXPECT_EQ(cratchit::list_fiscal_years(project)->size(), 1u); // nothing written by the plan
  auto const imported = cratchit::import_sie(project, *plan);
  ASSERT_TRUE(imported) << imported.error();
  EXPECT_EQ(cratchit::list_fiscal_years(project),
            (std::vector<cratchit::FiscalYear>{{"2026-01-01", "2026-12-31"}, {"2025-01-01", "2025-12-31"}}));
  EXPECT_EQ(imported->moved_to, project.folder / "importerade_sie_filer" / "next.se");
  EXPECT_TRUE(fs::exists(imported->moved_to));
  EXPECT_FALSE(fs::exists(file));
}

TEST_F(SieImportTest, NewVouchersAreAddedToAYearInTheBooks) {
  auto const file = write("more.se", sie_file(2025, balanced_balances(), sale_and_cost(2025) + sale(2025, 3, "0401")));
  auto const plan = cratchit::plan_sie_import(project, file);
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_FALSE(plan->new_year);
  EXPECT_EQ(plan->import.vouchers_added, 1u);
  EXPECT_EQ(plan->import.vouchers_known, 2u);
  ASSERT_TRUE(cratchit::import_sie(project, *plan));
  auto const books = cratchit::books::read_books(project.folder);
  ASSERT_TRUE(books) << books.error();
  EXPECT_EQ(books->years[0].vouchers.size(), 3u);
}

TEST_F(SieImportTest, ASecondFileOfTheSameNameGetsANumber) {
  fs::create_directories(project.folder / "importerade_sie_filer");
  std::ofstream(project.folder / "importerade_sie_filer" / "next.se") << "earlier\n";
  auto const plan = cratchit::plan_sie_import(project, write("next.se", sie_file(2026, "", sale(2026, 1, "0110"))));
  ASSERT_TRUE(plan) << plan.error();
  auto const imported = cratchit::import_sie(project, *plan);
  ASSERT_TRUE(imported) << imported.error();
  EXPECT_EQ(imported->moved_to, project.folder / "importerade_sie_filer" / "next_2.se");
}

TEST_F(SieImportTest, NothingNewIsRefused) {
  auto const plan = cratchit::plan_sie_import(project, write("same.se", sie_file(2025, balanced_balances(), sale_and_cost(2025))));
  ASSERT_FALSE(plan);
  EXPECT_EQ(plan.error(), "Allt i SIE-filen finns redan i bokföringen");
}

TEST_F(SieImportTest, RemovingAVoucherIsRefused) {
  // The file lacks A 2, which the books have: importing would demote it
  auto const plan = cratchit::plan_sie_import(project, write("less.se", sie_file(2025, "", sale(2025, 1, "0201"))));
  ASSERT_FALSE(plan);
  EXPECT_TRUE(plan.error().starts_with("Importen skulle ta bort eller byta ut verifikationer")) << plan.error();
  auto const books = cratchit::books::read_books(project.folder);
  ASSERT_TRUE(books);
  EXPECT_EQ(books->years[0].vouchers.size(), 2u);
}

TEST_F(SieImportTest, AnotherOrgnrIsRefused) {
  auto const plan = cratchit::plan_sie_import(
      project, write("other.se", sie_file(2026, "", sale(2026, 1, "0110"), "\"Test AB\"", "556000-0002")));
  ASSERT_FALSE(plan);
  EXPECT_NE(plan.error().find("556000-0002"), std::string::npos) << plan.error();
}

TEST_F(SieImportTest, NoSieFileIsRefused) {
  EXPECT_FALSE(cratchit::plan_sie_import(project, write("notes.se", "notes\n")));
  EXPECT_FALSE(cratchit::plan_sie_import(project, root / "gone.se"));
}

namespace {

// The 2025 file with A 2's rows (10 to bank costs) replaced by rows
std::string with_a2_rows(std::string const& rows) {
  auto file = sie_file(2025, balanced_balances(), sale_and_cost(2025));
  std::string const original = "#TRANS 6570 {} 10\r\n#TRANS 1930 {} -10\r\n";
  file.replace(file.find(original), original.size(), rows);
  return file;
}

// A 2 with both rows struck and new ones added for 12, as Fortnox writes a changed voucher
std::string const struck_and_added = "#BTRANS 6570 {} 10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                     "#BTRANS 1930 {} -10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                     "#RTRANS 6570 {} 12 \"\" \"\" \"\" \"Kjell\"\r\n#TRANS 6570 {} 12\r\n"
                                     "#RTRANS 1930 {} -12 \"\" \"\" \"\" \"Kjell\"\r\n#TRANS 1930 {} -12\r\n";

} // namespace

TEST_F(SieImportTest, StruckAndAddedRowsChangeAVoucher) {
  auto const plan = cratchit::plan_sie_import(project, write("changed.se", with_a2_rows(struck_and_added)));
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_EQ(plan->import.vouchers_added, 0u);
  ASSERT_EQ(plan->changed.size(), 1u);
  EXPECT_EQ(cratchit::change_text(plan->changed[0]), "A 2  2025-03-01  Bank cost: 2 rader strukna, 2 rader tillagda");
  ASSERT_TRUE(cratchit::import_sie(project, *plan));
  auto const books = cratchit::books::read_books(project.folder);
  ASSERT_TRUE(books) << books.error();
  EXPECT_EQ(books->years[0].vouchers[1].postings.size(), 4u);
}

TEST_F(SieImportTest, AnotherTextOfAVoucherIsRefused) {
  auto file = with_a2_rows(struck_and_added);
  file.replace(file.find("\"Bank cost\""), 11, "\"Bank fee\"");
  auto const plan = cratchit::plan_sie_import(project, write("text.se", file));
  ASSERT_FALSE(plan);
  EXPECT_EQ(plan.error(), "Verifikationen A 2 har ett annat datum eller en annan text i filen. cratchit tar bara emot "
                          "strukna och tillagda rader i befintliga verifikationer");
}

TEST(ChangeText, SaysWhatChanged) {
  EXPECT_EQ(cratchit::change_text({"A", "12", "2025-06-03", "Loopia", 1, 1}), "A 12  2025-06-03  Loopia: 1 rad struken, 1 rad tillagd");
  EXPECT_EQ(cratchit::change_text({"A", "12", "2025-06-03", "Loopia", 0, 3}), "A 12  2025-06-03  Loopia: 3 rader tillagda");
}

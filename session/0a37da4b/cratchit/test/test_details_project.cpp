#include "core/details_project.hpp"

#include "core/books/books.hpp"
#include "core/path_value.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using cratchit::FiscalYear;
using cratchit::parse_first_fiscal_year;
using cratchit::parse_orgnr;

TEST(Orgnr, TakesTenDigitsWithOrWithoutTheDash) {
  EXPECT_EQ(parse_orgnr("556677-8899"), "556677-8899");
  EXPECT_EQ(parse_orgnr("5566778899"), "556677-8899");
  EXPECT_EQ(parse_orgnr(" 556782-8172 "), "556782-8172");
}

TEST(Orgnr, RefusesAWrongCheckDigit) {
  auto const orgnr = parse_orgnr("556677-8898");
  ASSERT_FALSE(orgnr);
  EXPECT_TRUE(orgnr.error().starts_with("Kontrollsiffran")) << orgnr.error();
}

TEST(Orgnr, RefusesWhatIsNoOrgnr) {
  EXPECT_FALSE(parse_orgnr(""));
  EXPECT_FALSE(parse_orgnr("556677-889"));
  EXPECT_FALSE(parse_orgnr("55667788990"));
  EXPECT_FALSE(parse_orgnr("5566-778899"));
  EXPECT_FALSE(parse_orgnr("556677 8899"));
  EXPECT_FALSE(parse_orgnr("abcdef-ghij"));
}

TEST(FirstFiscalYear, TakesFirstAndLastDay) {
  EXPECT_EQ(parse_first_fiscal_year("2026-01-01 2026-12-31"), (FiscalYear{"2026-01-01", "2026-12-31"}));
  EXPECT_EQ(parse_first_fiscal_year("2026-05-01 – 2027-04-30"), (FiscalYear{"2026-05-01", "2027-04-30"}));
  EXPECT_EQ(parse_first_fiscal_year(" 2026-05-01  -  2027-04-30 "), (FiscalYear{"2026-05-01", "2027-04-30"}));
}

TEST(FirstFiscalYear, MayBeShorterOrUpToEighteenMonths) {
  EXPECT_TRUE(parse_first_fiscal_year("2026-11-01 2026-12-31"));
  EXPECT_TRUE(parse_first_fiscal_year("2026-07-01 2027-12-31"));  // 18 months
  EXPECT_FALSE(parse_first_fiscal_year("2026-06-01 2027-12-31")); // 19 months
}

TEST(FirstFiscalYear, StartsFirstAndEndsLastInAMonth) {
  EXPECT_FALSE(parse_first_fiscal_year("2026-01-02 2026-12-31"));
  EXPECT_FALSE(parse_first_fiscal_year("2026-01-01 2026-12-30"));
  EXPECT_TRUE(parse_first_fiscal_year("2027-03-01 2028-02-29")); // leap year
  EXPECT_FALSE(parse_first_fiscal_year("2026-03-01 2027-02-29"));
}

TEST(FirstFiscalYear, RefusesOtherText) {
  EXPECT_FALSE(parse_first_fiscal_year(""));
  EXPECT_FALSE(parse_first_fiscal_year("2026"));
  EXPECT_FALSE(parse_first_fiscal_year("2026-12-31 2026-01-01"));
  EXPECT_FALSE(parse_first_fiscal_year("2026-01-01 till 2026-12-31"));
}

namespace {

// A workspace folder of its own for each test, removed afterwards
class DetailsProjectTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_details_project_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  void TearDown() override { fs::remove_all(root); }

  cratchit::Workspace workspace() const { return {root, root / ".cratchit" / "workspace.txt", {}}; }

  fs::path root;
};

} // namespace

TEST_F(DetailsProjectTest, FolderIsNamedAfterTheCompanyInTheWorkspace) {
  EXPECT_EQ(cratchit::new_project_folder(workspace(), "Exempel AB"), root / "Exempel AB");
  EXPECT_FALSE(cratchit::new_project_folder(workspace(), ""));
  EXPECT_FALSE(cratchit::new_project_folder(workspace(), "A/B"));
  fs::create_directories(root / "Finns AB");
  EXPECT_FALSE(cratchit::new_project_folder(workspace(), "Finns AB"));
}

TEST_F(DetailsProjectTest, CreatesProjectAndBooksWithTheYear) {
  auto const folder = root / "Exempel AB";
  auto const project =
      cratchit::create_project_from_details(folder, "Exempel AB", "556677-8899", {"2026-05-01", "2027-04-30"}, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  auto const entries = cratchit::read_path_values(folder / ".cratchit" / "project.txt");
  ASSERT_TRUE(entries) << entries.error();
  EXPECT_EQ(cratchit::find_value(*entries, "created"), "2026-11-09");
  EXPECT_EQ(cratchit::find_value(*entries, "uuid"), project->uuid);
  EXPECT_EQ(cratchit::find_value(*entries, "name"), "Exempel AB");
  EXPECT_EQ(cratchit::find_value(*entries, "orgnr"), "556677-8899");
  EXPECT_TRUE(cratchit::open_project(folder, project->uuid));
  EXPECT_EQ(cratchit::list_fiscal_years(*project), (std::vector<FiscalYear>{{"2026-05-01", "2027-04-30"}}));
  auto const books = cratchit::books::read_books(folder);
  ASSERT_TRUE(books) << books.error();
  EXPECT_EQ(books->years.size(), 1u);
  EXPECT_TRUE(books->accounts.empty());
  // Only project.txt and books.txt
  std::vector<fs::path> files;
  for (auto const& entry : fs::recursive_directory_iterator(folder)) {
    if (entry.is_regular_file()) files.push_back(fs::relative(entry.path(), folder));
  }
  std::ranges::sort(files);
  EXPECT_EQ(files, (std::vector<fs::path>{fs::path(".cratchit") / "books" / "books.txt", fs::path(".cratchit") / "project.txt"}));
  auto const books_txt = cratchit::read_path_values(folder / ".cratchit" / "books" / "books.txt");
  ASSERT_TRUE(books_txt) << books_txt.error();
  EXPECT_EQ(books_txt->size(), 1u);
}

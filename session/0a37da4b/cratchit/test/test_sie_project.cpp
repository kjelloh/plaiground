#include "core/sie_project.hpp"

#include "core/fiscal_year.hpp"
#include "core/path_value.hpp"
#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using sie_test::balanced_balances;
using sie_test::sale_and_cost;
using sie_test::sie_file;

namespace {

// An empty folder of its own for each test (standing in for the workspace), removed afterwards
class SieProjectTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_sie_project_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path write(std::string const& name, std::string const& text) const {
    std::ofstream(root / name, std::ios::binary) << text;
    return root / name;
  }

  fs::path root;
};

} // namespace

TEST_F(SieProjectTest, PlanReadsTheFileAndWritesNothing) {
  auto const file = write("test.se", sie_file(2025, balanced_balances(), sale_and_cost(2025)));
  auto const plan = cratchit::plan_project_from_sie(file);
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_EQ(plan->name, "Test AB");
  EXPECT_EQ(plan->orgnr, "556000-0001");
  EXPECT_EQ(plan->folder, root / "Test AB");
  EXPECT_EQ(plan->import.vouchers_added, 2u);
  EXPECT_EQ(cratchit::format_iso_date(plan->import.year_start), "2025-01-01");
  EXPECT_FALSE(fs::exists(root / "Test AB"));
  EXPECT_TRUE(fs::exists(file));
}

TEST_F(SieProjectTest, CreateMakesTheProjectImportsTheFileAndMovesIt) {
  auto const file = write("test.se", sie_file(2025, balanced_balances(), sale_and_cost(2025)));
  auto const plan = cratchit::plan_project_from_sie(file);
  ASSERT_TRUE(plan) << plan.error();
  auto const project = cratchit::create_project_from_sie(*plan, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  auto const folder = root / "Test AB";
  EXPECT_EQ(project->folder, folder);
  EXPECT_EQ(project->name, "Test AB");
  EXPECT_TRUE(cratchit::is_uuid_like(project->uuid));
  // project.txt
  auto const entries = cratchit::read_path_values(folder / ".cratchit" / "project.txt");
  ASSERT_TRUE(entries) << entries.error();
  EXPECT_EQ(cratchit::find_value(*entries, "created"), "2026-11-09");
  EXPECT_EQ(cratchit::find_value(*entries, "uuid"), project->uuid);
  EXPECT_EQ(cratchit::find_value(*entries, "name"), "Test AB");
  EXPECT_EQ(cratchit::find_value(*entries, "orgnr"), "556000-0001");
  // The project opens, and its fiscal year is listed
  EXPECT_TRUE(cratchit::open_project(folder, project->uuid));
  auto const years = cratchit::list_fiscal_years(*project);
  ASSERT_TRUE(years) << years.error();
  EXPECT_EQ(*years, (std::vector<cratchit::FiscalYear>{{"2025-01-01", "2025-12-31"}}));
  // The books, as take 3 keeps them
  EXPECT_TRUE(fs::exists(folder / ".cratchit" / "books" / "accounts.csv"));
  EXPECT_TRUE(fs::exists(folder / ".cratchit" / "books" / "2025-01-01" / "vouchers.csv"));
  EXPECT_TRUE(fs::exists(folder / ".cratchit" / "books" / "2025-01-01" / "postings.csv"));
  auto const books = cratchit::books::read_books(folder);
  ASSERT_TRUE(books) << books.error();
  ASSERT_EQ(books->years.size(), 1u);
  EXPECT_EQ(books->years[0].vouchers.size(), 2u);
  // The SIE file moved into the project folder, and no copy is stashed
  EXPECT_FALSE(fs::exists(file));
  EXPECT_TRUE(fs::exists(folder / "test.se"));
  EXPECT_FALSE(fs::exists(folder / ".cratchit" / "imports"));
}

TEST_F(SieProjectTest, ExistingFolderIsAnError) {
  auto const file = write("test.se", sie_file(2025, balanced_balances(), sale_and_cost(2025)));
  fs::create_directories(root / "Test AB");
  auto const plan = cratchit::plan_project_from_sie(file);
  ASSERT_FALSE(plan);
  EXPECT_TRUE(plan.error().starts_with("Mappen ")) << plan.error();
}

TEST_F(SieProjectTest, FileInAProjectFolderIsAnError) {
  auto const file = write("test.se", sie_file(2025, balanced_balances(), sale_and_cost(2025)));
  fs::create_directories(root / ".cratchit");
  std::ofstream(root / ".cratchit" / "project.txt") << "uuid = 1\nname = Annat AB\n";
  auto const plan = cratchit::plan_project_from_sie(file);
  ASSERT_FALSE(plan);
  EXPECT_TRUE(plan.error().starts_with("SIE-filen ligger i mappen för en bokföring")) << plan.error();
}

TEST_F(SieProjectTest, NoSieFileIsAnError) {
  auto const file = write("notes.se", "Some notes\n");
  auto const plan = cratchit::plan_project_from_sie(file);
  ASSERT_FALSE(plan);
  EXPECT_NE(plan.error().find("ingen SIE-fil"), std::string::npos) << plan.error();
}

TEST_F(SieProjectTest, MissingFileIsAnError) {
  EXPECT_FALSE(cratchit::plan_project_from_sie(root / "gone.se"));
}

TEST_F(SieProjectTest, CompanyNameThatCannotBeAFolderIsAnError) {
  auto const file = write("test.se", sie_file(2025, balanced_balances(), sale_and_cost(2025), "\"A/B AB\""));
  auto const plan = cratchit::plan_project_from_sie(file);
  ASSERT_FALSE(plan);
  EXPECT_NE(plan.error().find("namn på en mapp"), std::string::npos) << plan.error();
}

TEST(SieProject, FolderNames) {
  EXPECT_TRUE(cratchit::is_folder_name("The ITfied AB"));
  EXPECT_TRUE(cratchit::is_folder_name("Bokföring Åkesson & Co"));
  EXPECT_FALSE(cratchit::is_folder_name(""));
  EXPECT_FALSE(cratchit::is_folder_name(".."));
  EXPECT_FALSE(cratchit::is_folder_name("A/B"));
  EXPECT_FALSE(cratchit::is_folder_name("A:B"));
  EXPECT_FALSE(cratchit::is_folder_name("AB."));
}

TEST(SieProject, SieFileNames) {
  EXPECT_TRUE(cratchit::is_sie_file_name("x.se"));
  EXPECT_TRUE(cratchit::is_sie_file_name("x.SE"));
  EXPECT_TRUE(cratchit::is_sie_file_name("x.si"));
  EXPECT_TRUE(cratchit::is_sie_file_name("x.sie"));
  EXPECT_FALSE(cratchit::is_sie_file_name("x.se.txt"));
  EXPECT_FALSE(cratchit::is_sie_file_name("x"));
}

TEST(SieProject, NewUuidsAreVersionFourAndDiffer) {
  auto const a = cratchit::new_uuid();
  EXPECT_TRUE(cratchit::is_uuid_like(a)) << a;
  EXPECT_EQ(a[14], '4');
  EXPECT_NE(a, cratchit::new_uuid());
}

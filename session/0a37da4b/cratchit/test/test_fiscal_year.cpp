#include "core/fiscal_year.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using cratchit::FiscalYear;
using cratchit::label_of;

namespace {

// A project folder of its own for each test, removed afterwards
class FiscalYearTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    folder = fs::temp_directory_path() / (std::string("cratchit_fiscal_year_") + info->name());
    fs::remove_all(folder);
    fs::create_directories(folder / ".cratchit");
  }
  void TearDown() override { fs::remove_all(folder); }

  cratchit::Project project() const { return {folder, folder / ".cratchit" / "project.txt", "1", "AB"}; }

  void books(std::string const& text) const {
    fs::create_directories(folder / ".cratchit" / "books");
    std::ofstream(folder / ".cratchit" / "books" / "books.txt") << text;
  }

  fs::path folder;
};

} // namespace

TEST_F(FiscalYearTest, NoBooksMeansNoFiscalYears) {
  auto const years = cratchit::list_fiscal_years(project());
  ASSERT_TRUE(years) << years.error();
  EXPECT_TRUE(years->empty());
  EXPECT_FALSE(fs::exists(folder / ".cratchit" / "books"));
}

TEST_F(FiscalYearTest, YearsAreReadFromBooksTheLatestFirst) {
  books("chart_type = EUBAS97\nyear.2024-05-01 = 2025-04-30\nyear.2026-05-01 = 2027-04-30\nyear.2025-05-01 = 2026-04-30\n");
  auto const years = cratchit::list_fiscal_years(project());
  ASSERT_TRUE(years) << years.error();
  EXPECT_EQ(*years, (std::vector<FiscalYear>{{"2026-05-01", "2027-04-30"},
                                             {"2025-05-01", "2026-04-30"},
                                             {"2024-05-01", "2025-04-30"}}));
}

TEST_F(FiscalYearTest, DateThatIsNotIsoIsAnError) {
  books("year.2025-05-01 = 30 april 2026\n");
  auto const years = cratchit::list_fiscal_years(project());
  ASSERT_FALSE(years);
  EXPECT_NE(years.error().find("books.txt"), std::string::npos);
}

TEST(FiscalYearLabel, CalendarYearIsOneYear) { EXPECT_EQ(label_of({"2025-01-01", "2025-12-31"}), "2025"); }

TEST(FiscalYearLabel, BrokenYearIsTwoYears) { EXPECT_EQ(label_of({"2025-05-01", "2026-04-30"}), "2025/26"); }

TEST(FiscalYearLabel, AcrossACenturyBothAreWhole) { EXPECT_EQ(label_of({"1999-07-01", "2000-06-30"}), "1999/2000"); }

// A year is listed from books.txt alone: whether it has any books (a folder of
// its own with vouchers) does not matter
TEST_F(FiscalYearTest, YearWithoutBooksIsListed) {
  books("year.2024-05-01 = 2025-04-30\nyear.2025-05-01 = 2026-04-30\n");
  fs::create_directories(folder / ".cratchit" / "books" / "2024-05-01");
  std::ofstream(folder / ".cratchit" / "books" / "2024-05-01" / "vouchers.csv") << "series,number,date,text,amount\n";
  // 2025-05-01 has no folder at all
  auto const years = cratchit::list_fiscal_years(project());
  ASSERT_TRUE(years) << years.error();
  EXPECT_EQ(*years, (std::vector<FiscalYear>{{"2025-05-01", "2026-04-30"}, {"2024-05-01", "2025-04-30"}}));
}

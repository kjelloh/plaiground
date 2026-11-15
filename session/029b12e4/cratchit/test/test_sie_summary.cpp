#include "core/sie_summary.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// An SIE file of its own for each test, removed afterwards
struct SieFile {
  explicit SieFile(std::string const& text) {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    path = fs::temp_directory_path() / (std::string("cratchit_sie_summary_") + info->name() + ".se");
    std::ofstream(path, std::ios::binary) << text;
  }
  ~SieFile() { fs::remove(path); }
  fs::path path;
};

std::string voucher(std::string const& series, std::string const& number, std::string const& date) {
  return "#VER " + series + " " + number + " " + date + " \"T\"\r\n{\r\n#TRANS 1930 {} 1.00\r\n#TRANS 3010 {} -1.00\r\n}\r\n";
}

} // namespace

TEST(SieSummary, GivesYearGeneratedVouchersLastDateAndHighestNumberPerSeries) {
  SieFile file("#FLAGGA 0\r\n#SIETYP 4\r\n#GEN 20261112\r\n#FNAMN \"Test AB\"\r\n#RAR 0 20250501 20260430\r\n"
               "#RAR -1 20240501 20250430\r\n" +
               voucher("A", "9", "20250601") + voucher("A", "10", "20260115") + voucher("M", "3", "20260430") +
               voucher("D", "1", "20250701"));
  auto const summary = cratchit::summarize_sie_file(file.path);
  ASSERT_TRUE(summary) << summary.error();
  EXPECT_EQ(summary->vouchers, 4u);
  EXPECT_EQ(cratchit::summary_line(*summary),
            "2025-05-01 – 2026-04-30  skapad 2026-11-12  4 ver. t.o.m. 2026-04-30  A 10 · D 1 · M 3");
  EXPECT_EQ(cratchit::describe_sie_file(file.path), (std::vector<std::string>{cratchit::summary_line(*summary)}));
}

TEST(SieSummary, LeavesOutWhatTheFileLacks) {
  SieFile file("#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n");
  auto const summary = cratchit::summarize_sie_file(file.path);
  ASSERT_TRUE(summary) << summary.error();
  EXPECT_EQ(cratchit::summary_line(*summary), "0 ver.");
}

TEST(SieSummary, SaysWhyAFileCannotBeRead) {
  SieFile file("Det här är ingen SIE-fil\r\n");
  EXPECT_EQ(cratchit::describe_sie_file(file.path), (std::vector<std::string>{"(går inte att läsa: #SIETYP saknas)"}));
  EXPECT_EQ(cratchit::describe_sie_file(file.path.parent_path() / "finns_inte.se"),
            (std::vector<std::string>{"(går inte att läsa: filen går inte att öppna)"}));
}

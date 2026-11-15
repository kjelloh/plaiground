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

// Listing the files found with Tab

namespace {

// Files of their own for each test, in a folder removed afterwards
struct SieFolder {
  SieFolder() {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_sie_list_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  ~SieFolder() { fs::remove_all(root); }
  fs::path add(std::string const& name, std::string const& text) {
    std::ofstream(root / name, std::ios::binary) << text;
    return root / name;
  }
  fs::path root;
};

std::string sie(std::string const& year, std::string const& generated, std::string const& vouchers) {
  return "#FLAGGA 0\r\n#SIETYP 4\r\n" + (generated.empty() ? std::string() : "#GEN " + generated + "\r\n") +
         (year.empty() ? std::string() : "#RAR 0 " + year + "\r\n") + vouchers;
}

} // namespace

TEST(SieList, GroupsByFiscalYearLatestFirstAndTheLatestCreatedFirstInColumns) {
  SieFolder folder;
  auto const old_2025 = folder.add("a.se", sie("20250101 20251231", "20251001", voucher("A", "1", "20250301")));
  auto const new_2025 = folder.add("bb.se", sie("20250101 20251231", "20260115",
                                                voucher("A", "1", "20250301") + voucher("A", "2", "20251231")));
  auto const only_2026 = folder.add("c.se", sie("20260101 20261231", "", ""));
  auto const no_year = folder.add("d.se", sie("", "20260101", ""));
  auto const broken = folder.add("e.se", "ingen SIE-fil\r\n");
  fs::create_directories(folder.root / "arkiv");
  auto const lines = cratchit::list_sie_files({old_2025, new_2025, only_2026, no_year, broken}, {folder.root / "arkiv"});
  auto const separator = std::string(1, static_cast<char>(fs::path::preferred_separator));
  EXPECT_EQ(lines, (std::vector<std::string>{
                       "2026-01-01 – 2026-12-31",
                       "  c.se " + std::string(2 + 17 + 2, ' ') + "0 ver.", // no created date: its column stays empty
                       "2025-01-01 – 2025-12-31",
                       "  bb.se  skapad 2026-01-15  2 ver.  t.o.m. 2025-12-31  A 2",
                       "  a.se   skapad 2025-10-01  1 ver.  t.o.m. 2025-03-01  A 1",
                       "Utan räkenskapsår",
                       "  d.se   skapad 2026-01-01  0 ver.",
                       "Går inte att läsa",
                       "  e.se   #SIETYP saknas",
                       "Mappar",
                       "  arkiv" + separator,
                   }));
}

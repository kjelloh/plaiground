#include "core/data_folder.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// An empty folder of its own for each test, removed afterwards
class DataFolderTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_test_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path root;
};

std::string read_file(fs::path const& path) {
  std::ifstream in(path);
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

} // namespace

TEST_F(DataFolderTest, FreshStartCreatesFolderAndFramework) {
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  ASSERT_TRUE(folder.has_value()) << folder.error();
  ASSERT_EQ(folder->initiated.size(), 1u);
  EXPECT_EQ(folder->initiated[0], fs::path(".cratchit") / "framework.txt");
  EXPECT_EQ(folder->framework_file, root / ".cratchit" / "framework.txt");
  EXPECT_EQ(read_file(folder->framework_file), "created = 2026-10-20\n");
}

TEST_F(DataFolderTest, MissingFrameworkIsAnError) {
  fs::create_directory(root / ".cratchit");
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  EXPECT_FALSE(folder.has_value());
  EXPECT_FALSE(fs::exists(root / ".cratchit" / "framework.txt"));
}

TEST_F(DataFolderTest, ExistingFrameworkIsLeftAlone) {
  fs::create_directory(root / ".cratchit");
  std::ofstream(root / ".cratchit" / "framework.txt") << "created = 2020-01-01\n";
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  ASSERT_TRUE(folder.has_value()) << folder.error();
  EXPECT_TRUE(folder->initiated.empty());
  EXPECT_EQ(read_file(folder->framework_file), "created = 2020-01-01\n");
}

TEST_F(DataFolderTest, DotCratchitFileIsAnError) {
  std::ofstream(root / ".cratchit") << "not a folder\n";
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  EXPECT_FALSE(folder.has_value());
}

TEST_F(DataFolderTest, FrameworkFolderIsAnError) {
  fs::create_directories(root / ".cratchit" / "framework.txt");
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  EXPECT_FALSE(folder.has_value());
}

TEST(TodayIso, HasDateShape) {
  auto const today = cratchit::today_iso();
  ASSERT_EQ(today.size(), 10u);
  EXPECT_EQ(today[4], '-');
  EXPECT_EQ(today[7], '-');
}

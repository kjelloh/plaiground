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

void write_file(fs::path const& path, std::string const& text) {
  fs::create_directories(path.parent_path());
  std::ofstream(path) << text;
}

} // namespace

TEST_F(DataFolderTest, FreshStartCreatesFrameworkAndRuntime) {
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  ASSERT_TRUE(folder.has_value()) << folder.error();
  EXPECT_EQ(folder->framework_file, root / ".cratchit" / "framework.txt");
  EXPECT_EQ(folder->runtime_file, root / ".cratchit" / "runtime.txt");
  EXPECT_EQ(read_file(folder->framework_file), "created = 2026-10-20\nruntime = .\n");
  EXPECT_EQ(read_file(folder->runtime_file), "created = 2026-10-20\n");
  ASSERT_EQ(folder->initiated.size(), 2u);
  EXPECT_EQ(folder->initiated[0].folder, root);
  EXPECT_EQ(folder->initiated[0].file, fs::path(".cratchit") / "framework.txt");
  EXPECT_EQ(folder->initiated[1].folder, root);
  EXPECT_EQ(folder->initiated[1].file, fs::path(".cratchit") / "runtime.txt");
}

TEST_F(DataFolderTest, SecondStartChangesNothing) {
  ASSERT_TRUE(cratchit::open_data_folder(root, "2026-10-20").has_value());
  auto const folder = cratchit::open_data_folder(root, "2030-01-01");
  ASSERT_TRUE(folder.has_value()) << folder.error();
  EXPECT_TRUE(folder->initiated.empty());
  EXPECT_EQ(read_file(folder->framework_file), "created = 2026-10-20\nruntime = .\n");
  EXPECT_EQ(read_file(folder->runtime_file), "created = 2026-10-20\n");
}

TEST_F(DataFolderTest, SelfRuntimeGetsMissingRuntimeFile) {
  write_file(root / ".cratchit" / "framework.txt", "created = 2020-01-01\nruntime = .\n");
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  ASSERT_TRUE(folder.has_value()) << folder.error();
  ASSERT_EQ(folder->initiated.size(), 1u);
  EXPECT_EQ(folder->initiated[0].file, fs::path(".cratchit") / "runtime.txt");
  EXPECT_EQ(read_file(root / ".cratchit" / "runtime.txt"), "created = 2026-10-20\n");
  EXPECT_EQ(read_file(folder->framework_file), "created = 2020-01-01\nruntime = .\n");
}

TEST_F(DataFolderTest, MissingRuntimeEntryMeansSelf) {
  write_file(root / ".cratchit" / "framework.txt", "created = 2020-01-01\n");
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  ASSERT_TRUE(folder.has_value()) << folder.error();
  EXPECT_EQ(folder->runtime_file, root / ".cratchit" / "runtime.txt");
  EXPECT_EQ(read_file(folder->framework_file), "created = 2020-01-01\n");
}

TEST_F(DataFolderTest, SelfByRelativePathIsSelf) {
  fs::create_directories(root / "books");
  write_file(root / "books" / ".cratchit" / "framework.txt", "runtime = ../books\n");
  auto const folder = cratchit::open_data_folder(root / "books", "2026-10-20");
  ASSERT_TRUE(folder.has_value()) << folder.error();
  EXPECT_EQ(folder->runtime_file, root / "books" / ".cratchit" / "runtime.txt");
}

TEST_F(DataFolderTest, RuntimeElsewhereIsFollowed) {
  fs::create_directories(root / "books");
  write_file(root / "runtime" / ".cratchit" / "runtime.txt", "created = 2020-01-01\n");
  write_file(root / "books" / ".cratchit" / "framework.txt", "runtime = ../runtime\n");
  auto const folder = cratchit::open_data_folder(root / "books", "2026-10-20");
  ASSERT_TRUE(folder.has_value()) << folder.error();
  EXPECT_TRUE(folder->initiated.empty());
  EXPECT_TRUE(fs::equivalent(folder->runtime_file, root / "runtime" / ".cratchit" / "runtime.txt"));
  EXPECT_FALSE(fs::exists(root / "books" / ".cratchit" / "runtime.txt"));
}

TEST_F(DataFolderTest, AbsoluteRuntimeIsFollowed) {
  fs::create_directories(root / "books");
  write_file(root / "runtime" / ".cratchit" / "runtime.txt", "created = 2020-01-01\n");
  write_file(root / "books" / ".cratchit" / "framework.txt", "runtime = " + (root / "runtime").string() + "\n");
  auto const folder = cratchit::open_data_folder(root / "books", "2026-10-20");
  ASSERT_TRUE(folder.has_value()) << folder.error();
  EXPECT_TRUE(fs::equivalent(folder->runtime_file, root / "runtime" / ".cratchit" / "runtime.txt"));
}

TEST_F(DataFolderTest, RuntimeElsewhereIsNeverCreated) {
  fs::create_directories(root / "books");
  fs::create_directories(root / "runtime");
  write_file(root / "books" / ".cratchit" / "framework.txt", "runtime = ../runtime\n");
  auto const folder = cratchit::open_data_folder(root / "books", "2026-10-20");
  EXPECT_FALSE(folder.has_value());
  EXPECT_FALSE(fs::exists(root / "runtime" / ".cratchit"));
}

TEST_F(DataFolderTest, MissingRuntimeFolderIsAnError) {
  write_file(root / ".cratchit" / "framework.txt", "runtime = no_such_folder\n");
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  EXPECT_FALSE(folder.has_value());
  EXPECT_FALSE(fs::exists(root / "no_such_folder"));
}

TEST_F(DataFolderTest, EmptyRuntimeEntryIsAnError) {
  write_file(root / ".cratchit" / "framework.txt", "runtime =\n");
  EXPECT_FALSE(cratchit::open_data_folder(root, "2026-10-20").has_value());
}

TEST_F(DataFolderTest, MalformedFrameworkIsAnError) {
  write_file(root / ".cratchit" / "framework.txt", "nonsense\n");
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  ASSERT_FALSE(folder.has_value());
  EXPECT_NE(folder.error().find("framework.txt"), std::string::npos);
}

TEST_F(DataFolderTest, MissingFrameworkIsAnError) {
  fs::create_directory(root / ".cratchit");
  auto const folder = cratchit::open_data_folder(root, "2026-10-20");
  EXPECT_FALSE(folder.has_value());
  EXPECT_FALSE(fs::exists(root / ".cratchit" / "framework.txt"));
}

TEST_F(DataFolderTest, DotCratchitFileIsAnError) {
  std::ofstream(root / ".cratchit") << "not a folder\n";
  EXPECT_FALSE(cratchit::open_data_folder(root, "2026-10-20").has_value());
}

TEST_F(DataFolderTest, FrameworkFolderIsAnError) {
  fs::create_directories(root / ".cratchit" / "framework.txt");
  EXPECT_FALSE(cratchit::open_data_folder(root, "2026-10-20").has_value());
}

TEST_F(DataFolderTest, RuntimeFileFolderIsAnError) {
  write_file(root / ".cratchit" / "framework.txt", "runtime = .\n");
  fs::create_directories(root / ".cratchit" / "runtime.txt");
  EXPECT_FALSE(cratchit::open_data_folder(root, "2026-10-20").has_value());
}

TEST(TodayIso, HasDateShape) {
  auto const today = cratchit::today_iso();
  ASSERT_EQ(today.size(), 10u);
  EXPECT_EQ(today[4], '-');
  EXPECT_EQ(today[7], '-');
}

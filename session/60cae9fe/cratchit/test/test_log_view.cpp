#include "core/log_view.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using cratchit::LogView;

namespace {

// A log file of its own for each test, removed afterwards
class LogViewTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_test_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
    file = root / "log.txt";
    std::ofstream(file) << "one\ntwo\nthree\r\nfour\nfive\n";
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path root;
  fs::path file;
};

} // namespace

TEST_F(LogViewTest, LinesComeNewestFirst) {
  EXPECT_EQ(cratchit::lines_newest_first(file), (std::vector<std::string>{"five", "four", "three", "two", "one"}));
}

TEST_F(LogViewTest, MissingFileHasNoLines) {
  EXPECT_TRUE(cratchit::lines_newest_first(root / "none.txt").empty());
}

TEST_F(LogViewTest, ShowsEverythingWithoutPaging) {
  LogView view(0);
  std::ostringstream out;
  EXPECT_EQ(view.open(file, out), LogView::Result::Leave);
  EXPECT_EQ(out.str(), std::format("Log {}, newest first:\nfive\nfour\nthree\ntwo\none\n(start of log)\n", file.string()));
}

TEST_F(LogViewTest, PagesBackwardsInTime) {
  LogView view(2);
  std::ostringstream out;
  EXPECT_EQ(view.open(file, out), LogView::Result::Stay);
  EXPECT_TRUE(out.str().ends_with("five\nfour\n"));
  out.str("");
  EXPECT_EQ(view.on_key(U' ', out), LogView::Result::Stay);
  EXPECT_EQ(out.str(), "three\ntwo\n");
  out.str("");
  EXPECT_EQ(view.on_key(U'\n', out), LogView::Result::Leave);
  EXPECT_EQ(out.str(), "one\n(start of log)\n");
}

TEST_F(LogViewTest, DashOrQLeaves) {
  LogView view(2);
  std::ostringstream out;
  view.open(file, out);
  EXPECT_EQ(view.on_key(U'x', out), LogView::Result::Stay);
  EXPECT_EQ(view.on_key(U'-', out), LogView::Result::Leave);
  view.open(file, out);
  EXPECT_EQ(view.on_key(U'q', out), LogView::Result::Leave);
}

TEST_F(LogViewTest, EmptyLogSaysSo) {
  LogView view(2);
  std::ostringstream out;
  EXPECT_EQ(view.open(root / "none.txt", out), LogView::Result::Leave);
  EXPECT_NE(out.str().find("is empty"), std::string::npos);
}

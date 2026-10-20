#include "core/workspace.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// An empty folder of its own for each test, removed afterwards
class WorkspaceTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_workspace_") + info->name());
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

std::string separator() {
  return std::string(1, static_cast<char>(fs::path::preferred_separator));
}

} // namespace

TEST_F(WorkspaceTest, MissingEntryIsNone) {
  write_file(root / ".cratchit" / "runtime.txt", "created = 2026-10-20\n");
  auto const entry = cratchit::workspace_entry(root / ".cratchit" / "runtime.txt");
  ASSERT_TRUE(entry.has_value()) << entry.error();
  EXPECT_FALSE(entry->has_value());
}

TEST_F(WorkspaceTest, EmptyEntryIsNone) {
  write_file(root / ".cratchit" / "runtime.txt", "workspace =\n");
  auto const entry = cratchit::workspace_entry(root / ".cratchit" / "runtime.txt");
  ASSERT_TRUE(entry.has_value()) << entry.error();
  EXPECT_FALSE(entry->has_value());
}

TEST_F(WorkspaceTest, RelativeEntryIsFromTheRuntimeFolder) {
  write_file(root / "runtime" / ".cratchit" / "runtime.txt", "workspace = ../books\n");
  auto const entry = cratchit::workspace_entry(root / "runtime" / ".cratchit" / "runtime.txt");
  ASSERT_TRUE(entry.has_value()) << entry.error();
  EXPECT_EQ(**entry, root / "books");
}

TEST_F(WorkspaceTest, AbsoluteEntryIsKept) {
  write_file(root / ".cratchit" / "runtime.txt", "workspace = " + (root / "books").string() + "\n");
  auto const entry = cratchit::workspace_entry(root / ".cratchit" / "runtime.txt");
  ASSERT_TRUE(entry.has_value()) << entry.error();
  EXPECT_EQ(**entry, root / "books");
}

TEST_F(WorkspaceTest, UnreadableRuntimeIsAnError) {
  EXPECT_FALSE(cratchit::workspace_entry(root / ".cratchit" / "runtime.txt").has_value());
}

TEST_F(WorkspaceTest, SaveAddsEntryAndKeepsOthers) {
  auto const runtime_file = root / ".cratchit" / "runtime.txt";
  write_file(runtime_file, "created = 2026-10-20\nother = x\n");
  ASSERT_TRUE(cratchit::save_workspace_entry(runtime_file, root / "books").has_value());
  EXPECT_EQ(read_file(runtime_file), "created = 2026-10-20\nother = x\nworkspace = " + (root / "books").string() + "\n");
}

TEST_F(WorkspaceTest, SaveReplacesEntryInPlace) {
  auto const runtime_file = root / ".cratchit" / "runtime.txt";
  write_file(runtime_file, "workspace = old\ncreated = 2026-10-20\n");
  ASSERT_TRUE(cratchit::save_workspace_entry(runtime_file, root / "books").has_value());
  EXPECT_EQ(read_file(runtime_file), "workspace = " + (root / "books").string() + "\ncreated = 2026-10-20\n");
}

TEST_F(WorkspaceTest, FreshLocationIsInitiated) {
  auto const workspace = cratchit::open_workspace(root, "2026-10-20", false);
  ASSERT_TRUE(workspace.has_value()) << workspace.error();
  EXPECT_EQ(workspace->workspace_file, root / ".cratchit" / "workspace.txt");
  EXPECT_EQ(read_file(workspace->workspace_file), "created = 2026-10-20\n");
  ASSERT_EQ(workspace->initiated.size(), 1u);
  EXPECT_EQ(workspace->initiated[0].folder, root);
  EXPECT_EQ(workspace->initiated[0].file, fs::path(".cratchit") / "workspace.txt");
}

TEST_F(WorkspaceTest, ExistingWorkspaceIsNotChanged) {
  write_file(root / ".cratchit" / "workspace.txt", "created = 2020-01-01\n");
  auto const workspace = cratchit::open_workspace(root, "2026-10-20", true);
  ASSERT_TRUE(workspace.has_value()) << workspace.error();
  EXPECT_TRUE(workspace->initiated.empty());
  EXPECT_EQ(read_file(workspace->workspace_file), "created = 2020-01-01\n");
}

TEST_F(WorkspaceTest, MissingFileIsAnErrorUnlessJustChosen) {
  fs::create_directories(root / ".cratchit");
  EXPECT_FALSE(cratchit::open_workspace(root, "2026-10-20", false).has_value());
  EXPECT_FALSE(fs::exists(root / ".cratchit" / "workspace.txt"));
  auto const workspace = cratchit::open_workspace(root, "2026-10-20", true);
  ASSERT_TRUE(workspace.has_value()) << workspace.error();
  EXPECT_EQ(workspace->initiated.size(), 1u);
  EXPECT_EQ(read_file(root / ".cratchit" / "workspace.txt"), "created = 2026-10-20\n");
}

TEST_F(WorkspaceTest, MalformedWorkspaceFileIsAnError) {
  write_file(root / ".cratchit" / "workspace.txt", "nonsense\n");
  auto const workspace = cratchit::open_workspace(root, "2026-10-20", true);
  ASSERT_FALSE(workspace.has_value());
  EXPECT_NE(workspace.error().find("workspace.txt"), std::string::npos);
}

TEST_F(WorkspaceTest, DotCratchitFileIsAnError) {
  std::ofstream(root / ".cratchit") << "not a folder\n";
  EXPECT_FALSE(cratchit::open_workspace(root, "2026-10-20", true).has_value());
}

TEST_F(WorkspaceTest, MissingFolderIsAnError) {
  EXPECT_FALSE(cratchit::open_workspace(root / "gone", "2026-10-20", true).has_value());
  EXPECT_FALSE(fs::exists(root / "gone"));
}

TEST_F(WorkspaceTest, UniqueFolderIsCompletedWithSeparator) {
  fs::create_directories(root / "books");
  write_file(root / "bonus.txt", "a file, not a folder\n");
  auto const completion = cratchit::complete_folder((root / "bo").string());
  EXPECT_EQ(completion.added, "oks" + separator());
  EXPECT_TRUE(completion.candidates.empty());
}

TEST_F(WorkspaceTest, SeveralFoldersAreCompletedAsFarAsTheyAgree) {
  fs::create_directories(root / "books2025");
  fs::create_directories(root / "books2026");
  auto const completion = cratchit::complete_folder((root / "b").string());
  EXPECT_EQ(completion.added, "ooks202");
  EXPECT_TRUE(completion.candidates.empty());
}

TEST_F(WorkspaceTest, SeveralFoldersAreListedWhenNothingCanBeAdded) {
  fs::create_directories(root / "books2026");
  fs::create_directories(root / "books2025");
  auto const completion = cratchit::complete_folder((root / "books202").string());
  EXPECT_EQ(completion.added, "");
  EXPECT_EQ(completion.candidates, (std::vector<std::string>{"books2025", "books2026"}));
}

TEST_F(WorkspaceTest, NoMatchCompletesNothing) {
  fs::create_directories(root / "books");
  auto const completion = cratchit::complete_folder((root / "x").string());
  EXPECT_EQ(completion.added, "");
  EXPECT_TRUE(completion.candidates.empty());
}

TEST_F(WorkspaceTest, HiddenFoldersNeedADot) {
  fs::create_directories(root / ".cratchit");
  fs::create_directories(root / "books");
  EXPECT_EQ(cratchit::complete_folder(root.string() + separator()).added, "books" + separator());
  EXPECT_EQ(cratchit::complete_folder((root / ".").string()).added, "cratchit" + separator());
}

TEST(XdgDocuments, HomeIsReplaced) {
  auto const text = "# comment\nXDG_DESKTOP_DIR=\"$HOME/Desktop\"\nXDG_DOCUMENTS_DIR=\"$HOME/Dokument\"\n";
  EXPECT_EQ(cratchit::xdg_documents_folder(text, "/home/me"), fs::path("/home/me/Dokument"));
}

TEST(XdgDocuments, FullPathIsKept) {
  EXPECT_EQ(cratchit::xdg_documents_folder("XDG_DOCUMENTS_DIR=\"/data/docs\"\n", "/home/me"), fs::path("/data/docs"));
}

TEST(XdgDocuments, MissingOrRelativeIsNone) {
  EXPECT_FALSE(cratchit::xdg_documents_folder("XDG_DESKTOP_DIR=\"$HOME/Desktop\"\n", "/home/me").has_value());
  EXPECT_FALSE(cratchit::xdg_documents_folder("XDG_DOCUMENTS_DIR=\"docs\"\n", "/home/me").has_value());
}

TEST(DefaultWorkspace, EndsInCratchit) {
  auto const folder = cratchit::default_workspace_folder();
  if (folder) {
    EXPECT_EQ(folder->filename(), "cratchit");
    EXPECT_TRUE(folder->is_absolute());
  }
}

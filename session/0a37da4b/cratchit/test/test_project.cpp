#include "core/project.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// A workspace of its own for each test, in an empty temporary folder, removed afterwards
class ProjectTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_project_test_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
    workspace = *cratchit::open_workspace(root / "cratchit", "2026-11-08");
  }
  void TearDown() override { fs::remove_all(root); }

  // A project folder holding project.txt with text
  static fs::path add_project(fs::path const& folder, std::string const& text) {
    fs::create_directories(folder / ".cratchit");
    std::ofstream(folder / ".cratchit" / "project.txt") << text;
    return folder;
  }

  // Adds a line to workspace.txt
  void remember(std::string const& line) { std::ofstream(workspace.file, std::ios::app) << line << '\n'; }

  fs::path root;
  cratchit::Workspace workspace;
};

} // namespace

TEST_F(ProjectTest, FreshWorkspaceRemembersNoProjects) {
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects.has_value()) << projects.error();
  EXPECT_TRUE(projects->empty());
}

TEST_F(ProjectTest, RememberedProjectsAreListedByName) {
  add_project(workspace.folder / "b", "uuid = 2\nname = Beta AB\n");
  add_project(root / "elsewhere", "created = 2026-10-23\nuuid = 1\nname = Gamma AB\norgnr = 556782-8172\n");
  add_project(workspace.folder / "c", "uuid = 3\nname = Alfa AB\n");
  remember("project.2 = b");
  remember("project.1 = " + (root / "elsewhere").string());
  remember("project.3 = " + (workspace.folder / "c").string());
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects.has_value()) << projects.error();
  ASSERT_EQ(projects->size(), 3u);
  EXPECT_EQ((*projects)[0].caption, "Alfa AB");
  EXPECT_EQ((*projects)[0].uuid, "3");
  EXPECT_EQ((*projects)[1].caption, "Beta AB");
  EXPECT_EQ((*projects)[1].folder, workspace.folder / "b"); // relative is read from the workspace folder
  EXPECT_EQ((*projects)[2].caption, "Gamma AB");
  EXPECT_EQ((*projects)[2].folder, root / "elsewhere");
}

TEST_F(ProjectTest, FoldersAreNotSearched) {
  add_project(workspace.folder / "not remembered", "uuid = 1\nname = Alfa AB\n");
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects.has_value()) << projects.error();
  EXPECT_TRUE(projects->empty());
}

TEST_F(ProjectTest, ProjectThatCannotBeOpenedIsListedByFolderName) {
  add_project(workspace.folder / "Fel uuid", "uuid = 2\nname = Alfa AB\n");
  remember("project.1 = Fel uuid");
  remember("project.9 = Saknas");
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects.has_value()) << projects.error();
  ASSERT_EQ(projects->size(), 2u);
  EXPECT_EQ((*projects)[0].caption, "Fel uuid");
  EXPECT_EQ((*projects)[1].caption, "Saknas");
}

TEST_F(ProjectTest, EntryWithoutUuidOrFolderIsAnError) {
  remember("project. = x");
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_FALSE(projects.has_value());
  EXPECT_NE(projects.error().find("saknar uuid"), std::string::npos);
}

TEST_F(ProjectTest, MissingWorkspaceFileIsAnError) {
  fs::remove(workspace.file);
  EXPECT_FALSE(cratchit::list_projects(workspace).has_value());
}

TEST_F(ProjectTest, ProjectIsOpenedWithItsNameAndNotChanged) {
  std::string const text = "created = 2026-10-23\nuuid = bffb66d8-a6bc-4cdc-bace-68bb0de0b1c4\nname = The ITfied AB\n";
  auto const folder = add_project(root / "The ITfied AB", text);
  auto const project = cratchit::open_project(folder, "bffb66d8-a6bc-4cdc-bace-68bb0de0b1c4");
  ASSERT_TRUE(project.has_value()) << project.error();
  EXPECT_EQ(project->folder, folder);
  EXPECT_EQ(project->file, folder / ".cratchit" / "project.txt");
  EXPECT_EQ(project->uuid, "bffb66d8-a6bc-4cdc-bace-68bb0de0b1c4");
  EXPECT_EQ(project->name, "The ITfied AB");
  std::ifstream in(project->file);
  EXPECT_EQ(std::string(std::istreambuf_iterator<char>(in), {}), text);
}

TEST_F(ProjectTest, OtherUuidIsAnError) {
  auto const project = cratchit::open_project(add_project(root / "x", "uuid = 2\nname = Alfa AB\n"), "1");
  ASSERT_FALSE(project.has_value());
  EXPECT_NE(project.error().find("uuid 2 är inte 1"), std::string::npos);
}

TEST_F(ProjectTest, ProjectWithoutUuidIsAnError) {
  auto const project = cratchit::open_project(add_project(root / "x", "name = Alfa AB\n"), "1");
  ASSERT_FALSE(project.has_value());
  EXPECT_NE(project.error().find("'uuid' saknas"), std::string::npos);
}

TEST_F(ProjectTest, ProjectWithoutNameIsAnError) {
  auto const project = cratchit::open_project(add_project(root / "x", "uuid = 1\n"), "1");
  ASSERT_FALSE(project.has_value());
  EXPECT_NE(project.error().find("'name' saknas"), std::string::npos);
}

TEST_F(ProjectTest, MalformedProjectIsAnError) {
  EXPECT_FALSE(cratchit::open_project(add_project(root / "x", "nonsense\n"), "1").has_value());
}

TEST_F(ProjectTest, FolderWithoutProjectFileIsAnError) {
  fs::create_directories(root / "x");
  auto const project = cratchit::open_project(root / "x", "1");
  ASSERT_FALSE(project.has_value());
  EXPECT_NE(project.error().find("saknas"), std::string::npos);
  EXPECT_FALSE(fs::exists(root / "x" / ".cratchit"));
}

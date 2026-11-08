#include "core/project.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// An empty folder of its own for each test, standing in for the workspace, removed afterwards
class ProjectTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    workspace = fs::temp_directory_path() / (std::string("cratchit_project_test_") + info->name());
    fs::remove_all(workspace);
    fs::create_directories(workspace);
  }
  void TearDown() override { fs::remove_all(workspace); }

  // A project folder in the workspace with project.txt holding text
  fs::path add_project(std::string const& folder_name, std::string const& text) {
    auto const folder = workspace / folder_name;
    fs::create_directories(folder / ".cratchit");
    std::ofstream(folder / ".cratchit" / "project.txt") << text;
    return folder;
  }

  fs::path workspace;
};

} // namespace

TEST_F(ProjectTest, EmptyWorkspaceHasNoProjects) {
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects.has_value()) << projects.error();
  EXPECT_TRUE(projects->empty());
}

TEST_F(ProjectTest, ProjectsAreListedByNameAndOtherThingsAreSkipped) {
  add_project("b", "name = Beta AB\n");
  add_project("a", "created = 2026-10-23\nname = Gamma AB\norgnr = 556782-8172\n");
  add_project("c", "name = Alfa AB\n");
  fs::create_directories(workspace / "not a project");
  std::ofstream(workspace / "notes.txt") << "mine\n";
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects.has_value()) << projects.error();
  ASSERT_EQ(projects->size(), 3u);
  EXPECT_EQ((*projects)[0].caption, "Alfa AB");
  EXPECT_EQ((*projects)[0].folder, workspace / "c");
  EXPECT_EQ((*projects)[1].caption, "Beta AB");
  EXPECT_EQ((*projects)[2].caption, "Gamma AB");
}

TEST_F(ProjectTest, UnreadableProjectIsListedByFolderName) {
  add_project("Trasigt", "nonsense\n");
  auto const projects = cratchit::list_projects(workspace);
  ASSERT_TRUE(projects.has_value()) << projects.error();
  ASSERT_EQ(projects->size(), 1u);
  EXPECT_EQ((*projects)[0].caption, "Trasigt");
}

TEST_F(ProjectTest, MissingWorkspaceIsAnError) {
  auto const projects = cratchit::list_projects(workspace / "missing");
  ASSERT_FALSE(projects.has_value());
  EXPECT_NE(projects.error().find("Kan inte läsa"), std::string::npos);
}

TEST_F(ProjectTest, ProjectIsOpenedWithItsNameAndNotChanged) {
  std::string const text = "created = 2026-10-23\nuuid = bffb66d8-a6bc-4cdc-bace-68bb0de0b1c4\nname = The ITfied AB\n";
  auto const folder = add_project("The ITfied AB", text);
  auto const project = cratchit::open_project(folder);
  ASSERT_TRUE(project.has_value()) << project.error();
  EXPECT_EQ(project->folder, folder);
  EXPECT_EQ(project->file, folder / ".cratchit" / "project.txt");
  EXPECT_EQ(project->name, "The ITfied AB");
  std::ifstream in(project->file);
  EXPECT_EQ(std::string(std::istreambuf_iterator<char>(in), {}), text);
}

TEST_F(ProjectTest, ProjectWithoutNameIsAnError) {
  auto const project = cratchit::open_project(add_project("x", "created = 2026-10-23\n"));
  ASSERT_FALSE(project.has_value());
  EXPECT_NE(project.error().find("'name' saknas"), std::string::npos);
}

TEST_F(ProjectTest, MalformedProjectIsAnError) {
  EXPECT_FALSE(cratchit::open_project(add_project("x", "nonsense\n")).has_value());
}

TEST_F(ProjectTest, FolderWithoutProjectFileIsAnError) {
  fs::create_directories(workspace / "x");
  auto const project = cratchit::open_project(workspace / "x");
  ASSERT_FALSE(project.has_value());
  EXPECT_NE(project.error().find("saknas"), std::string::npos);
  EXPECT_FALSE(fs::exists(workspace / "x" / ".cratchit"));
}

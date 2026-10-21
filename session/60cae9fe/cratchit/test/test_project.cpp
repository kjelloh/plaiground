#include "core/project.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

constexpr char const* uuid_a = "3f2b8c1e-9a4d-4e7f-b6a2-1c5d8e0f7a93";
constexpr char const* uuid_b = "0a1b2c3d-4e5f-4a6b-8c7d-8e9f0a1b2c3d";

// An empty folder of its own for each test, removed afterwards
class ProjectTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_project_") + info->name());
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

TEST(Uuid, FormatSetsVersionAndVariant) {
  EXPECT_EQ(cratchit::format_uuid(0, 0), "00000000-0000-4000-8000-000000000000");
  EXPECT_EQ(cratchit::format_uuid(~0ULL, ~0ULL), "ffffffff-ffff-4fff-bfff-ffffffffffff");
  EXPECT_EQ(cratchit::format_uuid(0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL), "01234567-89ab-4def-bedc-ba9876543210");
}

TEST(Uuid, NewIsAValidVersion4Uuid) {
  auto const uuid = cratchit::new_uuid();
  EXPECT_TRUE(cratchit::is_uuid(uuid)) << uuid;
  EXPECT_EQ(uuid[14], '4');
  EXPECT_NE(std::string("89ab").find(uuid[19]), std::string::npos) << uuid;
  EXPECT_NE(cratchit::new_uuid(), uuid);
}

TEST(Uuid, OnlyLowercase8_4_4_4_12HexIsAUuid) {
  EXPECT_TRUE(cratchit::is_uuid(uuid_a));
  EXPECT_FALSE(cratchit::is_uuid("3F2B8C1E-9A4D-4E7F-B6A2-1C5D8E0F7A93"));
  EXPECT_FALSE(cratchit::is_uuid("3f2b8c1e9a4d4e7fb6a21c5d8e0f7a93"));
  EXPECT_FALSE(cratchit::is_uuid("3f2b8c1e-9a4d-4e7f-b6a2-1c5d8e0f7a9g"));
  EXPECT_FALSE(cratchit::is_uuid(""));
}

TEST_F(ProjectTest, ListedProjectsAreInFileOrderWithRelativeFoldersFromTheWorkspace) {
  auto const file = root / "ws" / ".cratchit" / "workspace.txt";
  write_file(file, std::format("created = 2026-10-21\nproject.{} = books\nproject = {}\nproject.{} = /elsewhere/x\n",
                               uuid_b, uuid_a, uuid_a));
  auto const listed = cratchit::listed_projects(file);
  ASSERT_TRUE(listed.has_value()) << listed.error();
  ASSERT_EQ(listed->size(), 2u);
  EXPECT_EQ((*listed)[0].uuid, uuid_b);
  EXPECT_EQ((*listed)[0].folder, root / "ws" / "books");
  EXPECT_EQ((*listed)[1].uuid, uuid_a);
  EXPECT_EQ((*listed)[1].folder, fs::path("/elsewhere/x").lexically_normal());
}

TEST_F(ProjectTest, ListedPathWithoutUuidIsAnError) {
  auto const file = root / ".cratchit" / "workspace.txt";
  write_file(file, "project.itfied = books\n");
  auto const listed = cratchit::listed_projects(file);
  ASSERT_FALSE(listed.has_value());
  EXPECT_NE(listed.error().find("'project.itfied'"), std::string::npos) << listed.error();
}

TEST_F(ProjectTest, LastProjectIsTheProjectEntry) {
  auto const file = root / ".cratchit" / "workspace.txt";
  write_file(file, "created = 2026-10-21\n");
  EXPECT_FALSE(cratchit::last_project(file).value().has_value());
  write_file(file, std::format("project = {}\n", uuid_a));
  EXPECT_EQ(cratchit::last_project(file).value(), uuid_a);
}

TEST_F(ProjectTest, RememberListsAndSetsTheLastProject) {
  auto const file = root / ".cratchit" / "workspace.txt";
  write_file(file, "created = 2026-10-21\n");
  EXPECT_EQ(cratchit::remember_project(file, {uuid_a, "/a"}).value(), true);
  EXPECT_EQ(read_file(file), std::format("created = 2026-10-21\nproject.{} = /a\nproject = {}\n", uuid_a, uuid_a));
  EXPECT_EQ(cratchit::remember_project(file, {uuid_b, "/b"}).value(), true);
  EXPECT_EQ(read_file(file), std::format("created = 2026-10-21\nproject.{} = /a\nproject.{} = /b\nproject = {}\n",
                                         uuid_a, uuid_b, uuid_b));
}

TEST_F(ProjectTest, RememberKeepsAListedFolderAndLeavesAnUnchangedFile) {
  auto const file = root / ".cratchit" / "workspace.txt";
  auto const text = std::format("project.{} = books\nproject = {}\n", uuid_a, uuid_a);
  write_file(file, text);
  EXPECT_EQ(cratchit::remember_project(file, {uuid_a, root / "books"}).value(), false);
  EXPECT_EQ(read_file(file), text);
}

TEST_F(ProjectTest, CreateWritesProjectFile) {
  auto const project = cratchit::create_project(root, "ITfied AB", uuid_a, "2026-10-21");
  ASSERT_TRUE(project.has_value()) << project.error();
  EXPECT_EQ(read_file(root / ".cratchit" / "project.txt"),
            std::format("created = 2026-10-21\nuuid = {}\nname = ITfied AB\n", uuid_a));
  EXPECT_EQ(project->uuid, uuid_a);
  EXPECT_EQ(project->name, "ITfied AB");
  ASSERT_EQ(project->initiated.size(), 1u);
  EXPECT_EQ(project->initiated[0].file, fs::path(".cratchit") / "project.txt");
}

TEST_F(ProjectTest, CreateBesideAWorkspaceFileKeepsIt) {
  write_file(root / ".cratchit" / "workspace.txt", "created = 2026-10-20\n");
  ASSERT_TRUE(cratchit::create_project(root, "X", uuid_a, "2026-10-21").has_value());
  EXPECT_EQ(read_file(root / ".cratchit" / "workspace.txt"), "created = 2026-10-20\n");
}

TEST_F(ProjectTest, CreateOverAProjectIsAnError) {
  ASSERT_TRUE(cratchit::create_project(root, "X", uuid_a, "2026-10-21").has_value());
  auto const again = cratchit::create_project(root, "Y", uuid_b, "2026-10-21");
  ASSERT_FALSE(again.has_value());
  EXPECT_NE(read_file(root / ".cratchit" / "project.txt").find("name = X"), std::string::npos);
}

TEST_F(ProjectTest, CreateInAMissingFolderIsAnError) {
  EXPECT_FALSE(cratchit::create_project(root / "gone", "X", uuid_a, "2026-10-21").has_value());
}

TEST_F(ProjectTest, ReadNeedsUuidAndName) {
  write_file(root / ".cratchit" / "project.txt", "created = 2026-10-21\nname = X\n");
  EXPECT_FALSE(cratchit::read_project(root).has_value());
  write_file(root / ".cratchit" / "project.txt", std::format("uuid = {}\n", uuid_a));
  EXPECT_FALSE(cratchit::read_project(root).has_value());
  write_file(root / ".cratchit" / "project.txt", std::format("uuid = {}\nname = Förening\n", uuid_a));
  auto const project = cratchit::read_project(root);
  ASSERT_TRUE(project.has_value()) << project.error();
  EXPECT_EQ(project->name, "Förening");
}

TEST_F(ProjectTest, ReadWithoutProjectFileIsAnError) {
  EXPECT_FALSE(cratchit::read_project(root).has_value());
  EXPECT_FALSE(cratchit::read_project(root / "gone").has_value());
}

TEST_F(ProjectTest, ListedProjectMustHoldTheListedUuid) {
  ASSERT_TRUE(cratchit::create_project(root, "X", uuid_a, "2026-10-21").has_value());
  EXPECT_TRUE(cratchit::open_listed_project({uuid_a, root}).has_value());
  auto const wrong = cratchit::open_listed_project({uuid_b, root});
  ASSERT_FALSE(wrong.has_value());
  EXPECT_NE(wrong.error().find(uuid_b), std::string::npos) << wrong.error();
}

TEST(DefaultProjectFolder, IsTheNameInTheWorkspace) {
  EXPECT_EQ(cratchit::default_project_folder("/ws", "ITfied AB"), fs::path("/ws") / "ITfied AB");
  EXPECT_EQ(cratchit::default_project_folder("/ws", "Räkenskaper"), fs::path("/ws") / fs::path(u8"Räkenskaper"));
}

TEST(DefaultProjectFolder, NoneForNamesThatCannotBeAFolder) {
  for (auto const* name : {"", ".", "..", "A/B", "A\\B", "A:B", "A?", "A.", "A "}) {
    EXPECT_FALSE(cratchit::default_project_folder("/ws", name).has_value()) << name;
  }
}

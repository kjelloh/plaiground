#include "ui/add_project_menu.hpp"
#include "ui/project_menu.hpp"
#include "ui/root_menu.hpp"
#include "ui/test_menu.hpp"
#include "ui/workspace_menu.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using pyroceph::tui::Navigator;
using pyroceph::tui::OptionsList;
using pyroceph::tui::View;

namespace {

// A project folder with project.txt holding text, remembered in workspace.txt of
// the workspace in workspace_folder as `project.<uuid> = <folder_name>`
void add_project(fs::path const& workspace_folder, std::string const& folder_name, std::string const& uuid,
                 std::string const& text) {
  fs::create_directories(workspace_folder / folder_name / ".cratchit");
  std::ofstream(workspace_folder / folder_name / ".cratchit" / "project.txt") << text;
  std::ofstream(workspace_folder / ".cratchit" / "workspace.txt", std::ios::app)
      << "project." << uuid << " = " << folder_name << '\n';
}

// An empty folder of its own for each test, standing in for Documents, removed afterwards
class RootMenuTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    documents = fs::temp_directory_path() / (std::string("cratchit_ui_test_") + info->name());
    fs::remove_all(documents);
    fs::create_directories(documents);
  }
  void TearDown() override { fs::remove_all(documents); }

  fs::path documents;
};

} // namespace

TEST_F(RootMenuTest, IsCratchitWithWorkspaceAsZeroAndTestMenuAsNine) {
  auto const root = cratchit::ui::make_root_menu(documents / "cratchit");
  EXPECT_EQ(root->name(), "Cratchit");
  EXPECT_TRUE(root->view().empty());
  EXPECT_EQ(root->options(), (OptionsList{{U'0', "Samlingsmapp"}, {U'9', "Testmeny"}}));
}

TEST_F(RootMenuTest, ZeroCreatesTheWorkspaceTheFirstTime) {
  auto const folder = documents / "cratchit";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(navigator.current().view(), (View{"Ny samlingsmapp skapad.", folder.string()}));
  EXPECT_TRUE(fs::is_regular_file(folder / ".cratchit" / "workspace.txt"));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Cratchit");
}

TEST_F(RootMenuTest, ZeroOpensAnExistingWorkspace) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2020-01-01"));
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(navigator.current().view(), (View{folder.string()}));
}

TEST_F(RootMenuTest, ZeroLeadsToAnErrorWhenTheWorkspaceCannotBeOpened) {
  // Documents itself is missing, and only the workspace folder is created
  auto const folder = documents / "saknas" / "cratchit";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Fel");
  ASSERT_EQ(navigator.current().view().size(), 1u);
  EXPECT_TRUE(navigator.current().view()[0].starts_with("Kunde inte öppna samlingsmappen: "));
  EXPECT_FALSE(fs::exists(documents / "saknas"));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Cratchit");
}

TEST(RootMenu, ZeroLeadsToAnErrorWhenTheWorkspaceFolderIsNotKnown) {
  Navigator navigator(cratchit::ui::make_root_menu(std::unexpected(std::string("Hittar inte mappen Dokument"))));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Fel");
  EXPECT_EQ(navigator.current().view(), (View{"Kunde inte öppna samlingsmappen: Hittar inte mappen Dokument"}));
}

TEST_F(RootMenuTest, NineLeadsToTheTestMenu) {
  Navigator navigator(cratchit::ui::make_root_menu(documents / "cratchit"));
  EXPECT_TRUE(navigator.press(U'9'));
  EXPECT_EQ(navigator.current().name(), "Testmeny");
  EXPECT_FALSE(fs::exists(documents / "cratchit"));
}

TEST_F(RootMenuTest, OtherKeysAreNotMapped) {
  auto root = cratchit::ui::make_root_menu(documents / "cratchit");
  EXPECT_EQ(root->on_key(U'1'), nullptr);
}

TEST(TestMenu, KeepsItsStates) {
  Navigator navigator(cratchit::ui::make_test_menu());
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Projekt"}, {U'2', "Bokslut"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Projekt");
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Öppna projekt");
  navigator.back();
  navigator.back();
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(TestMenu, ProjektHasOneOpenThatWorksAndOneThatFails) {
  Navigator navigator(cratchit::ui::make_test_menu());
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Öppna"}, {U'2', "Öppna (misslyckas)"}}));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Fel");
  EXPECT_EQ(navigator.current().view(), (pyroceph::tui::View{"Kunde inte öppna projektet."}));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Projekt");
}

TEST(TestMenu, BokslutTakesAYearOfFourDigits) {
  Navigator navigator(cratchit::ui::make_test_menu());
  navigator.press(U'2');
  auto const accepted = navigator.current().on_input(U"2025");
  ASSERT_TRUE(accepted);
  EXPECT_EQ(*accepted, "Bokslut för 2025 noterat.");
  EXPECT_FALSE(navigator.current().on_input(U"25"));
}

TEST_F(RootMenuTest, WorkspaceListsItsProjectsAndOpensOne) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  add_project(folder, "itfied", "1", "created = 2026-10-23\nuuid = 1\nname = The ITfied AB\n");
  add_project(folder, "annat", "2", "uuid = 2\nname = Annat AB\n");
  add_project(folder, "not remembered", "3", "uuid = 3\nname = Glömt AB\n");
  // The workspace lists only what it remembers: forget the last one again
  std::ofstream(folder / ".cratchit" / "workspace.txt") << "created = 2026-11-08\nproject.1 = itfied\nproject.2 = annat\n";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Annat AB"}, {U'2', "The ITfied AB"}, {U'+', "Lägg till bokföring"}}));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "The ITfied AB");
  EXPECT_EQ(navigator.current().view(), (View{(folder / "itfied").string()}));
  EXPECT_TRUE(navigator.current().options().empty());
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
}

TEST_F(RootMenuTest, ProjectThatCannotBeOpenedLeadsToAnError) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  add_project(folder, "Utan namn", "1", "uuid = 1\n");
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Utan namn"}, {U'+', "Lägg till bokföring"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Fel");
  ASSERT_EQ(navigator.current().view().size(), 1u);
  EXPECT_TRUE(navigator.current().view()[0].starts_with("Kunde inte öppna bokföringen: "));
  EXPECT_TRUE(navigator.current().view()[0].ends_with("'name' saknas"));
}

namespace {

cratchit::Workspace workspace_at(fs::path const& folder) {
  return {folder, folder / ".cratchit" / "workspace.txt", {}};
}

} // namespace

TEST(WorkspaceMenu, WithoutProjectsShowsItsFolderAndOffersOnlyToAdd) {
  auto workspace = cratchit::ui::make_workspace_menu(workspace_at("/x/cratchit"), {});
  EXPECT_EQ(workspace->view(), (View{fs::path("/x/cratchit").string()}));
  EXPECT_EQ(workspace->options(), (OptionsList{{U'+', "Lägg till bokföring"}}));
  EXPECT_EQ(workspace->on_key(U'1'), nullptr);
  EXPECT_EQ(workspace->on_key(U'0'), nullptr);
  auto const input = workspace->on_input(U"x");
  ASSERT_FALSE(input);
  EXPECT_EQ(input.error(), pyroceph::tui::no_input_error);
}

TEST(WorkspaceMenu, OffersNineProjectsAndSaysWhenThereAreMore) {
  std::vector<cratchit::ListedProject> projects;
  for (char c = 'a'; c <= 'j'; ++c) projects.push_back({std::string(1, c), fs::path("/x/cratchit") / std::string(1, c), std::string(1, c)});
  auto workspace = cratchit::ui::make_workspace_menu(workspace_at("/x/cratchit"), projects);
  auto const options = workspace->options();
  ASSERT_EQ(options.size(), 10u);
  EXPECT_EQ(options[0], (pyroceph::tui::Option{U'1', "a"}));
  EXPECT_EQ(options[8], (pyroceph::tui::Option{U'9', "i"}));
  EXPECT_EQ(options[9], (pyroceph::tui::Option{U'+', "Lägg till bokföring"}));
  EXPECT_EQ(workspace->view(), (View{fs::path("/x/cratchit").string(), "Fler bokföringar finns."}));
}

TEST(ProjectMenu, IsNamedByTheProjectAndShowsItsFolder) {
  auto project = cratchit::ui::make_project_menu({"/x/p", "/x/p/.cratchit/project.txt", "1", "The ITfied AB"});
  EXPECT_EQ(project->name(), "The ITfied AB");
  EXPECT_EQ(project->view(), (View{fs::path("/x/p").string()}));
  EXPECT_TRUE(project->options().empty());
  EXPECT_EQ(project->on_key(U'1'), nullptr);
}

TEST(WorkspaceMenu, PlusLeadsToAddWithOpenExistingAndNew) {
  Navigator navigator(cratchit::ui::make_workspace_menu(workspace_at("/x/cratchit"), {}));
  ASSERT_TRUE(navigator.press(U'+'));
  EXPECT_EQ(navigator.current().name(), "Lägg till");
  EXPECT_TRUE(navigator.current().view().empty());
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Öppna befintlig"}, {U'2', "Ny bokföring"}}));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
}

TEST(AddProjectMenu, OpenExistingAndNewAreEmptyForNow) {
  for (auto const& [key, name] : {std::pair{U'1', "Öppna befintlig"}, std::pair{U'2', "Ny bokföring"}}) {
    Navigator navigator(cratchit::ui::make_add_project_menu());
    ASSERT_TRUE(navigator.press(key));
    EXPECT_EQ(navigator.current().name(), name);
    EXPECT_TRUE(navigator.current().view().empty());
    EXPECT_TRUE(navigator.current().options().empty());
    EXPECT_EQ(navigator.current().on_key(U'1'), nullptr);
    EXPECT_FALSE(navigator.current().on_input(U"x"));
    EXPECT_TRUE(navigator.back());
    EXPECT_EQ(navigator.current().name(), "Lägg till");
  }
}

TEST(AddProjectMenu, OtherKeysAreNotMapped) {
  EXPECT_EQ(cratchit::ui::make_add_project_menu()->on_key(U'3'), nullptr);
}

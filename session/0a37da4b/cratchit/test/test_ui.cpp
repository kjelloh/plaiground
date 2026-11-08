#include "ui/root_menu.hpp"
#include "ui/test_menu.hpp"
#include "ui/workspace_menu.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

using pyroceph::tui::Navigator;
using pyroceph::tui::OptionsList;
using pyroceph::tui::View;

namespace {

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

TEST(WorkspaceMenu, ShowsItsFolderAndHasNoOptionsYet) {
  cratchit::Workspace opened{"/x/cratchit", "/x/cratchit/.cratchit/workspace.txt", {}};
  auto workspace = cratchit::ui::make_workspace_menu(opened);
  EXPECT_EQ(workspace->view(), (View{fs::path("/x/cratchit").string()}));
  EXPECT_TRUE(workspace->options().empty());
  EXPECT_EQ(workspace->on_key(U'1'), nullptr);
  auto const input = workspace->on_input(U"x");
  ASSERT_FALSE(input);
  EXPECT_EQ(input.error(), pyroceph::tui::no_input_error);
}

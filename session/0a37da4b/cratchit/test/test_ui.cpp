#include "core/path_value.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/add_project_menu.hpp"
#include "ui/design_insufficiency.hpp"
#include "ui/help_topics.hpp"
#include "ui/project_menu.hpp"
#include "ui/root_menu.hpp"
#include "ui/test_menu.hpp"
#include "ui/workspace_menu.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
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
  EXPECT_EQ(navigator.current().view(), (View{"cratchit skapade en ny samlingsmapp.", folder.string()}));
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
  EXPECT_TRUE(navigator.current().view()[0].starts_with("cratchit kunde inte öppna samlingsmappen: "));
  EXPECT_FALSE(fs::exists(documents / "saknas"));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Cratchit");
}

TEST(RootMenu, ZeroLeadsToAnErrorWhenTheWorkspaceFolderIsNotKnown) {
  Navigator navigator(cratchit::ui::make_root_menu(std::unexpected(std::string("Hittar inte mappen Dokument"))));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().name(), "Fel");
  EXPECT_EQ(navigator.current().view(), (View{"cratchit kunde inte öppna samlingsmappen: Hittar inte mappen Dokument"}));
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
  EXPECT_EQ(accepted->reply, "Bokslut för 2025 noterat.");
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
  EXPECT_EQ(navigator.current().view(), (View{(folder / "itfied").string(), "Bokföringen har inga räkenskapsår än."}));
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
  EXPECT_TRUE(navigator.current().view()[0].starts_with("cratchit kunde inte öppna bokföringen: "));
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
  EXPECT_EQ(workspace->view(), (View{fs::path("/x/cratchit").string(), "Samlingsmappen har fler bokföringar än cratchit visar här."}));
}

TEST(ProjectMenu, IsNamedByTheProjectAndShowsItsFolder) {
  auto project = cratchit::ui::make_project_menu({"/x/p", "/x/p/.cratchit/project.txt", "1", "The ITfied AB"});
  EXPECT_EQ(project->name(), "The ITfied AB");
  EXPECT_EQ(project->view(), (View{fs::path("/x/p").string(), "Bokföringen har inga räkenskapsår än."}));
  EXPECT_TRUE(project->options().empty());
  EXPECT_EQ(project->on_key(U'1'), nullptr);
}

namespace {

// A project in folder whose books.txt holds text
cratchit::Project project_with_books(fs::path const& folder, std::string const& text) {
  fs::create_directories(folder / ".cratchit" / "books");
  std::ofstream(folder / ".cratchit" / "books" / "books.txt") << text;
  return {folder, folder / ".cratchit" / "project.txt", "1", "The ITfied AB"};
}

} // namespace

TEST_F(RootMenuTest, ProjectOffersItsFiscalYearsTheCurrentAsZero) {
  Navigator navigator(cratchit::ui::make_project_menu(project_with_books(
      documents / "p", "chart_type = EUBAS97\nyear.2024-05-01 = 2025-04-30\nyear.2025-05-01 = 2026-04-30\n")));
  EXPECT_EQ(navigator.current().view(), (View{(documents / "p").string()}));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "2025/26 (2025-05-01 – 2026-04-30)"},
                                                        {U'1', "2024/25 (2024-05-01 – 2025-04-30)"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "2024/25");
  EXPECT_EQ(navigator.current().view(), (View{"Räkenskapsår 2024-05-01 – 2025-04-30"}));
  EXPECT_TRUE(navigator.current().options().empty());
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "The ITfied AB");
}

TEST_F(RootMenuTest, ProjectSaysWhenItsFiscalYearsCannotBeRead) {
  auto project = cratchit::ui::make_project_menu(project_with_books(documents / "p", "year.2025 = 2026-04-30\n"));
  ASSERT_EQ(project->view().size(), 2u);
  EXPECT_TRUE(project->view()[1].starts_with("cratchit kunde inte läsa räkenskapsåren: "));
  EXPECT_TRUE(project->options().empty());
}

TEST_F(RootMenuTest, ProjectOffersTenFiscalYearsAndSaysWhenThereAreMore) {
  std::string text;
  for (int year = 2009; year < 2020; ++year) text += std::format("year.{}-01-01 = {}-12-31\n", year, year);
  auto project = cratchit::ui::make_project_menu(project_with_books(documents / "p", text));
  ASSERT_EQ(project->options().size(), 10u);
  EXPECT_EQ(project->options().front(), (pyroceph::tui::Option{U'0', "2019 (2019-01-01 – 2019-12-31)"}));
  EXPECT_EQ(project->options().back(), (pyroceph::tui::Option{U'9', "2010 (2010-01-01 – 2010-12-31)"}));
  EXPECT_EQ(project->view().back(), "Bokföringen har fler räkenskapsår än cratchit visar här.");
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

TEST(AddProjectMenu, OneLeadsToOpenExisting) {
  Navigator navigator(cratchit::ui::make_add_project_menu(workspace_at("/x/cratchit"), "/x"));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Öppna befintlig");
  EXPECT_FALSE(navigator.current().view().empty());
  EXPECT_TRUE(navigator.current().options().empty());
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of("/x"));
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Lägg till");
}

TEST_F(RootMenuTest, OpenExistingCompletesFolderNames) {
  fs::create_directories(documents / "The ITfied AB");
  auto state = cratchit::ui::make_open_existing_project(workspace_at(documents / "cratchit"), documents);
  auto const typed = state->start_input() + U"The";
  auto const separator = static_cast<char32_t>(fs::path::preferred_separator);
  EXPECT_EQ(state->complete(typed).added, U" ITfied AB" + std::u32string(1, separator));
}

TEST_F(RootMenuTest, OpenExistingOpensTheProjectAndTheWorkspaceRemembersIt) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  fs::create_directories(documents / "elsewhere" / "itfied" / ".cratchit");
  std::ofstream(documents / "elsewhere" / "itfied" / ".cratchit" / "project.txt") << "uuid = 7\nname = The ITfied AB\n";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'+', "Lägg till bokföring"}}));
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'1'));
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(documents / "elsewhere" / "itfied"));
  ASSERT_TRUE(result) << result.error();
  EXPECT_EQ(result->reply, "Samlingsmappen kommer ihåg The ITfied AB.");
  ASSERT_TRUE(result->next);
  navigator.enter(std::move(result->next));
  EXPECT_EQ(navigator.current().name(), "The ITfied AB");
  EXPECT_EQ(navigator.current().view(),
            (View{(documents / "elsewhere" / "itfied").string(), "Bokföringen har inga räkenskapsår än."}));
  auto const entries = cratchit::read_path_values(folder / ".cratchit" / "workspace.txt");
  ASSERT_TRUE(entries);
  EXPECT_EQ(cratchit::find_value(*entries, "project.7"), (documents / "elsewhere" / "itfied").string());
  // Back in the workspace, the project is listed
  for (int i = 0; i < 3; ++i) ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "The ITfied AB"}, {U'+', "Lägg till bokföring"}}));
}

TEST_F(RootMenuTest, OpenExistingRefusesAFolderWithoutAProject) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  auto state = cratchit::ui::make_open_existing_project(*cratchit::open_workspace(folder, "2026-11-08"), documents);
  auto const result = state->on_input(pyroceph::tui::path_input_of(documents));
  ASSERT_FALSE(result);
  EXPECT_TRUE(result.error().starts_with("cratchit kunde inte öppna bokföringen: "));
  EXPECT_TRUE(result.error().ends_with("saknas"));
}

TEST(AddProjectMenu, NewOffersFromSieAndFromDetails) {
  Navigator navigator(cratchit::ui::make_add_project_menu(workspace_at("/x/cratchit"), "/x"));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Ny bokföring");
  EXPECT_TRUE(navigator.current().view().empty());
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Från SIE"}, {U'2', "Från uppgifter"}}));
  EXPECT_EQ(navigator.current().on_key(U'3'), nullptr);
  EXPECT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().name(), "Lägg till");
}

TEST(AddProjectMenu, FromSieStartsInTheStartFolder) {
  Navigator navigator(cratchit::ui::make_add_project_menu(workspace_at("/x/cratchit"), "/x/cratchit"));
  ASSERT_TRUE(navigator.press(U'2'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Från SIE");
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of("/x/cratchit"));
}

TEST(AddProjectMenu, OtherKeysAreNotMapped) {
  EXPECT_EQ(cratchit::ui::make_add_project_menu(workspace_at("/x/cratchit"), "/x")->on_key(U'3'), nullptr);
}

TEST_F(RootMenuTest, EveryStateHasItsHelpTopic) {
  namespace topic = cratchit::ui::help_topic;
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-08"));
  add_project(folder, "itfied", "1", "uuid = 1\nname = The ITfied AB\n");
  project_with_books(folder / "itfied", "year.2025-05-01 = 2026-04-30\n");
  add_project(folder, "trasig", "2", "uuid = 2\n");
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  EXPECT_EQ(navigator.current().help_topic(), topic::root);
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().help_topic(), topic::workspace);
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().help_topic(), topic::project);
  ASSERT_TRUE(navigator.press(U'0'));
  EXPECT_EQ(navigator.current().help_topic(), topic::fiscal_year);
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'2')); // trasig has no name
  EXPECT_EQ(navigator.current().help_topic(), topic::error);
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'+'));
  EXPECT_EQ(navigator.current().help_topic(), topic::add_project);
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().help_topic(), topic::open_existing);
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().help_topic(), topic::new_project);
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().help_topic(), topic::from_sie);
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().help_topic(), topic::from_details);
  auto next = [&navigator](std::u32string_view text) {
    auto result = navigator.current().on_input(text);
    ASSERT_TRUE(result) << result.error();
    navigator.enter(std::move(result->next));
  };
  next(U"Exempel AB");
  EXPECT_EQ(navigator.current().help_topic(), topic::details_orgnr);
  next(U"556677-8899");
  EXPECT_EQ(navigator.current().help_topic(), topic::details_year);
  next(U"2026-01-01 2026-12-31");
  EXPECT_EQ(navigator.current().help_topic(), topic::confirm_details_project);
  EXPECT_EQ(cratchit::ui::make_design_insufficiency_state("Val", "något")->help_topic(), topic::design_insufficiency);
}

TEST(WorkspaceMenu, OpenExistingStartsInTheWorkspace) {
  Navigator navigator(cratchit::ui::make_workspace_menu(workspace_at("/x/cratchit"), {}));
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().start_input(), pyroceph::tui::path_input_of("/x/cratchit"));
}

TEST_F(RootMenuTest, FromSieCreatesTheProjectAndTheWorkspaceRemembersIt) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-09"));
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n"
         "#VER A 1 20250201 \"Sale\"\r\n{\r\n#TRANS 1930 {} 500.00\r\n#TRANS 3010 {} -500.00\r\n}\r\n";
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'2'));
  ASSERT_TRUE(navigator.press(U'1'));
  // Tab completes the SIE file
  auto const typed = navigator.current().start_input() + U"te";
  EXPECT_EQ(navigator.current().complete(typed).added, U"st.se");
  auto result = navigator.current().on_input(pyroceph::tui::path_input_of(folder) + U"test.se");
  ASSERT_TRUE(result) << result.error();
  ASSERT_TRUE(result->next);
  navigator.enter(std::move(result->next));
  EXPECT_EQ(navigator.current().name(), "Bekräfta");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Skapa bokföringen"}}));
  EXPECT_FALSE(fs::exists(folder / "Test AB")); // nothing created before 1
  // 1 creates the project and goes back to where the user started, the workspace, which lists it
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_TRUE(fs::exists(folder / "Test AB" / "test.se"));
  EXPECT_EQ(navigator.path().size(), 2u);
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Test AB"}, {U'+', "Lägg till bokföring"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Test AB");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "2025 (2025-01-01 – 2025-12-31)"}}));
}

TEST_F(RootMenuTest, FromDetailsCreatesTheProjectAndTheWorkspaceRemembersIt) {
  auto const folder = documents / "cratchit";
  ASSERT_TRUE(cratchit::open_workspace(folder, "2026-11-09"));
  Navigator navigator(cratchit::ui::make_root_menu(folder));
  ASSERT_TRUE(navigator.press(U'0'));
  ASSERT_TRUE(navigator.press(U'+'));
  ASSERT_TRUE(navigator.press(U'2'));
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Från uppgifter");
  EXPECT_TRUE(navigator.current().start_input().empty());
  // Each step refuses what it cannot take, and leads on with what it can
  EXPECT_FALSE(navigator.current().on_input(U"A/B"));
  auto name = navigator.current().on_input(U"Exempel AB");
  ASSERT_TRUE(name) << name.error();
  navigator.enter(std::move(name->next));
  EXPECT_EQ(navigator.current().name(), "Exempel AB");
  EXPECT_FALSE(navigator.current().on_input(U"556677-8898")); // wrong check digit
  auto orgnr = navigator.current().on_input(U"5566778899");
  ASSERT_TRUE(orgnr) << orgnr.error();
  navigator.enter(std::move(orgnr->next));
  EXPECT_EQ(navigator.current().name(), "556677-8899");
  EXPECT_TRUE(navigator.current().start_input().ends_with(U"-01-01 " + navigator.current().start_input().substr(0, 4) + U"-12-31"));
  auto year = navigator.current().on_input(U"2026-05-01 2027-04-30");
  ASSERT_TRUE(year) << year.error();
  navigator.enter(std::move(year->next));
  EXPECT_EQ(navigator.current().name(), "Bekräfta");
  EXPECT_FALSE(fs::exists(folder / "Exempel AB")); // nothing created before 1
  // Back to the name: what was given is offered again
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().start_input(), U"5566778899");
  ASSERT_TRUE(navigator.back());
  EXPECT_EQ(navigator.current().start_input(), U"Exempel AB");
  // On to Bekräfta again, and create
  for (auto const text : {U"Exempel AB", U"556677-8899", U"2026-05-01 2027-04-30"}) {
    auto result = navigator.current().on_input(text);
    ASSERT_TRUE(result) << result.error();
    navigator.enter(std::move(result->next));
  }
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Samlingsmapp");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Exempel AB"}, {U'+', "Lägg till bokföring"}}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Exempel AB");
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'0', "2026/27 (2026-05-01 – 2027-04-30)"}}));
}

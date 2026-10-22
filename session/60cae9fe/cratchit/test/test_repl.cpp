#include "core/repl.hpp"

#include "core/data_folder.hpp"
#include "core/key_input.hpp"
#include "core/project.hpp"
#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

using cratchit::Repl;
using cratchit::Section;
using cratchit::Step;

namespace {

// Feeds the keys of a string in charset one at a time, then reports end of input
cratchit::KeySource keys_from(std::string keys, cratchit::Charset charset = cratchit::Charset::Utf8) {
  auto reader = std::make_shared<cratchit::KeyReader>(
      charset, [keys = std::move(keys), pos = std::size_t{0}]() mutable -> std::optional<char> {
        if (pos == keys.size()) {
          return std::nullopt;
        }
        return keys[pos++];
      });
  return [reader] { return reader->next(); };
}

Section const manual{"Manual", "", {{"A", "Text A\n", {}}}};

std::string const main_keys = "Keys at cratchit>:\n"
                              "  w  Workspace menu\n"
                              "  l  Show the log\n"
                              "  h  Help (the manual)\n"
                              "  q  Quit\n";

} // namespace

TEST(Repl, HOpensHelp) {
  Repl repl(manual, 0, {});
  std::ostringstream out;
  EXPECT_EQ(repl.on_key(U'h', out), Step::Continue);
  EXPECT_NE(out.str().find("  1  A"), std::string::npos);
  EXPECT_EQ(repl.prompt(), "help> ");
}

TEST(Repl, QQuits) {
  Repl repl(manual, 0, {});
  std::ostringstream out;
  EXPECT_EQ(repl.on_key(U'q', out), Step::Quit);
}

TEST(Repl, CtrlDQuits) {
  Repl repl(manual, 0, {});
  std::ostringstream out;
  EXPECT_EQ(repl.on_key(U'\x04', out), Step::Quit);
}

TEST(Repl, CtrlDQuitsFromHelp) {
  Repl repl(manual, 0, {});
  std::ostringstream out;
  repl.on_key(U'h', out);
  EXPECT_EQ(repl.on_key(U'\x04', out), Step::Quit);
}

TEST(Repl, QInHelpLeavesHelpOnly) {
  Repl repl(manual, 0, {});
  std::ostringstream out;
  repl.on_key(U'h', out);
  EXPECT_EQ(repl.on_key(U'q', out), Step::Continue);
  EXPECT_EQ(repl.prompt(), "cratchit> ");
}

TEST(Repl, UnknownKeyListsTheKeys) {
  Repl repl(manual, 0, {});
  std::ostringstream out;
  EXPECT_EQ(repl.on_key(U'x', out), Step::Continue);
  EXPECT_EQ(out.str(), "Unknown key.\n" + main_keys);
}

TEST(Repl, EnterListsTheKeys) {
  Repl repl(manual, 0, {});
  EXPECT_FALSE(repl.ignores(U'\n'));
  EXPECT_FALSE(repl.ignores(U'\r'));
  EXPECT_TRUE(repl.ignores(U' '));
  EXPECT_TRUE(repl.ignores(U'\t'));
  std::ostringstream out;
  EXPECT_EQ(repl.on_key(U'\r', out), Step::Continue);
  EXPECT_EQ(out.str(), main_keys);
}

TEST(Repl, EscapeDoesNothing) {
  Repl repl(manual, 0, {});
  EXPECT_TRUE(repl.ignores(U'\x1b'));
  std::ostringstream out;
  repl.on_key(U'h', out);
  EXPECT_TRUE(repl.ignores(U'\x1b'));
}

TEST(Repl, SpaceTurnsThePageOnlyWhilePaging) {
  Section long_manual{"Manual", "1\n2\n3\n4\n", {}};
  Repl repl(long_manual, 2, {});
  EXPECT_TRUE(repl.ignores(U' '));
  std::ostringstream out;
  repl.on_key(U'h', out);
  EXPECT_FALSE(repl.ignores(U' '));
  EXPECT_FALSE(repl.ignores(U'\n'));
}

TEST(RunRepl, BrowsesHelpAndQuits) {
  std::ostringstream out;
  Repl repl(manual, 0, {});
  cratchit::run_repl(repl, keys_from("h1--q"), out);
  EXPECT_EQ(out.str(), "cratchit> h\n"
                       "Manual\n"
                       "\n"
                       "  1  A\n"
                       "\n"
                       "Press a number to open a section, - to go back, q to leave help.\n"
                       "help> 1\n"
                       "A\n"
                       "\n"
                       "Text A\n"
                       "\n"
                       "Press - to go back, q to leave help.\n"
                       "help 1> -\n"
                       "Manual\n"
                       "\n"
                       "  1  A\n"
                       "\n"
                       "Press a number to open a section, - to go back, q to leave help.\n"
                       "help> -\n"
                       "cratchit> q\n");
}

TEST(RunRepl, StopsAtEndOfInput) {
  std::ostringstream out;
  Repl repl(manual, 0, {});
  cratchit::run_repl(repl, keys_from(""), out);
  EXPECT_EQ(out.str(), "cratchit> \n");
}

TEST(RunRepl, EnterListsTheKeysAndSpaceIsIgnored) {
  std::ostringstream out;
  Repl repl(manual, 0, {});
  cratchit::run_repl(repl, keys_from("\n q\n"), out);
  EXPECT_EQ(out.str(), "cratchit> \n" + main_keys + "cratchit> q\n");
}

namespace {

namespace fs = std::filesystem;

std::string read_file(fs::path const& path) {
  std::ifstream in(path);
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

// A fresh start folder of its own for each test, with framework and runtime initiated
class WorkspaceRepl : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_repl_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root / "start");
    auto const folder = cratchit::open_data_folder(root / "start", "2026-10-20");
    ASSERT_TRUE(folder.has_value()) << folder.error();
    runtime_file = folder->runtime_file;
  }
  void TearDown() override { fs::remove_all(root); }

  Repl make_repl() { return Repl(manual, 0, {runtime_file, root / "start", root / "Documents" / "cratchit", "2026-10-20"}); }

  // Open the workspace as on start, then feed keys
  std::string run(std::string keys, cratchit::Charset charset = cratchit::Charset::Utf8) {
    auto repl = make_repl();
    std::ostringstream out;
    repl.open_workspace(out);
    cratchit::run_repl(repl, keys_from(std::move(keys), charset), out);
    return out.str();
  }

  std::string choices() const {
    return "Choose a workspace folder:\n"
           "  1  " + (root / "start").string() + "\n"
           "  2  " + (root / "Documents" / "cratchit").string() + "\n"
           "  3  Type a folder path\n"
           "\n"
           "Press a number to choose, - to go back, h for help, q to quit.\n";
  }

  fs::path root;
  fs::path runtime_file;
};

} // namespace

TEST_F(WorkspaceRepl, NoEntryOffersChoices) {
  EXPECT_EQ(run(""), "No workspace is set in " + runtime_file.string() + ".\n" + choices() + "workspace?> \n");
}

TEST_F(WorkspaceRepl, DefaultIsCreatedSavedAndOpened) {
  auto const folder = root / "Documents" / "cratchit";
  auto const output = run("2q");
  EXPECT_NE(output.find("workspace?> 2\nCreated folder " + folder.string() + "\n"
                        "Initiated .cratchit/workspace.txt in " + folder.string() + "\n"
                        "Updated .cratchit/runtime.txt in " + (root / "start").string() + "\n"
                        "Opened workspace " + folder.string() + "\n"
                        "workspace> q\n"),
            std::string::npos)
      << output;
  EXPECT_EQ(read_file(folder / ".cratchit" / "workspace.txt"), "created = 2026-10-20\n");
  EXPECT_EQ(read_file(runtime_file), "created = 2026-10-20\nworkspace = " + folder.string() + "\n");
}

TEST_F(WorkspaceRepl, StartFolderGetsWorkspaceFileInItsOwnCratchit) {
  run("1");
  EXPECT_EQ(read_file(root / "start" / ".cratchit" / "workspace.txt"), "created = 2026-10-20\n");
  EXPECT_EQ(read_file(runtime_file), "created = 2026-10-20\nworkspace = " + (root / "start").string() + "\n");
}

TEST_F(WorkspaceRepl, SavedWorkspaceOpensOnStart) {
  run("1");
  auto const output = run("-q");
  EXPECT_EQ(output, "Opened workspace " + (root / "start").string() + "\nworkspace> -\ncratchit> q\n");
}

TEST_F(WorkspaceRepl, MissingWorkspaceFolderOffersChoices) {
  std::ofstream(runtime_file) << "workspace = ../gone\n";
  auto const output = run("");
  EXPECT_EQ(output, "Workspace folder " + (root / "gone").string() + " does not exist.\n" + choices() + "workspace?> \n");
}

TEST_F(WorkspaceRepl, WorkspaceWithoutItsFileIsAnError) {
  fs::create_directories(root / "books" / ".cratchit");
  std::ofstream(runtime_file) << "workspace = " << (root / "books").string() << "\n";
  auto const output = run("");
  EXPECT_TRUE(output.starts_with("Error: ")) << output;
  EXPECT_TRUE(output.ends_with("cratchit> \n")) << output;
  EXPECT_FALSE(fs::exists(root / "books" / ".cratchit" / "workspace.txt"));
}

TEST_F(WorkspaceRepl, TypedPathIsCompletedAndOpened) {
  fs::create_directories(root / "books");
  auto const folder = root / "books";
  // The path starts at the root folder, so only the rest is typed
  auto const typed = (root / "bo").string().substr(root.root_path().string().size());
  auto const output = run("3" + typed + "\t\n");
  EXPECT_NE(output.find("path> " + root.root_path().string() + typed + "oks" +
                        std::string(1, static_cast<char>(fs::path::preferred_separator)) + "\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("Opened workspace " + folder.string() + "\nworkspace> \n"), std::string::npos) << output;
  EXPECT_EQ(read_file(runtime_file), "created = 2026-10-20\nworkspace = " + folder.string() + "\n");
}

TEST_F(WorkspaceRepl, MissingTypedFolderIsCreatedOnYes) {
  auto const folder = root / "new";
  auto const typed = folder.string().substr(folder.root_path().string().size());
  auto const output = run("3" + typed + "\ny");
  EXPECT_NE(output.find("Folder " + folder.string() + " does not exist.\nCreate it? (y/n) y\nCreated folder "),
            std::string::npos)
      << output;
  EXPECT_TRUE(fs::is_regular_file(folder / ".cratchit" / "workspace.txt"));
}

TEST_F(WorkspaceRepl, NonAsciiPathTypedInCp437IsCreated) {
  auto const typed = root.string().substr(root.root_path().string().size());
  run("3" + typed + "/R\x84kenskaper\ny", cratchit::Charset::Cp437);
  EXPECT_TRUE(fs::is_regular_file(root / fs::path(u8"R\u00e4kenskaper") / ".cratchit" / "workspace.txt"));
}

TEST_F(WorkspaceRepl, NonAsciiPathTypedInUtf8IsCreated) {
  auto const typed = root.string().substr(root.root_path().string().size());
  run("3" + typed + "/R\xc3\xa4kenskaper\ny", cratchit::Charset::Utf8);
  EXPECT_TRUE(fs::is_regular_file(root / fs::path(u8"R\u00e4kenskaper") / ".cratchit" / "workspace.txt"));
}

TEST_F(WorkspaceRepl, NonAsciiKeyIsEchoed) {
  auto repl = make_repl();
  std::ostringstream out;
  cratchit::run_repl(repl, keys_from("\x84", cratchit::Charset::Cp437), out);
  EXPECT_EQ(out.str(), "cratchit> \xc3\xa4\nUnknown key.\n" + main_keys + "cratchit> \n");
}

TEST_F(WorkspaceRepl, NoKeepsTheTypedPath) {
  auto const typed = (root / "new").string().substr(root.root_path().string().size());
  auto const output = run("3" + typed + "\nn\x7f\x7f\x7fstart\n");
  EXPECT_NE(output.find("Create it? (y/n) n\npath> " + root.root_path().string() + typed + "\b \b\b \b\b \bstart\n"),
            std::string::npos)
      << output;
  EXPECT_FALSE(fs::exists(root / "new"));
  EXPECT_NE(output.find("Opened workspace " + (root / "start").string()), std::string::npos) << output;
}

TEST_F(WorkspaceRepl, EmptyPathGoesBackToChoices) {
  std::string erase_root(root.root_path().string().size(), '\x7f');
  auto const output = run("3" + erase_root + "\n-q");
  EXPECT_NE(output.find("\n" + choices() + "workspace?> -\ncratchit> q\n"), std::string::npos) << output;
}

TEST_F(WorkspaceRepl, QIsTypedInAPathButQuitsElsewhere) {
  auto repl = make_repl();
  std::ostringstream out;
  repl.open_workspace(out);
  EXPECT_EQ(repl.on_key(U'3', out), Step::Continue);
  EXPECT_TRUE(repl.is_typing());
  EXPECT_EQ(repl.on_key(U'q', out), Step::Continue);
  EXPECT_FALSE(repl.ignores(U' '));
  EXPECT_EQ(repl.on_key(U'\x04', out), Step::Quit);
}

TEST_F(WorkspaceRepl, HelpReturnsToTheWorkspace) {
  run("1");
  auto const output = run("hqq");
  EXPECT_NE(output.find("help> q\nworkspace> q\n"), std::string::npos) << output;
}

TEST_F(WorkspaceRepl, WKeyOffersTheSavedWorkspaceAndBrowsing) {
  run("1");
  auto repl = make_repl();
  std::ostringstream out;
  cratchit::run_repl(repl, keys_from("wx-q"), out);
  EXPECT_EQ(out.str(), "cratchit> w\n"
                       "Workspace:\n"
                       "  w  Open " + (root / "start").string() + "\n"
                       "  b  Browse for a new workspace\n"
                       "\n"
                       "Press w or b, - to go back, h for help, q to quit.\n"
                       "workspace?> x\n"
                       "Press w or b, - to go back, h for help, q to quit.\n"
                       "workspace?> -\n"
                       "cratchit> q\n");
}

TEST_F(WorkspaceRepl, WKeyTwiceOpensTheSavedWorkspace) {
  run("1");
  auto const before = read_file(runtime_file);
  auto repl = make_repl();
  std::ostringstream out;
  cratchit::run_repl(repl, keys_from("ww"), out);
  EXPECT_TRUE(out.str().ends_with("workspace?> w\nOpened workspace " + (root / "start").string() + "\nworkspace> \n"))
      << out.str();
  EXPECT_EQ(read_file(runtime_file), before);
}

TEST_F(WorkspaceRepl, MenuWithoutSavedWorkspaceOffersOnlyBrowsing) {
  auto repl = make_repl();
  std::ostringstream out;
  cratchit::run_repl(repl, keys_from("ww"), out);
  EXPECT_EQ(out.str(), "cratchit> w\n"
                       "Workspace:\n"
                       "  b  Browse for a new workspace\n"
                       "\n"
                       "Press b, - to go back, h for help, q to quit.\n"
                       "workspace?> w\n"
                       "Press b, - to go back, h for help, q to quit.\n"
                       "workspace?> \n");
}

TEST_F(WorkspaceRepl, BrowsingSwitchesTheSavedWorkspace) {
  run("1");
  auto const folder = root / "Documents" / "cratchit";
  auto const output = run("-wb2");
  EXPECT_NE(output.find("workspace?> b\n" + choices() + "workspace?> 2\nCreated folder " + folder.string()),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("Opened workspace " + folder.string() + "\nworkspace> \n"), std::string::npos) << output;
  EXPECT_EQ(read_file(runtime_file), "created = 2026-10-20\nworkspace = " + folder.string() + "\n");
}

TEST_F(WorkspaceRepl, BackFromBrowsingShowsTheMenu) {
  run("1");
  auto const output = run("-wb-");
  EXPECT_TRUE(output.ends_with("workspace?> -\nWorkspace:\n"
                               "  w  Open " + (root / "start").string() + "\n"
                               "  b  Browse for a new workspace\n"
                               "\n"
                               "Press w or b, - to go back, h for help, q to quit.\n"
                               "workspace?> \n"))
      << output;
}

TEST_F(WorkspaceRepl, ChoosingTheSavedWorkspaceAgainKeepsRuntimeFile) {
  run("1");
  auto const output = run("-wb1");
  EXPECT_EQ(output.find("Updated"), std::string::npos) << output;
  EXPECT_NE(output.find("workspace?> 1\nOpened workspace " + (root / "start").string()), std::string::npos) << output;
}

TEST_F(WorkspaceRepl, BrokenSavedWorkspaceCanBeReplacedFromTheMenu) {
  fs::create_directories(root / "books" / ".cratchit");
  std::ofstream(runtime_file) << "workspace = " << (root / "books").string() << "\n";
  auto const output = run("wwwb1");
  EXPECT_NE(output.find("workspace?> w\nError: "), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("Opened workspace " + (root / "start").string() + "\nworkspace> \n")) << output;
  EXPECT_EQ(read_file(runtime_file), "workspace = " + (root / "start").string() + "\n");
}

TEST_F(WorkspaceRepl, LKeyPagesTheLogAndReturnsToThePrompt) {
  auto const log_file = root / "log.txt";
  std::ofstream(log_file) << "one\ntwo\nthree\n";
  Repl repl(manual, 2, {runtime_file, root / "start", root / "Documents" / "cratchit", "2026-10-20"}, log_file);
  std::ostringstream out;
  cratchit::run_repl(repl, keys_from("l l-lqq"), out);
  EXPECT_EQ(out.str(), "cratchit> l\n"
                       "Log " + log_file.string() + ", newest first:\n"
                       "three\n"
                       "two\n"
                       "--More-- \n"
                       "one\n"
                       "(start of log)\n"
                       "cratchit> l\n"
                       "Log " + log_file.string() + ", newest first:\n"
                       "three\n"
                       "two\n"
                       "--More-- -\n"
                       "cratchit> l\n"
                       "Log " + log_file.string() + ", newest first:\n"
                       "three\n"
                       "two\n"
                       "--More-- q\n"
                       "cratchit> q\n");
}

TEST_F(WorkspaceRepl, LKeyWorksInTheWorkspace) {
  auto const log_file = root / "log.txt";
  std::ofstream(log_file) << "one\n";
  auto repl = Repl(manual, 0, {runtime_file, root / "start", root / "Documents" / "cratchit", "2026-10-20"}, log_file);
  std::ostringstream out;
  repl.open_workspace(out);
  cratchit::run_repl(repl, keys_from("1l"), out);
  EXPECT_TRUE(out.str().ends_with("workspace> l\nLog " + log_file.string() + ", newest first:\none\n(start of log)\n"
                                  "workspace> \n"))
      << out.str();
}

namespace {

constexpr char const* listed_uuid = "3f2b8c1e-9a4d-4e7f-b6a2-1c5d8e0f7a93";

std::string projects_menu(fs::path const& workspace, std::string const& lines, bool any) {
  return "Projects in " + workspace.string() + ":\n" + lines +
         "  n  New project\n"
         "  a  Add an existing project folder\n"
         "\n"
         "Press " + (any ? "a number, " : "") + "n or a, - to go back, h for help, q to quit.\n";
}

} // namespace

TEST_F(WorkspaceRepl, PKeyShowsAnEmptyProjectMenu) {
  run("1");
  auto const output = run("p-q");
  EXPECT_EQ(output, "Opened workspace " + (root / "start").string() + "\nworkspace> p\n" +
                        projects_menu(root / "start", "", false) + "project?> -\nworkspace> q\n");
}

TEST_F(WorkspaceRepl, NewProjectIsCreatedListedAndOpened) {
  auto const folder = root / "start" / "ITfied AB";
  auto const output = run("1pnITfied AB\n1");
  EXPECT_NE(output.find("project?> n\n"
                        "Type the official name of the person, company or organisation. Backspace erases, Enter "
                        "accepts.\nEnter on an empty name goes back.\n"
                        "name> ITfied AB\n"
                        "Choose a folder for ITfied AB:\n"
                        "  1  " + folder.string() + "\n"
                        "  2  Type a folder path\n"
                        "\n"
                        "Press a number to choose, - to go back, h for help, q to quit.\n"
                        "project?> 1\n"
                        "Created folder " + folder.string() + "\n"
                        "Initiated .cratchit/project.txt in " + folder.string() + "\n"
                        "Updated .cratchit/workspace.txt in " + (root / "start").string() + "\n"
                        "Opened project ITfied AB in " + folder.string() + "\n"
                        "project> \n"),
            std::string::npos)
      << output;
  auto const project = cratchit::read_project(folder);
  ASSERT_TRUE(project.has_value()) << project.error();
  EXPECT_EQ(project->name, "ITfied AB");
  EXPECT_EQ(read_file(root / "start" / ".cratchit" / "workspace.txt"),
            "created = 2026-10-20\nproject." + project->uuid + " = " + folder.string() + "\nproject = " +
                project->uuid + "\n");
}

TEST_F(WorkspaceRepl, LastProjectOpensOnStart) {
  run("1pnX\n1");
  auto const output = run("--q");
  EXPECT_EQ(output, "Opened workspace " + (root / "start").string() + "\n"
                    "Opened project X in " + (root / "start" / "X").string() + "\n"
                    "project> -\nworkspace> -\ncratchit> q\n");
}

TEST_F(WorkspaceRepl, WKeyTwiceReopensTheLastProject) {
  run("1pnX\n1");
  auto const output = run("--ww");
  EXPECT_TRUE(output.ends_with("Opened project X in " + (root / "start" / "X").string() + "\nproject> \n")) << output;
}

TEST_F(WorkspaceRepl, ProjectMenuListsProjectsAndOpensTheChosen) {
  run("1pnA\n1");
  run("-pnB\n1");
  auto const before = read_file(root / "start" / ".cratchit" / "workspace.txt");
  auto const output = run("-p1");
  EXPECT_NE(output.find(projects_menu(root / "start",
                                      "  1  A  " + (root / "start" / "A").string() + "\n"
                                      "  2  B  " + (root / "start" / "B").string() + "\n",
                                      true) +
                        "project?> 1\nUpdated .cratchit/workspace.txt in " + (root / "start").string() + "\n"
                        "Opened project A in " + (root / "start" / "A").string() + "\nproject> \n"),
            std::string::npos)
      << output;
  EXPECT_EQ(read_file(root / "start" / ".cratchit" / "workspace.txt").size(), before.size());
}

TEST_F(WorkspaceRepl, EmptyNameGoesBackToTheProjectMenu) {
  run("1");
  auto const output = run("pn  \n-q");
  EXPECT_TRUE(output.ends_with(projects_menu(root / "start", "", false) + "project?> -\nworkspace> q\n")) << output;
}

TEST_F(WorkspaceRepl, NameThatCannotBeAFolderOffersOnlyTyping) {
  run("1");
  auto const output = run("pnA/B\n");
  EXPECT_NE(output.find("Choose a folder for A/B:\n  2  Type a folder path\n"), std::string::npos) << output;
}

TEST_F(WorkspaceRepl, NewProjectInATypedFolderIsCreatedOnYes) {
  run("1");
  auto const folder = root / "elsewhere" / "books";
  auto const output = run("pnX\n2../elsewhere/books\ny");
  EXPECT_NE(output.find("path> " + (root / "start" / "").string() + "../elsewhere/books\n"
                        "Folder " + folder.string() + " does not exist.\n"
                        "Create it? (y/n) y\n"
                        "Created folder " + folder.string() + "\n"
                        "Initiated .cratchit/project.txt in " + folder.string() + "\n"),
            std::string::npos)
      << output;
  EXPECT_TRUE(output.ends_with("Opened project X in " + folder.string() + "\nproject> \n")) << output;
}

TEST_F(WorkspaceRepl, NewProjectOverAProjectIsAnError) {
  run("1pnX\n1");
  auto const output = run("-pnY\n2X\n");
  EXPECT_NE(output.find("Error: "), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("Choose a folder for Y:\n  1  " + (root / "start" / "Y").string() +
                               "\n  2  Type a folder path\n\n"
                               "Press a number to choose, - to go back, h for help, q to quit.\nproject?> \n"))
      << output;
}

TEST_F(WorkspaceRepl, ExistingProjectIsAddedAndOpened) {
  fs::create_directories(root / "shared");
  ASSERT_TRUE(cratchit::create_project(root / "shared", "Föreningen", listed_uuid, "2026-10-01").has_value());
  run("1");
  auto const output = run("pa../shared\n");
  EXPECT_NE(output.find("project?> a\nThe folder must hold .cratchit/project.txt.\n"), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("Updated .cratchit/workspace.txt in " + (root / "start").string() + "\n"
                               "Opened project Föreningen in " + (root / "shared").string() + "\nproject> \n"))
      << output;
  EXPECT_EQ(read_file(root / "start" / ".cratchit" / "workspace.txt"),
            "created = 2026-10-20\nproject." + std::string(listed_uuid) + " = " + (root / "shared").string() +
                "\nproject = " + listed_uuid + "\n");
}

TEST_F(WorkspaceRepl, AddingAFolderWithoutProjectIsAnError) {
  fs::create_directories(root / "plain");
  run("1");
  auto const output = run("pa../plain\n");
  EXPECT_NE(output.find("Error: "), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("path> " + (root / "start" / "").string() + "../plain\n")) << output;
}

TEST_F(WorkspaceRepl, AddingACopiedProjectIsAnError) {
  fs::create_directories(root / "shared");
  ASSERT_TRUE(cratchit::create_project(root / "shared", "X", listed_uuid, "2026-10-01").has_value());
  fs::copy(root / "shared", root / "copy", fs::copy_options::recursive);
  run("1pa../shared\n");
  auto const before = read_file(root / "start" / ".cratchit" / "workspace.txt");
  auto const output = run("-pa../copy\n");
  EXPECT_NE(output.find("Error: Project X (" + std::string(listed_uuid) + ") is already listed with folder " +
                        (root / "shared").string()),
            std::string::npos)
      << output;
  EXPECT_EQ(read_file(root / "start" / ".cratchit" / "workspace.txt"), before);
}

TEST_F(WorkspaceRepl, MissingProjectFolderIsShownAndCannotBeOpened) {
  run("1");
  std::ofstream(root / "start" / ".cratchit" / "workspace.txt")
      << "created = 2026-10-20\nproject." << listed_uuid << " = gone\n";
  auto const output = run("p1");
  EXPECT_NE(output.find("  1  (not found)  " + (root / "start" / "gone").string() + "\n"), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("project?> 1\nError: Project folder " + (root / "start" / "gone").string() +
                               " does not exist\nproject?> \n"))
      << output;
}

TEST_F(WorkspaceRepl, UnlistedLastProjectIsAnErrorAtTheWorkspace) {
  run("1");
  std::ofstream(root / "start" / ".cratchit" / "workspace.txt") << "project = " << listed_uuid << "\n";
  auto const output = run("");
  EXPECT_TRUE(output.ends_with("Error: The last project " + std::string(listed_uuid) + " is not listed in " +
                               (root / "start" / ".cratchit" / "workspace.txt").string() + "\nworkspace> \n"))
      << output;
}

TEST_F(WorkspaceRepl, ManyProjectsScrollWithZeroAndNine) {
  run("1");
  std::ofstream file(root / "start" / ".cratchit" / "workspace.txt");
  for (int i = 0; i < 10; ++i) {
    file << std::format("project.{:08x}-0000-4000-8000-000000000000 = p{}\n", i, i);
  }
  file.close();
  auto const output = run("p9");
  EXPECT_NE(output.find("  8  (not found)  " + (root / "start" / "p7").string() + "\n  9  ...down\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("project?> 9\nProjects in " + (root / "start").string() + ":\n  0  ...up\n"
                        "  1  (not found)  " + (root / "start" / "p8").string() + "\n"
                        "  2  (not found)  " + (root / "start" / "p9").string() + "\n  n  New project\n"),
            std::string::npos)
      << output;
}

namespace {

// An SIE file in the start folder, where the import path starts
void write_sie(fs::path const& file, std::string const& text) {
  std::ofstream(file, std::ios::binary) << text;
}

std::string good_sie() {
  return sie_test::sie_file(2025, sie_test::balanced_balances(), sie_test::sale_and_cost(2025));
}

} // namespace

TEST_F(WorkspaceRepl, IKeyChecksTheFileAndImportsOnYes) {
  run("1pnTest\n1");
  auto const folder = root / "start" / "Test";
  auto const file = root / "start" / "test.se";
  write_sie(file, good_sie());
  auto const output = run("itest.se\ny");
  EXPECT_NE(output.find("project> i\n"
                        "Type the path of the SIE file to import. Tab completes a folder or file name, Backspace "
                        "erases,\nEnter accepts. Enter on an empty path goes back.\n"
                        "path> " + (root / "start").string() + "/test.se\n"
                        "SIE file " + file.string() + "\n"
                        "  Read as      CP437\n"
                        "  Program      Test 1.0, generated 2026-10-19\n"
                        "  Company      Test AB, 556000-0001\n"
                        "  Fiscal year  2025-01-01 - 2025-12-31\n"
                        "  Accounts     4: 4 new, 0 changed\n"
                        "  Vouchers     2: 2 new, 0 already in the books\n"
                        "Notes:\n"
                        "  - The project gets organisation number 556000-0001 (#ORGNR)\n"
                        "  - The project's name changes from \"Test\" to \"Test AB\" (#FNAMN)\n"
                        "Import it? (y/n) y\n"
                        "Initiated .cratchit/books/books.txt in " + folder.string() + "\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("Updated .cratchit/project.txt in " + folder.string() + "\n"
                        "Stashed the file as .cratchit/imports/2026-10-20_1_test.se in " + folder.string() + "\n"
                        "Imported 2 vouchers into fiscal year 2025-01-01 - 2025-12-31 (0 already in the books).\n"
                        "project> \n"),
            std::string::npos)
      << output;
  auto const project = cratchit::read_project(folder);
  ASSERT_TRUE(project.has_value()) << project.error();
  EXPECT_EQ(project->name, "Test AB");
  EXPECT_EQ(project->orgnr, "556000-0001");
  EXPECT_TRUE(fs::exists(folder / ".cratchit" / "imports" / "2026-10-20_1_test.se.txt"));
  EXPECT_TRUE(fs::exists(folder / ".cratchit" / "books" / "2025-01-01" / "vouchers.csv"));
  // The same file again has nothing new
  auto const again = run("itest.se\n");
  EXPECT_NE(again.find("  Vouchers     2: 0 new, 2 already in the books\n"
                       "Everything in the file is already in the books. Nothing to import.\nproject> \n"),
            std::string::npos)
      << again;
}

TEST_F(WorkspaceRepl, NoImportsNothing) {
  run("1pnTest\n1");
  write_sie(root / "start" / "test.se", good_sie());
  auto const output = run("itest.se\nn");
  EXPECT_TRUE(output.ends_with("Import it? (y/n) n\nNothing is imported.\nproject> \n")) << output;
  EXPECT_FALSE(fs::exists(root / "start" / "Test" / ".cratchit" / "books"));
}

TEST_F(WorkspaceRepl, FileWithErrorsIsNotImported) {
  run("1pnTest\n1");
  write_sie(root / "start" / "bad.se",
            sie_test::sie_file(2025, "", "#VER A 1 20250101 \"\"\n{\n#TRANS 1930 {} 1\n}\n"));
  auto const output = run("ibad.se\n");
  EXPECT_TRUE(output.ends_with("Errors:\n  - line 16: voucher A 1 does not balance: its amounts add up to 1.00\n"
                               "Nothing is imported.\nproject> \n"))
      << output;
  EXPECT_FALSE(fs::exists(root / "start" / "Test" / ".cratchit" / "books"));
}

TEST_F(WorkspaceRepl, MissingSieFileKeepsThePath) {
  run("1pnTest\n1");
  auto const output = run("inope.se\n");
  EXPECT_TRUE(output.ends_with("No file " + (root / "start" / "nope.se").string() + ".\npath> " +
                               (root / "start").string() + "/nope.se\n"))
      << output;
}

TEST_F(WorkspaceRepl, RKeyWithoutBooks) {
  run("1pnTest\n1");
  auto const output = run("r");
  EXPECT_TRUE(output.ends_with("project> r\nNo books yet. Press i to import an SIE file.\nproject> \n")) << output;
}

TEST_F(WorkspaceRepl, RKeyShowsYearsAndReports) {
  run("1pnTest\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("itest.se\ny");
  auto const output = run("r11x3 1930\n3 9999\n2--");
  EXPECT_NE(output.find("project> r\n"
                        "Fiscal years of Test AB, 556000-0001:\n"
                        "  1  2025-01-01 - 2025-12-31\n"
                        "\n"
                        "Press a number to choose, - to go back, h for help, q to quit.\n"
                        "report?> 1\n"
                        "Reports for 2025-01-01 - 2025-12-31:\n"
                        "  1  Trial balance (saldobalans)\n"
                        "  2  Vouchers\n"
                        "  3  Account ledger\n"
                        "\n"
                        "Press a number to choose, - to go back, h for help, q to quit.\n"
                        "report?> 1\n"
                        "Trial balance (saldobalans) 2025-01-01 - 2025-12-31\n"
                        "Test AB, 556000-0001\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("report?> x\nPress a number to choose, - to go back, h for help, q to quit.\n"
                        "report?> 3\n"
                        "Type the account number. Backspace erases, Enter accepts. Enter on an empty number goes "
                        "back.\n"
                        "account>  1930\n"
                        "Account 1930 Bank ledger 2025-01-01 - 2025-12-31\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("account>  9999\nNo account 9999 in the books.\nReports for"), std::string::npos) << output;
  EXPECT_NE(output.find("report?> 2\nVouchers 2025-01-01 - 2025-12-31\n"), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("report?> -\nFiscal years of Test AB, 556000-0001:\n  1  2025-01-01 - 2025-12-31\n\n"
                               "Press a number to choose, - to go back, h for help, q to quit.\n"
                               "report?> -\nproject> \n"))
      << output;
}

TEST_F(WorkspaceRepl, LongReportsArePaged) {
  run("1pnTest\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("itest.se\ny");
  auto repl = Repl(manual, 3, {runtime_file, root / "start", root / "Documents" / "cratchit", "2026-10-20"});
  std::ostringstream out;
  repl.open_workspace(out);
  cratchit::run_repl(repl, keys_from("r11 -"), out);
  EXPECT_NE(out.str().find("report?> 1\nTrial balance (saldobalans) 2025-01-01 - 2025-12-31\nTest AB, 556000-0001\n"
                           "\n--More-- \nAccount "),
            std::string::npos)
      << out.str();
  EXPECT_TRUE(out.str().ends_with("--More-- -\nReports for 2025-01-01 - 2025-12-31:\n  1  Trial balance (saldobalans)\n"
                                  "  2  Vouchers\n  3  Account ledger\n\n"
                                  "Press a number to choose, - to go back, h for help, q to quit.\nreport?> \n"))
      << out.str();
}

TEST_F(WorkspaceRepl, EnterListsTheKeysOfTheWorkspace) {
  run("1");
  auto const at_workspace = run("\n");
  EXPECT_TRUE(at_workspace.ends_with("workspace> \n"
                                     "Keys at workspace>:\n"
                                     "  p  Projects\n"
                                     "  l  Show the log\n"
                                     "  -  Back to cratchit>\n"
                                     "  h  Help (the manual)\n"
                                     "  q  Quit\n"
                                     "workspace> \n"))
      << at_workspace;
}

TEST_F(WorkspaceRepl, EnterAndUnknownKeysListTheKeysOfAProject) {
  run("1pnTest\n1");
  std::string const keys = "Keys at project>:\n"
                           "  i  Import an SIE file\n"
                           "  r  Reports\n"
                           "  l  Show the log\n"
                           "  -  Back to workspace>\n"
                           "  h  Help (the manual)\n"
                           "  q  Quit\n";
  auto const output = run("\nx");
  EXPECT_TRUE(output.ends_with("project> \n" + keys + "project> x\nUnknown key.\n" + keys + "project> \n")) << output;
}

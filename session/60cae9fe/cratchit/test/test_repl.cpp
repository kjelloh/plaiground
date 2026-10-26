#include "core/repl.hpp"

#include "core/data_folder.hpp"
#include "core/key_input.hpp"
#include "core/project.hpp"
#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
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
                              "  1  Workspace menu\n";

std::string const main_unknown = "Unknown key. Other keys: Enter list, l log, h help, q quit.\n";

// The keys listed at postings> after the voucher; f only with a template
std::string postings_keys(std::string const& voucher, bool flip) {
  return "\n"
         "  1  Save as " + voucher + "\n" +
         (flip ? "  2  Flip the template's rows (debit becomes credit)\n" : "") +
         "postings> ";
}

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
  EXPECT_EQ(out.str(), main_unknown);
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
                       "help> 1\n"
                       "A\n"
                       "\n"
                       "Text A\n"
                       "help 1> -\n"
                       "Manual\n"
                       "\n"
                       "  1  A\n"
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
           "";
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
  EXPECT_EQ(out.str(), "cratchit> \xc3\xa4\n" + main_unknown + "cratchit> \n");
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
  cratchit::run_repl(repl, keys_from("1x-q"), out);
  EXPECT_EQ(out.str(), "cratchit> 1\n"
                       "Workspace:\n"
                       "  1  Open " + (root / "start").string() + "\n"
                       "  2  Browse for a new workspace\n"
                       "workspace?> x\n"
                       "Unknown key. Other keys: Enter list, - back, h help, q quit.\n"
                       "workspace?> -\n"
                       "cratchit> q\n");
}

TEST_F(WorkspaceRepl, WKeyTwiceOpensTheSavedWorkspace) {
  run("1");
  auto const before = read_file(runtime_file);
  auto repl = make_repl();
  std::ostringstream out;
  cratchit::run_repl(repl, keys_from("11"), out);
  EXPECT_TRUE(out.str().ends_with("workspace?> 1\nOpened workspace " + (root / "start").string() + "\nworkspace> \n"))
      << out.str();
  EXPECT_EQ(read_file(runtime_file), before);
}

TEST_F(WorkspaceRepl, MenuWithoutSavedWorkspaceOffersOnlyBrowsing) {
  auto repl = make_repl();
  std::ostringstream out;
  cratchit::run_repl(repl, keys_from("11"), out);
  EXPECT_EQ(out.str(), "cratchit> 1\n"
                       "Workspace:\n"
                       "  2  Browse for a new workspace\n"
                       "workspace?> 1\n"
                       "Unknown key. Other keys: Enter list, - back, h help, q quit.\n"
                       "workspace?> \n");
}

TEST_F(WorkspaceRepl, BrowsingSwitchesTheSavedWorkspace) {
  run("1");
  auto const folder = root / "Documents" / "cratchit";
  auto const output = run("-122");
  EXPECT_NE(output.find("workspace?> 2\n" + choices() + "workspace?> 2\nCreated folder " + folder.string()),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("Opened workspace " + folder.string() + "\nworkspace> \n"), std::string::npos) << output;
  EXPECT_EQ(read_file(runtime_file), "created = 2026-10-20\nworkspace = " + folder.string() + "\n");
}

TEST_F(WorkspaceRepl, BackFromBrowsingShowsTheMenu) {
  run("1");
  auto const output = run("-12-");
  EXPECT_TRUE(output.ends_with("workspace?> -\nWorkspace:\n"
                               "  1  Open " + (root / "start").string() + "\n"
                               "  2  Browse for a new workspace\n"
                               "workspace?> \n"))
      << output;
}

TEST_F(WorkspaceRepl, ChoosingTheSavedWorkspaceAgainKeepsRuntimeFile) {
  run("1");
  auto const output = run("-121");
  EXPECT_EQ(output.find("Updated"), std::string::npos) << output;
  EXPECT_NE(output.find("workspace?> 1\nOpened workspace " + (root / "start").string()), std::string::npos) << output;
}

TEST_F(WorkspaceRepl, BrokenSavedWorkspaceCanBeReplacedFromTheMenu) {
  fs::create_directories(root / "books" / ".cratchit");
  std::ofstream(runtime_file) << "workspace = " << (root / "books").string() << "\n";
  auto const output = run("11121");
  EXPECT_NE(output.find("workspace?> 1\nError: "), std::string::npos) << output;
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

std::string projects_menu(fs::path const& workspace, std::size_t listed) {
  return "Projects in " + workspace.string() + ":\n" +
         (listed > 0 ? std::format("  1  Open a project ({} listed)\n", listed) : std::string{}) +
         "  2  New project\n"
         "  3  Add an existing project folder\n"
         "";
}

std::string project_list(fs::path const& workspace, std::string const& lines) {
  return "Projects in " + workspace.string() + ":\n" + lines +
         "";
}

} // namespace

TEST_F(WorkspaceRepl, PKeyShowsAnEmptyProjectMenu) {
  run("1");
  auto const output = run("1-q");
  EXPECT_EQ(output, "Opened workspace " + (root / "start").string() + "\nworkspace> 1\n" +
                        projects_menu(root / "start", 0) + "project?> -\nworkspace> q\n");
}

TEST_F(WorkspaceRepl, EnterShowsTheMenuAgainAndAnUnknownKeyNamesTheOtherKeys) {
  run("1");
  auto const output = run("1\nx");
  EXPECT_TRUE(output.ends_with("project?> \n" + projects_menu(root / "start", 0) +
                               "project?> x\nUnknown key. Other keys: Enter list, - back, h help, q quit.\n"
                               "project?> \n"))
      << output;
}

TEST_F(WorkspaceRepl, NewProjectIsCreatedListedAndOpened) {
  auto const folder = root / "start" / "ITfied AB";
  auto const output = run("112ITfied AB\n1");
  EXPECT_NE(output.find("project?> 2\n"
                        "Type the official name of the person, company or organisation. Backspace erases, Enter "
                        "accepts.\nEnter on an empty name goes back.\n"
                        "name> ITfied AB\n"
                        "Choose a folder for ITfied AB:\n"
                        "  1  " + folder.string() + "\n"
                        "  2  Type a folder path\n"
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
  run("112X\n1");
  auto const output = run("--q");
  EXPECT_EQ(output, "Opened workspace " + (root / "start").string() + "\n"
                    "Opened project X in " + (root / "start" / "X").string() + "\n"
                    "project> -\nworkspace> -\ncratchit> q\n");
}

TEST_F(WorkspaceRepl, WKeyTwiceReopensTheLastProject) {
  run("112X\n1");
  auto const output = run("--11");
  EXPECT_TRUE(output.ends_with("Opened project X in " + (root / "start" / "X").string() + "\nproject> \n")) << output;
}

TEST_F(WorkspaceRepl, ProjectMenuListsProjectsAndOpensTheChosen) {
  run("112A\n1");
  run("-12B\n1");
  auto const before = read_file(root / "start" / ".cratchit" / "workspace.txt");
  auto const output = run("-111");
  EXPECT_NE(output.find(projects_menu(root / "start", 2) + "project?> 1\n" +
                        project_list(root / "start",
                                     "  1  A  " + (root / "start" / "A").string() + "\n"
                                     "  2  B  " + (root / "start" / "B").string() + "\n") +
                        "open?> 1\nUpdated .cratchit/workspace.txt in " + (root / "start").string() + "\n"
                        "Opened project A in " + (root / "start" / "A").string() + "\nproject> \n"),
            std::string::npos)
      << output;
  EXPECT_EQ(read_file(root / "start" / ".cratchit" / "workspace.txt").size(), before.size());
}

TEST_F(WorkspaceRepl, EmptyNameGoesBackToTheProjectMenu) {
  run("1");
  auto const output = run("12  \n-q");
  EXPECT_TRUE(output.ends_with(projects_menu(root / "start", 0) + "project?> -\nworkspace> q\n")) << output;
}

TEST_F(WorkspaceRepl, NameThatCannotBeAFolderOffersOnlyTyping) {
  run("1");
  auto const output = run("12A/B\n");
  EXPECT_NE(output.find("Choose a folder for A/B:\n  2  Type a folder path\n"), std::string::npos) << output;
}

TEST_F(WorkspaceRepl, NewProjectInATypedFolderIsCreatedOnYes) {
  run("1");
  auto const folder = root / "elsewhere" / "books";
  auto const output = run("12X\n2../elsewhere/books\ny");
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
  run("112X\n1");
  auto const output = run("-12Y\n2X\n");
  EXPECT_NE(output.find("Error: "), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("Choose a folder for Y:\n  1  " + (root / "start" / "Y").string() +
                               "\n  2  Type a folder path\nproject?> \n"))
      << output;
}

TEST_F(WorkspaceRepl, ExistingProjectIsAddedAndOpened) {
  fs::create_directories(root / "shared");
  ASSERT_TRUE(cratchit::create_project(root / "shared", "Föreningen", listed_uuid, "2026-10-01").has_value());
  run("1");
  auto const output = run("13../shared\n");
  EXPECT_NE(output.find("project?> 3\nThe folder must hold .cratchit/project.txt.\n"), std::string::npos) << output;
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
  auto const output = run("13../plain\n");
  EXPECT_NE(output.find("Error: "), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("path> " + (root / "start" / "").string() + "../plain\n")) << output;
}

TEST_F(WorkspaceRepl, AddingACopiedProjectIsAnError) {
  fs::create_directories(root / "shared");
  ASSERT_TRUE(cratchit::create_project(root / "shared", "X", listed_uuid, "2026-10-01").has_value());
  fs::copy(root / "shared", root / "copy", fs::copy_options::recursive);
  run("113../shared\n");
  auto const before = read_file(root / "start" / ".cratchit" / "workspace.txt");
  auto const output = run("-13../copy\n");
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
  auto const output = run("111");
  EXPECT_NE(output.find("  1  (not found)  " + (root / "start" / "gone").string() + "\n"), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("open?> 1\nError: Project folder " + (root / "start" / "gone").string() +
                               " does not exist\nopen?> \n"))
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
  auto const output = run("119");
  EXPECT_NE(output.find("  8  (not found)  " + (root / "start" / "p7").string() + "\n  9  ...down\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("open?> 9\n" + project_list(root / "start",
                                                     "  0  ...up\n"
                                                     "  1  (not found)  " + (root / "start" / "p8").string() + "\n"
                                                     "  2  (not found)  " + (root / "start" / "p9").string() + "\n")),
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
  run("112Test\n1");
  auto const folder = root / "start" / "Test";
  auto const file = root / "start" / "test.se";
  write_sie(file, good_sie());
  auto const output = run("2test.se\ny");
  EXPECT_NE(output.find("project> 2\n"
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
  auto const again = run("2test.se\n");
  EXPECT_NE(again.find("  Vouchers     2: 0 new, 2 already in the books\n"
                       "Everything in the file is already in the books. Nothing to import.\nproject> \n"),
            std::string::npos)
      << again;
}

TEST_F(WorkspaceRepl, NoImportsNothing) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  auto const output = run("2test.se\nn");
  EXPECT_TRUE(output.ends_with("Import it? (y/n) n\nNothing is imported.\nproject> \n")) << output;
  EXPECT_FALSE(fs::exists(root / "start" / "Test" / ".cratchit" / "books"));
}

TEST_F(WorkspaceRepl, FileWithErrorsIsNotImported) {
  run("112Test\n1");
  write_sie(root / "start" / "bad.se",
            sie_test::sie_file(2025, "", "#VER A 1 20250101 \"\"\n{\n#TRANS 1930 {} 1\n}\n"));
  auto const output = run("2bad.se\n");
  EXPECT_TRUE(output.ends_with("Errors:\n  - line 16: voucher A 1 does not balance: its amounts add up to 1.00\n"
                               "Nothing is imported.\nproject> \n"))
      << output;
  EXPECT_FALSE(fs::exists(root / "start" / "Test" / ".cratchit" / "books"));
}

TEST_F(WorkspaceRepl, MissingSieFileKeepsThePath) {
  run("112Test\n1");
  auto const output = run("2nope.se\n");
  EXPECT_TRUE(output.ends_with("No file " + (root / "start" / "nope.se").string() + ".\npath> " +
                               (root / "start").string() + "/nope.se\n"))
      << output;
}

TEST_F(WorkspaceRepl, RKeyWithoutBooks) {
  run("112Test\n1");
  auto const output = run("5");
  EXPECT_TRUE(output.ends_with("project> 5\nNo books yet. Press 2 at project> to import an SIE file.\nproject> \n")) << output;
}

TEST_F(WorkspaceRepl, RKeyShowsYearsAndReports) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny");
  auto const output = run("511x3 1930\n3 9999\n2--");
  EXPECT_NE(output.find("project> 5\n"
                        "Fiscal years of Test AB, 556000-0001:\n"
                        "  1  2025-01-01 - 2025-12-31\n"
                        "report?> 1\n"
                        "Reports for 2025-01-01 - 2025-12-31:\n"
                        "  1  Trial balance (saldobalans)\n"
                        "  2  Vouchers\n"
                        "  3  Account ledger\n"
                        "report?> 1\n"
                        "Trial balance (saldobalans) 2025-01-01 - 2025-12-31\n"
                        "Test AB, 556000-0001\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("report?> x\nUnknown key. Other keys: Enter list, - back, h help, q quit.\n"
                        "report?> 3\n"
                        "Type the account number. Backspace erases, Enter accepts. Enter on an empty number goes "
                        "back.\n"
                        "account>  1930\n"
                        "Account 1930 Bank ledger 2025-01-01 - 2025-12-31\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("account>  9999\nNo account 9999 in the books.\nReports for"), std::string::npos) << output;
  EXPECT_NE(output.find("report?> 2\nVouchers 2025-01-01 - 2025-12-31\n"), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("report?> -\nFiscal years of Test AB, 556000-0001:\n  1  2025-01-01 - 2025-12-31\n"
                               "report?> -\nproject> \n"))
      << output;
}

TEST_F(WorkspaceRepl, LongReportsArePaged) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny");
  auto repl = Repl(manual, 3, {runtime_file, root / "start", root / "Documents" / "cratchit", "2026-10-20"});
  std::ostringstream out;
  repl.open_workspace(out);
  cratchit::run_repl(repl, keys_from("511 -"), out);
  EXPECT_NE(out.str().find("report?> 1\nTrial balance (saldobalans) 2025-01-01 - 2025-12-31\nTest AB, 556000-0001\n"
                           "\n--More-- \nAccount "),
            std::string::npos)
      << out.str();
  EXPECT_TRUE(out.str().ends_with("--More-- -\nReports for 2025-01-01 - 2025-12-31:\n  1  Trial balance (saldobalans)\n"
                                  "  2  Vouchers\n  3  Account ledger\nreport?> \n"))
      << out.str();
}

TEST_F(WorkspaceRepl, EnterListsTheKeysOfTheWorkspace) {
  run("1");
  auto const at_workspace = run("\n");
  EXPECT_TRUE(at_workspace.ends_with("workspace> \n"
                                     "Keys at workspace>:\n"
                                     "  1  Projects\n"
                                     "workspace> \n"))
      << at_workspace;
}

TEST_F(WorkspaceRepl, EnterAndUnknownKeysListTheKeysOfAProject) {
  run("112Test\n1");
  std::string const keys = "Keys at project>:\n"
                           "  1  Fiscal years\n"
                           "  2  Import an SIE file\n"
                           "  3  Import an account statement\n"
                           "  4  Statement entries not yet booked\n"
                           "  5  Reports\n";
  auto const output = run("\nx");
  EXPECT_TRUE(output.ends_with("project> \n" + keys +
                               "project> x\nUnknown key. Other keys: Enter list, - back, l log, h help, q quit.\n"
                               "project> \n"))
      << output;
}

TEST_F(WorkspaceRepl, YKeyWithoutBooks) {
  run("112Test\n1");
  auto const output = run("1");
  EXPECT_TRUE(output.ends_with("project> 1\nNo books yet. Press 2 at project> to import an SIE file.\nproject> \n")) << output;
}

TEST_F(WorkspaceRepl, EKeyEntersAndSavesAVoucher) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  auto const folder = root / "start" / "Test";
  auto const output = run("1Bank cost march 35 2025-03-31\ny\n\n1");
  EXPECT_NE(output.find("2025> 1\n"
                        "Type the heading, amount and date of the voucher, e.g. Telia faktura oktober 1118,00 "
                        "2026-10-22.\n"
                        "No date means today. Backspace erases, Enter accepts. Enter on an empty line goes back.\n"
                        "voucher> Bank cost march 35 2025-03-31\n"
                        "  Heading  Bank cost march\n"
                        "  Amount   35.00\n"
                        "  Date     2025-03-31   (fiscal year 2025-01-01 - 2025-12-31)\n"
                        "OK? (y/n) y\n"
                        "Earlier vouchers like this one:\n"
                        "  1  A 2  2025-03-01         10.00  Bank cost\n"
                        "       Account        Debit       Credit\n"
                        "       6570           10.00               Bank costs\n"
                        "       1930                        10.00  Bank\n"
                        "  0  None: type the rows yourself\n"
                        "template?> \n"
                        "Series (Enter = A, as A 2 2025-03-01 \"Bank cost\"):\n"
                        "  A  Redovisning\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("  N  Lager\n"
                        "\n"
                        "Press a letter or Enter to choose, - to go back, q to quit.\n"
                        "series> \n"
                        "Suggested from A 2, scaled to the amount:\n"
                        "Voucher A 3 2025-03-31 \"Bank cost march\"\n"
                        "  Account        Debit       Credit\n"
                        "  6570           35.00               Bank costs\n"
                        "  1930                        35.00  Bank\n"
                        "  Total          35.00        35.00\n" +
                            postings_keys("A 3", true) + "1\n"
                        "Updated .cratchit/books/2025-01-01/vouchers.csv in " + folder.string() + "\n"
                        "Updated .cratchit/books/2025-01-01/postings.csv in " + folder.string() + "\n"
                        "Saved voucher A 3 into fiscal year 2025-01-01 - 2025-12-31.\n"
                        "2025> \n"),
            std::string::npos)
      << output;
  auto const books = cratchit::books::read_books(folder);
  ASSERT_TRUE(books.has_value()) << books.error();
  auto const& saved = books->years.front().vouchers.back();
  EXPECT_EQ(saved.series, "A");
  EXPECT_EQ(saved.number, "3");
  EXPECT_EQ(saved.event.heading, "Bank cost march");
  EXPECT_EQ(saved.event.amount, 3500);
  EXPECT_EQ(cratchit::format_iso_date(*saved.registered), "2026-10-20");
  ASSERT_EQ(saved.postings.size(), 2u);
  EXPECT_EQ(saved.postings[1].amount, -3500);
}

TEST_F(WorkspaceRepl, NKeyNotesAnEventAndWPostsIt) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  auto const folder = root / "start" / "Test";
  auto const events = folder / ".cratchit" / "books" / "2025-01-01" / "events.csv";
  EXPECT_FALSE(fs::exists(events));
  auto output = run("2Bank cost march -35 2025-03-31\ny");
  EXPECT_NE(output.find("2025> 2\n"
                        "Type the heading, amount and date of the event to post later, e.g. Telia faktura oktober "
                        "1118,00 2026-10-22.\n"
                        "No date means today. Backspace erases, Enter accepts. Enter on an empty line goes back.\n"
                        "event> Bank cost march -35 2025-03-31\n"
                        "  Heading  Bank cost march\n"
                        "  Amount   -35.00\n"
                        "  Date     2025-03-31   (fiscal year 2025-01-01 - 2025-12-31)\n"
                        "OK? (y/n) y\n"
                        "Initiated .cratchit/books/2025-01-01/events.csv in " + folder.string() + "\n"
                        "Noted: 2025-03-31 Bank cost march 35.00 (1 waiting). Press 3 to post it.\n"
                        "2025> \n"),
            std::string::npos)
      << output;
  std::ifstream in(events);
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  EXPECT_EQ(text, "date,text,amount,noted\n2025-03-31,Bank cost march,35.00,2026-10-20\n");
  in.close();

  output = run("3110\n6570\n1930\n\n1");
  EXPECT_NE(output.find("2025> 3\n"
                        "Waiting events in fiscal year 2025-01-01 - 2025-12-31:\n"
                        "  1  2025-03-31         35.00  Bank cost march\n"
                        "waiting?> 1\n"
                        "  2025-03-31         35.00  Bank cost march\n"
                        "\n"
                        "  1  Post it: enter its voucher\n"
                        "waiting> 1\n"
                        "Earlier vouchers like this one:\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("template?> 0\nSeries (Enter = A, as A 2 2025-03-01 \"Bank cost\"):\n"), std::string::npos)
      << output;
  EXPECT_EQ(output.find("Suggested"), std::string::npos) << output;
  EXPECT_NE(output.find(postings_keys("A 3", false) + "1\n"
                        "Updated .cratchit/books/2025-01-01/vouchers.csv in " + folder.string() + "\n"
                        "Updated .cratchit/books/2025-01-01/postings.csv in " + folder.string() + "\n"
                        "Removed .cratchit/books/2025-01-01/events.csv in " + folder.string() + "\n"
                        "Saved voucher A 3 into fiscal year 2025-01-01 - 2025-12-31.\n"
                        "The event is no longer waiting (0 still waiting).\n"
                        "2025> \n"),
            std::string::npos)
      << output;
  EXPECT_FALSE(fs::exists(events));
  auto const books = cratchit::books::read_books(folder);
  ASSERT_TRUE(books.has_value()) << books.error();
  EXPECT_TRUE(books->years.front().waiting.empty());
  EXPECT_EQ(books->years.front().vouchers.back().event,
            (cratchit::books::Event{"Bank cost march", 3500, cratchit::parse_iso_date("2025-03-31").value()}));
}

TEST_F(WorkspaceRepl, WaitingEventsStayUntilPostedOrDeleted) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  auto const folder = root / "start" / "Test";
  EXPECT_TRUE(run("3").ends_with(
      "2025> 3\nNo events are waiting in fiscal year 2025-01-01 - 2025-12-31. Press 2 to note one.\n2025> \n"));
  run("2Telia 1118 2025-04-01\ny");
  run("2Kvitto 249 2025-04-02\ny");
  // - at series> goes back to the list; - at post> with no rows drops the voucher, not the event
  auto output = run("321-11A-\n");
  EXPECT_NE(output.find("waiting> 1\nSeries"), std::string::npos) << output;
  EXPECT_NE(output.find("series> -\nWaiting events in fiscal year 2025-01-01 - 2025-12-31:\n"), std::string::npos)
      << output;
  EXPECT_NE(output.find("post> -\nDropped the voucher. Nothing is saved. The event is still waiting.\n2025> \n"),
            std::string::npos)
      << output;
  // d asks first; n goes back to the event, - from there to the list
  std::string const kvitto = "  2025-04-02        249.00  Kvitto\n"
                             "\n"
                             "  1  Post it: enter its voucher\n";
  output = run("32dn-");
  EXPECT_NE(output.find("waiting?> 2\n" + kvitto + "waiting> d\nDelete it? (y/n) n\n" + kvitto +
                        "waiting> -\nWaiting events in"),
            std::string::npos)
      << output;
  // d is not listed, but named by an unknown key
  output = run("31x");
  EXPECT_TRUE(output.ends_with("waiting> x\nUnknown key. Other keys: Enter list, d delete, - back, h help, q quit.\n"
                               "waiting> \n"))
      << output;
  output = run("31dy");
  EXPECT_NE(output.find("Delete it? (y/n) y\n"
                        "Updated .cratchit/books/2025-01-01/events.csv in " + folder.string() + "\n"
                        "Deleted the waiting event 2025-04-01 \"Telia\" 1118.00.\n"
                        "Waiting events in fiscal year 2025-01-01 - 2025-12-31:\n"
                        "  1  2025-04-02        249.00  Kvitto\n"),
            std::string::npos)
      << output;
  output = run("31dy");
  EXPECT_NE(output.find("Removed .cratchit/books/2025-01-01/events.csv in " + folder.string() + "\n"
                        "Deleted the waiting event 2025-04-02 \"Kvitto\" 249.00.\n"
                        "No events are waiting now.\n"
                        "2025> \n"),
            std::string::npos)
      << output;
}

TEST_F(WorkspaceRepl, WaitingEventsAreListedOldestFirst) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  run("2Telia 1118 2025-04-01\ny");
  run("2Hyra 5000 2025-05-01\ny");
  run("2Kvitto 249 2025-03-15\ny");
  run("2Telia rest 10 2025-04-01\ny");
  auto const output = run("3");
  EXPECT_NE(output.find("Waiting events in fiscal year 2025-01-01 - 2025-12-31:\n"
                        "  1  2025-03-15        249.00  Kvitto\n"
                        "  2  2025-04-01       1118.00  Telia\n"
                        "  3  2025-04-01         10.00  Telia rest\n"
                        "  4  2025-05-01       5000.00  Hyra\n"),
            std::string::npos)
      << output;
}

TEST_F(WorkspaceRepl, EmptyVoucherLineGoesBackToTheYear) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  EXPECT_TRUE(run("1\n").ends_with("voucher> \n2025> \n")) << run("1\n");
}

TEST_F(WorkspaceRepl, VoucherLineIsRefusedAndKeptForEditing) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  // Backspace (DEL) edits the kept line
  auto const output = run("1Bank 35 2027-01-01\n" + std::string(10, '\x7f') + "2025-01-01\nn");
  EXPECT_NE(output.find("voucher> Bank 35 2027-01-01\n"
                        "Refused: 2027-01-01 is outside fiscal year 2025-01-01 - 2025-12-31.\n"
                        "voucher> Bank 35 2027-01-01"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("  Date     2025-01-01   (fiscal year 2025-01-01 - 2025-12-31)\nOK? (y/n) n\n"
                        "voucher> Bank 35 2025-01-01"),
            std::string::npos)
      << output;
}

TEST_F(WorkspaceRepl, QuestionMarkHintsTheCounterAccounts) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  auto const output = run("1Lunch 10 2025-05-05\nyb?\n1930 -10\n?\n? 6570\n? bank\n6570\n\n?-\n-\n-\n");
  auto const hint_1930 = "Hint for 1930 C Bank, from vouchers and waiting events with 1930 on credit:\n"
                         "  Counter accounts in the latest 1:\n"
                         "    6570   D   1  Bank costs\n"
                         "  Latest:\n"
                         "    A 2      2025-03-01         10.00  Bank cost\n"
                         "      Account        Debit       Credit\n"
                         "      6570           10.00               Bank costs\n"
                         "      1930                        10.00  Bank\n";
  auto const hint_6570 = "Hint for 6570 D Bank costs, from vouchers and waiting events with 6570 on debit:\n"
                         "  Counter accounts in the latest 1:\n"
                         "    1930   C   1  Bank\n"
                         "  Latest:\n"
                         "    A 2      2025-03-01         10.00  Bank cost\n"
                         "      Account        Debit       Credit\n"
                         "      6570           10.00               Bank costs\n"
                         "      1930                        10.00  Bank\n";
  EXPECT_NE(output.find("to the amount. ? shows the counter accounts used before with the rows' accounts, ? 1930 with 1930.\n"
                        "post> ?\n"
                        "No rows yet. Type ? and an account for its hint, e.g. ? 1930.\n"),
            std::string::npos)
      << output;
  // An account in the rows: its side; not in the rows: both sides; not an account: the matches
  EXPECT_NE(output.find(std::string{"post> ?\n"} + hint_1930 + "post> ? 6570\n" + hint_6570 +
                        "Hint for 6570 C Bank costs: no voucher or waiting event has 6570 on credit.\n"
                        "post> ? bank\nAccounts matching bank:\n  1930  Bank\n  6570  Bank costs\npost> 6570\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find(postings_keys("B 1", false) + "?\n" + hint_1930 + hint_6570 + "postings> -\n"),
            std::string::npos)
      << output;
}

TEST_F(WorkspaceRepl, RowsListAccountsRemoveAndDrop) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  auto const output = run("1Lunch 100 2025-05-05\nyb" "bank\n1930 -100\n\n-\n-\n");
  EXPECT_NE(output.find("Voucher B 1 (Kundfakturor) 2025-05-05 \"Lunch\" 100.00\n"), std::string::npos) << output;
  EXPECT_NE(output.find("post> bank\nAccounts matching bank:\n  1930  Bank\n  6570  Bank costs\n"), std::string::npos)
      << output;
  EXPECT_NE(output.find("post> \n  Remaining 100.00 (debit 0.00, credit 100.00, amount 100.00).\n"), std::string::npos)
      << output;
  EXPECT_NE(output.find("post> -\nRemoved  1930                       100.00  Bank\n  No rows.\n"), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("post> -\nDropped the voucher. Nothing is saved.\n2025> \n")) << output;
}

TEST_F(WorkspaceRepl, SpacedAmountMustBeChosen) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  auto const output = run("1Faktura 87 500 2025-05-05\nx3-\n2n");
  EXPECT_NE(output.find("voucher> Faktura 87 500 2025-05-05\n"
                        "The amount is unclear: spaces may separate thousands. Which is it?\n"
                        "  1           500.00  Faktura 87\n"
                        "  2         87500.00  Faktura\n"
                        "amount?> x\n"
                        "Unknown key. Other keys: Enter list, - change the line, h help, q quit.\n"
                        "amount?> 3\n"
                        "Unknown key. Other keys: Enter list, - change the line, h help, q quit.\n"
                        "amount?> -\n"
                        "voucher> Faktura 87 500 2025-05-05\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("amount?> 2\n"
                        "  Heading  Faktura\n"
                        "  Amount   87500.00\n"
                        "  Date     2025-05-05   (fiscal year 2025-01-01 - 2025-12-31)\n"
                        "OK? (y/n) n\n"
                        "voucher> Faktura 87 500 2025-05-05"),
            std::string::npos)
      << output;
}

TEST_F(WorkspaceRepl, RowsMustAddUpToTheAmountToSave) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  auto const output = run("1Bank cost 35 2025-03-31\ny0\n6570 10\n1930\n\n6570\n6570 25\n1930 -25\n\n");
  EXPECT_NE(output.find("post> 1930\n"
                        "  1930                        10.00  Bank\n"
                        "  Debit 10.00, credit 10.00, amount 35.00: debit and credit must each reach the amount.\n"
                        "post> \n"
                        "  Debit 10.00, credit 10.00, amount 35.00: debit and credit must each reach the amount.\n"
                        "post> 6570\n"
                        "Type an amount: the rows add up to zero, but not to the amount.\n"
                        "post> 6570 25\n"),
            std::string::npos)
      << output;
  EXPECT_TRUE(output.ends_with("  Balanced. Enter saves.\npost> \nVoucher A 3 2025-03-31 \"Bank cost\"\n"
                               "  Account        Debit       Credit\n"
                               "  6570           10.00               Bank costs\n"
                               "  1930                        10.00  Bank\n"
                               "  6570           25.00               Bank costs\n"
                               "  1930                        25.00  Bank\n"
                               "  Total          35.00        35.00\n" +
                               postings_keys("A 3", false) + "\n"))
      << output;
}

TEST_F(WorkspaceRepl, TemplateGuidesTheRows) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  // 2 picks A 1; - at series> goes back to the list, - there back to the line
  auto const output = run("1Bank cost sale 25 2025-04-01\ny2--\ny1\n--\n-\n3010 -20\n\n-\n-\n-\n");
  EXPECT_NE(output.find("OK? (y/n) y\n"
                        "Earlier vouchers like this one:\n"
                        "  1  A 2  2025-03-01         10.00  Bank cost\n"
                        "       Account        Debit       Credit\n"
                        "       6570           10.00               Bank costs\n"
                        "       1930                        10.00  Bank\n"
                        "  2  A 1  2025-02-01        500.00  Sale\n"
                        "       Account        Debit       Credit\n"
                        "       1930          500.00               Bank\n"
                        "       3010                       500.00  Sales\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("template?> 2\nSeries (Enter = A, as A 1 2025-02-01 \"Sale\"):\n"), std::string::npos) << output;
  EXPECT_NE(output.find("series> -\nEarlier vouchers like this one:\n"), std::string::npos) << output;
  EXPECT_NE(output.find("template?> -\nvoucher> Bank cost sale 25 2025-04-01"), std::string::npos) << output;
  // The whole voucher is suggested, scaled to 25.00; - puts its rows in place, - steps back to a suggestion
  EXPECT_NE(output.find("series> \n"
                        "Suggested from A 2, scaled to the amount:\n"
                        "Voucher A 3 2025-04-01 \"Bank cost sale\"\n"
                        "  Account        Debit       Credit\n"
                        "  6570           25.00               Bank costs\n"
                        "  1930                        25.00  Bank\n"
                        "  Total          25.00        25.00\n" +
                            postings_keys("A 3", true) + "-\n"
                        
                        "Voucher A 3 (Redovisning) 2025-04-01 \"Bank cost sale\" 25.00\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("The suggested rows are in place. - removes the last one and suggests it again: Enter takes the\n"
                        "suggestion, or type your own row instead.\n"
                        "  Balanced. Enter saves.\n"
                        "post> -\n"
                        "Removed  1930                        25.00  Bank\n"
                        "  Remaining -25.00 (debit 25.00, credit 0.00, amount 25.00).\n"
                        "  Suggested 1930                        25.00  Bank\n"
                        "post> -\n"
                        "Removed  6570           25.00               Bank costs\n"
                        "  No rows.\n"
                        "  Suggested 6570           25.00               Bank costs\n"),
            std::string::npos)
      << output;
  // A typed row replaces the suggestion, and the last row takes what remains
  EXPECT_NE(output.find("  Suggested 6570           25.00               Bank costs\n"
                        "post> 3010 -20\n"
                        "  3010                        20.00  Sales\n"
                        "  Remaining 20.00 (debit 0.00, credit 20.00, amount 25.00).\n"
                        "  Suggested 1930           20.00               Bank\n"
                        "post> \n"
                        "  1930           20.00               Bank\n"
                        "  Debit 20.00, credit 20.00, amount 25.00: debit and credit must each reach the amount.\n"
                        "post> -\n"
                        "Removed  1930           20.00               Bank\n"
                        "  Remaining 20.00 (debit 0.00, credit 20.00, amount 25.00).\n"
                        "  Suggested 1930           20.00               Bank\n"),
            std::string::npos)
      << output;
  EXPECT_TRUE(output.ends_with("post> -\nDropped the voucher. Nothing is saved.\n2025> \n")) << output;
}

TEST_F(WorkspaceRepl, FAtPostingsFlipsTheTemplate) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  // Enter takes template A 2 and series A; 2 flips, x and Enter list the keys, 2 flips back, 2 again and 1 saves
  auto const output = run("1Bank refund 35 2025-04-03\ny\n\n2x\n221");
  auto const flipped = std::string("2\n"
                                   "Suggested from A 2, scaled to the amount and flipped:\n"
                                   "Voucher A 3 2025-04-03 \"Bank refund\"\n"
                                   "  Account        Debit       Credit\n"
                                   "  6570                        35.00  Bank costs\n"
                                   "  1930           35.00               Bank\n"
                                   "  Total          35.00        35.00\n") +
                       postings_keys("A 3", true);
  EXPECT_NE(output.find("series> \n"
                        "Suggested from A 2, scaled to the amount:\n"
                        "Voucher A 3 2025-04-03 \"Bank refund\"\n"
                        "  Account        Debit       Credit\n"
                        "  6570           35.00               Bank costs\n"
                        "  1930                        35.00  Bank\n"
                        "  Total          35.00        35.00\n" +
                        postings_keys("A 3", true) + flipped + "x\n"
                        "Unknown key. Other keys: Enter list, ? hint of counter accounts, - change the rows, h help, "
                        "q quit.\n"
                        "postings> \n"
                        "Voucher A 3 2025-04-03 \"Bank refund\"\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("postings> 2\nSuggested from A 2, scaled to the amount:\n"), std::string::npos) << output;
  EXPECT_NE(output.find(flipped + "1\n"), std::string::npos) << output;
  EXPECT_TRUE(output.ends_with("Saved voucher A 3 into fiscal year 2025-01-01 - 2025-12-31.\n2025> \n")) << output;
  auto const books = cratchit::books::read_books(root / "start" / "Test");
  ASSERT_TRUE(books.has_value()) << books.error();
  auto const& saved = books->years.front().vouchers.back();
  EXPECT_EQ(saved.event.amount, 3500);
  ASSERT_EQ(saved.postings.size(), 2u);
  EXPECT_EQ(saved.postings[0].amount, -3500);
  EXPECT_EQ(saved.postings[1].amount, 3500);
  // Without a template, 2 (flip) is not offered
  auto const plain = run("1Lunch 20 2025-04-04\ny\n6570 20\n1930\n\n2-\n-\n-\n");
  EXPECT_NE(plain.find(postings_keys("A 4", false) + "2\nUnknown key. Other keys:"), std::string::npos) << plain;
}

TEST_F(WorkspaceRepl, VKeyViewsVouchersAndCorrectsAnEnteredOne) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  run("1Bank cost march 35 2025-03-31\ny\n\n1");
  auto output = run("4xa1");
  EXPECT_NE(output.find("2025> 4\n"
                        "Series in fiscal year 2025-01-01 - 2025-12-31:\n"
                        "  A  Redovisning                         3 vouchers, last A 3 2025-03-31\n"
                        "\n"
                        "Press a letter to list that series, - to go back, q to quit.\n"
                        "series?> x\n"
                        "Press a letter to list that series, - to go back, q to quit.\n"
                        "series?> a\n"
                        "Vouchers A in fiscal year 2025-01-01 - 2025-12-31, newest first:\n"
                        "  1  A 3     2025-03-31         35.00  Bank cost march  (entered)\n"
                        "  2  A 2     2025-03-01         10.00  Bank cost  (imported)\n"
                        "  3  A 1     2025-02-01        500.00  Sale  (imported)\n"
                        "voucher?> 1\n"
                        "Voucher A 3 2025-03-31 \"Bank cost march\" 35.00\n"
                        "  Account        Debit       Credit\n"
                        "  6570           35.00               Bank costs\n"
                        "  1930                        35.00  Bank\n"
                        "  Total          35.00        35.00\n"
                        "  Registered 2026-10-20, entered in cratchit\n"
                        "\n"
                        "  1  Correct it\n"
                        "  2  Use it as a template\n"
                        "A 3> \n"),
            std::string::npos)
      << output;
  // An imported voucher is not corrected
  output = run("4a31-");
  EXPECT_NE(output.find("A 1> 1\nA 1 came with an SIE import, so cratchit does not correct it. Correct it in the program "
                        "it\ncame from and import the file again.\nA 1> -\nVouchers A"),
            std::string::npos)
      << output;
  // The correction reverses the rows, in the same series, and then the right voucher can follow
  output = run("4a11 2025-04-01\ny\n1y" + std::string(5, '\x7f') + "40 2025-03-31\ny\n1");
  EXPECT_NE(output.find("A 3> 1\n"), std::string::npos) << output;
  EXPECT_NE(output.find("voucher> Rättelse av A 3: Bank cost march 35.00 2025-04-01\n"
                        "  Heading  Rättelse av A 3: Bank cost march\n"
                        "  Amount   35.00\n"
                        "  Date     2025-04-01   (fiscal year 2025-01-01 - 2025-12-31)\n"
                        "OK? (y/n) y\n"
                        "Series (Enter = A, as A 3 2025-03-31 \"Bank cost march\"):\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("series> \n"
                        "The rows of A 3, reversed:\n"
                        "Voucher A 4 2025-04-01 \"Rättelse av A 3: Bank cost march\"\n"
                        "  Account        Debit       Credit\n"
                        "  6570                        35.00  Bank costs\n"
                        "  1930           35.00               Bank\n"
                        "  Total          35.00        35.00\n" +
                            postings_keys("A 4", true) + "1\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("Saved voucher A 4 into fiscal year 2025-01-01 - 2025-12-31.\n"
                        "A 4 reverses A 3. The right voucher can follow, with A 3 as the template.\n"
                        "Enter the right voucher now? (y/n) y\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("voucher> Bank cost march 35.00"), std::string::npos) << output;
  EXPECT_NE(output.find("  Heading  Bank cost march\n  Amount   40.00\n"), std::string::npos) << output;
  EXPECT_NE(output.find("OK? (y/n) y\nSeries (Enter = A, as A 3 2025-03-31 \"Bank cost march\"):\n"), std::string::npos)
      << output;
  EXPECT_NE(output.find("Suggested from A 3, scaled to the amount:\n"
                        "Voucher A 5 2025-03-31 \"Bank cost march\"\n"
                        "  Account        Debit       Credit\n"
                        "  6570           40.00               Bank costs\n"),
            std::string::npos)
      << output;
  EXPECT_TRUE(output.ends_with("Saved voucher A 5 into fiscal year 2025-01-01 - 2025-12-31.\n2025> \n")) << output;
  auto const books = cratchit::books::read_books(root / "start" / "Test");
  ASSERT_TRUE(books.has_value()) << books.error();
  auto const& vouchers = books->years.front().vouchers;
  ASSERT_EQ(vouchers.size(), 5u);
  EXPECT_EQ(vouchers[3].event.heading, "Rättelse av A 3: Bank cost march");
  EXPECT_EQ(vouchers[3].postings[0].amount, -3500);
  EXPECT_EQ(vouchers[3].postings[1].amount, 3500);
  EXPECT_EQ(vouchers[4].postings[0].amount, 4000);
}

TEST_F(WorkspaceRepl, TKeyUsesTheViewedVoucherAsTheTemplate) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  // No template list: the viewed voucher is the template, even with no shared word
  auto const output = run("4a12Lunch 20 2025-04-02\ny\n");
  EXPECT_NE(output.find("A 2> 2\nType the heading"), std::string::npos) << output;
  EXPECT_NE(output.find("OK? (y/n) y\nSeries (Enter = A, as A 2 2025-03-01 \"Bank cost\"):\n"), std::string::npos)
      << output;
  EXPECT_NE(output.find("Suggested from A 2, scaled to the amount:\n"
                        "Voucher A 3 2025-04-02 \"Lunch\"\n"
                        "  Account        Debit       Credit\n"
                        "  6570           20.00               Bank costs\n"
                        "  1930                        20.00  Bank\n"
                        "  Total          20.00        20.00\n" +
                            postings_keys("A 3", true)),
            std::string::npos)
      << output;
  // A closed year refuses it
  auto const closed = run("84a12");
  EXPECT_NE(closed.find("A 2> 2\nFiscal year 2025-01-01 - 2025-12-31 is closed for new vouchers. Press 8 to open it.\n"
                        "A 2> "),
            std::string::npos)
      << closed;
}

TEST_F(WorkspaceRepl, SKeyImportsAnAccountStatement) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny");
  auto const folder = root / "start" / "Test";
  fs::create_directories(root / "start" / "bank");
  // Two postings match the books' postings on 1930; one is after the fiscal year
  std::ofstream(root / "start" / "bank" / "nordea.csv", std::ios::binary)
      << "\xEF\xBB\xBF" "Datum;Belopp;Text;Saldo;\n"
         "2026/01/02;-1,85;AVGIFTER;488,15;\n"
         "2025/03/01;-10,00;Bank cost;490,00;\n"
         "2025/02/01;500,00;Sale;500,00;\n";
  std::ofstream(root / "start" / "bank" / "old.skv", std::ios::binary) << "2025-04-05;Ränta;1;\n";
  auto output = run("3bank\n2");
  EXPECT_NE(output.find("project> 3\n"
                        "Type the path of the account statement file, or of a folder to list its files. Tab completes a\n"
                        "folder or file name, Backspace erases, Enter accepts. Enter on an empty path goes back.\n"
                        "path> " + (root / "start").string() + "/bank\n"
                        "Files in " + (root / "start" / "bank").string() + ":\n"
                        "  1  nordea.csv\n"
                        "       3 entries, 2025-02-01 - 2026-01-02, new\n"
                        "  2  old.skv\n"
                        "       The file does not start with a UTF-8 byte order mark (BOM), so its encoding is unknown. "
                        "Save it as UTF-8 with a program that writes a BOM.\n"
                        "statement?> 2\n"
                        "Error: " + (root / "start" / "bank" / "old.skv").string() + ": The file does not start"),
            std::string::npos)
      << output;
  EXPECT_TRUE(output.ends_with("Nothing is imported.\nproject> \n")) << output;

  output = run("3bank/nordea.csv\n\ny");
  EXPECT_NE(output.find("Statement file " + (root / "start" / "bank" / "nordea.csv").string() + "\n"
                        "  Entries   3, 2025-02-01 - 2026-01-02 (lines 2-4, newest first)\n"
                        "  Saldo     0.00 before, 488.15 after; it adds up on every line that has one\n"
                        "Notes:\n"
                        "  - Line 1 is not an entry.\n"
                        "Suggested account 1930 Bank: 2 of 3 entries match postings on 1930 in the books.\n"
                        "Type the BAS account the statement is for, e.g. 1930. A number prefix or a word of the name "
                        "lists\naccounts. Enter accepts; Enter on an empty line imports nothing.\n"
                        "account> 1930\n"
                        "  Account   1930 Bank\n"
                        "  New       2\n"
                        "  Known     0 (in statements already imported)\n"
                        "  Outside   1 (outside every fiscal year in the books), not imported:\n"
                        "    2026-01-02         -1.85  AVGIFTER\n"
                        "Import it? (y/n) y\n"
                        "Stashed the file as .cratchit/statements/2026-10-20_1_nordea.csv in " + folder.string() + "\n"
                        "Imported 2 entries for account 1930 (0 already known, 1 outside). Press 4 to book them.\n"
                        "project> \n"),
            std::string::npos)
      << output;
  EXPECT_TRUE(fs::exists(folder / ".cratchit" / "statements" / "2026-10-20_1_nordea.csv.txt"));

  // Again: the folder shows it, and nothing is new; at the year, the posting after it is left out too
  output = run("3bank\n1\n");
  EXPECT_NE(output.find("       3 entries, 2025-02-01 - 2026-01-02, 2 already imported\n"), std::string::npos) << output;
  EXPECT_NE(output.find("Suggested account 1930 Bank: 2 of 3 entries match postings on 1930 in the books.\n"),
            std::string::npos)
      << output;
  EXPECT_TRUE(output.ends_with("Nothing to import: no entry is new inside the books.\nproject> \n")) << output;
  output = run("105bank/nordea.csv\n\n");
  EXPECT_NE(output.find("  Outside   1 (outside fiscal year 2025-01-01 - 2025-12-31), not imported:\n"),
            std::string::npos)
      << output;
  EXPECT_TRUE(output.ends_with("Nothing to import: no entry is new inside the books.\n2025> \n")) << output;
  // An account that is not in the books lists the matches and asks again; an empty line imports nothing
  output = run("5bank/nordea.csv\n" + std::string(4, '\x7f') + "ban\n" + std::string(3, '\x7f') + "\n");
  EXPECT_NE(output.find("ban\nAccounts matching ban:\n  1930  Bank\n  6570  Bank costs\naccount> "),
            std::string::npos)
      << output;
  EXPECT_TRUE(output.ends_with("Nothing is imported.\n2025> \n")) << output;
}

TEST_F(WorkspaceRepl, AKeyTurnsStatementEntriesIntoAWaitingEventAndAVoucher) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny");
  auto const folder = root / "start" / "Test";
  // A transfer from 1930 to 2099, in two statements; the bank cost is booked already
  std::ofstream(root / "start" / "bank.csv", std::ios::binary)
      << "\xEF\xBB\xBF" "Datum;Belopp;Text\n2025-04-02;-100,00;TILL 2099\n2025-03-01;-10,00;Bank cost\n";
  std::ofstream(root / "start" / "other.csv", std::ios::binary)
      << "\xEF\xBB\xBF" "Datum;Belopp;Text\n2025-04-02;100,00;FRÅN 1930\n";
  run("3bank.csv\n\ny");
  run("3other.csv\n2099\ny");
  auto output = run("411" + std::string(9, '\x7f') + "Överföring\n121");
  EXPECT_NE(output.find("project> 4\n"
                        "Statement entries not yet booked:\n"
                        "  1  1930   2025-04-02       -100.00  TILL 2099\n"
                        "  2  2099   2025-04-02        100.00  FRÅN 1930\n"
                        "entries?> 1\n"
                        "  1930   2025-04-02       -100.00  Bank\n"
                        "  TILL 2099\n"
                        "  From .cratchit/statements/2026-10-20_1_bank.csv:2, fiscal year 2025-01-01 - 2025-12-31\n"
                        "\n"
                        "  1  New waiting event with it\n"
                        "entry?> 1\n"
                        "Type the heading of the event. Backspace erases, Enter accepts. Enter on an empty line goes back.\n"
                        "heading> TILL 2099"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("Initiated .cratchit/books/2025-01-01/events.csv in " + folder.string() + "\n"
                        "Initiated .cratchit/books/2025-01-01/event_postings.csv in " + folder.string() + "\n"
                        "Noted: 2025-04-02 Överföring 100.00 with 1930 -100.00 (1 waiting). Remaining 100.00 "
                        "(debit 0.00, credit 100.00, amount 100.00).\n"
                        "Statement entries not yet booked:\n"
                        "  1  2099   2025-04-02        100.00  FRÅN 1930\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("entry?> 2\n"
                        "Add the entry to a waiting event in fiscal year 2025-01-01 - 2025-12-31:\n"
                        "  1  2025-04-02        100.00  Överföring\n"
                        "       Account        Debit       Credit\n"
                        "       1930                       100.00  2025-04-02  Bank\n"
                        "       Total           0.00       100.00\n"
                        "       Remaining 100.00 (debit 0.00, credit 100.00, amount 100.00).\n"
                        "event?> 1\n"
                        "Updated .cratchit/books/2025-01-01/event_postings.csv in " + folder.string() + "\n"
                        "Added 2099 100.00 to 2025-04-02 \"Överföring\". Balanced. Enter saves.\n"
                        "No statement entries are waiting now.\n"
                        "project> \n"),
            std::string::npos)
      << output;

  // Posted from the waiting list: the event's postings are the rows, and it balances
  output = run("10311\n\n1");
  EXPECT_NE(output.find("       Account        Debit       Credit\n"
                        "       1930                       100.00  2025-04-02  Bank\n"
                        "       2099          100.00               2025-04-02  Result\n"
                        "       Total         100.00       100.00\n"
                        "       Balanced. Enter saves.\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("The event's postings are the first rows:\n"
                        "  1930                       100.00  Bank\n"
                        "  2099          100.00               Result\n"
                        "  Balanced. Enter saves.\n"
                        "post> \n"
                        "Voucher A 3 2025-04-02 \"Överföring\"\n"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("Removed .cratchit/books/2025-01-01/events.csv in " + folder.string() + "\n"
                        "Removed .cratchit/books/2025-01-01/event_postings.csv in " + folder.string() + "\n"
                        "Saved voucher A 3 into fiscal year 2025-01-01 - 2025-12-31.\n"),
            std::string::npos)
      << output;
  auto const books = cratchit::books::read_books(folder);
  ASSERT_TRUE(books.has_value()) << books.error();
  auto const& saved = books->years.front().vouchers.back();
  ASSERT_EQ(saved.postings.size(), 2u);
  EXPECT_EQ(saved.postings[0].text, "TILL 2099");
  EXPECT_EQ(saved.postings[0].date, cratchit::parse_iso_date("2025-04-02"));
  EXPECT_TRUE(saved.postings[0].source.empty());
  // Booked now, so nothing waits
  EXPECT_TRUE(run("6").ends_with("2025> 6\nNo statement entries are waiting in this fiscal year. Press 5 to import a "
                                 "statement.\n2025> \n"));
}

TEST_F(WorkspaceRepl, YKeyOpensAYearWhichIsRememberedAndCanBeClosed) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny");
  auto const folder = root / "start" / "Test";
  auto const output = run("1x0\n");
  EXPECT_NE(output.find("project> 1\n"
                        "Fiscal years of Test AB, 556000-0001 (0 is the newest):\n"
                        "  0  2025-01-01 - 2025-12-31  open\n"
                        "year?> x\n"
                        "Unknown key. Other keys: Enter list, - back, h help, q quit.\n"
                        "year?> 0\n"
                        "Updated .cratchit/project.txt in " + folder.string() + "\n"
                        "Opened fiscal year 2025-01-01 - 2025-12-31, open for new vouchers.\n"
                        "2025> \n"
                        "Keys at 2025>:\n"
                        "  1  Enter a voucher\n"
                        "  2  Note an event to post later\n"
                        "  3  Waiting events (0)\n"
                        "  4  View vouchers (correct one, or use it as a template)\n"
                        "  5  Import an account statement (only the entries in this year)\n"
                        "  6  Statement entries not yet booked in this year\n"
                        "  7  Reports of this year\n"
                        "  8  Close the year for new vouchers\n"
                        "  9  Find matches among the waiting events\n"
                        "2025> \n"),
            std::string::npos)
      << output;
  // Reopened with the project; closing stops new vouchers until opened again
  auto const closing = run("818y-");
  EXPECT_NE(closing.find("Opened fiscal year 2025-01-01 - 2025-12-31, open for new vouchers.\n"
                         "2025> 8\n"
                         "Updated .cratchit/books/books.txt in " + folder.string() + "\n"
                         "Fiscal year 2025-01-01 - 2025-12-31 is now closed for new vouchers.\n"
                         "2025> 1\n"
                         "Fiscal year 2025-01-01 - 2025-12-31 is closed for new vouchers. Press 8 to open it.\n"
                         "2025> 8\n"
                         "Updated .cratchit/books/books.txt in " + folder.string() + "\n"
                         "Fiscal year 2025-01-01 - 2025-12-31 is now open for new vouchers.\n"
                         "2025> y\nUnknown key. Other keys: Enter list, - back, l log, h help, q quit.\n"),
            std::string::npos)
      << closing;
  EXPECT_TRUE(closing.ends_with("2025> -\nproject> \n")) << closing;
  auto const project = cratchit::read_project(folder);
  ASSERT_TRUE(project.has_value()) << project.error();
  EXPECT_EQ(project->year, "2025-01-01");
}

TEST_F(WorkspaceRepl, RKeyInAYearShowsItsReports) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  auto const output = run("7-");
  EXPECT_TRUE(output.ends_with("2025> 7\nReports for 2025-01-01 - 2025-12-31:\n  1  Trial balance (saldobalans)\n"
                               "  2  Vouchers\n  3  Account ledger\nreport?> -\n2025> \n"))
      << output;
}

TEST_F(WorkspaceRepl, NoDateOutsideTheYearAsksForIt) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  auto const output = run("1Bank 35\n");
  EXPECT_NE(output.find("voucher> Bank 35\nNo date, and today is outside the fiscal year. Type the date.\n"),
            std::string::npos)
      << output;
}

TEST_F(WorkspaceRepl, MAtTheWaitingListFindsMatchesToMakeOneOrDelete) {
  run("112Test\n1");
  write_sie(root / "start" / "test.se", good_sie());
  run("2test.se\ny10");
  auto const folder = root / "start" / "Test";
  EXPECT_NE(run("2Hyra 5000 2025-04-01\ny\n").find("  9  Find matches among the waiting events\n"), std::string::npos);
  EXPECT_NE(run("9").find("2025> 9\n"
                          "No possible matches among the waiting events (the same amount, at most 4 days apart).\n"
                          "2025> \n"),
            std::string::npos);
  run("2Bank fee 10 2025-03-03\ny");
  run("2Kvitto 249 2025-04-02\ny");
  run("2Kvitto Clas 249 2025-04-04\ny");
  auto output = run("9");
  EXPECT_NE(output.find("2025> 9\n"
                        "Possible matches in fiscal year 2025-01-01 - 2025-12-31 (the same amount, at most 4 days "
                        "apart):\n"
                        "  1  books  2025-03-03         10.00  Bank fee\n"
                        "       A 2  2025-03-01         10.00  Bank cost\n"
                        "            2 days apart, shared words: bank\n"
                        "  2  same   2025-04-02        249.00  Kvitto\n"
                        "            2025-04-04        249.00  Kvitto Clas\n"
                        "            2 days apart, shared words: kvitto\n"
                        "match?> \n"),
            std::string::npos)
      << output;
  // 1 makes one event of the two: the older event's line to change, then OK
  output = run("921\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bKvitto Clas Ohlson 249 2025-04-02\ny");
  EXPECT_NE(output.find("same?> 1\n"
                        "Type the heading, amount and date of the one event; the older event's are typed in for you.\n"
                        "Backspace erases, Enter accepts. Enter on an empty line goes back.\n"
                        "event> Kvitto 249.00 2025-04-02"),
            std::string::npos)
      << output;
  EXPECT_NE(output.find("OK? (y/n) y\n"
                        "Updated .cratchit/books/2025-01-01/events.csv in " + folder.string() + "\n"
                        "Made one event: 2025-04-02 Kvitto Clas Ohlson 249.00 (3 waiting). No postings.\n"
                        "Possible matches in"),
            std::string::npos)
      << output;
  // d deletes the waiting event as already in the books; no matches left goes back to the waiting list
  output = run("91dy");
  EXPECT_NE(output.find("  Voucher A 2 2025-03-01 \"Bank cost\" 10.00\n"), std::string::npos) << output;
  EXPECT_NE(output.find("booked?> d\n"
                        "Delete it? (y/n) y\n"
                        "Updated .cratchit/books/2025-01-01/events.csv in " + folder.string() + "\n"
                        "Deleted the waiting event 2025-03-03 \"Bank fee\" 10.00: it is in the books as A 2.\n"
                        "No more possible matches.\n"
                        "Waiting events in fiscal year 2025-01-01 - 2025-12-31:\n"
                        "  1  2025-04-01       5000.00  Hyra\n"
                        "  2  2025-04-02        249.00  Kvitto Clas Ohlson\n"),
            std::string::npos)
      << output;
}

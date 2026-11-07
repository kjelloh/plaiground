#include "pyroceph/tui/prompt_loop.hpp"

#include "menu_state.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

using charset::Encoding;
using pyroceph::tui::Navigator;
using test::menu;
using pyroceph::tui::CodePoint;
using pyroceph::tui::KeyReader;

namespace {

KeyReader keys_of(std::vector<std::uint8_t> bytes) {
  auto shared = std::make_shared<std::vector<std::uint8_t>>(std::move(bytes));
  auto next = std::make_shared<std::size_t>(0);
  return KeyReader(
      [shared, next]() -> std::optional<std::uint8_t> {
        if (*next == shared->size()) return std::nullopt;
        return (*shared)[(*next)++];
      },
      Encoding::utf8);
}

// A temporary file, removed when closed. MSVC marks std::tmpfile as unsafe
// (C4996) and wants tmpfile_s, which only it has.
std::FILE* temporary_file() {
#ifdef _MSC_VER
  std::FILE* file = nullptr;
  if (tmpfile_s(&file) != 0) return nullptr;
  return file;
#else
  return std::tmpfile();
#endif
}

// Runs the loop and gives back what it wrote
std::string run(KeyReader& keys, Navigator& navigator) {
  auto* out = temporary_file();
  if (out == nullptr) {
    ADD_FAILURE() << "No temporary file";
    return {};
  }
  pyroceph::tui::run_prompt_loop(keys, navigator, out);
  std::rewind(out);
  std::string written;
  for (int c; (c = std::fgetc(out)) != EOF;) written.push_back(static_cast<char>(c));
  std::fclose(out);
  return written;
}

std::unique_ptr<pyroceph::tui::State> tree() {
  return menu("Huvudmeny", {{U'1', "Bokslut", [] { return menu("Bokslut", {{U'\u00E5', "År", [] { return menu("År"); }}}); }}});
}

} // namespace

TEST(PromptLoop, ShowsTheFirstStateAtOnce) {
  Navigator navigator(tree());
  auto keys = keys_of({});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \n");
}

TEST(PromptLoop, ShowsEachNewStateWithoutCommonKeys) {
  Navigator navigator(tree());
  auto keys = keys_of({'1', '-'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\n  \u00E5 År\nHuvudmeny > Bokslut> "
            "\n  1 Bokslut\nHuvudmeny> "
            "\n");
}

TEST(PromptLoop, NonAsciiKeyIsAChoice) {
  Navigator navigator(tree());
  auto keys = keys_of({'1', 0xC3, 0xA5});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "År");
}

TEST(PromptLoop, HelpListsCommonKeysAndKeepsTheState) {
  Navigator navigator(tree());
  auto keys = keys_of({'1', '?'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\n  \u00E5 År\nHuvudmeny > Bokslut> "
            "\n  -       Tillbaka"
            "\n  ?       Visa gemensamma val"
            "\n  Enter   Visa valen igen"
            "\n  Esc     Töm indata (även Ctrl-U)"
            "\n  q       Avsluta"
            "\n  Ctrl-D  Avsluta (även Ctrl-C och Ctrl-Z)"
            "\nHuvudmeny > Bokslut> "
            "\n");
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, EnterShowsTheChoicesAgain) {
  Navigator navigator(tree());
  auto keys = keys_of({'1', '\n', '\r'}); // LF as on macOS and Linux, CR as from a Windows console
  auto const bokslut = std::string("\n  \u00E5 År\nHuvudmeny > Bokslut> ");
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> " + bokslut + bokslut + bokslut + "\n");
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(CommonKeys, BackOnlyWhenTheNavigatorCanGoBack) {
  Navigator navigator(tree());
  auto has_back = [&] {
    for (auto const& common : pyroceph::tui::common_keys(navigator)) {
      if (common.key == "-") return true;
    }
    return false;
  };
  EXPECT_FALSE(has_back());
  navigator.press(U'1');
  EXPECT_TRUE(has_back());
}

TEST(PromptLoop, StateIsAskedBeforeTheCommonKeys) {
  Navigator navigator(menu("Rot", {{U'?', "Fråga", [] { return menu("Fråga"); }}}));
  auto keys = keys_of({'?', '-'});
  EXPECT_EQ(run(keys, navigator),
            "\n  ? Fråga\nRot> "
            "\nRot > Fråga> " // ? was the state's, Fråga does not map - so it goes back
            "\n  ? Fråga\nRot> "
            "\n");
}

TEST(PromptOf, RootIsItsName) {
  Navigator navigator(tree());
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny> ");
}

TEST(PromptOf, StateNamesFromTheRoot) {
  Navigator navigator(
      menu("Huvudmeny", {{U'1', "Öppna", [] { return menu("Projekt", {{U'2', "Välj år", [] { return menu("År 2025"); }}}); }}}));
  navigator.press(U'1');
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny > Projekt> "); // the state's name, not the caption
  navigator.press(U'2');
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny > Projekt > År 2025> ");
  navigator.back();
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny > Projekt> ");
}

TEST(PromptLoop, QuitKeysEndTheLoop) {
  for (std::uint8_t quit : {std::uint8_t{0x03}, std::uint8_t{0x04}, std::uint8_t{0x1A}}) {
    Navigator navigator(tree());
    auto keys = keys_of({quit, '1'});
    run(keys, navigator);
    EXPECT_EQ(navigator.current().name(), "Huvudmeny") << int{quit};
  }
}

namespace {

// Bokslut takes four digits and refuses anything else
std::unique_ptr<pyroceph::tui::State> tree_with_input() {
  auto year = [](std::u32string_view text) -> pyroceph::tui::InputResult {
    if (text == U"2025") return std::string("Noterat.");
    if (text == U"tyst") return std::string();
    return std::unexpected(std::string("Fel år."));
  };
  return menu("Huvudmeny", {{U'1', "Bokslut", [=] { return menu("Bokslut", {}, year); }}});
}

} // namespace

TEST(PromptLoop, KeyThatIsNoActionStartsInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'9', 'x'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: 9x\n");
}

TEST(PromptLoop, ActionKeysAreTextWhileTyping) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'9', '1', '-', '?'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: 91-?\n");
  EXPECT_EQ(navigator.current().name(), "Huvudmeny");
}

TEST(PromptLoop, BackspaceToEmptyGoesBackToNavigating) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'9', 0x7F, '1'}); // after erasing, 1 is a choice again
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, WindowsBackspaceErasesToo) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'9', 'x', 0x08, 0x08});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: 9x\b \b\b \b\b\b> \n");
}

TEST(PromptLoop, CtrlUEmptiesInputAndGoesBackToNavigating) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'9', 'x', 0x15, '1'});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, AcceptedInputShowsReplyAndGoesBackToNavigating) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'1', '2', '0', '2', '5', '\n', '-'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\nHuvudmeny > Bokslut> \b\b: 2025"
            "\nNoterat."
            "\nHuvudmeny > Bokslut> "
            "\n  1 Bokslut\nHuvudmeny> " // - went back, so navigating again
            "\n");
}

TEST(PromptLoop, EmptyReplyShowsNothingButTheState) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'1', 't', 'y', 's', 't', '\n'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\nHuvudmeny > Bokslut> \b\b: tyst"
            "\nHuvudmeny > Bokslut> "
            "\n");
}

TEST(PromptLoop, RefusedInputShowsErrorAndKeepsTheInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'1', '2', '0', '2', '\n', '5', '\n'}); // 2 0 2, refused, then 5 makes 2025
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\nHuvudmeny > Bokslut> \b\b: 202"
            "\nFel år.\nHuvudmeny > Bokslut: 202"
            "5"
            "\nNoterat."
            "\nHuvudmeny > Bokslut> "
            "\n");
}

TEST(PromptLoop, StateWithoutOnInputRefusesInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'x', '\n'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> \b\b: x"
            "\nHär finns inget att göra med indata.\nHuvudmeny: x"
            "\n");
}

TEST(PromptLoop, BackAtRootIsNoInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'-', '1'});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, ControlKeyThatIsNoActionIsIgnored) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({0x15, 0x7F, '1'});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, ArrowsDoNotGoIntoTheInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'x', 0x1B, '[', 'A', 0x1B, '[', '1', '5', '~', 'y'}); // x, up, F5, y
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: xy\n");
}

TEST(PromptLoop, ArrowsDoNothingWhileNavigating) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({0x1B, '[', 'B', '1'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \nHuvudmeny > Bokslut> \n");
}

TEST(PromptLoop, EscEmptiesTheInput) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'x', 'y', 0x1B, '1'}); // Esc then 1: 1 is a choice again
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> \b\b: xy\b \b\b \b\b\b> "
            "\nHuvudmeny > Bokslut> \n");
}

TEST(PromptLoop, DeleteErasesLikeBackspace) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'x', 'y', 0x1B, '[', '3', '~'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: xy\b \b\n");
}

TEST(PromptLoop, QuitKeyEndsTheLoopWhileNavigating) {
  Navigator navigator(tree());
  auto keys = keys_of({'1', 'q', '-'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \n  \u00E5 År\nHuvudmeny > Bokslut> \n");
  EXPECT_EQ(navigator.current().name(), "Bokslut");
}

TEST(PromptLoop, QuitKeyIsTextWhileTyping) {
  Navigator navigator(tree_with_input());
  auto keys = keys_of({'x', 'q'});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \b\b: xq\n");
}

TEST(PromptLoop, StateIsAskedBeforeTheQuitKey) {
  Navigator navigator(menu("Rot", {{U'q', "Kvar", [] { return menu("Kvar"); }}}));
  auto keys = keys_of({'q', 'q', '-'}); // the root maps q, Kvar does not, so the second q ends the loop
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name(), "Kvar");
}

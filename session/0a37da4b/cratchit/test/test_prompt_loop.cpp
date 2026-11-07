#include "pyroceph/tui/prompt_loop.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using charset::Encoding;
using pyroceph::tui::Navigator;
using pyroceph::tui::State;
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

// Runs the loop and gives back what it wrote
std::string run(KeyReader& keys, Navigator& navigator) {
  auto* out = std::tmpfile();
  pyroceph::tui::run_prompt_loop(keys, navigator, out);
  std::rewind(out);
  std::string written;
  for (int c; (c = std::fgetc(out)) != EOF;) written.push_back(static_cast<char>(c));
  std::fclose(out);
  return written;
}

State tree() {
  return State{"Huvudmeny", {{U'1', "Bokslut", State{"Bokslut", {{U'\u00E5', "År", State{"År", {}}}}}}}};
}

} // namespace

TEST(PromptLoop, ShowsTheFirstStateAtOnce) {
  auto const root = tree();
  Navigator navigator(root);
  auto keys = keys_of({});
  EXPECT_EQ(run(keys, navigator), "\n  1 Bokslut\nHuvudmeny> \n");
}

TEST(PromptLoop, ShowsEachNewStateWithoutCommonKeys) {
  auto const root = tree();
  Navigator navigator(root);
  auto keys = keys_of({'1', 'x', '-'}); // x is no choice and is ignored
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\n  \u00E5 År\nHuvudmeny > Bokslut> "
            "\n  1 Bokslut\nHuvudmeny> "
            "\n");
}

TEST(PromptLoop, NonAsciiKeyIsAChoice) {
  auto const root = tree();
  Navigator navigator(root);
  auto keys = keys_of({'1', 0xC3, 0xA5});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name, "År");
}

TEST(PromptLoop, HelpListsCommonKeysAndKeepsTheState) {
  auto const root = tree();
  Navigator navigator(root);
  auto keys = keys_of({'1', '?'});
  EXPECT_EQ(run(keys, navigator),
            "\n  1 Bokslut\nHuvudmeny> "
            "\n  \u00E5 År\nHuvudmeny > Bokslut> "
            "\n  -       Tillbaka"
            "\n  ?       Visa gemensamma val"
            "\n  Ctrl-D  Avsluta (även Ctrl-C och Ctrl-Z)"
            "\nHuvudmeny > Bokslut> "
            "\n");
  EXPECT_EQ(navigator.current().name, "Bokslut");
}

TEST(CommonKeys, BackOnlyWhenTheNavigatorCanGoBack) {
  auto const root = tree();
  Navigator navigator(root);
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

TEST(PromptLoop, ChoiceWithHelpKeyCannotBeReached) {
  auto const root = State{"Rot", {{U'?', "Nås aldrig", State{"Nås aldrig", {}}}}};
  Navigator navigator(root);
  auto keys = keys_of({'?'});
  run(keys, navigator);
  EXPECT_EQ(navigator.current().name, "Rot");
}

TEST(PromptOf, RootIsItsName) {
  auto const root = tree();
  Navigator navigator(root);
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny> ");
}

TEST(PromptOf, StateNamesFromTheRoot) {
  auto const root = State{"Huvudmeny", {{U'1', "Öppna", State{"Projekt", {{U'2', "Välj år", State{"År 2025", {}}}}}}}};
  Navigator navigator(root);
  navigator.press(U'1');
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny > Projekt> "); // the state's name, not the caption
  navigator.press(U'2');
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny > Projekt > År 2025> ");
  navigator.press(U'-');
  EXPECT_EQ(pyroceph::tui::prompt_of(navigator), "Huvudmeny > Projekt> ");
}

TEST(PromptLoop, QuitKeysEndTheLoop) {
  for (std::uint8_t quit : {0x03, 0x04, 0x1A}) {
    auto const root = tree();
    Navigator navigator(root);
    auto keys = keys_of({quit, '1'});
    run(keys, navigator);
    EXPECT_EQ(navigator.current().name, "Huvudmeny") << int{quit};
  }
}
